"""!
@package animation.controller

@brief Animations management

Classes:
 - controller::AnimationController

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""
import os
import wx

try:
    import visvis.vvmovie as vv
    hasVisvis = True
except ImportError:
    # if visvis.vvmovie is in grass python library
    # import grass.visvis as vv
    # 
    # question: if integrate visvis, if integrate visvis.vvmovie or only 
    # images2swf.py, images2gif.py?
    hasVisvis = False

from core.gcmd import GException, GError, GMessage
import grass.script as grass

from temporal_manager import TemporalManager
from dialogs import InputDialog, EditDialog, AnimationData, ExportDialog
from utils import TemporalMode, Orientation, RenderText, WxImageToPil

class AnimationController(wx.EvtHandler):
    def __init__(self, frame, sliders, animations, mapwindows, providers, bitmapPool):
        wx.EvtHandler.__init__(self)

        self.mapwindows = mapwindows

        self.frame = frame
        self.sliders = sliders
        self.slider = self.sliders['temporal']
        self.animationToolbar = None

        self.temporalMode = None
        self.animationData = []

        self.timer = wx.Timer(self, id = wx.NewId())

        self.animations = animations
        self.bitmapPool = bitmapPool
        self.bitmapProviders = providers
        for anim, win, provider in zip(self.animations, self.mapwindows, self.bitmapProviders):
            anim.SetCallbackUpdateFrame(lambda index, dataId, win = win, provider = provider : self.UpdateFrame(index, win, provider, dataId))
            anim.SetCallbackEndAnimation(lambda index, dataId, win = win, provider = provider: self.UpdateFrameEnd(index, win, provider, dataId))
            anim.SetCallbackOrientationChanged(self.OrientationChangedInReverseMode)

        for slider in self.sliders.values():
            slider.SetCallbackSliderChanging(self.SliderChanging)
            slider.SetCallbackSliderChanged(self.SliderChanged)
            slider.SetCallbackFrameIndexChanged(self.ChangeFrame)

        self.runAfterReleasingSlider = None

        self.temporalManager = TemporalManager()
        self.Bind(wx.EVT_TIMER, self.OnTimerTick, self.timer)

        self.timeTick = 200

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

    timeTick = property(fget = GetTimeTick, fset = SetTimeTick)
        
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

    def UpdateFrameEnd(self, index, win, provider, dataId):
        if self.timer.IsRunning():
            self.timer.Stop()
            self.DisableSliderIfNeeded()

        self.animationToolbar.Stop()
        
        self.UpdateFrame(index, win, provider, dataId)

    def UpdateFrame(self, index, win, provider, dataId):
        bitmap = provider.GetBitmap(dataId)
        if dataId is None:
            dataId = ''
        win.DrawBitmap(bitmap, dataId)
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

    temporalMode = property(fget = GetTemporalMode, fset = SetTemporalMode)

    def GetTimeGranularity(self):
        if self.temporalMode == TemporalMode.TEMPORAL:
            return self.temporalManager.GetGranularity()

        return None

    def EditAnimations(self):
        # running = False
        # if self.timer.IsRunning():
        #     running = True
        self.EndAnimation()

        dlg = EditDialog(parent = self.frame, evalFunction = self.EvaluateInput,
                          animationData = self.animationData, maxAnimations = len(self.animations))
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
            GMessage(parent = self.frame, message = _("Maximum number of animations is %s.") % len(self.animations))
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
        dlg = InputDialog(parent = self.frame, mode = 'add', animationData = animData)
        if dlg.ShowModal() == wx.ID_CANCEL:
            dlg.Destroy()
            return
        dlg.Destroy()
        # check compatibility
        if animData.windowIndex in indices:
            GMessage(parent = self.frame, message = _("More animations are using one window."
                                                      " Please select different window for each animation."))
            return
        try:
            temporalMode, tempManager = self.EvaluateInput(self.animationData + [animData])
        except GException, e:
            GError(parent = self.frame, message = e.value, showTraceback = False)
            return
        # if ok, set temporal mode
        self.temporalMode = temporalMode
        self.temporalManager = tempManager
        # add data
        windowIndex = animData.windowIndex
        self.animationData.append(animData)
        self._setAnimations()

    def SetAnimations(self, inputs=None, dataType=None):
        """!Set animation data directly.

        @param raster list of lists of raster maps or None
        @param strds list of strds or None
        @param inputs list of lists of raster maps or vector maps, 
               or a space time raster or vector dataset
        @param dataType The type of the input data must be one of 'rast', 'vect', 'strds' or 'strds'
        """
        try:
            animationData = []
            for i in range(len(self.animations)):
                if inputs is not None and inputs[i]:
                    if dataType == 'rast' or dataType == 'vect':
                        if type(inputs[i]) == list:
                            anim = AnimationData()
                            anim.SetDefaultValues(i, i)
                            anim.inputMapType = dataType
                            anim.inputData = ','.join(inputs[i])
                            animationData.append(anim)
                    elif dataType == 'strds' or dataType == 'stvds':
                        anim = AnimationData()
                        anim.SetDefaultValues(i, i)
                        anim.inputMapType = dataType
                        anim.inputData = inputs[i]
                        animationData.append(anim)

        except (GException, ValueError, IOError) as e:
            GError(parent = self.frame, message = str(e),
                   showTraceback = False, caption = _("Invalid input"))
            return
        try:
            temporalMode, tempManager = self.EvaluateInput(animationData)
        except GException, e:
            GError(parent = self.frame, message = e.value, showTraceback = False)
            return
        self.animationData = animationData
        self.temporalManager = tempManager
        self.temporalMode = temporalMode
        self._setAnimations()

    def _setAnimations(self):
        indices = [anim.windowIndex for anim in self.animationData]

        self._updateWindows(activeIndices = indices)

        if self.temporalMode == TemporalMode.TEMPORAL:
            timeLabels, mapNamesDict = self.temporalManager.GetLabelsAndMaps()
        else:
            timeLabels, mapNamesDict = None, None

        self._updateSlider(timeLabels = timeLabels)
        self._updateAnimations(activeIndices = indices, mapNamesDict = mapNamesDict)
        wx.Yield()
        self._updateBitmapData()
        # if running:
        #     self.PauseAnimation(False)
        #     # self.StartAnimation()
        # else:
        self.EndAnimation()

    def _updateSlider(self, timeLabels = None):
        if self.temporalMode == TemporalMode.NONTEMPORAL:
            self.frame.SetSlider('nontemporal')
            self.slider = self.sliders['nontemporal']
            frameCount = len(self.animationData[0].mapData) # should be the same for all
            self.slider.SetFrames(frameCount)
        elif self.temporalMode == TemporalMode.TEMPORAL:
            self.frame.SetSlider('temporal')
            self.slider = self.sliders['temporal']
            self.slider.SetTemporalType(self.temporalManager.temporalType)
            self.slider.SetFrames(timeLabels)
        else:
            self.frame.SetSlider(None)
            self.slider = None

    def _updateAnimations(self, activeIndices, mapNamesDict = None):
        if self.temporalMode == TemporalMode.NONTEMPORAL:
            for i in range(len(self.animations)):
                if i not in activeIndices:
                    self.animations[i].SetActive(False)
                    continue
                anim = [anim for anim in self.animationData if anim.windowIndex == i][0]
                self.animations[i].SetFrames(anim.mapData)
                self.animations[i].SetActive(True)
        else:
            for i in range(len(self.animations)):
                if i not in activeIndices:
                    self.animations[i].SetActive(False)
                    continue
                anim = [anim for anim in self.animationData if anim.windowIndex == i][0]
                self.animations[i].SetFrames(mapNamesDict[anim.inputData])
                self.animations[i].SetActive(True)

    def _updateWindows(self, activeIndices):
        # add or remove window
        for windowIndex in range(len(self.animations)):
            if not self.frame.IsWindowShown(windowIndex) and windowIndex in activeIndices:
                self.frame.AddWindow(windowIndex)
            elif self.frame.IsWindowShown(windowIndex) and windowIndex not in activeIndices:
                self.frame.RemoveWindow(windowIndex)

    def _updateBitmapData(self):
        # unload data:
        for prov in self.bitmapProviders:
            prov.Unload()

        # load data
        for animData in self.animationData:
            if animData.viewMode == '2d':
                self._load2DData(animData)
            else:
                self._load3DData(animData)

        # clear bitmapPool
        usedNames = []
        for prov in self.bitmapProviders:
            names = prov.GetDataNames()
            if names:
                usedNames.extend(names)
        self.bitmapPool.Clear(usedNames)

    def _load2DData(self, animationData):
        prov = self.bitmapProviders[animationData.windowIndex]
        prov.SetData(datasource = animationData.mapData, dataType=animationData.inputMapType)

        self.bitmapProviders[animationData.windowIndex].Load()

    def _load3DData(self, animationData):
        prov = self.bitmapProviders[animationData.windowIndex]
        nviz = animationData.GetNvizCommands()
        prov.SetData(datasource = nviz['commands'], 
                     dataNames = animationData.mapData, dataType = 'nviz',
                     suffix = animationData.nvizParameter,
                     nvizRegion = nviz['region'])

        self.bitmapProviders[animationData.windowIndex].Load()

    def EvaluateInput(self, animationData):
        stds = 0
        maps = 0
        mapCount = set()
        tempManager = None
        windowIndex = []
        for anim in animationData:

            mapCount.add(len(anim.mapData))
            windowIndex.append(anim.windowIndex)

            if anim.inputMapType in ('rast', 'vect'):
                maps += 1
            elif anim.inputMapType in ('strds', 'stvds'):
                stds += 1

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
                if anim.inputMapType not in ('strds', 'stvds'):
                    continue
                tempManager.AddTimeSeries(anim.inputData, anim.inputMapType)
            message = tempManager.EvaluateInputData()
            if message:
                GMessage(parent = self.frame, message = message)

        return temporalMode, tempManager

    def Reload(self):
        self.EndAnimation()

        activeIndices = [anim.windowIndex for anim in self.animationData]
        for index in activeIndices:
            self.bitmapProviders[index].Load(force = True)

        self.EndAnimation()

    def Export(self):
        if not self.animationData:
            GMessage(parent = self.frame, message = _("No animation to export."))
            return
        dlg = ExportDialog(self.frame, temporal = self.temporalMode,
                           timeTick = self.timeTick, visvis = hasVisvis)
        if dlg.ShowModal() == wx.ID_OK:
            decorations = dlg.GetDecorations()
            exportInfo = dlg.GetExportInformation()
            dlg.Destroy()
        else:
            dlg.Destroy()
            return

        self._export(exportInfo, decorations)

    def _export(self, exportInfo, decorations):
        size = self.frame.animationPanel.GetSize()
        if self.temporalMode == TemporalMode.TEMPORAL:
            timeLabels, mapNamesDict = self.temporalManager.GetLabelsAndMaps()
            frameCount = len(timeLabels)
        else:
            frameCount = len(self.animationData[0].mapData) # should be the same for all

        animWinSize = []
        animWinPos = []
        animWinIndex = []
        # determine position and sizes of bitmaps
        for i, (win, anim) in enumerate(zip(self.mapwindows, self.animations)):
            if anim.IsActive():
                pos = tuple([pos1 + pos2 for pos1, pos2 in zip(win.GetPosition(), win.GetAdjustedPosition())])
                animWinPos.append(pos)
                animWinSize.append(win.GetAdjustedSize())
                animWinIndex.append(i)
        
        images = []
        for frameIndex in range(frameCount):
            image = wx.EmptyImage(*size)
            image.Replace(0, 0, 0, 255, 255, 255)
            # collect bitmaps of all windows and paste them into the one
            for i in range(len(animWinSize)):
                frameId = self.animations[animWinIndex[i]].GetFrame(frameIndex)
                bitmap = self.bitmapProviders[animWinIndex[i]].GetBitmap(frameId)
                im = wx.ImageFromBitmap(bitmap)
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

        # export
        if exportInfo['method'] == 'sequence':
            busy = wx.BusyInfo(message = _("Exporting images, please wait..."), parent = self.frame)
            wx.Yield()
            zeroPadding = len(str(len(images)))
            for i, image in enumerate(images):
                filename = "%s_%s.%s" % (exportInfo['prefix'], str(i + 1).zfill(zeroPadding),
                                         exportInfo['format']['ext'])
                image.SaveFile(os.path.join(exportInfo['directory'], filename), exportInfo['format']['type'])

            busy.Destroy()

        elif exportInfo['method'] in ('gif', 'swf', 'avi'):
            pilImages = [WxImageToPil(image) for image in images]
            

            busy = wx.BusyInfo(message = _("Exporting animation, please wait..."), parent = self.frame)
            wx.Yield()
            try:
                if exportInfo['method'] == 'gif':
                    vv.writeGif(filename = exportInfo['file'], images = pilImages,
                                duration = self.timeTick / float(1000), repeat = True)
                elif exportInfo['method'] == 'swf':
                    vv.writeSwf(filename = exportInfo['file'], images = pilImages,
                                duration = self.timeTick / float(1000), repeat = True)
                elif exportInfo['method'] == 'avi':
                    vv.writeAvi(filename = exportInfo['file'], images = pilImages,
                                duration = self.timeTick / float(1000),
                                encoding = exportInfo['encoding'],
                                inputOptions = '-sameq')
            except Exception, e:
                del busy
                GError(parent = self.frame, message = str(e))
                return
            del busy


            # image.SaveFile('/home/anna/testy/grass/export/export_%s.png' % frameIndex, wx.BITMAP_TYPE_PNG)


            


        

        # for anim in self.animationData


        


#def test():
#    import gettext
#    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)
#
#    import grass.script as grass
#    import wx
#    app = wx.PySimpleApp()
#    wx.InitAllImageHandlers()
#    # app.MainLoop()
#
#    bitmaps = {}
#    rasters = ['elevation.dem']
#    # rasters = ['streams']
#    # rasters = grass.read_command("g.mlist", type = 'rast', fs = ',', quiet = True).strip().split(',')
#
#    # print nrows, ncols
#    size = (300,100)
#    newSize, scale = ComputeScale(size)
#    # print scale
#    LoadRasters(rasters = rasters, bitmaps = bitmaps, scale = scale, size = newSize)
#
#    for b in bitmaps.keys():
#        bitmaps[b].SaveFile('/home/anna/testy/ctypes/' + b + '.png', wx.BITMAP_TYPE_PNG)
#
#if __name__ == '__main__':
#
#    test()
