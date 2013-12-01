"""!
@package animation.controller

@brief Animations management

Classes:
 - controller::AnimationController

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""
import os
import wx

from core.gcmd import GException, GError, GMessage
from core.utils import _
from grass.imaging import writeAvi, writeGif, writeIms, writeSwf

from animation.temporal_manager import TemporalManager
from animation.dialogs import InputDialog, EditDialog, ExportDialog
from animation.utils import TemporalMode, Orientation, RenderText, WxImageToPil, \
    sampleCmdMatrixAndCreateNames, layerListToCmdsMatrix, HashCmds
from animation.data import AnimationData


class AnimationController(wx.EvtHandler):
    def __init__(self, frame, sliders, animations, mapwindows, provider, bitmapPool, mapFilesPool):
        wx.EvtHandler.__init__(self)

        self.mapwindows = mapwindows

        self.frame = frame
        self.sliders = sliders
        self.slider = self.sliders['temporal']
        self.animationToolbar = None

        self.temporalMode = None
        self.animationData = []

        self.timer = wx.Timer(self, id=wx.NewId())

        self.animations = animations
        self.bitmapPool = bitmapPool
        self.mapFilesPool = mapFilesPool
        self.bitmapProvider = provider
        for anim, win in zip(self.animations, self.mapwindows):
            anim.SetCallbackUpdateFrame(
                lambda index, dataId, win=win: self.UpdateFrame(index, win, dataId))
            anim.SetCallbackEndAnimation(
                lambda index, dataId, win=win: self.UpdateFrameEnd(index, win, dataId))
            anim.SetCallbackOrientationChanged(self.OrientationChangedInReverseMode)

        for slider in self.sliders.values():
            slider.SetCallbackSliderChanging(self.SliderChanging)
            slider.SetCallbackSliderChanged(self.SliderChanged)
            slider.SetCallbackFrameIndexChanged(self.ChangeFrame)

        self.runAfterReleasingSlider = None

        self.temporalManager = TemporalManager()
        self.Bind(wx.EVT_TIMER, self.OnTimerTick, self.timer)

        self.timeTick = 200

        self._dialogs = {}

    def SetAnimationToolbar(self, toolbar):
        self.animationToolbar = toolbar

    def GetTimeTick(self):
        return self._timeTick

    def SetTimeTick(self, value):
        self._timeTick = value
        if self.timer.IsRunning():
            self.timer.Stop()
            self.timer.Start(self._timeTick)
        self.DisableSliderIfNeeded()

    timeTick = property(fget=GetTimeTick, fset=SetTimeTick)

    def OnTimerTick(self, event):
        for anim in self.animations:
            anim.Update()

    def StartAnimation(self):
        # if self.timer.IsRunning():
        #     self.timer.Stop()

        for anim in self.animations:
            if self.timer.IsRunning():
                anim.NextFrameIndex()
            anim.Start()
        if not self.timer.IsRunning():
            self.timer.Start(self.timeTick)
            self.DisableSliderIfNeeded()

    def PauseAnimation(self, paused):
        if paused:
            if self.timer.IsRunning():
                self.timer.Stop()
                self.DisableSliderIfNeeded()
        else:
            if not self.timer.IsRunning():
                self.timer.Start(self.timeTick)
                self.DisableSliderIfNeeded()

        for anim in self.animations:
            anim.Pause(paused)

    def EndAnimation(self):
        if self.timer.IsRunning():
            self.timer.Stop()
            self.DisableSliderIfNeeded()

        for anim in self.animations:
            anim.Stop()

    def UpdateFrameEnd(self, index, win, dataId):
        if self.timer.IsRunning():
            self.timer.Stop()
            self.DisableSliderIfNeeded()

        self.animationToolbar.Stop()

        self.UpdateFrame(index, win, dataId)

    def UpdateFrame(self, index, win, dataId):
        bitmap = self.bitmapProvider.GetBitmap(dataId)
        if dataId is None:
            dataId = ''
        win.DrawBitmap(bitmap)
        # self.frame.SetStatusText(dataId)
        self.slider.UpdateFrame(index)

    def SliderChanging(self, index):
        if self.runAfterReleasingSlider is None:
            self.runAfterReleasingSlider = self.timer.IsRunning()
        self.PauseAnimation(True)
        self.ChangeFrame(index)

    def SliderChanged(self):
        if self.runAfterReleasingSlider:
            self.PauseAnimation(False)
        self.runAfterReleasingSlider = None

    def ChangeFrame(self, index):
        for anim in self.animations:
            anim.FrameChangedFromOutside(index)

    def DisableSliderIfNeeded(self):
        if self.timer.IsRunning() and self._timeTick < 100:
            self.slider.EnableSlider(False)
        else:
            self.slider.EnableSlider(True)

    def OrientationChangedInReverseMode(self, mode):
        if mode == Orientation.FORWARD:
            self.animationToolbar.PlayForward()
        elif mode == Orientation.BACKWARD:
            self.animationToolbar.PlayBack()

    def SetReplayMode(self, mode):
        for anim in self.animations:
            anim.replayMode = mode

    def SetOrientation(self, mode):
        for anim in self.animations:
            anim.orientation = mode

    def SetTemporalMode(self, mode):
        self._temporalMode = mode

    def GetTemporalMode(self):
        return self._temporalMode

    temporalMode = property(fget=GetTemporalMode, fset=SetTemporalMode)

    def GetTimeGranularity(self):
        if self.temporalMode == TemporalMode.TEMPORAL:
            return self.temporalManager.GetGranularity()

        return None

    def EditAnimations(self):
        # running = False
        # if self.timer.IsRunning():
        #     running = True
        self.EndAnimation()
        dlg = EditDialog(parent=self.frame, evalFunction=self.EvaluateInput,
                         animationData=self.animationData, maxAnimations=len(self.animations))
        if dlg.ShowModal() == wx.ID_CANCEL:
            dlg.Destroy()
            return
        self.animationData, self.temporalMode, self.temporalManager = dlg.GetResult()
        dlg.Destroy()

        self._setAnimations()

    def AddAnimation(self):
        # check if we can add more animations
        found = False
        indices = [anim.windowIndex for anim in self.animationData]
        for windowIndex in range(len(self.animations)):
            if windowIndex not in indices:
                found = True
                break

        if not found:
            GMessage(parent=self.frame,
                     message=_("Maximum number of animations is %s.") % len(self.animations))
            return

        # running = False
        # if self.timer.IsRunning():
        #     running = True
        self.EndAnimation()
        #     self.PauseAnimation(True)

        animData = AnimationData()
        # number of active animations
        animationIndex = len([anim for anim in self.animations if anim.IsActive()])
        animData.SetDefaultValues(windowIndex, animationIndex)
        dlg = InputDialog(parent=self.frame, mode='add', animationData=animData)
        if dlg.ShowModal() == wx.ID_CANCEL:
            dlg.Destroy()
            return
        dlg.Destroy()
        # check compatibility
        if animData.windowIndex in indices:
            GMessage(parent=self.frame, message=_("More animations are using one window."
                                                  " Please select different window for each animation."))
            return
        try:
            temporalMode, tempManager = self.EvaluateInput(self.animationData + [animData])
        except GException, e:
            GError(parent=self.frame, message=e.value, showTraceback=False)
            return
        # if ok, set temporal mode
        self.temporalMode = temporalMode
        self.temporalManager = tempManager
        # add data
        windowIndex = animData.windowIndex
        self.animationData.append(animData)
        self._setAnimations()

    def SetAnimations(self, layerLists):
        """!Set animation data directly.

        @param layerLists list of layerLists
        """
        try:
            animationData = []
            for i in range(len(self.animations)):
                if layerLists[i]:
                    anim = AnimationData()
                    anim.SetDefaultValues(i, i)
                    anim.SetLayerList(layerLists[i])
                    animationData.append(anim)

        except (GException, ValueError, IOError) as e:
            GError(parent=self.frame, message=str(e),
                   showTraceback=False, caption=_("Invalid input"))
            return
        try:
            temporalMode, tempManager = self.EvaluateInput(animationData)
        except GException, e:
            GError(parent=self.frame, message=e.value, showTraceback=False)
            return
        self.animationData = animationData
        self.temporalManager = tempManager
        self.temporalMode = temporalMode
        self._setAnimations()

    def _setAnimations(self):
        indices = [anim.windowIndex for anim in self.animationData]

        self._updateWindows(activeIndices=indices)

        if self.temporalMode == TemporalMode.TEMPORAL:
            timeLabels, mapNamesDict = self.temporalManager.GetLabelsAndMaps()
        else:
            timeLabels, mapNamesDict = None, None
        for anim in self.animationData:
            if anim.viewMode == '2d':
                anim.cmdMatrix = layerListToCmdsMatrix(anim.layerList)
            else:
                anim.cmdMatrix = [(cmd,) for cmd in anim.GetNvizCommands()['commands']]
        self._updateSlider(timeLabels=timeLabels)
        self._updateAnimations(activeIndices=indices, mapNamesDict=mapNamesDict)
        wx.Yield()
        self._updateBitmapData()
        # if running:
        #     self.PauseAnimation(False)
        #     # self.StartAnimation()
        # else:
        self.EndAnimation()

    def _updateSlider(self, timeLabels=None):
        if self.temporalMode == TemporalMode.NONTEMPORAL:
            self.frame.SetSlider('nontemporal')
            self.slider = self.sliders['nontemporal']
            frameCount = self.animationData[0].mapCount
            self.slider.SetFrames(frameCount)
        elif self.temporalMode == TemporalMode.TEMPORAL:
            self.frame.SetSlider('temporal')
            self.slider = self.sliders['temporal']
            self.slider.SetTemporalType(self.temporalManager.temporalType)
            self.slider.SetFrames(timeLabels)
        else:
            self.frame.SetSlider(None)
            self.slider = None

    def _updateAnimations(self, activeIndices, mapNamesDict=None):
        if self.temporalMode == TemporalMode.NONTEMPORAL:
            for i in range(len(self.animations)):
                if i not in activeIndices:
                    self.animations[i].SetActive(False)
                    continue
                anim = [anim for anim in self.animationData if anim.windowIndex == i][0]
                self.animations[i].SetFrames([HashCmds(cmdList) for cmdList in anim.cmdMatrix])
                self.animations[i].SetActive(True)
        else:
            for i in range(len(self.animations)):
                if i not in activeIndices:
                    self.animations[i].SetActive(False)
                    continue
                anim = [anim for anim in self.animationData if anim.windowIndex == i][0]
                identifiers = sampleCmdMatrixAndCreateNames(anim.cmdMatrix,
                                                            mapNamesDict[anim.firstStdsNameType[0]])
                self.animations[i].SetFrames(identifiers)
                self.animations[i].SetActive(True)

    def _updateWindows(self, activeIndices):
        # add or remove window
        for windowIndex in range(len(self.animations)):
            if not self.frame.IsWindowShown(windowIndex) and windowIndex in activeIndices:
                self.frame.AddWindow(windowIndex)
            elif self.frame.IsWindowShown(windowIndex) and windowIndex not in activeIndices:
                self.frame.RemoveWindow(windowIndex)

    def _updateBitmapData(self):
        # unload previous data
        self.bitmapProvider.Unload()

        # load new data
        for animData in self.animationData:
            if animData.viewMode == '2d':
                self._set2DData(animData)
            else:
                self._load3DData(animData)
            self._loadLegend(animData)
        self.bitmapProvider.Load(nprocs=4)
        # clear pools
        self.bitmapPool.Clear()
        self.mapFilesPool.Clear()

    def _set2DData(self, animationData):
        opacities = [layer.opacity for layer in animationData.layerList if layer.active]
        self.bitmapProvider.SetCmds(animationData.cmdMatrix, opacities)

    def _load3DData(self, animationData):
        nviz = animationData.GetNvizCommands()
        self.bitmapProvider.SetCmds3D(nviz['commands'], nviz['region'])

    def _loadLegend(self, animationData):
        if animationData.legendCmd:
            try:
                bitmap = self.bitmapProvider.LoadOverlay(animationData.legendCmd)
                try:
                    from PIL import Image
                    for param in animationData.legendCmd:
                        if param.startswith('at'):
                            b, t, l, r = param.split('=')[1].split(',')
                            x, y = float(l) / 100., 1 - float(t) / 100.
                            break
                except ImportError:
                    x, y = 0, 0
                self.mapwindows[animationData.windowIndex].SetOverlay(bitmap, x, y)
            except GException:
                GError(message=_("Failed to display legend."))
        else:
            self.mapwindows[animationData.windowIndex].ClearOverlay()

    def EvaluateInput(self, animationData):
        stds = 0
        maps = 0
        mapCount = set()
        tempManager = None
        windowIndex = []
        for anim in animationData:
            for layer in anim.layerList:
                if layer.active and hasattr(layer, 'maps'):
                    if layer.mapType in ('strds', 'stvds'):
                        stds += 1
                    else:
                        maps += 1
                    mapCount.add(len(layer.maps))
            windowIndex.append(anim.windowIndex)

        if maps and stds:
            temporalMode = TemporalMode.NONTEMPORAL
        elif maps:
            temporalMode = TemporalMode.NONTEMPORAL
        elif stds:
            temporalMode = TemporalMode.TEMPORAL
        else:
            temporalMode = None

        if temporalMode == TemporalMode.NONTEMPORAL:
            if len(mapCount) > 1:
                raise GException(_("Inconsistent number of maps, please check input data."))
        elif temporalMode == TemporalMode.TEMPORAL:
            tempManager = TemporalManager()
            # these raise GException:
            for anim in animationData:
                tempManager.AddTimeSeries(*anim.firstStdsNameType)

            message = tempManager.EvaluateInputData()
            if message:
                GMessage(parent=self.frame, message=message)

        return temporalMode, tempManager

    def Reload(self):
        self.EndAnimation()

        self.bitmapProvider.Load(force=True)

        self.EndAnimation()

    def Export(self):
        if not self.animationData:
            GMessage(parent=self.frame, message=_("No animation to export."))
            return

        if 'export' in self._dialogs:
            self._dialogs['export'].Show()
            self._dialogs['export'].Raise()
        else:
            dlg = ExportDialog(self.frame, temporal=self.temporalMode,
                               timeTick=self.timeTick)
            dlg.doExport.connect(self._export)
            self._dialogs['export'] = dlg
            dlg.Show()

    def _export(self, exportInfo, decorations):
        size = self.frame.animationPanel.GetSize()
        if self.temporalMode == TemporalMode.TEMPORAL:
            timeLabels, mapNamesDict = self.temporalManager.GetLabelsAndMaps()
            frameCount = len(timeLabels)
        else:
            frameCount = self.animationData[0].mapCount  # should be the same for all

        animWinSize = []
        animWinPos = []
        animWinIndex = []
        legends = [anim.legendCmd for anim in self.animationData]
        # determine position and sizes of bitmaps
        for i, (win, anim) in enumerate(zip(self.mapwindows, self.animations)):
            if anim.IsActive():
                pos = win.GetPosition()
                animWinPos.append(pos)
                animWinSize.append(win.GetSize())
                animWinIndex.append(i)

        images = []
        busy = wx.BusyInfo(message=_("Preparing export, please wait..."), parent=self.frame)
        wx.Yield()
        for frameIndex in range(frameCount):
            image = wx.EmptyImage(*size)
            image.Replace(0, 0, 0, 255, 255, 255)
            # collect bitmaps of all windows and paste them into the one
            for i in animWinIndex:
                frameId = self.animations[i].GetFrame(frameIndex)
                bitmap = self.bitmapProvider.GetBitmap(frameId)
                im = wx.ImageFromBitmap(bitmap)

                # add legend if used
                legend = legends[i]
                if legend:
                    legendBitmap = self.bitmapProvider.LoadOverlay(legend)
                    x, y = self.mapwindows[i].GetOverlayPos()
                    legImage = wx.ImageFromBitmap(legendBitmap)
                    # not so nice result, can we handle the transparency otherwise?
                    legImage.ConvertAlphaToMask()
                    im.Paste(legImage, x, y)

                if im.GetSize() != animWinSize[i]:
                    im.Rescale(*animWinSize[i])
                image.Paste(im, *animWinPos[i])
            # paste decorations
            for decoration in decorations:
                # add image
                x = decoration['pos'][0] / 100. * size[0]
                y = decoration['pos'][1] / 100. * size[1]
                if decoration['name'] == 'image':
                    decImage = wx.Image(decoration['file'])
                elif decoration['name'] == 'time':
                    timeLabel = timeLabels[frameIndex]
                    if timeLabel[1]:
                        text = _("%(from)s %(dash)s %(to)s") % \
                            {'from': timeLabel[0], 'dash': u"\u2013", 'to': timeLabel[1]}
                    else:
                        text = _("%(start)s %(unit)s") % \
                            {'start': timeLabel[0], 'unit': timeLabel[2]}

                    decImage = RenderText(text, decoration['font']).ConvertToImage()
                elif decoration['name'] == 'text':
                    text = decoration['text']
                    decImage = RenderText(text, decoration['font']).ConvertToImage()

                image.Paste(decImage, x, y)

            images.append(image)
        del busy

        # export
        pilImages = [WxImageToPil(image) for image in images]
        busy = wx.BusyInfo(message=_("Exporting animation, please wait..."),
                           parent=self.frame)
        wx.Yield()
        try:
            if exportInfo['method'] == 'sequence':
                filename = os.path.join(exportInfo['directory'],
                                        exportInfo['prefix'] + '.' + exportInfo['format'].lower())
                writeIms(filename=filename, images=pilImages)
            elif exportInfo['method'] == 'gif':
                writeGif(filename=exportInfo['file'], images=pilImages,
                         duration=self.timeTick / float(1000), repeat=True)
            elif exportInfo['method'] == 'swf':
                writeSwf(filename=exportInfo['file'], images=pilImages,
                         duration=self.timeTick / float(1000), repeat=True)
            elif exportInfo['method'] == 'avi':
                writeAvi(filename=exportInfo['file'], images=pilImages,
                         duration=self.timeTick / float(1000),
                         encoding=exportInfo['encoding'],
                         inputOptions=exportInfo['options'])
        except Exception, e:
            del busy
            GError(parent=self.frame, message=str(e))
            return
        del busy
