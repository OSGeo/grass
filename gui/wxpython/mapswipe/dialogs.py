"""
@package mapswipe.dialogs

@brief Dialogs used in Map Swipe

Classes:
 - dialogs::SwipeMapDialog

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""

import copy
import wx
import wx.lib.scrolledpanel as SP
import wx.lib.colourselect as csel

from core import globalvar
from gui_core import gselect
from gui_core.widgets import SimpleValidator
from gui_core.preferences import PreferencesBaseDialog
from core.gcmd import GMessage
from core.layerlist import LayerList
from core.settings import UserSettings
from gui_core.wrap import SpinCtrl, Button, StaticText, StaticBox
from gui_core.simplelmgr import SimpleLayerManager, SIMPLE_LMGR_RASTER, \
    SIMPLE_LMGR_VECTOR, SIMPLE_LMGR_RGB, SIMPLE_LMGR_TB_LEFT, SIMPLE_LMGR_TB_RIGHT

from grass.pydispatch.signal import Signal


class SwipeMapDialog(wx.Dialog):
    """Dialog used to select maps.

    There are two modes - simple (only two raster maps),
    or two layer lists.
    """

    def __init__(self, parent, title=_("Select raster maps"),
                 first=None, second=None,
                 firstLayerList=None, secondLayerList=None):

        wx.Dialog.__init__(self, parent=parent, title=title,
                           style=wx.RESIZE_BORDER | wx.DEFAULT_DIALOG_STYLE)

        if firstLayerList is None:
            self._firstLayerList = LayerList()
        else:
            self._firstLayerList = copy.deepcopy(firstLayerList)
        if secondLayerList is None:
            self._secondLayerList = LayerList()
        else:
            self._secondLayerList = copy.deepcopy(secondLayerList)

        self._firstPanel = self._createSimplePanel()
        self._secondPanel = self._createAdvancedPanel()

        self.btnSwitch = Button(self)
        self.btnCancel = Button(self, id=wx.ID_CANCEL)
        self.btnApply = Button(self, id=wx.ID_APPLY)
        self.btnOK = Button(self, id=wx.ID_OK)
        self.btnOK.SetDefault()

        self.btnSwitch.Bind(wx.EVT_BUTTON, self.OnSwitchMode)
        self.btnApply.Bind(wx.EVT_BUTTON, lambda evt: self._apply())
        self.btnOK.Bind(wx.EVT_BUTTON, lambda evt: self._ok())
        self.btnCancel.Bind(wx.EVT_BUTTON, lambda evt: self.Close())
        self.Bind(wx.EVT_CLOSE, lambda evt: self.Hide())

        self.applyChanges = Signal('SwipeMapDialog.applyChanges')

        if first:
            self._firstRaster.SetValue(first)
        if second:
            self._secondRaster.SetValue(second)

        self._layout()

    def UnInit(self):
        self._firstLmgr.UnInit()
        self._secondLmgr.UnInit()

    def _layout(self):
        """Do layout"""
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        self._switchSizer = wx.BoxSizer()
        self._switchSizer.Add(self._firstPanel, proportion=1,
                              flag=wx.EXPAND | wx.ALL, border=5)
        self._switchSizer.Add(self._secondPanel, proportion=1,
                              flag=wx.EXPAND | wx.ALL, border=5)
        mainSizer.Add(self._switchSizer, proportion=1,
                      flag=wx.EXPAND | wx.ALL)

        self.btnSizer = wx.StdDialogButtonSizer()
        self.btnSizer.AddButton(self.btnCancel)
        self.btnSizer.AddButton(self.btnOK)
        self.btnSizer.AddButton(self.btnApply)
        self.btnSizer.Realize()

        mainSizer.Add(self.btnSwitch, proportion=0,
                      flag=wx.ALL | wx.ALIGN_LEFT, border=5)
        mainSizer.Add(self.btnSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)
        self.mainSizer = mainSizer
        self._switchMode(simple=True)
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

    def _createSimplePanel(self):
        panel = wx.Panel(self)
        sizer = wx.BoxSizer(wx.VERTICAL)

        self._firstRaster = gselect.Select(
            parent=panel,
            type='raster',
            size=globalvar.DIALOG_GSELECT_SIZE,
            validator=SimpleValidator(
                callback=self.ValidatorCallback))

        self._secondRaster = gselect.Select(
            parent=panel,
            type='raster',
            size=globalvar.DIALOG_GSELECT_SIZE,
            validator=SimpleValidator(
                callback=self.ValidatorCallback))
        sizer.Add(
            StaticText(
                panel,
                label=_("Name of top/left raster map:")),
            proportion=0,
            flag=wx.EXPAND | wx.ALL,
            border=5)
        sizer.Add(self._firstRaster, proportion=0,
                  flag=wx.EXPAND | wx.ALL, border=1)
        sizer.Add(
            StaticText(
                panel,
                label=_("Name of bottom/right raster map:")),
            proportion=0,
            flag=wx.EXPAND | wx.ALL,
            border=1)
        sizer.Add(self._secondRaster, proportion=0,
                  flag=wx.EXPAND | wx.ALL, border=1)

        self._firstRaster.SetFocus()

        panel.SetSizer(sizer)
        sizer.Fit(panel)

        return panel

    def _createAdvancedPanel(self):
        panel = wx.Panel(self)
        sizer = wx.BoxSizer(wx.HORIZONTAL)

        self._firstLmgr = SimpleLayerManager(
            parent=panel, layerList=self._firstLayerList,
            lmgrStyle=SIMPLE_LMGR_RASTER | SIMPLE_LMGR_RGB | SIMPLE_LMGR_VECTOR |
            SIMPLE_LMGR_TB_LEFT)
        self._secondLmgr = SimpleLayerManager(
            parent=panel, layerList=self._secondLayerList,
            lmgrStyle=SIMPLE_LMGR_RASTER | SIMPLE_LMGR_RGB | SIMPLE_LMGR_VECTOR |
            SIMPLE_LMGR_TB_RIGHT)
        sizer.Add(
            self._firstLmgr,
            proportion=1,
            flag=wx.EXPAND | wx.ALL,
            border=5)
        sizer.Add(
            self._secondLmgr,
            proportion=1,
            flag=wx.EXPAND | wx.ALL,
            border=5)
        panel.SetSizer(sizer)
        sizer.Fit(panel)

        return panel

    def _switchMode(self, simple):
        if simple:
            self._switchSizer.Show(self._firstPanel, show=True, recursive=True)
            self._switchSizer.Show(
                self._secondPanel, show=False, recursive=True)
            self.btnSwitch.SetLabel(_("Switch to advanced mode"))
            self.btnCancel.SetLabel(_("Cancel"))
        else:
            self._switchSizer.Show(
                self._firstPanel, show=False, recursive=True)
            self._switchSizer.Show(
                self._secondPanel, show=True, recursive=True)
            self.btnSwitch.SetLabel(_("Switch to simple mode"))
            self.btnCancel.SetLabel(_("Close"))

        self.Freeze()  # doesn't do anything (at least on Ubuntu)
        self.btnSizer.Show(self.btnApply, simple)
        self.btnSizer.Show(self.btnOK, simple)
        self.btnSizer.Layout()
        self._switchSizer.Layout()
        self.Fit()
        self.Thaw()

        self.applyChanges.emit()

    def OnSwitchMode(self, event):
        if self._switchSizer.IsShown(self._secondPanel):
            self._switchMode(simple=True)
        else:
            self._switchMode(simple=False)

    def ValidatorCallback(self, win):
        if self._switchSizer.IsShown(self._secondPanel):
            return

        if win == self._firstRaster.GetTextCtrl():
            GMessage(parent=self, message=_(
                "Name of the first map is missing."))
        else:
            GMessage(parent=self, message=_(
                "Name of the second map is missing."))

    def _ok(self):
        self._apply()
        self.Close()

    def _apply(self):
        # TODO check if not empty
        self.applyChanges.emit()

    def GetValues(self):
        """Get raster maps"""
        if self.IsSimpleMode():
            return (self._firstRaster.GetValue(),
                    self._secondRaster.GetValue())
        else:
            return (self._firstLayerList, self._secondLayerList)

    def IsSimpleMode(self):
        if self._switchSizer.IsShown(self._firstPanel):
            return True
        return False

    def GetFirstSimpleLmgr(self):
        return self._firstLmgr

    def GetSecondSimpleLmgr(self):
        return self._secondLmgr


class PreferencesDialog(PreferencesBaseDialog):
    """Mapswipe preferences dialog"""

    def __init__(self, parent, giface, title=_("Map Swipe settings"),
                 settings=UserSettings):
        PreferencesBaseDialog.__init__(
            self, parent=parent, giface=giface, title=title, settings=settings,
            size=(-1, 300))

        # create notebook pages
        self._createMirrorModePage(self.notebook)

        self.SetMinSize(self.GetBestSize())
        self.SetSize(self.size)

    def _createMirrorModePage(self, notebook):
        """Create notebook page for general settings"""
        panel = SP.ScrolledPanel(parent=notebook)
        panel.SetupScrolling(scroll_x=False, scroll_y=True)
        notebook.AddPage(page=panel, text=_("Mirror mode"))

        border = wx.BoxSizer(wx.VERTICAL)
        box = StaticBox(parent=panel, label=" %s " % _("Mirrored cursor"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(hgap=3, vgap=3)

        row = 0
        gridSizer.Add(
            StaticText(
                parent=panel,
                label=_("Color:")),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL,
            pos=(
                row,
                0))
        color = csel.ColourSelect(
            parent=panel,
            colour=UserSettings.Get(
                group='mapswipe',
                key='cursor',
                subkey='color'),
            size=globalvar.DIALOG_COLOR_SIZE)
        color.SetName('GetColour')
        self.winId['mapswipe:cursor:color'] = color.GetId()

        gridSizer.Add(color, pos=(row, 1), flag=wx.ALIGN_RIGHT)

        row += 1
        gridSizer.Add(
            StaticText(
                parent=panel,
                label=_("Shape:")),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL,
            pos=(
                row,
                0))
        cursors = wx.Choice(
            parent=panel,
            choices=self.settings.Get(
                group='mapswipe',
                key='cursor',
                subkey=[
                    'type',
                    'choices'],
                settings_type='internal'),
            name="GetSelection")
        cursors.SetSelection(self.settings.Get(group='mapswipe', key='cursor',
                                               subkey=['type', 'selection']))
        self.winId['mapswipe:cursor:type:selection'] = cursors.GetId()

        gridSizer.Add(cursors, flag=wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, pos=(row, 1))

        row += 1
        gridSizer.Add(
            StaticText(
                parent=panel,
                label=_("Line width:")),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL,
            pos=(
                row,
                0))
        width = SpinCtrl(
            parent=panel,
            min=1,
            max=10,
            initial=self.settings.Get(
                group='mapswipe',
                key='cursor',
                subkey='width'),
            name="GetValue")
        self.winId['mapswipe:cursor:width'] = width.GetId()

        gridSizer.Add(width, flag=wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, pos=(row, 1))

        row += 1
        gridSizer.Add(
            StaticText(
                parent=panel,
                label=_("Size:")),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL,
            pos=(
                row,
                0))
        size = SpinCtrl(
            parent=panel,
            min=4,
            max=50,
            initial=self.settings.Get(
                group='mapswipe',
                key='cursor',
                subkey='size'),
            name="GetValue")
        self.winId['mapswipe:cursor:size'] = size.GetId()

        gridSizer.Add(size, flag=wx.ALIGN_RIGHT |
                      wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, pos=(row, 1))

        gridSizer.AddGrowableCol(1)
        sizer.Add(
            gridSizer,
            proportion=1,
            flag=wx.ALL | wx.EXPAND,
            border=3)
        border.Add(sizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=3)
        panel.SetSizer(border)

        return panel
