"""!
@package vdigit

@brief Dialogs for wxGUI vector digitizer

Classes:
 - VDigitSettingsDialog
 - VDigitCategoryDialog
 - CategoryListCtrl
 - VDigitZBulkDialog
 - VDigitDuplicatesDialog
 - VDigitVBuildDialog

(C) 2007-2010 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import string
import copy
import textwrap
import traceback

from threading import Thread

import wx
import wx.lib.colourselect as csel
import wx.lib.mixins.listctrl as listmix

import gcmd
import dbm
from debug import Debug as Debug
import gselect
import globalvar
from units import Units
from preferences import globalSettings as UserSettings
try:
    from wxvdigit  import IVDigit
    haveVDigit = True
    GV_LINES = 10
    errorMsg = ''
except ImportError, err:
    haveVDigit = False
    GV_LINES = None
    errorMsg = err
    
class VDigit(IVDigit):
    def __init__(self, mapwindow):
        """!Base class of vector digitizer
        
        @param mapwindow reference to mapwindow (MapFrame) instance
        """
        IVDigit.__init__(self, mapwindow)
        
class VDigitSettingsDialog(wx.Dialog):
    def __init__(self, parent, title, style = wx.DEFAULT_DIALOG_STYLE):
        """!Standard settings dialog for digitization purposes
        """
        wx.Dialog.__init__(self, parent = parent, id = wx.ID_ANY, title = title, style = style)

        self.parent = parent # mapdisplay.BufferedWindow class instance

        # notebook
        notebook = wx.Notebook(parent = self, id = wx.ID_ANY, style = wx.BK_DEFAULT)
        self.__CreateSymbologyPage(notebook)
        parent.digit.SetCategory() # update category number (next to use)
        self.__CreateGeneralPage(notebook)
        self.__CreateAttributesPage(notebook)
        self.__CreateQueryPage(notebook)

        # buttons
        btnApply = wx.Button(self, wx.ID_APPLY)
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnSave = wx.Button(self, wx.ID_SAVE)
        btnSave.SetDefault()

        # bindigs
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnApply.SetToolTipString(_("Apply changes for this session"))
        btnApply.SetDefault()
        btnSave.Bind(wx.EVT_BUTTON, self.OnSave)
        btnSave.SetToolTipString(_("Close dialog and save changes to user settings file"))
        btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        btnCancel.SetToolTipString(_("Close dialog and ignore changes"))
        
        # sizers
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnCancel)
        btnSizer.AddButton(btnApply)
        btnSizer.AddButton(btnSave)
        btnSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item = notebook, proportion = 1, flag = wx.EXPAND | wx.ALL, border = 5)
        mainSizer.Add(item = btnSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)
        
        self.Bind(wx.EVT_CLOSE, self.OnCancel)
        
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

    def __CreateSymbologyPage(self, notebook):
        """!Create notebook page concerning with symbology settings"""

        panel = wx.Panel(parent = notebook, id = wx.ID_ANY)
        notebook.AddPage(page = panel, text = _("Symbology"))

        sizer = wx.BoxSizer(wx.VERTICAL)
        
        flexSizer = wx.FlexGridSizer (cols = 3, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(0)

        self.symbology = {}
        for label, key in self.__SymbologyData():
            textLabel = wx.StaticText(panel, wx.ID_ANY, label)
            color = csel.ColourSelect(panel, id = wx.ID_ANY,
                                      colour = UserSettings.Get(group = 'vdigit', key = 'symbol',
                                                              subkey = [key, 'color']), size = globalvar.DIALOG_COLOR_SIZE)
            isEnabled = UserSettings.Get(group = 'vdigit', key = 'symbol',
                                         subkey = [key, 'enabled'])
            if isEnabled is not None:
                enabled = wx.CheckBox(panel, id = wx.ID_ANY, label = "")
                enabled.SetValue(isEnabled)
                self.symbology[key] = (enabled, color)
            else:
                enabled = (1, 1)
                self.symbology[key] = (None, color)
            
            flexSizer.Add(textLabel, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
            flexSizer.Add(enabled, proportion = 0, flag = wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
            flexSizer.Add(color, proportion = 0, flag = wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)
            color.SetName("GetColour")
        
        sizer.Add(item = flexSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 10)
        
        panel.SetSizer(sizer)
        
        return panel

    def __CreateGeneralPage(self, notebook):
        """!Create notebook page concerning with symbology settings"""

        panel = wx.Panel(parent = notebook, id = wx.ID_ANY)
        notebook.AddPage(page = panel, text = _("General"))

        border = wx.BoxSizer(wx.VERTICAL)
        
        #
        # display section
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Display"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        flexSizer = wx.FlexGridSizer (cols = 3, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(0)
        # line width
        text = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Line width"))
        self.lineWidthValue = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (75, -1),
                                          initial = UserSettings.Get(group = 'vdigit', key = "lineWidth", subkey = 'value'),
                                          min = 1, max = 1e6)
        units = wx.StaticText(parent = panel, id = wx.ID_ANY, size = (115, -1),
                              label = UserSettings.Get(group = 'vdigit', key = "lineWidth", subkey = 'units'),
                              style = wx.ALIGN_LEFT)
        flexSizer.Add(text, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(self.lineWidthValue, proportion = 0, flag = wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
        flexSizer.Add(units, proportion = 0, flag = wx.ALIGN_RIGHT | wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL | wx.LEFT,
                      border = 10)

        sizer.Add(item = flexSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 1)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)

        #
        # snapping section
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Snapping"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        flexSizer = wx.FlexGridSizer(cols = 3, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(0)

        # snapping
        text = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Snapping threshold"))
        self.snappingValue = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (75, -1),
                                         initial = UserSettings.Get(group = 'vdigit', key = "snapping", subkey = 'value'),
                                         min = -1, max = 1e6)
        self.snappingValue.Bind(wx.EVT_SPINCTRL, self.OnChangeSnappingValue)
        self.snappingValue.Bind(wx.EVT_TEXT, self.OnChangeSnappingValue)
        self.snappingUnit = wx.Choice(parent = panel, id = wx.ID_ANY, size = (125, -1),
                                      choices = ["screen pixels", "map units"])
        self.snappingUnit.SetStringSelection(UserSettings.Get(group = 'vdigit', key = "snapping", subkey = 'units'))
        self.snappingUnit.Bind(wx.EVT_CHOICE, self.OnChangeSnappingUnits)
        flexSizer.Add(text, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(self.snappingValue, proportion = 0, flag = wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
        flexSizer.Add(self.snappingUnit, proportion = 0, flag = wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)

        vertexSizer = wx.BoxSizer(wx.VERTICAL)
        self.snapVertex = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                      label = _("Snap also to vertex"))
        self.snapVertex.SetValue(UserSettings.Get(group = 'vdigit', key = "snapToVertex", subkey = 'enabled'))
        vertexSizer.Add(item = self.snapVertex, proportion = 0, flag = wx.EXPAND)
        self.mapUnits = self.parent.MapWindow.Map.GetProjInfo()['units']
        self.snappingInfo = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                          label = _("Snapping threshold is %(value).1f %(units)s") % \
                                              {'value' : self.parent.digit.GetDisplay().GetThreshold(),
                                               'units' : self.mapUnits})
        vertexSizer.Add(item = self.snappingInfo, proportion = 0,
                        flag = wx.ALL | wx.EXPAND, border = 1)

        sizer.Add(item = flexSizer, proportion = 1, flag = wx.EXPAND)
        sizer.Add(item = vertexSizer, proportion = 1, flag = wx.EXPAND)
        border.Add(item = sizer, proportion = 0, flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)

        #
        # select box
        #
        box = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Select vector features"))
        # feature type
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        inSizer = wx.BoxSizer(wx.HORIZONTAL)
        self.selectFeature = {}
        for feature in ('point', 'line',
                        'centroid', 'boundary'):
            chkbox = wx.CheckBox(parent = panel, label = feature)
            self.selectFeature[feature] = chkbox.GetId()
            chkbox.SetValue(UserSettings.Get(group = 'vdigit', key = 'selectType',
                                             subkey = [feature, 'enabled']))
            inSizer.Add(item = chkbox, proportion = 0,
                        flag = wx.EXPAND | wx.ALL, border = 5)
        sizer.Add(item = inSizer, proportion = 0, flag = wx.EXPAND)
        # threshold
        flexSizer = wx.FlexGridSizer (cols = 3, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(0)
        text = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Select threshold"))
        self.selectThreshValue = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (75, -1),
                                             initial = UserSettings.Get(group = 'vdigit', key = "selectThresh", subkey = 'value'),
                                             min = 1, max = 1e6)
        units = wx.StaticText(parent = panel, id = wx.ID_ANY, size = (115, -1),
                              label = UserSettings.Get(group = 'vdigit', key = "lineWidth", subkey = 'units'),
                              style = wx.ALIGN_LEFT)
        flexSizer.Add(text, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(self.selectThreshValue, proportion = 0, flag = wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
        flexSizer.Add(units, proportion = 0, flag = wx.ALIGN_RIGHT | wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL | wx.LEFT,
                      border = 10)

        self.selectIn = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                    label = _("Select only features inside of selection bounding box"))
        self.selectIn.SetValue(UserSettings.Get(group = 'vdigit', key = "selectInside", subkey = 'enabled'))
        self.selectIn.SetToolTipString(_("By default are selected all features overlapping selection bounding box "))
        
        self.checkForDupl = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                        label = _("Check for duplicates"))
        self.checkForDupl.SetValue(UserSettings.Get(group = 'vdigit', key = "checkForDupl", subkey = 'enabled'))


        sizer.Add(item = flexSizer, proportion = 0, flag = wx.EXPAND)
        sizer.Add(item = self.selectIn, proportion = 0, flag = wx.EXPAND | wx.ALL, border = 1)
        sizer.Add(item = self.checkForDupl, proportion = 0, flag = wx.EXPAND | wx.ALL, border = 1)        
        border.Add(item = sizer, proportion = 0, flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)

        #
        # digitize lines box
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Digitize line features"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        self.intersect = wx.CheckBox(parent = panel, label = _("Break lines at intersection"))
        self.intersect.SetValue(UserSettings.Get(group = 'vdigit', key = 'breakLines', subkey = 'enabled'))
        
        sizer.Add(item = self.intersect, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 1)

        border.Add(item = sizer, proportion = 0, flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)

        #
        # save-on-exit box
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Save changes"))
        # save changes on exit?
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        self.save = wx.CheckBox(parent = panel, label = _("Save changes on exit"))
        self.save.SetValue(UserSettings.Get(group = 'vdigit', key = 'saveOnExit', subkey = 'enabled'))
        sizer.Add(item = self.save, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 1)
        border.Add(item = sizer, proportion = 0, flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)

        panel.SetSizer(border)
        
        return panel

    def __CreateQueryPage(self, notebook):
        """!Create notebook page for query tool"""
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY)
        notebook.AddPage(page = panel, text = _("Query tool"))

        border = wx.BoxSizer(wx.VERTICAL)

        #
        # query tool box
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Choose query tool"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        LocUnits = self.parent.MapWindow.Map.GetProjInfo()['units']

        self.queryBox = wx.CheckBox(parent = panel, id = wx.ID_ANY, label = _("Select by box"))
        self.queryBox.SetValue(UserSettings.Get(group = 'vdigit', key = "query", subkey = 'box'))

        sizer.Add(item = self.queryBox, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 1)
        sizer.Add((0, 5))

        #
        # length
        #
        self.queryLength = wx.RadioButton(parent = panel, id = wx.ID_ANY, label = _("length"))
        self.queryLength.Bind(wx.EVT_RADIOBUTTON, self.OnChangeQuery)
        sizer.Add(item = self.queryLength, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 1)
        flexSizer = wx.FlexGridSizer (cols = 4, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(0)
        txt = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Select lines"))
        self.queryLengthSL = wx.Choice (parent = panel, id = wx.ID_ANY, 
                                        choices  =  [_("shorter than"), _("longer than")])
        self.queryLengthSL.SetSelection(UserSettings.Get(group = 'vdigit', key = "queryLength", subkey = 'than-selection'))
        self.queryLengthValue = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (100, -1),
                                            initial = 1,
                                            min = 0, max = 1e6)
        self.queryLengthValue.SetValue(UserSettings.Get(group = 'vdigit', key = "queryLength", subkey = 'thresh'))
        units = wx.StaticText(parent = panel, id = wx.ID_ANY, label = "%s" % LocUnits)
        flexSizer.Add(txt, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(self.queryLengthSL, proportion = 0, flag = wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
        flexSizer.Add(self.queryLengthValue, proportion = 0, flag = wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
        flexSizer.Add(units, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        sizer.Add(item = flexSizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 1)

        #
        # dangle
        #
        self.queryDangle = wx.RadioButton(parent = panel, id = wx.ID_ANY, label = _("dangle"))
        self.queryDangle.Bind(wx.EVT_RADIOBUTTON, self.OnChangeQuery)
        sizer.Add(item = self.queryDangle, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 1)
        flexSizer = wx.FlexGridSizer (cols = 4, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(0)
        txt = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Select dangles"))
        self.queryDangleSL = wx.Choice (parent = panel, id = wx.ID_ANY, 
                                        choices = [_("shorter than"), _("longer than")])
        self.queryDangleSL.SetSelection(UserSettings.Get(group = 'vdigit', key = "queryDangle", subkey = 'than-selection'))
        self.queryDangleValue = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (100, -1),
                                       initial = 1,
                                       min = 0, max = 1e6)
        self.queryDangleValue.SetValue(UserSettings.Get(group = 'vdigit', key = "queryDangle", subkey = 'thresh'))
        units = wx.StaticText(parent = panel, id = wx.ID_ANY, label = "%s" % LocUnits)
        flexSizer.Add(txt, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(self.queryDangleSL, proportion = 0, flag = wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
        flexSizer.Add(self.queryDangleValue, proportion = 0, flag = wx.ALIGN_CENTER | wx.FIXED_MINSIZE)
        flexSizer.Add(units, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        sizer.Add(item = flexSizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 1)

        if UserSettings.Get(group = 'vdigit', key = "query", subkey = 'selection') == 0:
            self.queryLength.SetValue(True)
        else:
            self.queryDangle.SetValue(True)

        # enable & disable items
        self.OnChangeQuery(None)

        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)

        panel.SetSizer(border)
        
        return panel

    def __CreateAttributesPage(self, notebook):
        """!Create notebook page for query tool"""
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY)
        notebook.AddPage(page = panel, text = _("Attributes"))

        border = wx.BoxSizer(wx.VERTICAL)

        #
        # add new record
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Digitize new feature"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        # checkbox
        self.addRecord = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                     label = _("Add new record into table"))
        self.addRecord.SetValue(UserSettings.Get(group = 'vdigit', key = "addRecord", subkey = 'enabled'))
        sizer.Add(item = self.addRecord, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 1)
        # settings
        flexSizer = wx.FlexGridSizer(cols = 2, hgap = 3, vgap = 3)
        flexSizer.AddGrowableCol(0)
        settings = ((_("Layer"), 1), (_("Category"), 1), (_("Mode"), _("Next to use")))
        # layer
        text = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Layer"))
        self.layer = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (125, -1),
                                 min = 1, max = 1e3)
        self.layer.SetValue(int(UserSettings.Get(group = 'vdigit', key = "layer", subkey = 'value')))
        flexSizer.Add(item = text, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(item = self.layer, proportion = 0,
                      flag = wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL)
        # category number
        text = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Category number"))
        self.category = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = (125, -1),
                                    initial = UserSettings.Get(group = 'vdigit', key = "category", subkey = 'value'),
                                    min = -1e9, max = 1e9) 
        if UserSettings.Get(group = 'vdigit', key = "categoryMode", subkey = 'selection') != 1:
            self.category.Enable(False)
        flexSizer.Add(item = text, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(item = self.category, proportion = 0,
                      flag = wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL)
        # category mode
        text = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Category mode"))
        self.categoryMode = wx.Choice(parent = panel, id = wx.ID_ANY, size = (125, -1),
                                      choices = [_("Next to use"), _("Manual entry"), _("No category")])
        self.categoryMode.SetSelection(UserSettings.Get(group = 'vdigit', key = "categoryMode", subkey = 'selection'))
        flexSizer.Add(item = text, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(item = self.categoryMode, proportion = 0,
                      flag = wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL)

        sizer.Add(item = flexSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 1)
        border.Add(item = sizer, proportion = 0,
                   flag = wx.ALL | wx.EXPAND, border = 5)

        #
        # digitize new area
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Digitize new area"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        # add centroid
        self.addCentroid = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                       label = _("Add centroid to left/right area"))
        self.addCentroid.SetValue(UserSettings.Get(group = 'vdigit', key = "addCentroid", subkey = 'enabled'))
        sizer.Add(item = self.addCentroid, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 1)
        
        # attach category to boundary
        self.catBoundary = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                       label = _("Do not attach category to boundaries"))
        self.catBoundary.SetValue(UserSettings.Get(group = 'vdigit', key = "catBoundary", subkey = 'enabled'))
        sizer.Add(item = self.catBoundary, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 1)
        border.Add(item = sizer, proportion = 0,
                   flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)
        
        #
        # delete existing record
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Delete existing feature(s)"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        # checkbox
        self.deleteRecord = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                        label = _("Delete record from table"))
        self.deleteRecord.SetValue(UserSettings.Get(group = 'vdigit', key = "delRecord", subkey = 'enabled'))
        sizer.Add(item = self.deleteRecord, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 1)
        border.Add(item = sizer, proportion = 0,
                   flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)

        #
        # geometry attributes (currently only length and area are supported)
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY,
                              label = " %s " % _("Geometry attributes"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(hgap = 3, vgap = 3)
        gridSizer.AddGrowableCol(0)
        self.geomAttrb = { 'length' : { 'label' : _('length') },
                           'area' : { 'label' : _('area') },
                           'perimeter' : { 'label' : _('perimeter') } }

        digitToolbar = self.parent.toolbars['vdigit']
        try:
            vectorName = digitToolbar.GetLayer().GetName()
        except AttributeError:
            vectorName = None # no vector selected for editing
        layer = UserSettings.Get(group = 'vdigit', key = "layer", subkey = 'value')
        mapLayer = self.parent.toolbars['vdigit'].GetLayer()
        tree = self.parent.tree
        item = tree.FindItemByData('maplayer', mapLayer)
        row = 0
        for attrb in ['length', 'area', 'perimeter']:
            # checkbox
            check = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                label = self.geomAttrb[attrb]['label'])
            ### self.deleteRecord.SetValue(UserSettings.Get(group='vdigit', key="delRecord", subkey='enabled'))
            check.Bind(wx.EVT_CHECKBOX, self.OnGeomAttrb)
            # column (only numeric)
            column = gselect.ColumnSelect(parent = panel, size = (200, -1))
            column.InsertColumns(vector = vectorName,
                                 layer = layer, excludeKey = True,
                                 type = ['integer', 'double precision'])
            # units 
            if attrb == 'area':
                choices = Units.GetUnitsList('area')
            else:
                choices = Units.GetUnitsList('length')
            win_units = wx.Choice(parent = panel, id = wx.ID_ANY,
                                  choices = choices, size = (120, -1))
            
            # default values
            check.SetValue(False)
            if item and tree.GetPyData(item)[0]['vdigit'] and \
                    tree.GetPyData(item)[0]['vdigit'].has_key('geomAttr') and \
                    tree.GetPyData(item)[0]['vdigit']['geomAttr'].has_key(attrb):
                check.SetValue(True)
                column.SetStringSelection(tree.GetPyData(item)[0]['vdigit']['geomAttr'][attrb]['column'])
                if attrb == 'area':
                    type = 'area'
                else:
                    type = 'length'
                unitsIdx = Units.GetUnitsIndex(type, 
                                                tree.GetPyData(item)[0]['vdigit']['geomAttr'][attrb]['units'])
                win_units.SetSelection(unitsIdx)

            if not vectorName:
                check.Enable(False)
                column.Enable(False)
            
            if not check.IsChecked():
                column.Enable(False)
            
            self.geomAttrb[attrb]['check']  = check.GetId()
            self.geomAttrb[attrb]['column'] = column.GetId()
            self.geomAttrb[attrb]['units']  = win_units.GetId()

            gridSizer.Add(item = check,
                          flag = wx.ALIGN_CENTER_VERTICAL,
                          pos = (row, 0))
            gridSizer.Add(item = column,
                          pos = (row, 1))
            gridSizer.Add(item = win_units,
                          pos = (row, 2))
            row += 1
        
        note = '\n'.join(textwrap.wrap(_("Note: These settings are stored "
                                         " in the workspace not in the vector digitizer "
                                         "preferences."), 55))
        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                           label = note),
                      pos = (3, 0), span = (1, 3))
                      
        sizer.Add(item = gridSizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 1)
        border.Add(item = sizer, proportion = 0,
                   flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)

        # bindings
        self.Bind(wx.EVT_CHECKBOX, self.OnChangeAddRecord, self.addRecord)
        self.Bind(wx.EVT_CHOICE, self.OnChangeCategoryMode, self.categoryMode)
        self.Bind(wx.EVT_SPINCTRL, self.OnChangeLayer, self.layer)

        panel.SetSizer(border)
        
        return panel

    def __SymbologyData(self):
        """!Data for __CreateSymbologyPage()

        label | checkbox | color
        """

        return (
            #            ("Background", "symbolBackground"),
            (_("Highlight"), "highlight"),
            (_("Highlight (duplicates)"), "highlightDupl"),
            (_("Point"), "point"),
            (_("Line"), "line"),
            (_("Boundary (no area)"), "boundaryNo"),
            (_("Boundary (one area)"), "boundaryOne"),
            (_("Boundary (two areas)"), "boundaryTwo"),
            (_("Centroid (in area)"), "centroidIn"),
            (_("Centroid (outside area)"), "centroidOut"),
            (_("Centroid (duplicate in area)"), "centroidDup"),
            (_("Node (one line)"), "nodeOne"),
            (_("Node (two lines)"), "nodeTwo"),
            (_("Vertex"), "vertex"),
            (_("Area (closed boundary + centroid)"), "area"),
            (_("Direction"), "direction"),)

    def OnGeomAttrb(self, event):
        """!Register geometry attributes (enable/disable)"""
        checked = event.IsChecked()
        id = event.GetId()
        key = None
        for attrb, val in self.geomAttrb.iteritems():
            if val['check'] == id:
                key = attrb
                break
        
        column = self.FindWindowById(self.geomAttrb[key]['column'])
        if checked:
            column.Enable()
        else:
            column.Enable(False)
        
    def OnChangeCategoryMode(self, event):
        """!Change category mode"""
        mode = event.GetSelection()
        UserSettings.Set(group = 'vdigit', key = "categoryMode", subkey = 'selection', value = mode)
        if mode == 1: # manual entry
            self.category.Enable(True)
        elif self.category.IsEnabled(): # disable
            self.category.Enable(False)

        if mode == 2 and self.addRecord.IsChecked(): # no category
            self.addRecord.SetValue(False)

        self.parent.digit.SetCategory()
        self.category.SetValue(UserSettings.Get(group = 'vdigit', key = 'category', subkey = 'value'))

    def OnChangeLayer(self, event):
        """!Layer changed"""
        layer = event.GetInt()
        if layer > 0:
            UserSettings.Set(group = 'vdigit', key = 'layer', subkey = 'value', value = layer)
            self.parent.digit.SetCategory()
            self.category.SetValue(UserSettings.Get(group = 'vdigit', key = 'category', subkey = 'value'))
            
        event.Skip()

    def OnChangeAddRecord(self, event):
        """!Checkbox 'Add new record' status changed"""
        self.category.SetValue(self.parent.digit.SetCategory())
            
    def OnChangeSnappingValue(self, event):
        """!Change snapping value - update static text"""
        value = self.snappingValue.GetValue()
        
        if value < 0:
            region = self.parent.MapWindow.Map.GetRegion()
            res = (region['nsres'] + region['ewres']) / 2.
            threshold = self.parent.digit.driver.GetThreshold(value = res)
        else:
            if self.snappingUnit.GetStringSelection() == "map units":
                threshold = value
            else:
                threshold = self.parent.digit.driver.GetThreshold(value = value)
            
        if value == 0:
            self.snappingInfo.SetLabel(_("Snapping disabled"))
        elif value < 0:
            self.snappingInfo.SetLabel(_("Snapping threshold is %(value).1f %(units)s "
                                         "(based on comp. resolution)") % 
                                       {'value' : threshold,
                                        'units' : self.mapUnits.lower()})
        else:
            self.snappingInfo.SetLabel(_("Snapping threshold is %(value).1f %(units)s") % 
                                       {'value' : threshold,
                                        'units' : self.mapUnits.lower()})
        
        event.Skip()

    def OnChangeSnappingUnits(self, event):
        """!Snapping units change -> update static text"""
        value = self.snappingValue.GetValue()
        units = self.snappingUnit.GetStringSelection()
        threshold = self.parent.digit.driver.GetThreshold(value = value, units = units)

        if units == "map units":
            self.snappingInfo.SetLabel(_("Snapping threshold is %(value).1f %(units)s") % 
                                       {'value' : value,
                                        'units' : self.mapUnits})
        else:
            self.snappingInfo.SetLabel(_("Snapping threshold is %(value).1f %(units)s") % 
                                       {'value' : threshold,
                                        'units' : self.mapUnits})
            
        event.Skip()

    def OnChangeQuery(self, event):
        """!Change query"""
        if self.queryLength.GetValue():
            # length
            self.queryLengthSL.Enable(True)
            self.queryLengthValue.Enable(True)
            self.queryDangleSL.Enable(False)
            self.queryDangleValue.Enable(False)
        else:
            # dangle
            self.queryLengthSL.Enable(False)
            self.queryLengthValue.Enable(False)
            self.queryDangleSL.Enable(True)
            self.queryDangleValue.Enable(True)

    def OnSave(self, event):
        """!Button 'Save' clicked"""
        self.UpdateSettings()
        self.parent.toolbars['vdigit'].settingsDialog = None

        fileSettings = {}
        UserSettings.ReadSettingsFile(settings = fileSettings)
        fileSettings['vdigit'] = UserSettings.Get(group = 'vdigit')
        
        file = UserSettings.SaveToFile(fileSettings)
        self.parent.GetLayerManager().goutput.WriteLog(_('Vector digitizer settings saved to file <%s>.') % file)
        
        self.Destroy()

        event.Skip()
        
    def OnApply(self, event):
        """!Button 'Apply' clicked"""
        self.UpdateSettings()

    def OnCancel(self, event):
        """!Button 'Cancel' clicked"""
        self.parent.toolbars['vdigit'].settingsDialog = None
        self.Destroy()

        if event:
            event.Skip()
        
    def UpdateSettings(self):
        """!Update digitizer settings
        """
        # symbology
        for key, (enabled, color) in self.symbology.iteritems():
            if enabled:
                UserSettings.Set(group = 'vdigit', key = 'symbol',
                                 subkey = [key, 'enabled'],
                                 value = enabled.IsChecked())
                UserSettings.Set(group = 'vdigit', key = 'symbol',
                                 subkey = [key, 'color'],
                                 value = tuple(color.GetColour()))
            else:
                UserSettings.Set(group = 'vdigit', key = 'symbol',
                                 subkey = [key, 'color'],
                                 value = tuple(color.GetColour()))
        # display
        UserSettings.Set(group = 'vdigit', key = "lineWidth", subkey = 'value',
                         value = int(self.lineWidthValue.GetValue()))

        # snapping
        UserSettings.Set(group = 'vdigit', key = "snapping", subkey = 'value',
                         value = int(self.snappingValue.GetValue()))
        UserSettings.Set(group = 'vdigit', key = "snapping", subkey = 'units',
                         value = self.snappingUnit.GetStringSelection())
        UserSettings.Set(group = 'vdigit', key = "snapToVertex", subkey = 'enabled',
                         value = self.snapVertex.IsChecked())
        
        # digitize new feature
        UserSettings.Set(group = 'vdigit', key = "addRecord", subkey = 'enabled',
                         value = self.addRecord.IsChecked())
        UserSettings.Set(group = 'vdigit', key = "layer", subkey = 'value',
                         value = int(self.layer.GetValue()))
        UserSettings.Set(group = 'vdigit', key = "category", subkey = 'value',
                         value = int(self.category.GetValue()))
        UserSettings.Set(group = 'vdigit', key = "categoryMode", subkey = 'selection',
                         value = self.categoryMode.GetSelection())

        # digitize new area
        UserSettings.Set(group = 'vdigit', key = "addCentroid", subkey = 'enabled',
                         value = self.addCentroid.IsChecked())
        UserSettings.Set(group = 'vdigit', key = "catBoundary", subkey = 'enabled',
                         value = self.catBoundary.IsChecked())
        
        # delete existing feature
        UserSettings.Set(group = 'vdigit', key = "delRecord", subkey = 'enabled',
                         value = self.deleteRecord.IsChecked())

        # geometry attributes (workspace)
        mapLayer = self.parent.toolbars['vdigit'].GetLayer()
        tree = self.parent.tree
        item = tree.FindItemByData('maplayer', mapLayer)
        for key, val in self.geomAttrb.iteritems():
            checked = self.FindWindowById(val['check']).IsChecked()
            column  = self.FindWindowById(val['column']).GetValue()
            unitsIdx = self.FindWindowById(val['units']).GetSelection()
            if item and not tree.GetPyData(item)[0]['vdigit']: 
                tree.GetPyData(item)[0]['vdigit'] = { 'geomAttr' : dict() }
            
            if checked: # enable
                if key == 'area':
                    type = key
                else:
                    type = 'length'
                unitsKey = Units.GetUnitsKey(type, unitsIdx)
                tree.GetPyData(item)[0]['vdigit']['geomAttr'][key] = { 'column' : column,
                                                                       'units' : unitsKey }
            else:
                if item and tree.GetPyData(item)[0]['vdigit'] and \
                        tree.GetPyData(item)[0]['vdigit']['geomAttr'].has_key(key):
                    del tree.GetPyData(item)[0]['vdigit']['geomAttr'][key]
        
        # query tool
        if self.queryLength.GetValue():
            UserSettings.Set(group = 'vdigit', key = "query", subkey = 'selection',
                             value = 0)
        else:
            UserSettings.Set(group = 'vdigit', key = "query", subkey = 'type',
                             value = 1)
        UserSettings.Set(group = 'vdigit', key = "query", subkey = 'box',
                         value = self.queryBox.IsChecked())
        UserSettings.Set(group = 'vdigit', key = "queryLength", subkey = 'than-selection',
                         value = self.queryLengthSL.GetSelection())
        UserSettings.Set(group = 'vdigit', key = "queryLength", subkey = 'thresh',
                         value = int(self.queryLengthValue.GetValue()))
        UserSettings.Set(group = 'vdigit', key = "queryDangle", subkey = 'than-selection',
                         value = self.queryDangleSL.GetSelection())
        UserSettings.Set(group = 'vdigit', key = "queryDangle", subkey = 'thresh',
                         value = int(self.queryDangleValue.GetValue()))

        # select features
        for feature in ('point', 'line',
                        'centroid', 'boundary'):
            UserSettings.Set(group = 'vdigit', key = 'selectType',
                             subkey = [feature, 'enabled'],
                             value = self.FindWindowById(self.selectFeature[feature]).IsChecked())
        UserSettings.Set(group = 'vdigit', key = "selectThresh", subkey = 'value',
                         value = int(self.selectThreshValue.GetValue()))
        UserSettings.Set(group = 'vdigit', key = "checkForDupl", subkey = 'enabled',
                         value = self.checkForDupl.IsChecked())
        UserSettings.Set(group = 'vdigit', key = "selectInside", subkey = 'enabled',
                         value = self.selectIn.IsChecked())

        # on-exit
        UserSettings.Set(group = 'vdigit', key = "saveOnExit", subkey = 'enabled',
                         value = self.save.IsChecked())

        # break lines
        UserSettings.Set(group = 'vdigit', key = "breakLines", subkey = 'enabled',
                         value = self.intersect.IsChecked())
        
        self.parent.digit.GetDisplay().UpdateSettings()
        
        # redraw map if auto-rendering is enabled
        if self.parent.statusbarWin['render'].GetValue(): 
            self.parent.OnRender(None)

class VDigitCategoryDialog(wx.Dialog, listmix.ColumnSorterMixin):
    def __init__(self, parent, title,
                 map, query = None, cats = None,
                 pos = wx.DefaultPosition,
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):
        """!Dialog used to display/modify categories of vector objects
        
        @param parent
        @param title dialog title
        @param query {coordinates, qdist} - used by v.edit/v.what
        @param cats  directory of lines (layer/categories) - used by vdigit
        @param pos
        @param style
        """
        # parent
        self.parent = parent # mapdisplay.BufferedWindow class instance

        # map name
        self.map = map

        # line : {layer: [categories]}
        self.cats = {}
        
        # do not display dialog if no line is found (-> self.cats)
        if cats is None:
            if self.__GetCategories(query[0], query[1]) == 0 or not self.line:
                Debug.msg(3, "VDigitCategoryDialog(): nothing found!")
        else:
            self.cats = cats
            for line in cats.keys():
                for layer in cats[line].keys():
                    self.cats[line][layer] = list(cats[line][layer])
            
            layers = []
            for layer in self.parent.parent.digit.GetLayers():
                layers.append(str(layer))
        
        # make copy of cats (used for 'reload')
        self.cats_orig = copy.deepcopy(self.cats)
        
        wx.Dialog.__init__(self, parent = self.parent, id = wx.ID_ANY, title = title,
                          style = style, pos = pos)
        
        # list of categories
        box = wx.StaticBox(parent = self, id = wx.ID_ANY,
                           label = " %s " % _("List of categories - right-click to delete"))
        listSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        self.list = CategoryListCtrl(parent = self, id = wx.ID_ANY,
                                     style = wx.LC_REPORT |
                                     wx.BORDER_NONE |
                                     wx.LC_SORT_ASCENDING |
                                     wx.LC_HRULES |
                                     wx.LC_VRULES)
        # sorter
        self.fid = self.cats.keys()[0]
        self.itemDataMap = self.list.Populate(self.cats[self.fid])
        listmix.ColumnSorterMixin.__init__(self, 2)
        self.fidMulti = wx.Choice(parent = self, id = wx.ID_ANY,
                                  size = (150, -1))
        self.fidMulti.Bind(wx.EVT_CHOICE, self.OnFeature)
        self.fidText = wx.StaticText(parent = self, id = wx.ID_ANY)
        if len(self.cats.keys()) == 1:
            self.fidMulti.Show(False)
            self.fidText.SetLabel(str(self.fid))
        else:
            self.fidText.Show(False)
            choices = []
            for fid in self.cats.keys():
                choices.append(str(fid))
            self.fidMulti.SetItems(choices)
            self.fidMulti.SetSelection(0)
        
        listSizer.Add(item = self.list, proportion = 1, flag = wx.EXPAND)

        # add new category
        box = wx.StaticBox(parent = self, id = wx.ID_ANY,
                           label = " %s " % _("Add new category"))
        addSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        flexSizer = wx.FlexGridSizer (cols = 5, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(3)

        layerNewTxt = wx.StaticText(parent = self, id = wx.ID_ANY,
                                 label = "%s:" % _("Layer"))
        self.layerNew = wx.Choice(parent = self, id = wx.ID_ANY, size = (75, -1),
                                  choices = layers)
        if len(layers) > 0:
            self.layerNew.SetSelection(0)
        
        catNewTxt = wx.StaticText(parent = self, id = wx.ID_ANY,
                               label = "%s:" % _("Category"))

        try:
            newCat = max(self.cats[self.fid][1]) + 1
        except KeyError:
            newCat = 1
        self.catNew = wx.SpinCtrl(parent = self, id = wx.ID_ANY, size = (75, -1),
                                  initial = newCat, min = 0, max = 1e9)
        btnAddCat = wx.Button(self, wx.ID_ADD)
        flexSizer.Add(item = layerNewTxt, proportion = 0,
                      flag = wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(item = self.layerNew, proportion = 0,
                      flag = wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(item = catNewTxt, proportion = 0,
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT | wx.LEFT,
                      border = 10)
        flexSizer.Add(item = self.catNew, proportion = 0,
                      flag = wx.FIXED_MINSIZE | wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(item = btnAddCat, proportion = 0,
                      flag = wx.EXPAND | wx.ALIGN_RIGHT | wx.FIXED_MINSIZE)
        addSizer.Add(item = flexSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 5)

        # buttons
        btnApply = wx.Button(self, wx.ID_APPLY)
        btnApply.SetToolTipString(_("Apply changes"))
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnCancel.SetToolTipString(_("Ignore changes and close dialog"))
        btnOk = wx.Button(self, wx.ID_OK)
        btnOk.SetToolTipString(_("Apply changes and close dialog"))
        btnOk.SetDefault()

        # sizers
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnCancel)
        #btnSizer.AddButton(btnReload)
        #btnSizer.SetNegativeButton(btnReload)
        btnSizer.AddButton(btnApply)
        btnSizer.AddButton(btnOk)
        btnSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item = listSizer, proportion = 1,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)
        mainSizer.Add(item = addSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALIGN_CENTER |
                      wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        fidSizer = wx.BoxSizer(wx.HORIZONTAL)
        fidSizer.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY,
                                        label = _("Feature id:")),
                     proportion = 0, border = 5,
                     flag = wx.ALIGN_CENTER_VERTICAL)
        fidSizer.Add(item = self.fidMulti, proportion = 0,
                     flag = wx.EXPAND | wx.ALL,  border = 5)
        fidSizer.Add(item = self.fidText, proportion = 0,
                     flag = wx.EXPAND | wx.ALL,  border = 5)
        mainSizer.Add(item = fidSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL, border = 5)
        mainSizer.Add(item = btnSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)
        
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)
        self.SetAutoLayout(True)

        # set min size for dialog
        self.SetMinSize(self.GetBestSize())

        # bindings
        # buttons
        #btnReload.Bind(wx.EVT_BUTTON, self.OnReload)
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnOk.Bind(wx.EVT_BUTTON, self.OnOK)
        btnAddCat.Bind(wx.EVT_BUTTON, self.OnAddCat)
        btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)
                                     
        # list
        # self.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemSelected, self.list)
        # self.list.Bind(wx.EVT_RIGHT_DOWN, self.OnRightDown)
        self.list.Bind(wx.EVT_COMMAND_RIGHT_CLICK, self.OnRightUp) #wxMSW
        self.list.Bind(wx.EVT_RIGHT_UP, self.OnRightUp) #wxGTK
        self.Bind(wx.EVT_LIST_BEGIN_LABEL_EDIT, self.OnBeginEdit, self.list)
        self.Bind(wx.EVT_LIST_END_LABEL_EDIT, self.OnEndEdit, self.list)
        self.Bind(wx.EVT_LIST_COL_CLICK, self.OnColClick, self.list)

    def GetListCtrl(self):
        """!Used by ColumnSorterMixin"""
        return self.list

    def OnColClick(self, event):
        """!Click on column header (order by)"""
        event.Skip()
        
    def OnBeginEdit(self, event):
        """!Editing of item started"""
        event.Allow()

    def OnEndEdit(self, event):
        """!Finish editing of item"""
        itemIndex = event.GetIndex()
        layerOld = int (self.list.GetItem(itemIndex, 0).GetText())
        catOld = int (self.list.GetItem(itemIndex, 1).GetText())

        if event.GetColumn() == 0:
            layerNew = int(event.GetLabel())
            catNew = catOld
        else:
            layerNew = layerOld
            catNew = int(event.GetLabel())
        
        try:
            if layerNew not in self.cats[self.fid].keys():
                self.cats[self.fid][layerNew] = []
            self.cats[self.fid][layerNew].append(catNew)
            self.cats[self.fid][layerOld].remove(catOld)
        except:
            event.Veto()
            self.list.SetStringItem(itemIndex, 0, str(layerNew))
            self.list.SetStringItem(itemIndex, 1, str(catNew))
            dlg = wx.MessageDialog(self, _("Unable to add new layer/category <%(layer)s/%(category)s>.\n"
                                           "Layer and category number must be integer.\n"
                                           "Layer number must be greater then zero.") %
                                   { 'layer': self.layerNew.GetStringSelection(),
                                     'category' : str(self.catNew.GetValue()) },
                                   _("Error"), wx.OK | wx.ICON_ERROR)
            dlg.ShowModal()
            dlg.Destroy()
            return False

    def OnRightDown(self, event):
        """!Mouse right button down"""
        x = event.GetX()
        y = event.GetY()
        item, flags = self.list.HitTest((x, y))

        if item !=  wx.NOT_FOUND and \
                flags & wx.LIST_HITTEST_ONITEM:
            self.list.Select(item)

        event.Skip()

    def OnRightUp(self, event):
        """!Mouse right button up"""
        if not hasattr(self, "popupID1"):
            self.popupID1 = wx.NewId()
            self.popupID2 = wx.NewId()
            self.popupID3 = wx.NewId()
            self.Bind(wx.EVT_MENU, self.OnItemDelete,    id = self.popupID1)
            self.Bind(wx.EVT_MENU, self.OnItemDeleteAll, id = self.popupID2)
            self.Bind(wx.EVT_MENU, self.OnReload, id = self.popupID3)

        # generate popup-menu
        menu = wx.Menu()
        menu.Append(self.popupID1, _("Delete selected"))
        if self.list.GetFirstSelected() == -1:
            menu.Enable(self.popupID1, False)

        menu.Append(self.popupID2, _("Delete all"))
        menu.AppendSeparator()
        menu.Append(self.popupID3, _("Reload"))

        self.PopupMenu(menu)
        menu.Destroy()

    def OnItemSelected(self, event):
        """!Item selected"""
        event.Skip()

    def OnItemDelete(self, event):
        """!Delete selected item(s) from the list (layer/category pair)"""
        item = self.list.GetFirstSelected()
        while item != -1:
            layer = int (self.list.GetItem(item, 0).GetText())
            cat = int (self.list.GetItem(item, 1).GetText())
            self.list.DeleteItem(item)
            self.cats[self.fid][layer].remove(cat)

            item = self.list.GetFirstSelected()
            
        event.Skip()
        
    def OnItemDeleteAll(self, event):
        """!Delete all items from the list"""
        self.list.DeleteAllItems()
        self.cats[self.fid] = {}

        event.Skip()

    def OnFeature(self, event):
        """!Feature id changed (on duplicates)"""
        self.fid = int(event.GetString())
        
        self.itemDataMap = self.list.Populate(self.cats[self.fid],
                                              update = True)

        try:
            newCat = max(self.cats[self.fid][1]) + 1
        except KeyError:
            newCat = 1
            
        self.catNew.SetValue(newCat)
        
        event.Skip()
        
    def __GetCategories(self, coords, qdist):
        """!Get layer/category pairs for all available
        layers

        Return True line found or False if not found"""
        
        ret = gcmd.RunCommand('v.what',
                              parent = self,
                              quiet = True,
                              map = self.map,
                              east_north = '%f,%f' % \
                                  (float(coords[0]), float(coords[1])),
                              distance = qdist)

        if not ret:
            return False

        for item in ret.splitlines():
            litem = item.lower()
            if "id:" in litem: # get line id
                self.line = int(item.split(':')[1].strip())
            elif "layer:" in litem: # add layer
                layer = int(item.split(':')[1].strip())
                if layer not in self.cats.keys():
                    self.cats[layer] = []
            elif "category:" in litem: # add category
                self.cats[layer].append(int(item.split(':')[1].strip()))

        return True

    def OnReload(self, event):
        """!Reload button pressed"""
        # restore original list
        self.cats = copy.deepcopy(self.cats_orig)

        # polulate list
        self.itemDataMap = self.list.Populate(self.cats[self.fid],
                                              update = True)

        event.Skip()

    def OnCancel(self, event):
        """!Cancel button pressed"""
        self.parent.parent.dialogs['category'] = None
        if self.parent.parent.digit:
            self.parent.parent.digit.driver.SetSelected([])
            self.parent.UpdateMap(render = False)
        else:
            self.parent.parent.OnRender(None)
            
        self.Close()

    def OnApply(self, event):
        """!Apply button pressed"""
        for fid in self.cats.keys():
            newfid = self.ApplyChanges(fid)
            if fid == self.fid:
                self.fid = newfid
            
    def ApplyChanges(self, fid):
        cats = self.cats[fid]
        cats_orig = self.cats_orig[fid]

        # action : (catsFrom, catsTo)
        check = {'catadd': (cats,      cats_orig),
                 'catdel': (cats_orig, cats)}

        newfid = -1
        
        # add/delete new category
        for action, catsCurr in check.iteritems():
            for layer in catsCurr[0].keys():
                catList = []
                for cat in catsCurr[0][layer]:
                    if layer not in catsCurr[1].keys() or \
                            cat not in catsCurr[1][layer]:
                        catList.append(cat)
                if catList != []:
                    if action == 'catadd':
                        add = True
                    else:
                        add = False
                        
                    newfid = self.parent.parent.digit.SetLineCats(fid, layer,
                                                                      catList, add)
                    if len(self.cats.keys()) == 1:
                        self.fidText.SetLabel("%d" % newfid)
                    else:
                        choices = self.fidMulti.GetItems()
                        choices[choices.index(str(fid))] = str(newfid)
                        self.fidMulti.SetItems(choices)
                        self.fidMulti.SetStringSelection(str(newfid))
                    
                    self.cats[newfid] = self.cats[fid]
                    del self.cats[fid]
                    
                    fid = newfid
                    if self.fid < 0:
                        wx.MessageBox(parent = self, message = _("Unable to update vector map."),
                                      caption = _("Error"), style = wx.OK | wx.ICON_ERROR)
        
        self.cats_orig[fid] = copy.deepcopy(cats)
        
        return newfid

    def OnOK(self, event):
        """!OK button pressed"""
        self.OnApply(event)
        self.OnCancel(event)

    def OnAddCat(self, event):
        """!Button 'Add' new category pressed"""
        try:
            layer = int(self.layerNew.GetStringSelection())
            cat   = int(self.catNew.GetValue())
            if layer <= 0:
                raise ValueError
        except ValueError:
            dlg = wx.MessageDialog(self, _("Unable to add new layer/category <%(layer)s/%(category)s>.\n"
                                           "Layer and category number must be integer.\n"
                                           "Layer number must be greater then zero.") %
                                   {'layer' : str(self.layerNew.GetValue()),
                                    'category' : str(self.catNew.GetValue())},
                                   _("Error"), wx.OK | wx.ICON_ERROR)
            dlg.ShowModal()
            dlg.Destroy()
            return False

        if layer not in self.cats[self.fid].keys():
            self.cats[self.fid][layer] = []

        self.cats[self.fid][layer].append(cat)

        # reload list
        self.itemDataMap = self.list.Populate(self.cats[self.fid],
                                              update = True)

        # update category number for add
        self.catNew.SetValue(cat + 1)

        event.Skip()

        return True

    def GetLine(self):
        """!Get id of selected line of 'None' if no line is selected"""
        return self.cats.keys()

    def UpdateDialog(self, query = None, cats = None):
        """!Update dialog
        
        @param query {coordinates, distance} - v.edit/v.what
        @param cats  directory layer/cats    - vdigit
        Return True if updated otherwise False
        """
        # line: {layer: [categories]}
        self.cats = {}
        # do not display dialog if no line is found (-> self.cats)
        if cats is None:
            ret = self.__GetCategories(query[0], query[1])
        else:
            self.cats = cats
            for line in cats.keys():
                for layer in cats[line].keys():
                    self.cats[line][layer] = list(cats[line][layer])
            ret = 1
        if ret == 0 or len(self.cats.keys()) < 1:
            Debug.msg(3, "VDigitCategoryDialog(): nothing found!")
            return False
        
        # make copy of cats (used for 'reload')
        self.cats_orig = copy.deepcopy(self.cats)

        # polulate list
        self.fid = self.cats.keys()[0]
        self.itemDataMap = self.list.Populate(self.cats[self.fid],
                                              update = True)

        try:
            newCat = max(self.cats[self.fid][1]) + 1
        except KeyError:
            newCat = 1
        self.catNew.SetValue(newCat)
        
        if len(self.cats.keys()) == 1:
            self.fidText.Show(True)
            self.fidMulti.Show(False)
            self.fidText.SetLabel("%d" % self.fid)
        else:
            self.fidText.Show(False)
            self.fidMulti.Show(True)
            choices = []
            for fid in self.cats.keys():
                choices.append(str(fid))
            self.fidMulti.SetItems(choices)
            self.fidMulti.SetSelection(0)

        self.Layout()
        
        return True

class CategoryListCtrl(wx.ListCtrl,
                       listmix.ListCtrlAutoWidthMixin,
                       listmix.TextEditMixin):
    def __init__(self, parent, id, pos = wx.DefaultPosition,
                 size = wx.DefaultSize, style = 0):
        """!List of layers/categories"""
        self.parent = parent
        
        wx.ListCtrl.__init__(self, parent, id, pos, size, style)

        listmix.ListCtrlAutoWidthMixin.__init__(self)
        listmix.TextEditMixin.__init__(self)

    def Populate(self, cats, update = False):
        """!Populate the list"""
        itemData = {} # requested by sorter

        if not update:
            self.InsertColumn(0, _("Layer"))
            self.InsertColumn(1, _("Category"))
        else:
            self.DeleteAllItems()

        i = 1
        for layer in cats.keys():
            catsList = cats[layer]
            for cat in catsList:
                index = self.InsertStringItem(sys.maxint, str(catsList[0]))
                self.SetStringItem(index, 0, str(layer))
                self.SetStringItem(index, 1, str(cat))
                self.SetItemData(index, i)
                itemData[i] = (str(layer), str(cat))
                i = i + 1

        if not update:
            self.SetColumnWidth(0, 100)
            self.SetColumnWidth(1, wx.LIST_AUTOSIZE)

        self.currentItem = 0

        return itemData

class VDigitZBulkDialog(wx.Dialog):
    def __init__(self, parent, title, nselected, style = wx.DEFAULT_DIALOG_STYLE):
        """!Dialog used for Z bulk-labeling tool
        """
        wx.Dialog.__init__(self, parent = parent, id = wx.ID_ANY, title = title, style = style)

        self.parent = parent # mapdisplay.BufferedWindow class instance

        # panel  = wx.Panel(parent=self, id=wx.ID_ANY)

        border = wx.BoxSizer(wx.VERTICAL)
        
        txt = wx.StaticText(parent = self,
                            label = _("%d lines selected for z bulk-labeling") % nselected);
        border.Add(item = txt, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)

        box   = wx.StaticBox (parent = self, id = wx.ID_ANY, label = " %s " % _("Set value"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        flexSizer = wx.FlexGridSizer (cols = 2, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(0)

        # starting value
        txt = wx.StaticText(parent = self,
                            label = _("Starting value"));
        self.value = wx.SpinCtrl(parent = self, id = wx.ID_ANY, size = (150, -1),
                                 initial = 0,
                                 min = -1e6, max = 1e6)
        flexSizer.Add(txt, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(self.value, proportion = 0, flag = wx.ALIGN_CENTER | wx.FIXED_MINSIZE)

        # step
        txt = wx.StaticText(parent = self,
                            label = _("Step"))
        self.step  = wx.SpinCtrl(parent = self, id = wx.ID_ANY, size = (150, -1),
                                 initial = 0,
                                 min = 0, max = 1e6)
        flexSizer.Add(txt, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL)
        flexSizer.Add(self.step, proportion = 0, flag = wx.ALIGN_CENTER | wx.FIXED_MINSIZE)

        sizer.Add(item = flexSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 1)
        border.Add(item = sizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 0)

        # buttons
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnOk = wx.Button(self, wx.ID_OK)
        btnOk.SetDefault()

        # sizers
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnCancel)
        btnSizer.AddButton(btnOk)
        btnSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item = border, proportion = 1, flag = wx.EXPAND | wx.ALL, border = 5)
        mainSizer.Add(item = btnSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

class VDigitDuplicatesDialog(wx.Dialog):
    def __init__(self, parent, data, title = _("List of duplicates"),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
                 pos = wx.DefaultPosition):
        """!Show duplicated feature ids
        """
        wx.Dialog.__init__(self, parent = parent, id = wx.ID_ANY, title = title, style = style,
                           pos = pos)
        
        self.parent = parent # BufferedWindow
        self.data = data
        self.winList = []

        # panel  = wx.Panel(parent=self, id=wx.ID_ANY)

        # notebook
        self.notebook = wx.Notebook(parent = self, id = wx.ID_ANY, style = wx.BK_DEFAULT)

        id = 1
        for key in self.data.keys():
            panel = wx.Panel(parent = self.notebook, id = wx.ID_ANY)
            self.notebook.AddPage(page = panel, text = " %d " % (id))
            
            # notebook body
            border = wx.BoxSizer(wx.VERTICAL)

            win = CheckListFeature(parent = panel, data = list(self.data[key]))
            self.winList.append(win.GetId())

            border.Add(item = win, proportion = 1,
                       flag = wx.ALL | wx.EXPAND, border = 5)

            panel.SetSizer(border)

            id += 1

        # buttons
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnOk = wx.Button(self, wx.ID_OK)
        btnOk.SetDefault()

        # sizers
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnCancel)
        btnSizer.AddButton(btnOk)
        btnSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item = self.notebook, proportion = 1, flag = wx.EXPAND | wx.ALL, border = 5)
        mainSizer.Add(item = btnSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)
        self.SetAutoLayout(True)

        # set min size for dialog
        self.SetMinSize((250, 180))

    def GetUnSelected(self):
        """!Get unselected items (feature id)

        @return list of ids
        """
        ids = []
        for id in self.winList:
            wlist = self.FindWindowById(id)

            for item in range(wlist.GetItemCount()):
                if not wlist.IsChecked(item):
                    ids.append(int(wlist.GetItem(item, 0).GetText()))
                    
        return ids

class CheckListFeature(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin, listmix.CheckListCtrlMixin):
    def __init__(self, parent, data,
                 pos = wx.DefaultPosition, log = None):
        """!List of mapset/owner/group"""
        self.parent = parent
        self.data = data

        wx.ListCtrl.__init__(self, parent, wx.ID_ANY,
                             style = wx.LC_REPORT)

        listmix.CheckListCtrlMixin.__init__(self)

        self.log = log

        # setup mixins
        listmix.ListCtrlAutoWidthMixin.__init__(self)

        self.LoadData(self.data)

    def LoadData(self, data):
        """!Load data into list"""
        self.InsertColumn(0, _('Feature id'))
        self.InsertColumn(1, _('Layer (Categories)'))

        for item in data:
            index = self.InsertStringItem(sys.maxint, str(item[0]))
            self.SetStringItem(index, 1, str(item[1]))

        # enable all items by default
        for item in range(self.GetItemCount()):
            self.CheckItem(item, True)

        self.SetColumnWidth(col = 0, width = wx.LIST_AUTOSIZE_USEHEADER)
        self.SetColumnWidth(col = 1, width = wx.LIST_AUTOSIZE_USEHEADER)
                
    def OnCheckItem(self, index, flag):
        """!Mapset checked/unchecked"""
        pass
