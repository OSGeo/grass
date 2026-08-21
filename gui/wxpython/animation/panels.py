"""
@package animation.panels

@brief Animation Panel

Classes:
 - panels::AnimationToolPanel
 - panels::AnimationsPanel
 - panels::AnimationSliderBase
 - panels::SimpleAnimationSlider
 - panels::TimeAnimationSlider

(C) 2026 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.
"""

import wx
import wx.aui

import grass.script as gs
import grass.temporal as tgis
from grass.exceptions import FatalError
from main_window.page import MainPageBase
from core.gcmd import RunCommand, GError, GWarning
from core.layerlist import LayerList
from gui_core.widgets import IntegerValidator
from gui_core.wrap import StaticText, TextCtrl, Slider

from animation.data import AnimLayer
from animation.mapwindow import AnimationWindow
from animation.provider import BitmapProvider, BitmapPool, MapFilesPool, CleanUp
from animation.controller import AnimationController
from animation.anim import Animation
from animation.toolbars import MainToolbar, AnimationToolbar, MiscToolbar
from animation.dialogs import SpeedDialog, PreferencesDialog
from animation.utils import Orientation, ReplayMode, TemporalType

MAX_COUNT = 4

gs.set_raise_on_error(True)


class AnimationToolPanel(wx.Panel, MainPageBase):
    def __init__(
        self,
        parent,
        giface,
        id=wx.ID_ANY,
        statusbar=None,
        dockable=False,
        rasters=None,
        timeseries=None,
        **kwargs,
    ):
        self.parent = parent
        self._giface = giface
        self.statusbar = statusbar

        wx.Panel.__init__(self, parent=parent, id=id, **kwargs)
        MainPageBase.__init__(self, dockable)
        self.SetName("Animation")

        # Make sure the temporal database exists
        try:
            tgis.init()
        except FatalError as e:
            GWarning(parent=self, message=str(e))

        # Create temporal directory, deleted on close via CleanUp
        self.TMP_DIR = gs.tempdir()

        self.animations = [Animation() for _ in range(MAX_COUNT)]
        self.windows = []
        self.animationPanel = AnimationsPanel(
            self, self.windows, initialCount=MAX_COUNT
        )
        bitmapPool = BitmapPool()
        mapFilesPool = MapFilesPool()

        self._progressDlg = None
        self._progressDlgMax = None

        self.provider = BitmapProvider(
            bitmapPool=bitmapPool, mapFilesPool=mapFilesPool, tempDir=self.TMP_DIR
        )
        self.animationSliders = {}
        self.animationSliders["nontemporal"] = SimpleAnimationSlider(self)
        self.animationSliders["temporal"] = TimeAnimationSlider(self)
        self.controller = AnimationController(
            frame=self,
            sliders=self.animationSliders,
            animations=self.animations,
            mapwindows=self.windows,
            provider=self.provider,
            bitmapPool=bitmapPool,
            mapFilesPool=mapFilesPool,
        )
        for win in self.windows:
            win.Bind(wx.EVT_SIZE, self.FrameSizeChanged)

        # Connect provider signals once, not per window
        self.provider.mapsLoaded.connect(lambda: self.SetStatusText(""))
        self.provider.renderingStarted.connect(self._showRenderingProgress)
        self.provider.renderingContinues.connect(self._updateProgress)
        self.provider.renderingFinished.connect(self._closeProgress)
        self.provider.compositionStarted.connect(self._showRenderingProgress)
        self.provider.compositionContinues.connect(self._updateProgress)
        self.provider.compositionFinished.connect(self._closeProgress)

        self._mgr = wx.aui.AuiManager(self)

        # toolbars
        self.toolbars = {}
        self._addToolbars()
        self._addPanes()
        self._mgr.Update()

        self.dialogs = {}
        self.dialogs["speed"] = None
        self.dialogs["preferences"] = None

        self.Bind(wx.EVT_SIZE, self.OnSize)

    def OnSize(self, event):
        """Lay the managed panes out again when the panel is resized.

        The panes do not follow on their own when the notebook tab is dragged.
        The check is needed because a resize can still arrive after
        OnCloseWindow has called UnInit() on the manager.
        """
        if self._mgr.GetManagedWindow():
            self._mgr.Update()
        event.Skip()

    def SetStatusText(self, text):
        if self.statusbar:
            self.statusbar.SetStatusText(text)
        elif hasattr(self.parent, "SetStatusText"):
            self.parent.SetStatusText(text)

    def _addPanes(self):
        self._mgr.AddPane(
            self.animationPanel,
            wx.aui.AuiPaneInfo()
            .CentrePane()
            .Name("animPanel")
            .CaptionVisible(False)
            .PaneBorder(False)
            .Floatable(False)
            .BestSize((-1, -1))
            .CloseButton(False)
            .DestroyOnClose(True)
            .Layer(0),
        )
        for name, slider in self.animationSliders.items():
            self._mgr.AddPane(
                slider,
                wx.aui.AuiPaneInfo()
                .PaneBorder(False)
                .Name("slider_" + name)
                .Layer(1)
                .CaptionVisible(False)
                .BestSize(slider.GetBestSize())
                .DestroyOnClose(True)
                .CloseButton(False)
                .Bottom(),
            )
            self._mgr.GetPane("slider_" + name).Hide()

    def _addToolbars(self):
        self.toolbars["mainToolbar"] = MainToolbar(self)
        self._mgr.AddPane(
            self.toolbars["mainToolbar"],
            wx.aui.AuiPaneInfo()
            .Name("mainToolbar")
            .Caption(_("Main Toolbar"))
            .ToolbarPane()
            .Top()
            .LeftDockable(False)
            .RightDockable(False)
            .BottomDockable(True)
            .TopDockable(True)
            .CloseButton(False)
            .Layer(2)
            .Row(1)
            .Position(0)
            .BestSize(self.toolbars["mainToolbar"].GetBestSize()),
        )

        self.toolbars["animationToolbar"] = AnimationToolbar(self)
        self._mgr.AddPane(
            self.toolbars["animationToolbar"],
            wx.aui.AuiPaneInfo()
            .Name("animationToolbar")
            .Caption(_("Animation Toolbar"))
            .ToolbarPane()
            .Top()
            .LeftDockable(False)
            .RightDockable(False)
            .BottomDockable(True)
            .TopDockable(True)
            .CloseButton(False)
            .Layer(2)
            .Row(1)
            .Position(1)
            .BestSize(self.toolbars["animationToolbar"].GetBestSize()),
        )
        self.controller.SetAnimationToolbar(self.toolbars["animationToolbar"])

        self.toolbars["miscToolbar"] = MiscToolbar(self)
        self._mgr.AddPane(
            self.toolbars["miscToolbar"],
            wx.aui.AuiPaneInfo()
            .Name("miscToolbar")
            .Caption(_("Misc Toolbar"))
            .ToolbarPane()
            .Top()
            .LeftDockable(False)
            .RightDockable(False)
            .BottomDockable(True)
            .TopDockable(True)
            .CloseButton(False)
            .Layer(2)
            .Row(1)
            .Position(2)
            .BestSize(self.toolbars["miscToolbar"].GetBestSize()),
        )

    def SetAnimations(self, layerLists):
        """Set animation data

        :param layerLists: list of layerLists
        """
        self.controller.SetAnimations(layerLists)

    def AnimateStds(self, name, stds_type):
        """Animate a space time dataset, replacing the current animations

        :param name: name of the dataset including the mapset
        :param stds_type: type of the dataset, 'strds' or 'stvds'
        """
        layer = AnimLayer()
        layer.mapType = stds_type
        try:
            layer.name = name
        except ValueError as e:
            GError(parent=self, message=str(e), showTraceback=False)
            return

        layer.cmd = [{"strds": "d.rast", "stvds": "d.vect"}[stds_type], "map="]

        layerList = LayerList()
        layerList.AddLayer(layer)
        self.SetAnimations([layerList] + [None] * (MAX_COUNT - 1))

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
        if name == "nontemporal":
            self._mgr.GetPane("slider_nontemporal").Show()
            self._mgr.GetPane("slider_temporal").Hide()
        elif name == "temporal":
            self._mgr.GetPane("slider_temporal").Show()
            self._mgr.GetPane("slider_nontemporal").Hide()
        else:
            self._mgr.GetPane("slider_temporal").Hide()
            self._mgr.GetPane("slider_nontemporal").Hide()
        self._mgr.Update()

    def OnPlayForward(self, event):
        self.controller.SetOrientation(Orientation.FORWARD)
        self.controller.StartAnimation()

    def OnPlayBack(self, event):
        self.controller.SetOrientation(Orientation.BACKWARD)
        self.controller.StartAnimation()

    def OnPause(self, event):
        self.controller.PauseAnimation(paused=event.IsChecked())

    def OnStop(self, event):
        self.controller.EndAnimation()

    def OnOneDirectionReplay(self, event):
        mode = ReplayMode.REPEAT if event.IsChecked() else ReplayMode.ONESHOT
        self.controller.SetReplayMode(mode)

    def OnBothDirectionReplay(self, event):
        mode = ReplayMode.REVERSE if event.IsChecked() else ReplayMode.ONESHOT
        self.controller.SetReplayMode(mode)

    def OnAdjustSpeed(self, event):
        win = self.dialogs["speed"]
        if win:
            win.SetTemporalMode(self.controller.GetTemporalMode())
            win.SetTimeGranularity(self.controller.GetTimeGranularity())
            win.InitTimeSpin(self.controller.GetTimeTick())
            if win.IsShown():
                win.SetFocus()
            else:
                win.Show()
        else:
            win = SpeedDialog(
                self,
                temporalMode=self.controller.GetTemporalMode(),
                timeGranularity=self.controller.GetTimeGranularity(),
                initialSpeed=self.controller.timeTick,
            )
            win.CenterOnParent()
            self.dialogs["speed"] = win
            win.speedChanged.connect(self.ChangeSpeed)
            win.Show()

    def ChangeSpeed(self, ms):
        self.controller.timeTick = ms

    def Reload(self, event):
        self.controller.Reload()

    def _showRenderingProgress(self, count):
        self._progressDlg = wx.ProgressDialog(
            title=_("Loading data"),
            message="Loading data started, please be patient.",
            maximum=count + 1 if count > 0 else 100,
            parent=self,
            style=wx.PD_CAN_ABORT | wx.PD_APP_MODAL | wx.PD_SMOOTH,
        )
        self._progressDlgMax = count

    def _updateProgress(self, current, text):
        text += _(" ({c} out of {p})").format(c=current, p=self._progressDlgMax)
        keepGoing, skip = self._progressDlg.Update(current, text)
        if not keepGoing:
            self.provider.RequestStopRendering()

    def _closeProgress(self):
        if self._progressDlg:
            wx.CallAfter(self._progressDlg.Destroy)
            self._progressDlg = None

    def OnExportAnimation(self, event):
        self.controller.Export()

    def FrameSizeChanged(self, event):
        maxWidth = maxHeight = 0
        for win in self.windows:
            w, h = win.GetClientSize()
            if w >= maxWidth and h >= maxHeight:
                maxWidth, maxHeight = w, h
        self.provider.WindowSizeChanged(maxWidth, maxHeight)
        event.Skip()

    def OnPreferences(self, event):
        if not self.dialogs["preferences"]:
            dlg = PreferencesDialog(parent=self, giface=self._giface)
            self.dialogs["preferences"] = dlg
            dlg.formatChanged.connect(lambda: self.controller.UpdateAnimations())
            dlg.CenterOnParent()

        self.dialogs["preferences"].Show()

    def OnHelp(self, event):
        RunCommand("g.manual", quiet=True, entry="wxGUI.animation")

    def OnCloseWindow(self, event=None):
        """Clean up resources and close the panel."""
        if self.controller.timer.IsRunning():
            self.controller.timer.Stop()
        CleanUp(self.TMP_DIR)()
        self._mgr.UnInit()
        self._onCloseWindow(event)

    def __del__(self):
        """Remove the temporary directory if the panel was never closed.

        It might not be called, therefore we try to clean it all in
        OnCloseWindow. The attributes are checked because the panel may not
        have been fully constructed.
        """
        if hasattr(self, "controller") and hasattr(self.controller, "timer"):
            if self.controller.timer.IsRunning():
                self.controller.timer.Stop()
        if hasattr(self, "TMP_DIR"):
            CleanUp(self.TMP_DIR)()


