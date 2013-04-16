"""!
@package animation.frame

@brief Animation frame and different types of sliders

Classes:
 - frame::SwipeMapDialog
 - frame::AnimationFrame
 - frame::AnimationsPanel
 - frame::AnimationSliderBase
 - frame::SimpleAnimationSlider
 - frame::TimeAnimationSlider


(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""
import os
import sys
import wx
import wx.aui

if __name__ == '__main__':
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))
from core import globalvar
from gui_core.widgets import IntegerValidator
from core.gcmd import RunCommand

from mapwindow import AnimationWindow, BitmapProvider, BitmapPool
from controller import AnimationController
from anim import Animation
from toolbars import MainToolbar, AnimationToolbar, MiscToolbar
from dialogs import SpeedDialog
from utils import Orientation, ReplayMode, TemporalType


MAX_COUNT = 4

class AnimationFrame(wx.Frame):
    def __init__(self, parent = None, id = wx.ID_ANY, title = _("Animation tool"), rasters = None, timeseries = None):
        wx.Frame.__init__(self, parent, id, title = title, style = wx.DEFAULT_FRAME_STYLE, size = (800, 600))

        self.SetClientSize(self.GetSize())
        self.iconsize = (16, 16)

        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass_map.ico'), wx.BITMAP_TYPE_ICO))

        self.animations = [Animation() for i in range(MAX_COUNT)]
        self.windows = []
        self.animationPanel = AnimationsPanel(self, self.windows, initialCount = MAX_COUNT)
        bitmapPool = BitmapPool()
        self.providers = [BitmapProvider(frame = self, bitmapPool = bitmapPool) for i in range(MAX_COUNT)]
        self.animationSliders = {}
        self.animationSliders['nontemporal'] = SimpleAnimationSlider(self)
        self.animationSliders['temporal'] = TimeAnimationSlider(self)
        self.controller = AnimationController(frame = self, 
                                              sliders = self.animationSliders,
                                              animations = self.animations,
                                              mapwindows = self.windows,
                                              providers = self.providers,
                                              bitmapPool = bitmapPool)
        for win, provider in zip(self.windows, self.providers):
            win.Bind(wx.EVT_SIZE, lambda event, provider=provider,
                     sizeMethod=win.GetClientSize: self.FrameSizeChanged(event, provider, sizeMethod))
            provider.mapsLoaded.connect(lambda: self.SetStatusText(''))

        self.InitStatusbar()
        self._mgr = wx.aui.AuiManager(self)

        # toolbars
        self.toolbars = {}
        tb = ['miscToolbar', 'animationToolbar', 'mainToolbar']
        if sys.platform == 'win32':
            tb.reverse()
        for toolb in tb:
            self._addToolbar(toolb)

        self._addPanes()
        self._mgr.Update()

        self.dialogs = dict()
        self.dialogs['speed'] = None

        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

    def InitStatusbar(self):
        """!Init statusbar."""
        self.CreateStatusBar(number = 1, style = 0)

    def _addPanes(self):
        self._mgr.AddPane(self.animationPanel, wx.aui.AuiPaneInfo().CentrePane().
                  Name('animPanel').CentrePane().CaptionVisible(False).PaneBorder(False).
                  Floatable(False).BestSize((-1,-1)).
                  CloseButton(False).DestroyOnClose(True).Layer(0))
        for name, slider in self.animationSliders.iteritems():
            self._mgr.AddPane(slider, wx.aui.AuiPaneInfo().PaneBorder(False).Name('slider_' + name).
                              Layer(1).CaptionVisible(False).BestSize(slider.GetBestSize()).
                              DestroyOnClose(True).CloseButton(False).Bottom())
            self._mgr.GetPane('slider_' + name).Hide()

    def _addToolbar(self, name):
        """!Add defined toolbar to the window
        
        Currently known toolbars are:
         - 'mainToolbar'          - data management
         - 'animationToolbar'     - animation controls
         - 'miscToolbar'          - help, close
        """
        if name == "mainToolbar":
            self.toolbars[name] = MainToolbar(self)
            self._mgr.AddPane(self.toolbars[name],
                              wx.aui.AuiPaneInfo().
                              Name('mainToolbar').Caption(_("Main Toolbar")).
                              ToolbarPane().Top().
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(True).TopDockable(True).
                              CloseButton(False).Layer(2).Row(1).
                              BestSize((self.toolbars['mainToolbar'].GetBestSize())))
        elif name == 'animationToolbar':
            self.toolbars[name] = AnimationToolbar(self)
            self._mgr.AddPane(self.toolbars[name],
                              wx.aui.AuiPaneInfo().
                              Name('animationToolbar').Caption(_("Animation Toolbar")).
                              ToolbarPane().Top().
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(True).TopDockable(True).
                              CloseButton(False).Layer(2).Row(1).
                              BestSize((self.toolbars['animationToolbar'].GetBestSize())))
            self.controller.SetAnimationToolbar(self.toolbars['animationToolbar'])
        elif name == 'miscToolbar':
            self.toolbars[name] = MiscToolbar(self)
            self._mgr.AddPane(self.toolbars[name],
                              wx.aui.AuiPaneInfo().
                              Name('miscToolbar').Caption(_("Misc Toolbar")).
                              ToolbarPane().Top().
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(True).TopDockable(True).
                              CloseButton(False).Layer(2).Row(1).
                              BestSize((self.toolbars['miscToolbar'].GetBestSize())))
            self.controller.SetAnimationToolbar(self.toolbars['miscToolbar'])

    def SetAnimations(self, inputs=None, dataType=None):
        """!Set animation data
        
        @param inputs list of lists of raster maps or vector maps,  
               or a space time raster or vector dataset 
  	  @param dataType The type of the input data must be one of 'rast', 'vect', 'strds' or 'strds' 
        """ 
        self.controller.SetAnimations(inputs, dataType) 

    def OnAddAnimation(self, event):
        self.controller.AddAnimation()

    def AddWindow(self, index):
        self.animationPanel.AddWindow(index)

    def RemoveWindow(self, index):
        self.animationPanel.RemoveWindow(index)

    def IsWindowShown(self, index):
        return self.animationPanel.IsWindowShown(index)

    def OnEditAnimation(self, event):
        self.controller.EditAnimations()
        
    def SetSlider(self, name):
        if name == 'nontemporal':
            self._mgr.GetPane('slider_nontemporal').Show()
            self._mgr.GetPane('slider_temporal').Hide()

        elif name == 'temporal':
            self._mgr.GetPane('slider_temporal').Show()
            self._mgr.GetPane('slider_nontemporal').Hide()
        else:
            self._mgr.GetPane('slider_temporal').Hide()
            self._mgr.GetPane('slider_nontemporal').Hide()
        self._mgr.Update()

    def OnPlayForward(self, event):
        self.controller.SetOrientation(Orientation.FORWARD)
        self.controller.StartAnimation()

    def OnPlayBack(self, event):
        self.controller.SetOrientation(Orientation.BACKWARD)
        self.controller.StartAnimation()

    def OnPause(self, event):
        self.controller.PauseAnimation(paused = event.IsChecked())

    def OnStop(self, event):
        self.controller.EndAnimation()

    def OnOneDirectionReplay(self, event):
        if event.IsChecked():
            mode = ReplayMode.REPEAT
        else:
            mode = ReplayMode.ONESHOT
        self.controller.SetReplayMode(mode)

    def OnBothDirectionReplay(self, event):
        if event.IsChecked():
            mode = ReplayMode.REVERSE
        else:
            mode = ReplayMode.ONESHOT
        self.controller.SetReplayMode(mode)

    def OnAdjustSpeed(self, event):
        win = self.dialogs['speed']
        if win:
            win.SetTemporalMode(self.controller.GetTemporalMode())
            win.SetTimeGranularity(self.controller.GetTimeGranularity())
            win.InitTimeSpin(self.controller.GetTimeTick())
            if win.IsShown():
                win.SetFocus()
            else:
                win.Show()
        else: # start
            win = SpeedDialog(self, temporalMode = self.controller.GetTemporalMode(),
                              timeGranularity = self.controller.GetTimeGranularity(),
                              initialSpeed = self.controller.timeTick)
            self.dialogs['speed'] = win
            win.speedChanged.connect(self.ChangeSpeed)
            win.Show()

    def ChangeSpeed(self, ms):
        self.controller.timeTick = ms

    def Reload(self, event):
        self.controller.Reload()

    def OnExportAnimation(self, event):
        self.controller.Export()

    def FrameSizeChanged(self, event, provider, sizeMethod):
        provider.WindowSizeChanged(*sizeMethod())
        if self.animationPanel.shown:
            self.SetStatusText(_("Window size has changed, rerender maps if needed"))
        event.Skip()
                     
                     
    def OnHelp(self, event):
        RunCommand('g.manual',
                   quiet = True,
                   entry = 'wxGUI.animation')

    def OnCloseWindow(self, event):
        self.Destroy()

    def __del__(self):
        if hasattr(self, 'controller') and hasattr(self.controller, 'timer'):
            if self.controller.timer.IsRunning():
                self.controller.timer.Stop()


class AnimationsPanel(wx.Panel):
    def __init__(self, parent, windows, initialCount = 4):
        wx.Panel.__init__(self, parent, id = wx.ID_ANY, style = wx.NO_BORDER)
        self.shown = []
        self.count = initialCount
        self.mainSizer = wx.FlexGridSizer(rows = 2, hgap = 0, vgap = 0)
        for i in range(initialCount):
            w = AnimationWindow(self)
            windows.append(w)
            self.mainSizer.Add(item = w, proportion = 1, flag = wx.EXPAND)

        self.mainSizer.AddGrowableCol(0)
        self.mainSizer.AddGrowableCol(1)
        self.mainSizer.AddGrowableRow(0)
        self.mainSizer.AddGrowableRow(1)
        self.windows = windows
        self.SetSizerAndFit(self.mainSizer)

        for i in range(initialCount):
            self.mainSizer.Hide(windows[i])
        self.Layout()


    def AddWindow(self, index):
        if len(self.shown) == self.count:
            return
        self.mainSizer.Show(self.windows[index])
        self.shown.append(index)
        self.Layout()

    def RemoveWindow(self, index):
        if len(self.shown) == 0:
            return
        self.mainSizer.Hide(self.windows[index])
        self.shown.remove(index)
        self.Layout()

    def IsWindowShown(self, index):
        return self.mainSizer.IsShown(self.windows[index])


class AnimationSliderBase(wx.Panel):
    def __init__(self, parent):
        wx.Panel.__init__(self, parent = parent, id = wx.ID_ANY)
        self.label1 = wx.StaticText(self, id = wx.ID_ANY)
        self.slider = wx.Slider(self, id = wx.ID_ANY, style = wx.SL_HORIZONTAL)
        self.indexField = wx.TextCtrl(self, id = wx.ID_ANY, size = (40, -1),
                                      style = wx.TE_PROCESS_ENTER | wx.TE_RIGHT,
                                      validator = IntegerValidator())

        self.callbackSliderChanging = None
        self.callbackSliderChanged = None
        self.callbackFrameIndexChanged = None

        self.framesCount = 0

        self.enable = True

        self.slider.Bind(wx.EVT_SPIN, self.OnSliderChanging)
        self.slider.Bind(wx.EVT_SCROLL_THUMBRELEASE, self.OnSliderChanged)
        self.indexField.Bind(wx.EVT_TEXT_ENTER, self.OnFrameIndexChanged)

    def UpdateFrame(self, index):
        self._updateFrameIndex(index)
        if not self.enable:
            return

        self.slider.SetValue(index)

    def _updateFrameIndex(self, index):
        raise NotImplementedError

    def OnFrameIndexChanged(self, event):
        self._onFrameIndexChanged(event)

    def SetFrames(self, frames):
        self._setFrames(frames)
        
    def _setFrames(self, frames):
        raise NotImplementedError

    def SetCallbackSliderChanging(self, callback):
        self.callbackSliderChanging = callback

    def SetCallbackSliderChanged(self, callback):
        self.callbackSliderChanged = callback

    def SetCallbackFrameIndexChanged(self, callback):
        self.callbackFrameIndexChanged = callback

    def EnableSlider(self, enable = True):
        self.enable = enable
        self.slider.Enable(enable)
        self.indexField.Enable(enable)

    def OnSliderChanging(self, event):
        self.callbackSliderChanging(event.GetInt())

    def OnSliderChanged(self, event):
        self.callbackSliderChanged()

    def _onFrameIndexChanged(self, event):
        index = self.indexField.GetValue()
        index = self._validate(index)
        if index is not None:
            self.slider.SetValue(index)
            self.callbackFrameIndexChanged(index)

    def _validate(self, index):
        try:
            index = int(index)
        except ValueError:
            index = self.slider.GetValue()
            self.indexField.SetValue(str(index + 1))
            return None
        start, end = self.slider.GetRange()
        index -= 1
        if index > end:
            index = end
            self.indexField.SetValue(str(end + 1))
        elif index < start:
            index = start
            self.indexField.SetValue(str(start + 1))

        return index


class SimpleAnimationSlider(AnimationSliderBase):
    def __init__(self, parent):
        AnimationSliderBase.__init__(self, parent)

        self._setLabel()
        self._doLayout()

    def _doLayout(self):
        hbox = wx.BoxSizer(wx.HORIZONTAL)
        hbox.Add(item = self.indexField, proportion = 0,
                 flag = wx.ALIGN_CENTER | wx.LEFT, border = 5)
        hbox.Add(item = self.label1, proportion = 0, 
                 flag = wx.ALIGN_CENTER | wx.LEFT | wx.RIGHT, border = 5)
        hbox.Add(item = self.slider, proportion = 1, flag = wx.ALIGN_CENTER| wx.EXPAND, border = 0)
        self.SetSizerAndFit(hbox)

    def _setFrames(self, count):
        self.framesCount = count
        self.slider.SetRange(0, self.framesCount - 1)
        self._setLabel()

    def _setLabel(self):
        label = "/ %(framesCount)s" % {'framesCount': self.framesCount}
        self.label1.SetLabel(label)
        self.Layout()

    def _updateFrameIndex(self, index):
        self.indexField.SetValue(str(index + 1))


class TimeAnimationSlider(AnimationSliderBase):
    def __init__(self, parent):
        AnimationSliderBase.__init__(self, parent)
        self.timeLabels = []
        self.label2 = wx.StaticText(self, id = wx.ID_ANY)
        self.label3 = wx.StaticText(self, id = wx.ID_ANY)
        self.label2Length = 0
        self.temporalType = TemporalType.RELATIVE

        self._setLabel()
        self._doLayout()

    def _doLayout(self):
        vbox = wx.BoxSizer(wx.VERTICAL)
        hbox = wx.BoxSizer(wx.HORIZONTAL)
        hbox.Add(item = self.label1, proportion = 0, 
                 flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        hbox.AddStretchSpacer()
        hbox.Add(item = self.indexField, proportion = 0, 
                 flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        hbox.Add(item = self.label2, proportion = 0, 
                 flag = wx.ALIGN_CENTER_VERTICAL | wx.LEFT, border = 3)
        hbox.AddStretchSpacer()
        hbox.Add(item = self.label3, proportion = 0, 
                 flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        vbox.Add(item = hbox, proportion = 0, flag = wx.EXPAND, border = 0)

        hbox = wx.BoxSizer(wx.HORIZONTAL)
        hbox.Add(item = self.slider, proportion = 1, flag = wx.ALIGN_CENTER | wx.EXPAND, border = 0)
        vbox.Add(item = hbox, proportion = 0, flag = wx.EXPAND, border = 0)

        self._setTemporalType()
        self.SetSizerAndFit(vbox)

    def _setTemporalType(self):
        sizer = self.indexField.GetContainingSizer()
        # sizer.Show(self.indexField, False) # TODO: what to do?
        sizer.Show(self.indexField, self.temporalType == TemporalType.RELATIVE)
        self.Layout()

    def SetTemporalType(self, mode):
        self.temporalType = mode
        self._setTemporalType()

    def _setFrames(self, timeLabels):
        self.timeLabels = timeLabels
        self.framesCount = len(timeLabels)
        self.slider.SetRange(0, self.framesCount - 1)
        self._setLabel()

    def _setLabel(self):
        if self.timeLabels:
            if self.temporalType == TemporalType.ABSOLUTE:
                start = self.timeLabels[0][0]
                self.label1.SetLabel(start)
                if self.timeLabels[-1][1]:
                    end = self.timeLabels[-1][1]
                else:
                    end = self.timeLabels[-1][0]
                self.label3.SetLabel(end)
            else:
                unit = self.timeLabels[0][2]
                start = self.timeLabels[0][0]
                self.label1.SetLabel(start)
                if self.timeLabels[-1][1]:
                    end = self.timeLabels[-1][1]
                else:
                    end = self.timeLabels[-1][0]
                end = "%(end)s %(unit)s" % {'end': end, 'unit': unit}
                self.label3.SetLabel(end)

            self.label2Length = len(start)
            self._updateFrameIndex(0)

        else:
            self.label1.SetLabel("")
            self.label2.SetLabel("")
            self.label3.SetLabel("")
        self.Layout()

    def _updateFrameIndex(self, index):
        start = self.timeLabels[index][0]
        if self.timeLabels[index][1]: # interval
            if self.temporalType == TemporalType.ABSOLUTE:
                label = _("%(from)s %(dash)s %(to)s") % {'from': start, 'dash': u"\u2013", 'to': self.timeLabels[index][1]}
            else:
                label = _("to %(to)s") % {'to': self.timeLabels[index][1]}
        else:
            if self.temporalType == TemporalType.ABSOLUTE:
                label = start
            else:
                label = ''
        self.label2.SetLabel(label)
        if self.temporalType == TemporalType.RELATIVE:
            self.indexField.SetValue(start)
        if len(label) != self.label2Length:
            self.label2Length = len(label)
            self.Layout()

def test():
    import gettext
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)

    import grass.script as grass

    app = wx.PySimpleApp()
    wx.InitAllImageHandlers()

    frame = AnimationFrame(parent=None)
    frame.SetAnimations(inputs=None, dataType=None)

    frame.Show()
    app.MainLoop()

if __name__ == '__main__':

    test()
