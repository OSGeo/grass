"""!
@package vdigit.preferences

@brief wxGUI vector digitizer preferences dialogs

Classes:
 - preferences::VDigitSettingsDialog

(C) 2007-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import textwrap

import wx
import wx.lib.colourselect as csel

from core             import globalvar
from core.debug       import Debug
from gui_core.gselect import ColumnSelect
from core.units       import Units
from core.settings    import UserSettings

class VDigitSettingsDialog(wx.Dialog):
    def __init__(self, parent, giface, title = _("Digitization settings"),
                 style = wx.DEFAULT_DIALOG_STYLE):
        """!Standard settings dialog for digitization purposes
        """
        wx.Dialog.__init__(self, parent = parent, id = wx.ID_ANY, title = title, style = style)

        self._giface = giface
        self.parent = parent                     # MapFrame
        self.digit = self.parent.MapWindow.digit
        
        # notebook
        notebook = wx.Notebook(parent = self, id = wx.ID_ANY, style = wx.BK_DEFAULT)
        self._createGeneralPage(notebook)
        self._createSymbologyPage(notebook)
        self.digit.SetCategory()
        self._createAttributesPage(notebook)
        self._createQueryPage(notebook)

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

    def _createSymbologyPage(self, notebook):
        """!Create notebook page concerning symbology settings"""
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY)
        notebook.AddPage(page = panel, text = _("Symbology"))

        sizer = wx.BoxSizer(wx.VERTICAL)
        
        flexSizer = wx.FlexGridSizer (cols = 3, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(0)

        self.symbology = {}
        for label, key in self._symbologyData():
            textLabel = wx.StaticText(panel, wx.ID_ANY, label)
            color = csel.ColourSelect(panel, id = wx.ID_ANY,
                                      colour = UserSettings.Get(group = 'vdigit', key = 'symbol',
                                                                subkey = [key, 'color']),
                                      size = (40, 25))
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

    def _createGeneralPage(self, notebook):
        """!Create notebook page concerning general settings"""
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
                                      choices = [_("screen pixels"), _("map units")])
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
                                              {'value' : self.digit.GetDisplay().GetThreshold(),
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
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Digitize lines/boundaries"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        self.intersect = wx.CheckBox(parent = panel, label = _("Break lines at intersection"))
        self.intersect.SetValue(UserSettings.Get(group = 'vdigit', key = 'breakLines', subkey = 'enabled'))
        
        sizer.Add(item = self.intersect, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 1)

        border.Add(item = sizer, proportion = 0, flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)

        #
        # digitize areas box
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Digitize areas"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        self.closeBoundary = wx.CheckBox(parent = panel, label = _("Close boundary (snap to the start node)"))
        self.closeBoundary.SetValue(UserSettings.Get(group = 'vdigit', key = 'closeBoundary', subkey = 'enabled'))
        
        sizer.Add(item = self.closeBoundary, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 1)

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

    def _createQueryPage(self, notebook):
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

    def _createAttributesPage(self, notebook):
        """!Create notebook page for attributes"""
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
        if tree:
            item = tree.FindItemByData('maplayer', mapLayer)
        else:
            item = None
        row = 0
        for attrb in ['length', 'area', 'perimeter']:
            # checkbox
            check = wx.CheckBox(parent = panel, id = wx.ID_ANY,
                                label = self.geomAttrb[attrb]['label'])
            ### self.deleteRecord.SetValue(UserSettings.Get(group='vdigit', key="delRecord", subkey='enabled'))
            check.Bind(wx.EVT_CHECKBOX, self.OnGeomAttrb)
            # column (only numeric)
            column = ColumnSelect(parent = panel, size = (200, -1))
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
            if item and tree.GetLayerInfo(item, key = 'vdigit') and \
                    'geomAttr' in tree.GetLayerInfo(item, key = 'vdigit') and \
                    attrb in tree.GetLayerInfo(item, key = 'vdigit')['geomAttr']:
                check.SetValue(True)
                column.SetStringSelection(tree.GetLayerInfo(item, key = 'vdigit')['geomAttr'][attrb]['column'])
                if attrb == 'area':
                    type = 'area'
                else:
                    type = 'length'
                unitsIdx = Units.GetUnitsIndex(type, 
                                                tree.GetLayerInfo(item, key = 'vdigit')['geomAttr'][attrb]['units'])
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
                                         "in the workspace not in the vector digitizer "
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

    def _symbologyData(self):
        """!Data for _createSymbologyPage()

        label | checkbox | color
        """

        return (
            (_("Digitize new line segment"), "newSegment"),
            (_("Digitize new line/boundary"), "newLine"),
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
        """!Register geometry attributes (enable/disable)
        """
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
        """!Change category mode
        """
        mode = event.GetSelection()
        UserSettings.Set(group = 'vdigit', key = "categoryMode", subkey = 'selection', value = mode)
        if mode == 1: # manual entry
            self.category.Enable(True)
        elif self.category.IsEnabled(): # disable
            self.category.Enable(False)
        
        if mode == 2 and self.addRecord.IsChecked(): # no category
            self.addRecord.SetValue(False)
        
        self.digit.SetCategory()
        self.category.SetValue(UserSettings.Get(group = 'vdigit', key = 'category', subkey = 'value'))

    def OnChangeLayer(self, event):
        """!Layer changed
        """
        layer = event.GetInt()
        if layer > 0:
            UserSettings.Set(group = 'vdigit', key = 'layer', subkey = 'value', value = layer)
            self.digit.SetCategory()
            self.category.SetValue(UserSettings.Get(group = 'vdigit', key = 'category', subkey = 'value'))
            
        event.Skip()

    def OnChangeAddRecord(self, event):
        """!Checkbox 'Add new record' status changed
        """
        pass
        # self.category.SetValue(self.digit.SetCategory())
            
    def OnChangeSnappingValue(self, event):
        """!Change snapping value - update static text
        """
        value = self.snappingValue.GetValue()
        
        if value < 0:
            region = self.parent.MapWindow.Map.GetRegion()
            res = (region['nsres'] + region['ewres']) / 2.
            threshold = self.digit.GetDisplay().GetThreshold(value = res)
        else:
            if self.snappingUnit.GetStringSelection() == "map units":
                threshold = value
            else:
                threshold = self.digit.GetDisplay().GetThreshold(value = value)
            
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
        """!Snapping units change -> update static text
        """
        value = self.snappingValue.GetValue()
        units = self.snappingUnit.GetStringSelection()
        threshold = self.digit.GetDisplay().GetThreshold(value = value, units = units)

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
        """!Change query
        """
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
        """!Button 'Save' pressed
        """
        self.UpdateSettings()
        self.parent.toolbars['vdigit'].settingsDialog = None

        fileSettings = {}
        UserSettings.ReadSettingsFile(settings = fileSettings)
        fileSettings['vdigit'] = UserSettings.Get(group = 'vdigit')
        
        sfile = UserSettings.SaveToFile(fileSettings)
        self._giface.WriteLog(_('Vector digitizer settings saved to file <%s>.') % sfile)
        
        self.Destroy()

        event.Skip()
        
    def OnApply(self, event):
        """!Button 'Apply' pressed
        """
        self.UpdateSettings()

    def OnCancel(self, event):
        """!Button 'Cancel' pressed
        """
        self.parent.toolbars['vdigit'].settingsDialog = None
        self.Destroy()

        if event:
            event.Skip()
        
    def UpdateSettings(self):
        """!Update digitizer settings

        @todo Needs refactoring 
        """
        if self.parent.GetLayerManager():
            self.parent.GetLayerManager().WorkspaceChanged() # geometry attributes
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

        # delete existing feature
        UserSettings.Set(group = 'vdigit', key = "delRecord", subkey = 'enabled',
                         value = self.deleteRecord.IsChecked())

        # geometry attributes (workspace)
        mapLayer = self.parent.toolbars['vdigit'].GetLayer()
        tree = self._giface.GetLayerTree()
        if tree:
            item = tree.FindItemByData('maplayer', mapLayer)
        else:
            item = None
        for key, val in self.geomAttrb.iteritems():
            checked = self.FindWindowById(val['check']).IsChecked()
            column  = self.FindWindowById(val['column']).GetValue()
            unitsIdx = self.FindWindowById(val['units']).GetSelection()
            if item and not tree.GetLayerInfo(item, key = 'vdigit'): 
                tree.SetLayerInfo(item, key = 'vdigit', value = { 'geomAttr' : dict() })
            
            if checked: # enable
                if key == 'area':
                    type = key
                else:
                    type = 'length'
                unitsKey = Units.GetUnitsKey(type, unitsIdx)
                tree.GetLayerInfo(item, key = 'vdigit')['geomAttr'][key] = { 'column' : column,
                                                                       'units' : unitsKey }
            else:
                if item and tree.GetLayerInfo(item, key = 'vdigit') and \
                        key in tree.GetLayerInfo(item, key = 'vdigit')['geomAttr']:
                    del tree.GetLayerInfo(item, key = 'vdigit')['geomAttr'][key]
        
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

        # close boundary
        UserSettings.Set(group = 'vdigit', key = "closeBoundary", subkey = 'enabled',
                         value = self.closeBoundary.IsChecked())
        
        self.digit.UpdateSettings()
        
        # redraw map if auto-rendering is enabled
        if self.parent.IsAutoRendered(): 
            self.parent.OnRender(None)