class AnimationsPanel(wx.Panel):
    def __init__(self, parent, windows, initialCount=4):
        wx.Panel.__init__(self, parent, id=wx.ID_ANY, style=wx.NO_BORDER)
        self.shown = []
        self.count = initialCount
        self.mainSizer = wx.FlexGridSizer(cols=2, hgap=0, vgap=0)
        for i in range(initialCount):
            w = AnimationWindow(self)
            windows.append(w)
            self.mainSizer.Add(w, proportion=1, flag=wx.EXPAND)

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
        wx.Panel.__init__(self, parent=parent, id=wx.ID_ANY)
        self.label1 = StaticText(self, id=wx.ID_ANY)
        self.slider = Slider(self, id=wx.ID_ANY, style=wx.SL_HORIZONTAL)
        self.indexField = TextCtrl(
            self,
            id=wx.ID_ANY,
            size=(40, -1),
            style=wx.TE_PROCESS_ENTER | wx.TE_RIGHT,
            validator=IntegerValidator(),
        )

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

    def EnableSlider(self, enable=True):
        if enable and self.framesCount <= 1:
            enable = False  # we don't want to enable it
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
        hbox.Add(
            self.indexField, proportion=0, flag=wx.ALIGN_CENTER | wx.LEFT, border=5
        )
        hbox.Add(
            self.label1,
            proportion=0,
            flag=wx.ALIGN_CENTER | wx.LEFT | wx.RIGHT,
            border=5,
        )
        hbox.Add(self.slider, proportion=1, flag=wx.EXPAND, border=0)
        self.SetSizerAndFit(hbox)

    def _setFrames(self, count):
        self.framesCount = count
        if self.framesCount > 1:
            self.slider.SetRange(0, self.framesCount - 1)
            self.EnableSlider(True)
        else:
            self.EnableSlider(False)
        self._setLabel()

    def _setLabel(self):
        label = "/ %(framesCount)s" % {"framesCount": self.framesCount}
        self.label1.SetLabel(label)
        self.Layout()

    def _updateFrameIndex(self, index):
        self.indexField.SetValue(str(index + 1))


