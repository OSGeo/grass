"""
@package rdigit.toolbars

@brief rdigit toolbars and icons.

Classes:
 - toolbars::RDigitToolbar

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Anna Petrasova <kratochanna gmail.com>
"""

import wx

from gui_core.toolbars import BaseToolbar
from icons.icon import MetaIcon
from gui_core.widgets import FloatValidator
import wx.lib.colourselect as csel
from gui_core.wrap import TextCtrl, StaticText, ColourSelect


rdigitIcons = {'area': MetaIcon(img='polygon-create',
                                label=_('Digitize area')),
               'line': MetaIcon(img='line-create',
                                label=_('Digitize line')),
               'point': MetaIcon(img='point-create',
                                 label=_('Digitize point')),
               'save': MetaIcon(img='save', label=_("Save raster map")),
               'undo': MetaIcon(img='undo', label=_("Undo")),
               'help': MetaIcon(img='help', label=_("Raster Digitizer manual")),
               'quit': MetaIcon(img='quit', label=_("Quit raster digitizer"))}


class RDigitToolbar(BaseToolbar):
    """RDigit toolbar
    """

    def __init__(self, parent, giface, controller, toolSwitcher):
        """RDigit toolbar constructor
        """
        BaseToolbar.__init__(self, parent, toolSwitcher)
        self._controller = controller
        self._giface = giface
        self.InitToolbar(self._toolbarData())

        self._mapSelectionCombo = wx.ComboBox(
            self, id=wx.ID_ANY, value=_("Select raster map"),
            choices=[],
            size=(120, -1))
        self._mapSelectionCombo.Bind(wx.EVT_COMBOBOX, self.OnMapSelection)
        self._mapSelectionCombo.SetEditable(False)
        self.InsertControl(0, self._mapSelectionCombo)
        self._previousMap = self._mapSelectionCombo.GetValue()

        self._color = ColourSelect(parent=self, colour=wx.GREEN, size=(30, 30))
        self._color.Bind(
            csel.EVT_COLOURSELECT,
            lambda evt: self._changeDrawColor())
        self._color.SetToolTip(
            _("Set drawing color (not raster cell color)"))
        self.InsertControl(4, self._color)

        self._cellValues = set(['1'])
        # validator does not work with combobox, SetBackgroundColor is not
        # working
        self._valueCombo = wx.ComboBox(
            self, id=wx.ID_ANY, choices=list(
                self._cellValues), size=(
                80, -1), validator=FloatValidator())
        self._valueCombo.Bind(
            wx.EVT_COMBOBOX,
            lambda evt: self._cellValueChanged())
        self._valueCombo.Bind(wx.EVT_TEXT,
                              lambda evt: self._cellValueChanged())
        self._valueCombo.SetSelection(0)
        self._cellValueChanged()
        labelValue = StaticText(self, label=" %s" % _("Cell value:"))
        self.InsertControl(6, labelValue)
        self.InsertControl(7, self._valueCombo)

        # validator does not work with combobox, SetBackgroundColor is not
        # working
        self._widthValue = TextCtrl(
            self, id=wx.ID_ANY, value='0', size=(
                80, -1), validator=FloatValidator())
        self._widthValue.Bind(wx.EVT_TEXT,
                              lambda evt: self._widthValueChanged())
        self._widthValueChanged()
        self._widthValue.SetToolTip(
            _("Width of currently digitized line or diameter of a digitized point in map units."))
        labelWidth = StaticText(self, label=" %s" % _("Width:"))
        self.InsertControl(8, labelWidth)
        self.InsertControl(9, self._widthValue)

        for tool in (self.area, self.line, self.point):
            self.toolSwitcher.AddToolToGroup(
                group='mouseUse', toolbar=self, tool=tool)
        self.toolSwitcher.toggleToolChanged.connect(self.CheckSelectedTool)
        self._default = self.area
        # realize the toolbar
        self.Realize()
        # workaround Mac bug
        for t in (self._mapSelectionCombo, self._color, self._valueCombo,
                  self._widthValue, labelValue, labelWidth):
            t.Hide()
            t.Show()

    def _toolbarData(self):
        """Toolbar data"""
        return self._getToolbarData(
            (('area', rdigitIcons['area'],
              lambda event: self._controller.SelectType('area'),
              wx.ITEM_CHECK),
             ('line', rdigitIcons['line'],
              lambda event: self._controller.SelectType('line'),
              wx.ITEM_CHECK),
             ('point', rdigitIcons['point'],
              lambda event: self._controller.SelectType('point'),
              wx.ITEM_CHECK),
             (None,),
             (None,),
             ('undo', rdigitIcons['undo'],
              lambda event: self._controller.Undo()),
             ('save', rdigitIcons['save'],
              lambda event: self._controller.Save()),
             ('help', rdigitIcons['help'],
              lambda event: self._giface.Help('wxGUI.rdigit')),
             ('quit', rdigitIcons['quit'],
              lambda event: self._controller.Stop())))

    def CheckSelectedTool(self, id):
        if self.toolSwitcher.IsToolInGroup(tool=id, group='mouseUse') \
                and id not in (self.area, self.line, self.point):
            self._controller.SelectType(None)

    def UpdateRasterLayers(self, rasters):
        new = _("New raster map")
        items = [raster.name for raster in rasters if raster.name is not None]
        items.insert(0, new)
        self._mapSelectionCombo.SetItems(items)

    def OnMapSelection(self, event):
        """!Either map to edit or create new map selected."""
        idx = self._mapSelectionCombo.GetSelection()
        if idx == 0:
            ret = self._controller.SelectNewMap()
        else:
            ret = self._controller.SelectOldMap(
                self._mapSelectionCombo.GetString(idx))
        if not ret:
            # in wxpython 3 we can't set value which is not in the items
            # when not editable
            self._mapSelectionCombo.SetEditable(True)
            self._mapSelectionCombo.SetValue(self._previousMap)
            self._mapSelectionCombo.SetEditable(False)
        # we need to get back to previous
        self._previousMap = self._mapSelectionCombo.GetValue()

    def NewRasterAdded(self, name):
        idx = self._mapSelectionCombo.Append(name)
        self._mapSelectionCombo.SetSelection(idx)

    def UpdateCellValues(self, values=None):
        orig = self._valueCombo.GetValue()
        if not values:
            values = [orig]
        for value in values:
            self._cellValues.add(str(value))

        valList = sorted(list(self._cellValues), key=float)
        self._valueCombo.SetItems(valList)
        self._valueCombo.SetStringSelection(orig)

    def _cellValueChanged(self):
        value = self._valueCombo.GetValue()
        try:
            value = float(value)
            self._controller.SetCellValue(value)
        except ValueError:
            return

    def _widthValueChanged(self):
        value = self._widthValue.GetValue()
        try:
            value = float(value)
            self._controller.SetWidthValue(value)
        except ValueError:
            self._controller.SetWidthValue(0)
            return

    def _changeDrawColor(self):
        color = self._color.GetColour()
        self._controller.ChangeDrawColor(color=color)
