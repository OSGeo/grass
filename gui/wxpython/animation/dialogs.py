"""!
@package animation.dialogs

@brief Dialogs for animation management, changing speed of animation

Classes:
 - dialogs::SpeedDialog
 - dialogs::InputDialog
 - dialogs::EditDialog
 - dialogs::ExportDialog
 - dialogs::AnimSimpleLayerManager
 - dialogs::AddTemporalLayerDialog


(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""
import os
import sys
import wx
import copy
import datetime
import wx.lib.filebrowsebutton as filebrowse

if __name__ == '__main__':
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))

from core.gcmd import GMessage, GError, GException
from core import globalvar
from gui_core.dialogs import MapLayersDialog, GetImageHandlers
from gui_core.forms import GUI
from core.settings import UserSettings
from core.utils import _
from gui_core.gselect import Select

from animation.utils import TemporalMode, getRegisteredMaps
from animation.data import AnimationData, AnimLayer
from animation.toolbars import AnimSimpleLmgrToolbar, SIMPLE_LMGR_STDS
from gui_core.simplelmgr import SimpleLayerManager, \
    SIMPLE_LMGR_RASTER, SIMPLE_LMGR_VECTOR, SIMPLE_LMGR_TB_TOP

from grass.pydispatch.signal import Signal
import grass.script.core as gcore


class SpeedDialog(wx.Dialog):
    def __init__(self, parent, title=_("Adjust speed of animation"),
                 temporalMode=None, minimumDuration=20, timeGranularity=None,
                 initialSpeed=200):
        wx.Dialog.__init__(self, parent=parent, id=wx.ID_ANY, title=title,
                           style=wx.DEFAULT_DIALOG_STYLE)
        # signal emitted when speed has changed; has attribute 'ms'
        self.speedChanged = Signal('SpeedDialog.speedChanged')
        self.minimumDuration = minimumDuration
        # self.framesCount = framesCount
        self.defaultSpeed = initialSpeed
        self.lastAppliedValue = self.defaultSpeed
        self.lastAppliedValueTemp = self.defaultSpeed

        self._layout()

        self.temporalMode = temporalMode
        self.timeGranularity = timeGranularity

        self._fillUnitChoice(self.choiceUnits)
        self.InitTimeSpin(self.defaultSpeed)

    def SetTimeGranularity(self, gran):
        self._timeGranularity = gran

    def GetTimeGranularity(self):
        return self._timeGranularity

    timeGranularity = property(fset=SetTimeGranularity, fget=GetTimeGranularity)

    def SetTemporalMode(self, mode):
        self._temporalMode = mode
        self._setTemporalMode()

    def GetTemporalMode(self):
        return self._temporalMode

    temporalMode = property(fset=SetTemporalMode, fget=GetTemporalMode)

    def _layout(self):
        """!Layout window"""
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        #
        # simple mode
        #
        self.nontemporalBox = wx.StaticBox(parent=self, id=wx.ID_ANY,
                                           label=' %s ' % _("Simple mode"))
        box = wx.StaticBoxSizer(self.nontemporalBox, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(hgap=5, vgap=5)

        labelDuration = wx.StaticText(self, id=wx.ID_ANY, label=_("Frame duration:"))
        labelUnits = wx.StaticText(self, id=wx.ID_ANY, label=_("ms"))
        self.spinDuration = wx.SpinCtrl(self, id=wx.ID_ANY, min=self.minimumDuration,
                                        max=10000, initial=self.defaultSpeed)
        # TODO total time

        gridSizer.Add(item=labelDuration, pos=(0, 0), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)
        gridSizer.Add(item=self.spinDuration, pos=(0, 1), flag = wx.ALIGN_CENTER)
        gridSizer.Add(item=labelUnits, pos=(0, 2), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)
        gridSizer.AddGrowableCol(0)

        box.Add(item=gridSizer, proportion=1, border=5, flag=wx.ALL | wx.EXPAND)
        self.nontemporalSizer = gridSizer
        mainSizer.Add(item=box, proportion=0, flag=wx.EXPAND | wx.ALL, border=5)
        #
        # temporal mode
        #
        self.temporalBox = wx.StaticBox(parent=self, id=wx.ID_ANY,
                                        label=' %s ' % _("Temporal mode"))
        box = wx.StaticBoxSizer(self.temporalBox, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(hgap=5, vgap=5)

        labelTimeUnit = wx.StaticText(self, id=wx.ID_ANY, label=_("Time unit:"))
        labelDuration = wx.StaticText(self, id=wx.ID_ANY, label=_("Duration of time unit:"))
        labelUnits = wx.StaticText(self, id=wx.ID_ANY, label=_("ms"))
        self.spinDurationTemp = wx.SpinCtrl(self, id=wx.ID_ANY, min=self.minimumDuration,
                                            max=10000, initial=self.defaultSpeed)
        self.choiceUnits = wx.Choice(self, id=wx.ID_ANY)

        # TODO total time

        gridSizer.Add(item=labelTimeUnit, pos=(0, 0), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)
        gridSizer.Add(item=self.choiceUnits, pos=(0, 1), flag = wx.ALIGN_CENTER | wx.EXPAND)
        gridSizer.Add(item=labelDuration, pos=(1, 0), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)
        gridSizer.Add(item=self.spinDurationTemp, pos=(1, 1), flag = wx.ALIGN_CENTER | wx.EXPAND)
        gridSizer.Add(item=labelUnits, pos=(1, 2), flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT)
        gridSizer.AddGrowableCol(1)

        self.temporalSizer = gridSizer
        box.Add(item=gridSizer, proportion=1, border=5, flag=wx.ALL | wx.EXPAND)
        mainSizer.Add(item=box, proportion=0, flag=wx.EXPAND | wx.ALL, border=5)

        self.btnOk = wx.Button(self, wx.ID_OK)
        self.btnApply = wx.Button(self, wx.ID_APPLY)
        self.btnCancel = wx.Button(self, wx.ID_CANCEL)
        self.btnOk.SetDefault()

        self.btnOk.Bind(wx.EVT_BUTTON, self.OnOk)
        self.btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        self.btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        self.Bind(wx.EVT_CLOSE, self.OnCancel)
        # button sizer
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(self.btnOk)
        btnStdSizer.AddButton(self.btnApply)
        btnStdSizer.AddButton(self.btnCancel)
        btnStdSizer.Realize()

        mainSizer.Add(item=btnStdSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_RIGHT, border=5)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

    def _setTemporalMode(self):
        self.nontemporalBox.Enable(self.temporalMode == TemporalMode.NONTEMPORAL)
        self.temporalBox.Enable(self.temporalMode == TemporalMode.TEMPORAL)
        for child in self.temporalSizer.GetChildren():
            child.GetWindow().Enable(self.temporalMode == TemporalMode.TEMPORAL)
        for child in self.nontemporalSizer.GetChildren():
            child.GetWindow().Enable(self.temporalMode == TemporalMode.NONTEMPORAL)

        self.Layout()

    def _fillUnitChoice(self, choiceWidget):
        timeUnitsChoice = [_("year"), _("month"), _("day"), _("hour"), _("minute"), _("second")]
        timeUnits = ["years", "months", "days", "hours", "minutes", "seconds"]
        for item, cdata in zip(timeUnitsChoice, timeUnits):
            choiceWidget.Append(item, cdata)

        if self.temporalMode == TemporalMode.TEMPORAL:
            unit = self.timeGranularity[1]
            try:
                index = timeUnits.index(unit)
            except ValueError:
                index = 0
            choiceWidget.SetSelection(index)
        else:
            choiceWidget.SetSelection(0)

    def OnOk(self, event):
        self._apply()
        self.OnCancel(None)

    def OnApply(self, event):
        self._apply()

    def OnCancel(self, event):
        self.spinDuration.SetValue(self.lastAppliedValue)
        self.spinDurationTemp.SetValue(self.lastAppliedValueTemp)
        self.Hide()

    def InitTimeSpin(self, timeTick):
        if self.temporalMode == TemporalMode.TEMPORAL:
            index = self.choiceUnits.GetSelection()
            unit = self.choiceUnits.GetClientData(index)
            delta = self._timedelta(unit=unit, number=1)
            seconds1 = self._total_seconds(delta)

            number, unit = self.timeGranularity
            number = float(number)
            delta = self._timedelta(unit=unit, number=number)
            seconds2 = self._total_seconds(delta)
            value = timeTick
            ms = value * seconds1 / float(seconds2)
            self.spinDurationTemp.SetValue(ms)
        else:
            self.spinDuration.SetValue(timeTick)

    def _apply(self):
        if self.temporalMode == TemporalMode.NONTEMPORAL:
            ms = self.spinDuration.GetValue()
            self.lastAppliedValue = self.spinDuration.GetValue()
        elif self.temporalMode == TemporalMode.TEMPORAL:
            index = self.choiceUnits.GetSelection()
            unit = self.choiceUnits.GetClientData(index)
            delta = self._timedelta(unit=unit, number=1)
            seconds1 = self._total_seconds(delta)

            number, unit = self.timeGranularity
            number = float(number)
            delta = self._timedelta(unit=unit, number=number)
            seconds2 = self._total_seconds(delta)

            value = self.spinDurationTemp.GetValue()
            ms = value * seconds2 / float(seconds1)
            if ms < self.minimumDuration:
                GMessage(parent=self, message=_("Animation speed is too high."))
                return
            self.lastAppliedValueTemp = self.spinDurationTemp.GetValue()
        else:
            return

        self.speedChanged.emit(ms=ms)

    def _timedelta(self, unit, number):
        if unit in "years":
            delta = datetime.timedelta(days=365.25 * number)
        elif unit in "months":
            delta = datetime.timedelta(days=30.4375 * number)  # 365.25/12
        elif unit in "days":
            delta = datetime.timedelta(days=1 * number)
        elif unit in "hours":
            delta = datetime.timedelta(hours=1 * number)
        elif unit in "minutes":
            delta = datetime.timedelta(minutes=1 * number)
        elif unit in "seconds":
            delta = datetime.timedelta(seconds=1 * number)

        return delta

    def _total_seconds(self, delta):
        """!timedelta.total_seconds is new in version 2.7.
        """
        return delta.seconds + delta.days * 24 * 3600


class InputDialog(wx.Dialog):
    def __init__(self, parent, mode, animationData):
        wx.Dialog.__init__(self, parent=parent, id=wx.ID_ANY,
                           style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER)
        if mode == 'add':
            self.SetTitle(_("Add new animation"))
        elif mode == 'edit':
            self.SetTitle(_("Edit animation"))

        self.animationData = animationData
        self._tmpLegendCmd = None

        self._layout()
        self.OnViewMode(event=None)

    def _layout(self):
        mainSizer = wx.BoxSizer(wx.VERTICAL)

        self.windowChoice = wx.Choice(self, id=wx.ID_ANY,
                                      choices=[_("top left"), _("top right"),
                                               _("bottom left"), _("bottom right")])
        self.windowChoice.SetSelection(self.animationData.windowIndex)

        self.nameCtrl = wx.TextCtrl(self, id=wx.ID_ANY, value=self.animationData.name)

        self.nDChoice = wx.Choice(self, id=wx.ID_ANY)
        mode = self.animationData.viewMode
        index = 0
        for i, (viewMode, viewModeName) in enumerate(self.animationData.viewModes):
            self.nDChoice.Append(viewModeName, clientData=viewMode)
            if mode == viewMode:
                index = i

        self.nDChoice.SetSelection(index)
        self.nDChoice.SetToolTipString(_("Select 2D or 3D view"))
        self.nDChoice.Bind(wx.EVT_CHOICE, self.OnViewMode)

        gridSizer = wx.FlexGridSizer(cols=2, hgap=5, vgap=5)
        gridSizer.Add(item=wx.StaticText(self, id=wx.ID_ANY, label=_("Name:")),
                      flag=wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item=self.nameCtrl, proportion=1, flag=wx.EXPAND)
        gridSizer.Add(item=wx.StaticText(self, id=wx.ID_ANY, label=_("Window position:")),
                      flag=wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item=self.windowChoice, proportion=1, flag=wx.ALIGN_RIGHT)
        gridSizer.Add(item=wx.StaticText(self, id=wx.ID_ANY, label=_("View mode:")),
                      flag=wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item=self.nDChoice, proportion=1, flag=wx.ALIGN_RIGHT)
        gridSizer.AddGrowableCol(0, 1)
        gridSizer.AddGrowableCol(1, 1)
        mainSizer.Add(item=gridSizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=5)
        label = _("For 3D animation, please select only one space-time dataset\n"
                  "or one series of map layers.")
        self.warning3DLayers = wx.StaticText(self, label=label)
        self.warning3DLayers.SetForegroundColour(wx.SystemSettings_GetColour(wx.SYS_COLOUR_GRAYTEXT))
        mainSizer.Add(item=self.warning3DLayers, proportion=0, flag=wx.EXPAND | wx.LEFT, border=5)

        self.dataPanel = self._createDataPanel()
        self.threeDPanel = self._create3DPanel()
        mainSizer.Add(item=self.dataPanel, proportion=1, flag=wx.EXPAND | wx.ALL, border=3)
        mainSizer.Add(item=self.threeDPanel, proportion=0, flag=wx.EXPAND | wx.ALL, border=3)

        # buttons
        self.btnOk = wx.Button(self, wx.ID_OK)
        self.btnCancel = wx.Button(self, wx.ID_CANCEL)
        self.btnOk.SetDefault()
        self.btnOk.Bind(wx.EVT_BUTTON, self.OnOk)
        # button sizer
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(self.btnOk)
        btnStdSizer.AddButton(self.btnCancel)
        btnStdSizer.Realize()

        mainSizer.Add(item=btnStdSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_RIGHT, border=5)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

    def _createDataPanel(self):
        panel = wx.Panel(self)
        slmgrSizer = wx.BoxSizer(wx.VERTICAL)
        self._layerList = copy.deepcopy(self.animationData.layerList)
        self.simpleLmgr = AnimSimpleLayerManager(parent=panel,
                                                 layerList=self._layerList,
                                                 modal=True)
        self.simpleLmgr.SetMinSize((globalvar.DIALOG_GSELECT_SIZE[0], 120))
        slmgrSizer.Add(self.simpleLmgr, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)

        self.legend = wx.CheckBox(panel, label=_("Show raster legend"))
        self.legend.SetValue(bool(self.animationData.legendCmd))
        self.legendBtn = wx.Button(panel, label=_("Set options"))
        self.legend.Bind(wx.EVT_CHECKBOX, self.OnLegend)
        self.legendBtn.Bind(wx.EVT_BUTTON, self.OnLegendProperties)

        hbox = wx.BoxSizer(wx.HORIZONTAL)
        hbox.Add(item=self.legend, proportion=1, flag=wx.ALIGN_CENTER_VERTICAL)
        hbox.Add(item=self.legendBtn, proportion=0, flag=wx.LEFT, border=5)
        slmgrSizer.Add(item=hbox, proportion=0, flag=wx.EXPAND | wx.ALL, border=3)

        panel.SetSizerAndFit(slmgrSizer)
        panel.SetAutoLayout(True)

        return panel

    def _create3DPanel(self):
        panel = wx.Panel(self, id=wx.ID_ANY)
        dataStBox = wx.StaticBox(parent=panel, id=wx.ID_ANY,
                                 label=' %s ' % _("3D view parameters"))
        dataBoxSizer = wx.StaticBoxSizer(dataStBox, wx.VERTICAL)

        # workspace file
        self.fileSelector = \
            filebrowse.FileBrowseButton(parent=panel, id=wx.ID_ANY,
                                        size=globalvar.DIALOG_GSELECT_SIZE,
                                        labelText=_("Workspace file:"),
                                        dialogTitle=_("Choose workspace file to "
                                                      "import 3D view parameters"),
                                        buttonText=_('Browse'),
                                        startDirectory=os.getcwd(), fileMode=0,
                                        fileMask="GRASS Workspace File (*.gxw)|*.gxw")
        if self.animationData.workspaceFile:
            self.fileSelector.SetValue(self.animationData.workspaceFile)
        self.paramLabel = wx.StaticText(panel, wx.ID_ANY, label=_("Parameter for animation:"))
        self.paramChoice = wx.Choice(panel, id=wx.ID_ANY, choices=self.animationData.nvizParameters)
        self.paramChoice.SetStringSelection(self.animationData.nvizParameter)

        hbox = wx.BoxSizer(wx.HORIZONTAL)
        hbox.Add(item=self.fileSelector, proportion=1, flag=wx.EXPAND | wx.ALIGN_CENTER)
        dataBoxSizer.Add(item=hbox, proportion=0, flag=wx.EXPAND | wx.ALL, border=3)

        hbox = wx.BoxSizer(wx.HORIZONTAL)
        hbox.Add(item=self.paramLabel, proportion=1, flag=wx.ALIGN_CENTER_VERTICAL)
        hbox.Add(item=self.paramChoice, proportion=1, flag=wx.EXPAND)
        dataBoxSizer.Add(item=hbox, proportion=0, flag=wx.EXPAND | wx.ALL, border=3)

        panel.SetSizerAndFit(dataBoxSizer)
        panel.SetAutoLayout(True)

        return panel

    def OnViewMode(self, event):
        mode = self.nDChoice.GetSelection()
        self.Freeze()
        self.simpleLmgr.Activate3D(mode == 1)
        self.warning3DLayers.Show(mode == 1)
        sizer = self.threeDPanel.GetContainingSizer()
        sizer.Show(self.threeDPanel, mode == 1, True)
        sizer.Layout()
        self.Layout()
        self.Fit()
        self.Thaw()

    def OnLegend(self, event):
        if not self.legend.IsChecked():
            return
        if self._tmpLegendCmd or self.animationData.legendCmd:
            return
        cmd = ['d.legend', 'at=5,50,2,5']
        GUI(parent=self, modal=True).ParseCommand(cmd=cmd,
                                                  completed=(self.GetOptData, '', ''))

    def OnLegendProperties(self, event):
        """!Set options for legend"""
        if self._tmpLegendCmd:
            cmd = self._tmpLegendCmd
        elif self.animationData.legendCmd:
            cmd = self.animationData.legendCmd
        else:
            cmd = ['d.legend', 'at=5,50,2,5']

            mapName = self._getLegendMapHint()
            if mapName:
                cmd.append("map=%s" % mapName)

        GUI(parent=self, modal=True).ParseCommand(cmd=cmd,
                                                  completed=(self.GetOptData, '', ''))

    def GetOptData(self, dcmd, layer, params, propwin):
        """!Process decoration layer data"""
        if dcmd:
            self._tmpLegendCmd = dcmd

            if not self.legend.IsChecked():
                self.legend.SetValue(True)
        else:
            if not self._tmpLegendCmd and not self.animationData.legendCmd:
                self.legend.SetValue(False)

    def _update(self):
        if self.nDChoice.GetSelection() == 1 and len(self._layerList) > 1:
            raise GException(_("Only one series or space-time "
                               "dataset is accepted for 3D mode."))
        hasSeries = False
        for layer in self._layerList:
            if layer.active and hasattr(layer, 'maps'):
                hasSeries = True
                break
        if not hasSeries:
            raise GException(_("No map series or space-time dataset added."))

        self.animationData.layerList = self._layerList
        self.animationData.name = self.nameCtrl.GetValue()
        self.animationData.windowIndex = self.windowChoice.GetSelection()

        sel = self.nDChoice.GetSelection()
        self.animationData.viewMode = self.nDChoice.GetClientData(sel)
        self.animationData.legendCmd = None
        if self._tmpLegendCmd:
            if self.legend.IsChecked():
                self.animationData.legendCmd = self._tmpLegendCmd

        if self.threeDPanel.IsShown():
            self.animationData.workspaceFile = self.fileSelector.GetValue()
        if self.threeDPanel.IsShown():
            self.animationData.nvizParameter = self.paramChoice.GetStringSelection()

    def OnOk(self, event):
        try:
            self._update()
            self.EndModal(wx.ID_OK)
        except (GException, ValueError, IOError) as e:
            GError(message=str(e), showTraceback=False, caption=_("Invalid input"))


class EditDialog(wx.Dialog):
    def __init__(self, parent, evalFunction, animationData, maxAnimations):
        wx.Dialog.__init__(self, parent=parent, id=wx.ID_ANY,
                           style=wx.DEFAULT_DIALOG_STYLE)
        self.animationData = copy.deepcopy(animationData)
        self.eval = evalFunction
        self.SetTitle(_("Add, edit or remove animations"))
        self._layout()
        self.SetSize((300, -1))
        self.maxAnimations = maxAnimations
        self.result = None

    def _layout(self):
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        box = wx.StaticBox (parent=self, id=wx.ID_ANY, label=" %s " % _("List of animations"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridBagSizer = wx.GridBagSizer (hgap=5, vgap=5)
        gridBagSizer.AddGrowableCol(0)
        # gridBagSizer.AddGrowableCol(1,1)

        self.listbox = wx.ListBox(self, id=wx.ID_ANY, choices=[], style=wx.LB_SINGLE | wx.LB_NEEDED_SB)
        self.listbox.Bind(wx.EVT_LISTBOX_DCLICK, self.OnEdit)

        self.addButton = wx.Button(self, id=wx.ID_ANY, label=_("Add"))
        self.addButton.Bind(wx.EVT_BUTTON, self.OnAdd)
        self.editButton = wx.Button(self, id=wx.ID_ANY, label=_("Edit"))
        self.editButton.Bind(wx.EVT_BUTTON, self.OnEdit)
        self.removeButton = wx.Button(self, id=wx.ID_ANY, label=_("Remove"))
        self.removeButton.Bind(wx.EVT_BUTTON, self.OnRemove)

        self._updateListBox()

        gridBagSizer.Add(self.listbox, pos=(0, 0), span = (3, 1),
                         flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, border=0)
        gridBagSizer.Add(self.addButton, pos=(0, 1),
                         flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, border=0)
        gridBagSizer.Add(self.editButton, pos=(1, 1),
                         flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, border=0)
        gridBagSizer.Add(self.removeButton, pos=(2, 1),
                         flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, border=0)
        sizer.Add(gridBagSizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=5)
        mainSizer.Add(item=sizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL, border=5)

        # buttons
        self.btnOk = wx.Button(self, wx.ID_OK)
        self.btnCancel = wx.Button(self, wx.ID_CANCEL)
        self.btnOk.SetDefault()
        self.btnOk.Bind(wx.EVT_BUTTON, self.OnOk)
        # button sizer
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(self.btnOk)
        btnStdSizer.AddButton(self.btnCancel)
        btnStdSizer.Realize()

        mainSizer.Add(item=btnStdSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_RIGHT, border=5)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

    def _updateListBox(self):
        self.listbox.Clear()
        for anim in self.animationData:
            self.listbox.Append(anim.name, clientData=anim)
        if self.animationData:
            self.listbox.SetSelection(0)

    def _getNextIndex(self):
        indices = [anim.windowIndex for anim in self.animationData]
        for i in range(self.maxAnimations):
            if i not in indices:
                return i
        return None

    def OnAdd(self, event):
        windowIndex = self._getNextIndex()
        if windowIndex is None:
            GMessage(self, message=_("Maximum number of animations is %d.") % self.maxAnimations)
            return
        animData = AnimationData()
        # number of active animations
        animationIndex = len(self.animationData)
        animData.SetDefaultValues(windowIndex, animationIndex)
        dlg = InputDialog(parent=self, mode='add', animationData=animData)
        if dlg.ShowModal() == wx.ID_CANCEL:
            dlg.Destroy()
            return
        dlg.Destroy()
        self.animationData.append(animData)

        self._updateListBox()

    def OnEdit(self, event):
        index = self.listbox.GetSelection()
        if index == wx.NOT_FOUND:
            return

        animData = self.listbox.GetClientData(index)
        dlg = InputDialog(parent=self, mode='edit', animationData=animData)
        if dlg.ShowModal() == wx.ID_CANCEL:
            dlg.Destroy()
            return
        dlg.Destroy()

        self._updateListBox()

    def OnRemove(self, event):
        index = self.listbox.GetSelection()
        if index == wx.NOT_FOUND:
            return

        animData = self.listbox.GetClientData(index)
        self.animationData.remove(animData)

        self._updateListBox()

    def GetResult(self):
        return self.result

    def OnOk(self, event):
        indices = set([anim.windowIndex for anim in self.animationData])
        if len(indices) != len(self.animationData):
            GError(parent=self, message=_("More animations are using one window."
                                          " Please select different window for each animation."))
            return
        try:
            temporalMode, tempManager = self.eval(self.animationData)
        except GException, e:
            GError(parent=self, message=e.value, showTraceback=False)
            return
        self.result = (self.animationData, temporalMode, tempManager)

        self.EndModal(wx.ID_OK)


class ExportDialog(wx.Dialog):
    def __init__(self, parent, temporal, timeTick):
        wx.Dialog.__init__(self, parent=parent, id=wx.ID_ANY, title=_("Export animation"),
                           style=wx.DEFAULT_DIALOG_STYLE)
        self.decorations = []

        self.temporal = temporal
        self.timeTick = timeTick
        self._layout()

        # export animation
        self.doExport = Signal('ExportDialog::doExport')

        wx.CallAfter(self._hideAll)

    def _layout(self):
        notebook = wx.Notebook(self, id=wx.ID_ANY)
        mainSizer = wx.BoxSizer(wx.VERTICAL)

        notebook.AddPage(page=self._createExportFormatPanel(notebook), text=_("Format"))
        notebook.AddPage(page=self._createDecorationsPanel(notebook), text=_("Decorations"))
        mainSizer.Add(item=notebook, proportion=0,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_RIGHT, border=5)

        self.btnExport = wx.Button(self, wx.ID_OK)
        self.btnExport.SetLabel(_("Export"))
        self.btnCancel = wx.Button(self, wx.ID_CANCEL)
        self.btnExport.SetDefault()

        self.btnExport.Bind(wx.EVT_BUTTON, self.OnExport)

        # button sizer
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(self.btnExport)
        btnStdSizer.AddButton(self.btnCancel)
        btnStdSizer.Realize()

        mainSizer.Add(item=btnStdSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_RIGHT, border=5)
        self.SetSizer(mainSizer)

        # set the longest option to fit
        self.hidevbox.Show(self.fontBox, True)
        self.hidevbox.Show(self.imageBox, False)
        self.hidevbox.Show(self.textBox, True)
        self.hidevbox.Show(self.posBox, True)
        self.hidevbox.Show(self.informBox, False)
        mainSizer.Fit(self)

    def _createDecorationsPanel(self, notebook):
        panel = wx.Panel(notebook, id=wx.ID_ANY)
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self._createDecorationsList(panel), proportion=0, flag=wx.ALL | wx.EXPAND, border=10)
        sizer.Add(self._createDecorationsProperties(panel), proportion=0, flag=wx.ALL | wx.EXPAND, border=10)
        panel.SetSizer(sizer)
        sizer.Fit(panel)
        return panel

    def _createDecorationsList(self, panel):
        gridBagSizer = wx.GridBagSizer(hgap=5, vgap=5)

        gridBagSizer.AddGrowableCol(0)

        self.listbox = wx.ListBox(panel, id=wx.ID_ANY, choices=[],
                                  style=wx.LB_SINGLE | wx.LB_NEEDED_SB)
        self.listbox.Bind(wx.EVT_LISTBOX, self.OnSelectionChanged)

        gridBagSizer.Add(self.listbox, pos=(0, 0), span=(4, 1),
                         flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, border=0)

        buttonNames = ['time', 'image', 'text']
        buttonLabels = [_("Add time stamp"), _("Add image"), _("Add text")]
        i = 0
        for buttonName, buttonLabel in zip(buttonNames, buttonLabels):
            if buttonName == 'time' and self.temporal == TemporalMode.NONTEMPORAL:
                continue
            btn = wx.Button(panel, id=wx.ID_ANY, name=buttonName, label=buttonLabel)
            btn.Bind(wx.EVT_BUTTON, lambda evt, temp=buttonName: self.OnAddDecoration(evt, temp))
            gridBagSizer.Add(btn, pos=(i, 1), flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, border=0)
            i += 1
        removeButton = wx.Button(panel, id=wx.ID_ANY, label=_("Remove"))
        removeButton.Bind(wx.EVT_BUTTON, self.OnRemove)
        gridBagSizer.Add(removeButton, pos=(i, 1), flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, border=0)

        return gridBagSizer

    def _createDecorationsProperties(self, panel):
        self.hidevbox = wx.BoxSizer(wx.VERTICAL)
        # inform label
        self.informBox = wx.BoxSizer(wx.HORIZONTAL)
        if self.temporal == TemporalMode.TEMPORAL:
            label = _("Add time stamp, image or text decoration by one of the buttons above.")
        else:
            label = _("Add image or text decoration by one of the buttons above.")

        label = wx.StaticText(panel, id=wx.ID_ANY, label=label)
        label.Wrap(400)
        self.informBox.Add(label, proportion=1, flag=wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, border=5)
        self.hidevbox.Add(self.informBox, proportion=0, flag=wx.EXPAND | wx.BOTTOM, border=5)

        # font
        self.fontBox = wx.BoxSizer(wx.HORIZONTAL)
        self.fontBox.Add(wx.StaticText(panel, id=wx.ID_ANY, label=_("Font settings:")),
                         proportion=0, flag=wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, border=5)
        self.sampleLabel = wx.StaticText(panel, id=wx.ID_ANY, label=_("Sample text"))
        self.fontBox.Add(self.sampleLabel, proportion=1,
                         flag=wx.ALIGN_CENTER | wx.RIGHT | wx.LEFT, border=5)
        fontButton = wx.Button(panel, id=wx.ID_ANY, label=_("Set font"))
        fontButton.Bind(wx.EVT_BUTTON, self.OnFont)
        self.fontBox.Add(fontButton, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        self.hidevbox.Add(self.fontBox, proportion=0, flag=wx.EXPAND | wx.BOTTOM, border=5)

        # image
        self.imageBox = wx.BoxSizer(wx.HORIZONTAL)
        filetype, ltype = GetImageHandlers(wx.EmptyImage(10, 10))
        self.browse = filebrowse.FileBrowseButton(parent=panel, id=wx.ID_ANY, fileMask=filetype,
                                                  labelText=_("Image file:"),
                                                  dialogTitle=_('Choose image file'),
                                                  buttonText=_('Browse'),
                                                  startDirectory=os.getcwd(), fileMode=wx.OPEN,
                                                  changeCallback=self.OnSetImage)
        self.imageBox.Add(self.browse, proportion=1, flag=wx.EXPAND)
        self.hidevbox.Add(self.imageBox, proportion=0, flag=wx.EXPAND | wx.BOTTOM, border=5)
        # text
        self.textBox = wx.BoxSizer(wx.HORIZONTAL)
        self.textBox.Add(wx.StaticText(panel, id=wx.ID_ANY, label=_("Text:")),
                         proportion=0, flag=wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, border=5)
        self.textCtrl = wx.TextCtrl(panel, id=wx.ID_ANY)
        self.textCtrl.Bind(wx.EVT_TEXT, self.OnText)
        self.textBox.Add(self.textCtrl, proportion=1, flag=wx.EXPAND)
        self.hidevbox.Add(self.textBox, proportion=0, flag=wx.EXPAND)

        self.posBox = self._positionWidget(panel)
        self.hidevbox.Add(self.posBox, proportion=0, flag=wx.EXPAND | wx.TOP, border=5)
        return self.hidevbox

    def _positionWidget(self, panel):
        grid = wx.GridBagSizer(vgap=5, hgap=5)
        label = wx.StaticText(panel, id=wx.ID_ANY, label=_("Placement as percentage of"
                              " screen coordinates (X: 0, Y: 0 is top left):"))
        label.Wrap(400)
        self.spinX = wx.SpinCtrl(panel, id=wx.ID_ANY, min=0, max=100, initial=10)
        self.spinY = wx.SpinCtrl(panel, id=wx.ID_ANY, min=0, max=100, initial=10)
        self.spinX.Bind(wx.EVT_SPINCTRL, lambda evt, temp='X': self.OnPosition(evt, temp))
        self.spinY.Bind(wx.EVT_SPINCTRL, lambda evt, temp='Y': self.OnPosition(evt, temp))

        grid.Add(label, pos=(0, 0), span = (1, 4), flag = wx.EXPAND)
        grid.Add(wx.StaticText(panel, id=wx.ID_ANY, label=_("X:")), pos=(1, 0),
                 flag = wx.ALIGN_CENTER_VERTICAL)
        grid.Add(wx.StaticText(panel, id=wx.ID_ANY, label=_("Y:")), pos=(1, 2),
                 flag = wx.ALIGN_CENTER_VERTICAL)
        grid.Add(self.spinX, pos=(1, 1))
        grid.Add(self.spinY, pos=(1, 3))

        return grid

    def _createExportFormatPanel(self, notebook):
        panel = wx.Panel(notebook, id=wx.ID_ANY)
        borderSizer = wx.BoxSizer(wx.VERTICAL)

        hSizer = wx.BoxSizer(wx.HORIZONTAL)
        choices = [_("image sequence"), _("animated GIF"), _("SWF"), _("AVI")]
        self.formatChoice = wx.Choice(parent=panel, id=wx.ID_ANY,
                                      choices=choices)
        self.formatChoice.Bind(wx.EVT_CHOICE, lambda event: self.ChangeFormat(event.GetSelection()))
        hSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("Export to:")),
                   proportion=0, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=2)
        hSizer.Add(item=self.formatChoice, proportion=1,
                   flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND | wx.ALL, border=2)
        borderSizer.Add(item=hSizer, proportion=0, flag=wx.EXPAND | wx.ALL, border=3)

        helpSizer = wx.BoxSizer(wx.HORIZONTAL)
        helpSizer.AddStretchSpacer(1)
        self.formatPanelSizer = wx.BoxSizer(wx.VERTICAL)
        helpSizer.Add(self.formatPanelSizer, proportion=5, flag=wx.EXPAND)
        borderSizer.Add(helpSizer, proportion=1, flag=wx.EXPAND)
        self.formatPanels = []

        # panel for image sequence
        imSeqPanel = wx.Panel(parent=panel, id=wx.ID_ANY)
        prefixLabel = wx.StaticText(imSeqPanel, id=wx.ID_ANY, label=_("File prefix:"))
        self.prefixCtrl = wx.TextCtrl(imSeqPanel, id=wx.ID_ANY, value=_("animation_"))
        formatLabel = wx.StaticText(imSeqPanel, id=wx.ID_ANY, label=_("File format:"))
        imageTypes = ['PNG', 'JPEG', 'GIF', 'TIFF', 'PPM', 'BMP']
        self.imSeqFormatChoice = wx.Choice(imSeqPanel, choices=imageTypes)
        self.imSeqFormatChoice.SetSelection(0)
        self.dirBrowse = filebrowse.DirBrowseButton(parent=imSeqPanel, id=wx.ID_ANY,
                                                    labelText=_("Directory:"),
                                                    dialogTitle=_("Choose directory for export"),
                                                    buttonText=_("Browse"),
                                                    startDirectory=os.getcwd())

        dirGridSizer = wx.GridBagSizer(hgap=5, vgap=5)
        dirGridSizer.AddGrowableCol(1)
        dirGridSizer.Add(prefixLabel, pos=(0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        dirGridSizer.Add(self.prefixCtrl, pos=(0, 1), flag = wx.EXPAND)
        dirGridSizer.Add(formatLabel, pos=(1, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        dirGridSizer.Add(self.imSeqFormatChoice, pos=(1, 1), flag = wx.EXPAND)
        dirGridSizer.Add(self.dirBrowse, pos=(2, 0), flag = wx.EXPAND, span = (1, 2))
        imSeqPanel.SetSizer(dirGridSizer)
        dirGridSizer.Fit(imSeqPanel)

        self.formatPanelSizer.Add(item=imSeqPanel, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        self.formatPanels.append(imSeqPanel)

        # panel for gif
        gifPanel = wx.Panel(parent=panel, id=wx.ID_ANY)

        self.gifBrowse = filebrowse.FileBrowseButton(parent=gifPanel, id=wx.ID_ANY,
                                                     fileMask="GIF file (*.gif)|*.gif",
                                                     labelText=_("GIF file:"),
                                                     dialogTitle=_("Choose file to save animation"),
                                                     buttonText=_("Browse"),
                                                     startDirectory=os.getcwd(), fileMode=wx.SAVE)
        gifGridSizer = wx.GridBagSizer(hgap=5, vgap=5)
        gifGridSizer.AddGrowableCol(0)
        gifGridSizer.Add(self.gifBrowse, pos=(0, 0), flag = wx.EXPAND)
        gifPanel.SetSizer(gifGridSizer)
        gifGridSizer.Fit(gifPanel)

        self.formatPanelSizer.Add(item=gifPanel, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        self.formatPanels.append(gifPanel)

        # panel for swf
        swfPanel = wx.Panel(parent=panel, id=wx.ID_ANY)
        self.swfBrowse = filebrowse.FileBrowseButton(parent=swfPanel, id=wx.ID_ANY,
                                                     fileMask="SWF file (*.swf)|*.swf",
                                                     labelText=_("SWF file:"),
                                                     dialogTitle=_("Choose file to save animation"),
                                                     buttonText=_("Browse"),
                                                     startDirectory=os.getcwd(), fileMode=wx.SAVE)
        swfGridSizer = wx.GridBagSizer(hgap=5, vgap=5)
        swfGridSizer.AddGrowableCol(0)
        swfGridSizer.Add(self.swfBrowse, pos=(0, 0), flag = wx.EXPAND)
        swfPanel.SetSizer(swfGridSizer)
        swfGridSizer.Fit(swfPanel)

        self.formatPanelSizer.Add(item=swfPanel, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        self.formatPanels.append(swfPanel)

        # panel for avi
        aviPanel = wx.Panel(parent=panel, id=wx.ID_ANY)
        ffmpeg = gcore.find_program('ffmpeg', '--help')
        if not ffmpeg:
            warning = _("Program 'ffmpeg' was not found.\nPlease install it first "
                        "and make sure\nit's in the PATH variable.")
            warningLabel = wx.StaticText(parent=aviPanel, label=warning)
            warningLabel.SetForegroundColour(wx.RED)
        self.aviBrowse = filebrowse.FileBrowseButton(parent=aviPanel, id=wx.ID_ANY,
                                                     fileMask="AVI file (*.avi)|*.avi",
                                                     labelText=_("AVI file:"),
                                                     dialogTitle=_("Choose file to save animation"),
                                                     buttonText=_("Browse"),
                                                     startDirectory=os.getcwd(), fileMode=wx.SAVE)
        encodingLabel = wx.StaticText(parent=aviPanel, id=wx.ID_ANY, label=_("Video codec:"))
        self.encodingText = wx.TextCtrl(parent=aviPanel, id=wx.ID_ANY, value='mpeg4')
        optionsLabel = wx.StaticText(parent=aviPanel, label=_("Additional options:"))
        self.optionsText = wx.TextCtrl(parent=aviPanel)
        self.optionsText.SetToolTipString(_("Consider adding '-sameq' or '-qscale 1' "
                                            "if not satisfied with video quality. "
                                            "Options depend on ffmpeg version."))
        aviGridSizer = wx.GridBagSizer(hgap=5, vgap=5)
        aviGridSizer.AddGrowableCol(1)
        aviGridSizer.Add(self.aviBrowse, pos=(0, 0), span = (1, 2), flag = wx.EXPAND)
        aviGridSizer.Add(encodingLabel, pos=(1, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        aviGridSizer.Add(self.encodingText, pos=(1, 1), flag = wx.EXPAND)
        aviGridSizer.Add(optionsLabel, pos=(2, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        aviGridSizer.Add(self.optionsText, pos=(2, 1), flag=wx.EXPAND)
        if not ffmpeg:
            aviGridSizer.Add(warningLabel, pos=(3, 0), span=(1, 2),
                             flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND)

        aviPanel.SetSizer(aviGridSizer)
        aviGridSizer.Fit(aviPanel)

        self.formatPanelSizer.Add(item=aviPanel, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        self.formatPanels.append(aviPanel)

        fpsSizer = wx.BoxSizer(wx.HORIZONTAL)
        fps = 1000 / self.timeTick
        fpsSizer.Add(wx.StaticText(panel, id=wx.ID_ANY, label=_("Current frame rate: %.2f fps") % fps),
                     proportion=1, flag=wx.EXPAND)
        borderSizer.Add(fpsSizer, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5)

        panel.SetSizer(borderSizer)
        borderSizer.Fit(panel)
        self.ChangeFormat(index=0)

        return panel

    def ChangeFormat(self, index):
        for i, panel in enumerate(self.formatPanels):
            self.formatPanelSizer.Show(item=panel, show=(i == index))
        self.formatPanelSizer.Layout()

    def OnFont(self, event):
        index = self.listbox.GetSelection()
        # should not happen
        if index == wx.NOT_FOUND:
            return
        cdata = self.listbox.GetClientData(index)
        font = cdata['font']

        fontdata = wx.FontData()
        fontdata.EnableEffects(True)
        fontdata.SetColour('black')
        fontdata.SetInitialFont(font)

        dlg = wx.FontDialog(self, fontdata)

        if dlg.ShowModal() == wx.ID_OK:
            newfontdata = dlg.GetFontData()
            font = newfontdata.GetChosenFont()
            self.sampleLabel.SetFont(font)
            cdata['font'] = font
            self.Layout()

    def OnPosition(self, event, coord):
        index = self.listbox.GetSelection()
        # should not happen
        if index == wx.NOT_FOUND:
            return
        cdata = self.listbox.GetClientData(index)
        cdata['pos'][coord == 'Y'] = event.GetInt()

    def OnSetImage(self, event):
        index = self.listbox.GetSelection()
        # should not happen
        if index == wx.NOT_FOUND:
            return
        cdata = self.listbox.GetClientData(index)
        cdata['file'] = event.GetString()

    def OnAddDecoration(self, event, name):
        if name == 'time':
            timeInfo = {'name': name, 'font': self.GetFont(), 'pos': [10, 10]}
            self.decorations.append(timeInfo)
        elif name == 'image':
            imageInfo = {'name': name, 'file': '', 'pos': [10, 10]}
            self.decorations.append(imageInfo)
        elif name == 'text':
            textInfo = {'name': name, 'font': self.GetFont(), 'text': '', 'pos': [10, 10]}
            self.decorations.append(textInfo)

        self._updateListBox()
        self.listbox.SetSelection(self.listbox.GetCount() - 1)
        self.OnSelectionChanged(event=None)

    def OnSelectionChanged(self, event):
        index = self.listbox.GetSelection()
        if index == wx.NOT_FOUND:
            self._hideAll()
            return
        cdata = self.listbox.GetClientData(index)
        self.hidevbox.Show(self.fontBox, (cdata['name'] in ('time', 'text')))
        self.hidevbox.Show(self.imageBox, (cdata['name'] == 'image'))
        self.hidevbox.Show(self.textBox, (cdata['name'] == 'text'))
        self.hidevbox.Show(self.posBox, True)
        self.hidevbox.Show(self.informBox, False)

        self.spinX.SetValue(cdata['pos'][0])
        self.spinY.SetValue(cdata['pos'][1])
        if cdata['name'] == 'image':
            self.browse.SetValue(cdata['file'])
        elif cdata['name'] in ('time', 'text'):
            self.sampleLabel.SetFont(cdata['font'])
            if cdata['name'] == 'text':
                self.textCtrl.SetValue(cdata['text'])

        self.hidevbox.Layout()
        # self.Layout()

    def OnText(self, event):
        index = self.listbox.GetSelection()
        # should not happen
        if index == wx.NOT_FOUND:
            return
        cdata = self.listbox.GetClientData(index)
        cdata['text'] = event.GetString()

    def OnRemove(self, event):
        index = self.listbox.GetSelection()
        if index == wx.NOT_FOUND:
            return

        decData = self.listbox.GetClientData(index)
        self.decorations.remove(decData)

        self._updateListBox()
        if self.listbox.GetCount():
            self.listbox.SetSelection(0)
            self.OnSelectionChanged(event=None)

    def OnExport(self, event):
        for decor in self.decorations:
            if decor['name'] == 'image':
                if not os.path.exists(decor['file']):
                    if decor['file']:
                        GError(parent=self, message=_("File %s not found.") % decor['file'])
                    else:
                        GError(parent=self, message=_("Decoration image file is missing."))
                    return

        if self.formatChoice.GetSelection() == 0:
            name = self.dirBrowse.GetValue()
            if not os.path.exists(name):
                if name:
                    GError(parent=self, message=_("Directory %s not found.") % name)
                else:
                    GError(parent=self, message=_("Export directory is missing."))
                return
        elif self.formatChoice.GetSelection() == 1:
            if not self.gifBrowse.GetValue():
                GError(parent=self, message=_("Export file is missing."))
                return
        elif self.formatChoice.GetSelection() == 2:
            if not self.swfBrowse.GetValue():
                GError(parent=self, message=_("Export file is missing."))
                return

        # hide only to keep previous values
        self.Hide()
        self.doExport.emit(exportInfo=self.GetExportInformation(),
                           decorations=self.GetDecorations())

    def GetDecorations(self):
        return self.decorations

    def GetExportInformation(self):
        info = {}
        if self.formatChoice.GetSelection() == 0:
            info['method'] = 'sequence'
            info['directory'] = self.dirBrowse.GetValue()
            info['prefix'] = self.prefixCtrl.GetValue()
            info['format'] = self.imSeqFormatChoice.GetStringSelection()

        elif self.formatChoice.GetSelection() == 1:
            info['method'] = 'gif'
            info['file'] = self.gifBrowse.GetValue()

        elif self.formatChoice.GetSelection() == 2:
            info['method'] = 'swf'
            info['file'] = self.swfBrowse.GetValue()

        elif self.formatChoice.GetSelection() == 3:
            info['method'] = 'avi'
            info['file'] = self.aviBrowse.GetValue()
            info['encoding'] = self.encodingText.GetValue()
            info['options'] = self.optionsText.GetValue()

        return info

    def _updateListBox(self):
        self.listbox.Clear()
        names = {'time': _("Time stamp"), 'image': _("Image"), 'text': _("Text")}
        for decor in self.decorations:
            self.listbox.Append(names[decor['name']], clientData=decor)

    def _hideAll(self):
        self.hidevbox.Show(self.fontBox, False)
        self.hidevbox.Show(self.imageBox, False)
        self.hidevbox.Show(self.textBox, False)
        self.hidevbox.Show(self.posBox, False)
        self.hidevbox.Show(self.informBox, True)
        self.hidevbox.Layout()


class AnimSimpleLayerManager(SimpleLayerManager):
    """!Simple layer manager for animation tool.
    Allows to add space-time dataset or series of maps.
    """
    def __init__(self, parent, layerList,
                 lmgrStyle=SIMPLE_LMGR_RASTER | SIMPLE_LMGR_VECTOR |
                 SIMPLE_LMGR_TB_TOP | SIMPLE_LMGR_STDS,
                 toolbarCls=AnimSimpleLmgrToolbar, modal=True):
        SimpleLayerManager.__init__(self, parent, layerList, lmgrStyle, toolbarCls, modal)

    def OnAddStds(self, event):
        """!Opens dialog for specifying temporal dataset.
        Dummy layer is added first."""
        layer = AnimLayer()
        layer.hidden = True
        self._layerList.AddLayer(layer)
        self.SetStdsProperties(layer)
        event.Skip()

    def SetStdsProperties(self, layer):
        dlg = AddTemporalLayerDialog(parent=self, layer=layer)
        # first get hidden property, it's altered afterwards
        hidden = layer.hidden
        if dlg.ShowModal() == wx.ID_OK:
            layer = dlg.GetLayer()
            if hidden:
                signal = self.layerAdded
            else:
                signal = self.cmdChanged
            signal.emit(index=self._layerList.GetLayerIndex(layer), layer=layer)
        else:
            if hidden:
                self._layerList.RemoveLayer(layer)
        dlg.Destroy()
        self._update()
        self.anyChange.emit()

    def _layerChangeProperties(self, layer):
        """!Opens new module dialog or recycles it."""
        if layer in self._dialogs:
            dlg = self._dialogs[layer]
            if dlg.IsShown():
                dlg.Raise()
                dlg.SetFocus()
            else:
                dlg.Show()
        else:
            if not hasattr(layer, 'maps'):
                GUI(parent=self, giface=None,
                    modal=self._modal).ParseCommand(cmd=layer.cmd,
                                                    completed=(self.GetOptData, layer, ''))
            else:
                self.SetStdsProperties(layer)

    def Activate3D(self, activate=True):
        """!Activates/deactivates certain tool depending on 2D/3D view."""
        self._toolbar.EnableTools(['addRaster', 'addVector',
                                   'opacity', 'up', 'down'], not activate)


class AddTemporalLayerDialog(wx.Dialog):
    """!Dialog for adding space-time dataset/ map series."""
    def __init__(self, parent, layer, title=_("Add space-time dataset layer")):
        wx.Dialog.__init__(self, parent=parent, title=title)

        self.layer = layer
        self._mapType = None
        self._name = None
        self._cmd = None

        self.tselect = Select(parent=self, type='strds')
        iconTheme = UserSettings.Get(group='appearance', key='iconTheme', subkey='type')
        bitmapPath = os.path.join(globalvar.ETCICONDIR, iconTheme, 'layer-open.png')
        if os.path.isfile(bitmapPath) and os.path.getsize(bitmapPath):
            bitmap = wx.Bitmap(name=bitmapPath)
        else:
            bitmap = wx.ArtProvider.GetBitmap(id=wx.ART_MISSING_IMAGE, client=wx.ART_TOOLBAR)
        self.addManyMapsButton = wx.BitmapButton(self, bitmap=bitmap)
        self.addManyMapsButton.Bind(wx.EVT_BUTTON, self._onAddMaps)

        types = [('rast', _("Multiple raster maps")),
                 ('vect', _("Multiple vector maps")),
                 ('strds', _("Space time raster dataset")),
                 ('stvds', _("Space time vector dataset"))]
        self._types = dict(types)

        self.tchoice = wx.Choice(parent=self)
        for type_, text in types:
            self.tchoice.Append(text, clientData=type_)

        self.editBtn = wx.Button(parent=self, label='Set properties')

        self.okBtn = wx.Button(parent=self, id=wx.ID_OK)
        self.cancelBtn = wx.Button(parent=self, id=wx.ID_CANCEL)

        self.okBtn.Bind(wx.EVT_BUTTON, self._onOK)
        self.editBtn.Bind(wx.EVT_BUTTON, self._onProperties)
        self.tchoice.Bind(wx.EVT_CHOICE,
                          lambda evt: self._setType())
        self.tselect.Bind(wx.EVT_TEXT,
                          lambda evt: self._datasetChanged())

        if self.layer.mapType:
            self._setType(self.layer.mapType)
        else:
            self._setType('rast')
        if self.layer.name:
            self.tselect.SetValue(self.layer.name)
        if self.layer.cmd:
            self._cmd = self.layer.cmd

        self._layout()
        self.SetSize(self.GetBestSize())

    def _layout(self):
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        bodySizer = wx.BoxSizer(wx.VERTICAL)
        typeSizer = wx.BoxSizer(wx.HORIZONTAL)
        selectSizer = wx.BoxSizer(wx.HORIZONTAL)
        typeSizer.Add(wx.StaticText(self, label=_("Input data type:")),
                      flag=wx.ALIGN_CENTER_VERTICAL)
        typeSizer.AddStretchSpacer()
        typeSizer.Add(self.tchoice)
        bodySizer.Add(typeSizer, flag=wx.EXPAND | wx.BOTTOM, border=5)

        selectSizer.Add(self.tselect, flag=wx.RIGHT | wx.ALIGN_CENTER_VERTICAL, border=5)
        selectSizer.Add(self.addManyMapsButton, flag=wx.EXPAND)
        bodySizer.Add(selectSizer, flag=wx.BOTTOM, border=5)
        bodySizer.Add(self.editBtn, flag=wx.BOTTOM, border=5)
        mainSizer.Add(bodySizer, proportion=1, flag=wx.EXPAND | wx.ALL, border=10)

        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.okBtn)
        btnSizer.AddButton(self.cancelBtn)
        btnSizer.Realize()

        mainSizer.Add(btnSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL, border=10)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

    def _datasetChanged(self):
        if self._name != self.tselect.GetValue():
            self._name = self.tselect.GetValue()
            self._cmd = None

    def _setType(self, typeName=None):
        if typeName:
            self.tchoice.SetStringSelection(self._types[typeName])
            self.tselect.SetType(typeName)
            if typeName in ('strds', 'stvds'):
                self.tselect.SetType(typeName, multiple=False)
                self.addManyMapsButton.Disable()
            else:
                self.tselect.SetType(typeName, multiple=True)
                self.addManyMapsButton.Enable()
            self._mapType = typeName
            self.tselect.SetValue('')
        else:
            typeName = self.tchoice.GetClientData(self.tchoice.GetSelection())
            if typeName in ('strds', 'stvds'):
                self.tselect.SetType(typeName, multiple=False)
                self.addManyMapsButton.Disable()
            else:
                self.tselect.SetType(typeName, multiple=True)
                self.addManyMapsButton.Enable()
            if typeName != self._mapType:
                self._cmd = None
                self._mapType = typeName
                self.tselect.SetValue('')

    def _createDefaultCommand(self):
        cmd = []
        if self._mapType in ('rast', 'strds'):
            cmd.append('d.rast')
        elif self._mapType in ('vect', 'stvds'):
            cmd.append('d.vect')
        if self._name:
            if self._mapType in ('rast', 'vect'):
                cmd.append('map={}'.format(self._name.split(',')[0]))
            else:
                try:
                    maps = getRegisteredMaps(self._name, etype=self._mapType)
                    if maps:
                        cmd.append('map={}'.format(maps[0]))
                except gcore.ScriptError, e:
                    GError(parent=self, message=str(e), showTraceback=False)
                    return None
        return cmd

    def _onAddMaps(self, event):
        dlg = MapLayersDialog(self, title=_("Select raster/vector maps."))
        dlg.applyAddingMapLayers.connect(lambda mapLayers:
                                         self.tselect.SetValue(','.join(mapLayers)))
        index = 0 if self._mapType == 'rast' else 1
        dlg.layerType.SetSelection(index)
        dlg.LoadMapLayers(dlg.GetLayerType(cmd=True),
                          dlg.mapset.GetStringSelection())
        if dlg.ShowModal() == wx.ID_OK:
            self.tselect.SetValue(','.join(dlg.GetMapLayers()))

        dlg.Destroy()

    def _onProperties(self, event):
        self._checkInput()
        if self._cmd:
            GUI(parent=self, show=True, modal=True).ParseCommand(cmd=self._cmd,
                                                                 completed=(self._getOptData, '', ''))

    def _checkInput(self):
        if not self.tselect.GetValue():
            GMessage(parent=self, message=_("Please select maps or dataset first."))
            return

        if not self._cmd:
            self._cmd = self._createDefaultCommand()

    def _getOptData(self, dcmd, layer, params, propwin):
        if dcmd:
            self._cmd = dcmd

    def _onOK(self, event):
        self._checkInput()
        if self._cmd:
            try:
                self.layer.hidden = False
                self.layer.mapType = self._mapType
                self.layer.name = self._name
                self.layer.cmd = self._cmd
                event.Skip()
            except (GException, gcore.ScriptError), e:
                GError(parent=self, message=str(e))

    def GetLayer(self):
        return self.layer


def test():
    import wx.lib.inspection

    app = wx.PySimpleApp()

#    testTemporalLayer()
#    testAnimLmgr()
    testAnimInput()
    # wx.lib.inspection.InspectionTool().Show()

    app.MainLoop()


def testAnimInput():
    anim = AnimationData()
    anim.SetDefaultValues(animationIndex=0, windowIndex=0)

    dlg = InputDialog(parent=None, mode='add', animationData=anim)
    dlg.Show()


def testAnimEdit():
    anim = AnimationData()
    anim.SetDefaultValues(animationIndex=0, windowIndex=0)

    dlg = EditDialog(parent=None, animationData=[anim])
    dlg.Show()


def testExport():
    dlg = ExportDialog(parent=None, temporal=TemporalMode.TEMPORAL,
                       timeTick=200)
    if dlg.ShowModal() == wx.ID_OK:
        print dlg.GetDecorations()
        print dlg.GetExportInformation()
        dlg.Destroy()
    else:
        dlg.Destroy()


def testTemporalLayer():
    frame = wx.Frame(None)
    frame.Show()
    layer = AnimLayer()
    dlg = AddTemporalLayerDialog(parent=frame, layer=layer)
    if dlg.ShowModal() == wx.ID_OK:
        layer = dlg.GetLayer()
        print layer.name, layer.cmd, layer.mapType
        dlg.Destroy()
    else:
        dlg.Destroy()


def testAnimLmgr():
    from core.layerlist import LayerList

    frame = wx.Frame(None)
    mgr = AnimSimpleLayerManager(parent=frame, layerList=LayerList())
    frame.mgr = mgr
    frame.Show()


if __name__ == '__main__':
    gcore.set_raise_on_error(True)
    test()