class TimeAnimationSlider(AnimationSliderBase):
    def __init__(self, parent):
        AnimationSliderBase.__init__(self, parent)
        self.timeLabels = []
        self.label2 = StaticText(self, id=wx.ID_ANY)
        self.label3 = StaticText(self, id=wx.ID_ANY)
        self.label2Length = 0
        self.temporalType = TemporalType.RELATIVE

        self._setLabel()
        self._doLayout()

    def _doLayout(self):
        vbox = wx.BoxSizer(wx.VERTICAL)
        hbox = wx.BoxSizer(wx.HORIZONTAL)
        hbox.Add(self.label1, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL, border=0)
        hbox.AddStretchSpacer()
        hbox.Add(self.indexField, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL, border=0)
        hbox.Add(
            self.label2, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL | wx.LEFT, border=3
        )
        hbox.AddStretchSpacer()
        hbox.Add(self.label3, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL, border=0)
        vbox.Add(hbox, proportion=0, flag=wx.EXPAND, border=0)

        hbox = wx.BoxSizer(wx.HORIZONTAL)
        hbox.Add(self.slider, proportion=1, flag=wx.EXPAND, border=0)
        vbox.Add(hbox, proportion=0, flag=wx.EXPAND, border=0)

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
        if self.framesCount > 1:
            self.slider.SetRange(0, self.framesCount - 1)
            self.EnableSlider(True)
        else:
            self.EnableSlider(False)
        self._setLabel()
        # TODO: fix setting index values, until then:
        self.indexField.Disable()

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
                end = "%(end)s %(unit)s" % {"end": end, "unit": unit}
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
        if self.timeLabels[index][1]:  # interval
            if self.temporalType == TemporalType.ABSOLUTE:
                label = _("%(from)s %(dash)s %(to)s") % {
                    "from": start,
                    "dash": "\u2013",
                    "to": self.timeLabels[index][1],
                }
            else:
                label = _("to %(to)s") % {"to": self.timeLabels[index][1]}
        else:
            label = start if self.temporalType == TemporalType.ABSOLUTE else ""
        self.label2.SetLabel(label)
        if self.temporalType == TemporalType.RELATIVE:
            self.indexField.SetValue(start)
        if len(label) != self.label2Length:
            self.label2Length = len(label)
            self.Layout()
