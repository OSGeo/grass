"""
@package colorrules.py

@brief Dialog for interactive management of raster color tables and
vector rgb_column attributes.

Classes:
 - ColorTable
 - BuferedWindow

(C) 2008, 2010-2011 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton (Arizona State University)
@author Martin Landa <landa.martin gmail.com> (various updates)
@author Anna Kratochvilova (load/save raster color tables)
"""

import os
import sys
import shutil

import wx
import wx.lib.colourselect as csel
import wx.lib.scrolledpanel as scrolled

import grass.script as grass

import dbm
import gcmd
import globalvar
import gselect
import render
import utils
import menuform
from debug import Debug as Debug
from preferences import globalSettings as UserSettings
from nviz_mapdisp import wxUpdateProperties

class ColorTable(wx.Frame):
    def __init__(self, parent, raster, nviz = False, id = wx.ID_ANY, title = _("Set color table"),
                 style = wx.DEFAULT_FRAME_STYLE | wx.RESIZE_BORDER,
                 **kwargs):
        """!Dialog for interactively entering rules for map management
        commands

        @param raster True to raster otherwise vector
        @param nviz True if ColorTable is called from nviz thematic mapping
        """
        self.parent = parent # GMFrame
        self.raster = raster
        self.nviz = nviz # called from nviz - thematic mapping
        
        wx.Frame.__init__(self, parent, id, title, style = style, **kwargs)
        
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        
        # input map to change
        self.inmap = ''

        if self.raster:
            # raster properties
            self.properties = {
                # min cat in raster map
                'min' : None,
                # max cat in raster map
                'max' : None,
                }
        else:
            # vector properties
            self.properties = {
                # list of database layers for vector (minimum of 1)
                'layers' : [],
                # list of database columns for vector
                'columns' : [],
                # vector layer for attribute table to use for setting color
                'layer' : 1, 
                # vector attribute table used for setting color         
                'table' : '',
                # vector attribute column for assigning colors
                'column' : '', 
                # vector attribute column to use for storing colors
                'rgb' : '',
                }

        # rules for creating colortable
        self.ruleslines = {}

        # instance of render.Map to be associated with display
        self.Map   = render.Map()

        # reference to layer with preview
        self.layer = None          
        
        # set title
        if self.raster:
            self.SetTitle(_('Create new color table for raster map'))
        else:
            self.SetTitle(_('Create new color table for vector map'))    
        # layout
        self.__doLayout()
        
        # bindings
        self.Bind(wx.EVT_BUTTON, self.OnHelp, self.btnHelp)
        self.selectionInput.Bind(wx.EVT_TEXT, self.OnSelectionInput)
        self.Bind(wx.EVT_BUTTON, self.OnCancel, self.btnCancel)
        self.Bind(wx.EVT_BUTTON, self.OnApply, self.btnApply)
        self.Bind(wx.EVT_BUTTON, self.OnOK, self.btnOK)
        self.Bind(wx.EVT_BUTTON, self.OnPreview, self.btnPreview)
        self.Bind(wx.EVT_BUTTON, self.OnAddRules, self.btnAdd)
        self.Bind(wx.EVT_CLOSE,  self.OnCloseWindow)
        
        # additional bindings for raster/vector color management
        if self.raster:
            self.Bind(wx.EVT_BUTTON, self.OnSaveTable, self.btnSave)
        else:
            self.Bind(wx.EVT_COMBOBOX, self.OnLayerSelection, self.cb_vlayer)
            self.Bind(wx.EVT_COMBOBOX, self.OnColumnSelection, self.cb_vcol)
            self.Bind(wx.EVT_COMBOBOX, self.OnRGBColSelection, self.cb_vrgb)
            self.Bind(wx.EVT_BUTTON, self.OnAddColumn, self.btn_addCol)
        if not self.nviz:
            # set map layer from layer tree, first selected,
            # if not the right type, than select another
            if self.raster:
                elem = 'raster'
            else:
                elem = 'vector'
            try:
                sel = self.parent.curr_page.maptree.layer_selected
                if sel and self.parent.curr_page.maptree.GetPyData(sel)[0]['type'] == elem:
                    layer = sel
                else:
                    layer = self.parent.curr_page.maptree.FindItemByData(key = 'type', value = elem)
            except:
                layer = None
            if layer:
                mapLayer = self.parent.curr_page.maptree.GetPyData(layer)[0]['maplayer']
                name = mapLayer.GetName()
                type = mapLayer.GetType()
                self.selectionInput.SetValue(name)
                self.inmap = name
        else:
            self.inmap = self.parent.GetLayerData(nvizType = 'vector', nameOnly = True)
            self.OnSelectionInput(None)
            self.nvizInfo.SetLabel(_("Set color rules for vector map %s:") % self.inmap)
            
        self.SetMinSize(self.GetSize())
        
        self.CentreOnScreen()
        self.Show()
    
    def _createMapSelection(self):
        """!Create map selection part of dialog"""
        # top controls
        if self.raster:
            maplabel = _('Select raster map:')
        else:
            maplabel = _('Select vector map:')
        inputBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                label = " %s " % maplabel)
        self.inputSizer = wx.StaticBoxSizer(inputBox, wx.VERTICAL)
        if self.raster:
            elem = 'cell'
        else:
            elem = 'vector'
        self.selectionInput = gselect.Select(parent = self, id = wx.ID_ANY,
                                             size = globalvar.DIALOG_GSELECT_SIZE,
                                             type = elem)
        if self.raster:
            self.ovrwrtcheck = wx.CheckBox(parent = self, id = wx.ID_ANY,
                                           label = _('replace existing color table'))
            self.ovrwrtcheck.SetValue(UserSettings.Get(group = 'cmd', key = 'overwrite', subkey = 'enabled'))
        
        if self.raster:
            self.btnSave = wx.Button(parent = self, id = wx.ID_SAVE)
            self.btnSave.SetToolTipString(_('Save color table to file'))
            
        # layout
        self.inputSizer.Add(item = self.selectionInput,
                       flag = wx.ALIGN_CENTER_VERTICAL | wx.ALL | wx.EXPAND, border = 5)
        replaceSizer = wx.BoxSizer(wx.HORIZONTAL)
        if self.raster:
            replaceSizer.Add(item = self.ovrwrtcheck, proportion = 1,
                             flag = wx.ALL | wx.ALIGN_CENTER_VERTICAL, border = 1)
        
            replaceSizer.Add(item = self.btnSave, proportion = 0,
                            flag = wx.ALIGN_RIGHT | wx.ALL, border = 5)
        
        self.inputSizer.Add(item = replaceSizer, proportion = 1,
                       flag = wx.ALL | wx.EXPAND, border = 0)
    
        return self.inputSizer
    
    def _createVectorAttrb(self):
        """!Create part of dialog with layer/column selection"""
        self.cb_vl_label = wx.StaticText(parent = self, id = wx.ID_ANY,
                                             label = _('Layer:'))
        self.cb_vc_label = wx.StaticText(parent = self, id = wx.ID_ANY,
                                         label = _('Attribute column:'))
        self.cb_vrgb_label = wx.StaticText(parent = self, id = wx.ID_ANY,
                                           label = _('RGB color column:'))
        self.cb_vlayer = gselect.LayerSelect(self)
        self.cb_vcol = gselect.ColumnSelect(self)
        self.cb_vrgb = gselect.ColumnSelect(self)
        self.btn_addCol = wx.Button(parent = self, id = wx.ID_ANY,
                                             label = _('Add column'))
        self.btn_addCol.SetToolTipString(_("Add GRASSRGB column to current attribute table."))
        
        # layout
        inputBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                label = " %s " % _("Select vector columns"))
        inputSizer = wx.StaticBoxSizer(inputBox, wx.VERTICAL)
        vSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        vSizer.Add(self.cb_vl_label, pos = (0, 0),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.cb_vlayer,  pos = (0, 1),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.cb_vc_label, pos = (1, 0),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.cb_vcol, pos = (1, 1),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.cb_vrgb_label, pos = (2, 0),
                  flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.cb_vrgb, pos = (2, 1),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.btn_addCol, pos = (2, 2),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        inputSizer.Add(item = vSizer,
                       flag = wx.ALIGN_CENTER_VERTICAL | wx.ALL | wx.EXPAND, border = 5)
                
        return inputSizer
    
    def _createColorRulesPanel(self):
        """!Create rules panel"""
        cr_panel = scrolled.ScrolledPanel(parent = self, id = wx.ID_ANY,
                                          size = (180, 300),
                                          style = wx.TAB_TRAVERSAL | wx.SUNKEN_BORDER)
        cr_panel.SetupScrolling(scroll_x = False)
        self.cr_sizer = wx.GridBagSizer(vgap = 2, hgap = 4)
        
        cr_panel.SetSizer(self.cr_sizer)
        cr_panel.SetAutoLayout(True)
        
        return cr_panel        

    def _createPreview(self):
        """!Create preview"""
        # initialize preview display
        self.InitDisplay()
        self.preview = BufferedWindow(self, id = wx.ID_ANY, size = (400, 300),
                                      Map = self.Map)
        self.preview.EraseMap()
        
    def _createButtons(self):
        """!Create buttons for leaving dialog"""
        self.btnHelp = wx.Button(parent = self, id = wx.ID_HELP)
        self.btnCancel = wx.Button(parent = self, id = wx.ID_CANCEL)
        self.btnApply = wx.Button(parent = self, id = wx.ID_APPLY) 
        self.btnOK = wx.Button(parent = self, id = wx.ID_OK)
        
        self.btnOK.SetDefault()
        self.btnOK.Enable(False)
        self.btnApply.Enable(False)

        
        # layout
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(self.btnHelp,
                     flag = wx.LEFT | wx.RIGHT, border = 5)
        btnSizer.Add(self.btnCancel,
                     flag = wx.LEFT | wx.RIGHT, border = 5)
        btnSizer.Add(self.btnApply,
                     flag = wx.LEFT | wx.RIGHT, border = 5)
        btnSizer.Add(self.btnOK,
                     flag = wx.LEFT | wx.RIGHT, border = 5)
        
        return btnSizer
    
    def __doLayout(self):
        """!Do main layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        #
        # map selection
        #
        mapSelection = self._createMapSelection()
        sizer.Add(item = mapSelection, proportion = 0,
                  flag = wx.ALL | wx.EXPAND, border = 5)
        if self.nviz:
            sizerNviz = wx.BoxSizer(wx.HORIZONTAL)
            self.nvizInfo = wx.StaticText(parent = self, id = wx.ID_ANY,
                                 label = _('')) # set later
            sizerNviz.Add(self.nvizInfo, proportion = 0, flag = wx.LEFT | wx.EXPAND, border = 0)
            sizer.Add(item = sizerNviz, proportion = 0,
                  flag = wx.LEFT | wx.BOTTOM | wx.EXPAND, border = 5)
            sizer.Hide(mapSelection)
            # doesn't work
            sizer.Layout()
        #
        # set vector attributes
        #
        if not self.raster:
            vectorAttrb = self._createVectorAttrb()
            sizer.Add(item = vectorAttrb, proportion = 0,
                      flag = wx.ALL | wx.EXPAND, border = 5)
        #
        # body & preview
        #
        bodySizer =  wx.GridBagSizer(hgap = 5, vgap = 5)
        row = 0
        
        # label with range
        if self.raster:
            crlabel = _('Enter raster category values or percents')
        else:
            crlabel = _('Enter vector attribute values or ranges')
        self.cr_label = wx.StaticText(parent = self, id = wx.ID_ANY, label = crlabel)
        bodySizer.Add(item = self.cr_label, pos = (row, 0), span = (1, 3),
                      flag = wx.ALL, border = 5)
        row += 1
        
        # color table
        self.cr_panel = self._createColorRulesPanel()
        bodySizer.Add(item = self.cr_panel, pos = (row, 0), span = (1, 2))
        # add two rules as default
        self.AddRules(2)
        
        # preview window
        self._createPreview()
        bodySizer.Add(item = self.preview, pos = (row, 2),
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT, border = 10)
        bodySizer.AddGrowableRow(row)
        bodySizer.AddGrowableCol(2)
        row += 1
        
        # add rules
        self.numRules = wx.SpinCtrl(parent = self, id = wx.ID_ANY,
                                    min = 1, max = 1e6)
        self.btnAdd = wx.Button(parent = self, id = wx.ID_ADD)
        bodySizer.Add(item = self.numRules, pos = (row, 0),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        bodySizer.Add(item = self.btnAdd, pos = (row, 1))
        
        # preview button
        self.btnPreview = wx.Button(parent = self, id = wx.ID_ANY,
                                    label = _("Preview"))
        bodySizer.Add(item = self.btnPreview, pos = (row, 2),
                      flag = wx.ALIGN_RIGHT)
        self.btnPreview.Enable(False)
        self.btnPreview.SetToolTipString(_("Show preview of vector map "
                                           "(current Map Display extent is used)."))
        
        
        sizer.Add(item = bodySizer, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 5)
        #
        # buttons
        #
        btnSizer = self._createButtons()
        
        sizer.Add(item = wx.StaticLine(parent = self, id = wx.ID_ANY,
                                       style = wx.LI_HORIZONTAL), proportion = 0,
                                       flag = wx.EXPAND | wx.ALL, border = 5) 
        
        sizer.Add(item = btnSizer, proportion = 0,
                  flag = wx.ALL | wx.ALIGN_RIGHT, border = 5)
        
        self.SetSizer(sizer)
        sizer.Layout()
        sizer.Fit(self)
        self.Layout()
        
    def OnAddRules(self, event):
        """!Add rules button pressed"""
        nrules = self.numRules.GetValue()
        self.AddRules(nrules)
        
    def AddRules(self, nrules):
        """!Add rules"""
        snum = len(self.ruleslines.keys())
        for num in range(snum, snum + nrules):
            # enable
            enable = wx.CheckBox(parent = self.cr_panel, id = num)
            enable.SetValue(True)
            self.Bind(wx.EVT_CHECKBOX, self.OnRuleEnable, enable)
            # value
            txt_ctrl = wx.TextCtrl(parent = self.cr_panel, id = 1000 + num,
                                   size = (90, -1),
                                   style = wx.TE_NOHIDESEL)
            if not self.raster:
                txt_ctrl.SetToolTipString(_("Enter vector attribute values (e.g. 5) "
                                            "or ranges (e.g. 5 to 10)"))
            self.Bind(wx.EVT_TEXT, self.OnRuleValue, txt_ctrl)
            # color
            color_ctrl = csel.ColourSelect(self.cr_panel, id = 2000 + num,
                                           size  =  globalvar.DIALOG_COLOR_SIZE)
            self.Bind(csel.EVT_COLOURSELECT, self.OnRuleColor, color_ctrl)
            self.ruleslines[enable.GetId()] = { 'value' : '',
                                                'color': "0:0:0" }
            
            self.cr_sizer.Add(item = enable, pos = (num, 0),
                              flag = wx.ALIGN_CENTER_VERTICAL)
            self.cr_sizer.Add(item = txt_ctrl, pos = (num, 1),
                              flag = wx.ALIGN_CENTER | wx.RIGHT, border = 5)
            self.cr_sizer.Add(item = color_ctrl, pos = (num, 2),
                              flag = wx.ALIGN_CENTER | wx.RIGHT, border = 10)
        
        self.cr_panel.Layout()
        self.cr_panel.SetupScrolling(scroll_x = False)
        
    def InitDisplay(self):
        """!Initialize preview display, set dimensions and region
        """
        self.width = self.Map.width = 400
        self.height = self.Map.height = 300
        self.Map.geom = self.width, self.height

    def OnErase(self, event):
        """!Erase the histogram display
        """
        self.PreviewWindow.Draw(self.HistWindow.pdc, pdctype = 'clear')

    def OnCloseWindow(self, event):
        """!Window closed
        Also remove associated rendered images
        """
        self.Map.Clean()
        self.Destroy()
        
    def OnSelectionInput(self, event):
        """!Raster/vector map selected"""
        if event:
            self.inmap = event.GetString()

        if self.inmap:
            if self.raster:
                mapType = 'cell'
            else:
                mapType = 'vector'
            if not grass.find_file(name = self.inmap, element = mapType)['file']:
                self.inmap = None
        
        if not self.inmap:
            self.btnPreview.Enable(False)
            self.btnOK.Enable(False)
            self.btnApply.Enable(False)
            self.OnLoadTable(event)
            return
        
        if self.raster:
            info = grass.raster_info(map = self.inmap)
            
            if info:
                self.properties['min'] = info['min']
                self.properties['max'] = info['max']
                self.OnLoadTable(event)
            else:
                self.inmap = ''
                self.properties['min'] = self.properties['max'] = None
                self.btnPreview.Enable(False)
                self.btnOK.Enable(False)
                self.btnApply.Enable(False)
                self.preview.EraseMap()
                self.cr_label.SetLabel(_('Enter raster category values or percents'))
                return
            
            if info['datatype'] == 'CELL':
                mapRange = _('range')
            else:
                mapRange = _('fp range')
            self.cr_label.SetLabel(_('Enter raster category values or percents (%(range)s = %(min)d-%(max)d)') %
                                     { 'range' : mapRange,
                                       'min' : self.properties['min'],
                                       'max' : self.properties['max'] })
            enable = True                        
        else:
            # check for db connection
            self.dbInfo = gselect.VectorDBInfo(self.inmap)
            if not len(self.dbInfo.layers):
                wx.CallAfter(self.NoConnection, self.inmap)
                for combo in (self.cb_vlayer, self.cb_vcol, self.cb_vrgb):
                    combo.SetValue("")
                    combo.Disable()
                    combo.Clear()
                enable = False
                
            else:
            # initialize layer selection combobox
                for combo in (self.cb_vlayer, self.cb_vcol, self.cb_vrgb):
                    combo.Enable()
                self.cb_vlayer.InsertLayers(self.inmap)
                # initialize attribute table for layer=1
                self.properties['layer'] = self.cb_vlayer.GetString(0)
                self.cb_vlayer.SetStringSelection(self.properties['layer'])
                layer = int(self.properties['layer'])
                self.properties['table'] = self.dbInfo.layers[layer]['table']
                
                # initialize column selection comboboxes 
                self.OnLayerSelection(event = None)
                
                if self.CheckMapset():
                    enable = True
                    self.btn_addCol.Enable(True)
                else:
                    enable = False
                    wx.CallAfter(gcmd.GMessage, parent = self, 
                                 message = _("Selected map <%s> is not in current mapset <%s>. "
                                            "Attribute table cannot be edited. "
                                            "Color rules will not be saved.") % 
                                            (self.inmap, grass.gisenv()['MAPSET']))
                                  
                    self.btn_addCol.Enable(False)
            
        self.btnPreview.Enable(enable)
        self.btnOK.Enable(enable)
        self.btnApply.Enable(enable)
    
    def NoConnection(self, vectorName):
        dlg = wx.MessageDialog(parent = self,
                                message = _("Database connection for vector map <%s> "
                                            "is not defined in DB file.  Do you want to create and "
                                            "connect new attribute table?") % vectorName,
                                caption = _("No database connection defined"),
                                style = wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION | wx.CENTRE)
        if dlg.ShowModal() == wx.ID_YES:
            dlg.Destroy()
            menuform.GUI(parent = self).ParseCommand(['v.db.addtable', 'map=' + self.inmap], 
                                                      completed = (self.CreateTable, self.inmap, ''))
        else:
            dlg.Destroy()
    
    def CheckMapset(self):
        """!Check if current layer is in current mapset"""
        if self.raster:
            element = 'cell'
        else:
            element = 'vector'
        if grass.find_file(name = self.inmap,
                           element = element)['mapset'] == grass.gisenv()['MAPSET']:
            return True
        else:
            return False
    
    def OnLayerSelection(self, event):
        # reset choices in column selection comboboxes if layer changes
        vlayer = int(self.cb_vlayer.GetStringSelection())
        self.cb_vcol.InsertColumns(vector = self.inmap, layer = vlayer, dbInfo = self.dbInfo)
        self.cb_vcol.SetSelection(0)
        self.properties['column'] = self.cb_vcol.GetString(0)
        self.cb_vrgb.InsertColumns(vector = self.inmap, layer = vlayer, type = ["character"], dbInfo = self.dbInfo)
        found = self.cb_vrgb.FindString('GRASSRGB')
        if found != wx.NOT_FOUND:
            self.cb_vrgb.SetSelection(found)
            self.properties['rgb'] = self.cb_vrgb.GetString(found)
        else:
            self.properties['rgb'] = ''
        self.SetInfoString()
        self.Update()
        
    def OnColumnSelection(self, event):
        self.properties['column'] = event.GetString()
        self.SetInfoString()
    
    def OnAddColumn(self, event):
        """!Add GRASSRGB column if it doesn't exist"""
        if 'GRASSRGB' not in self.cb_vrgb.GetItems():
            ret = gcmd.RunCommand('v.db.addcolumn',
                                   map = self.inmap,
                                  layer = self.properties['layer'],
                                  columns = 'GRASSRGB varchar(20)')
            self.cb_vrgb.InsertColumns(self.inmap, self.properties['layer'], type = ["character"])
            self.cb_vrgb.SetStringSelection('GRASSRGB')
        else:
            gcmd.GMessage(parent = self,
                          message = _("GRASSRGB column already exists."))
                        
    def CreateTable(self, dcmd, layer, params, propwin):
        """!Create attribute table"""
        if dcmd:
            cmd = utils.CmdToTuple(dcmd)
            gcmd.RunCommand(cmd[0], **cmd[1])
            self.OnSelectionInput(None)
        else:
            for combo in (self.cb_vlayer, self.cb_vcol, self.cb_vrgb):
                combo.SetValue("")
                combo.Disable()
            
    def SetInfoString(self):
        """!Show information about vector map column type/range"""
        driver, db = self.dbInfo.GetDbSettings(int(self.properties['layer']))
        nrows = grass.db_describe(table = self.properties['table'], driver = driver, database = db)['nrows']
        self.properties['min'] = self.properties['max'] = ''
        type = self.dbInfo.GetTableDesc(self.properties['table'])\
                                                    [self.properties['column']]['type']
        ctype = self.dbInfo.GetTableDesc(self.properties['table'])\
                                                    [self.properties['column']]['ctype']
        if ctype == int or ctype == float:  
            if nrows < 500: # not too large
                ret = gcmd.RunCommand('v.db.select',
                                      quiet = True,
                                      read  = True,
                                      flags = 'c',
                                      map = self.inmap,
                                      layer = self.properties['layer'],
                                      columns = self.properties['column']).strip('\n')
                records = ret.split('\n')
                try:
                    self.properties['min'] = min(map(float, records))
                    self.properties['max'] = max(map(float, records))
                except ValueError:
                    self.properties['min'] = self.properties['max'] = ''
                    
        if self.properties['min'] or self.properties['max']:
            if ctype == int:
                self.cr_label.SetLabel(_("Enter vector attribute values or ranges (type: %s, range: %d - %d)")
                            % (type, self.properties['min'], self.properties['max']))
            elif ctype == float:
                self.cr_label.SetLabel(_("Enter vector attribute values or ranges (type: %s, range: %.1f - %.1f )")
                            % (type, self.properties['min'], self.properties['max']))
        else:
            self.cr_label.SetLabel(_("Enter vector attribute values or ranges (type: %s)") % type)
        
    def OnRGBColSelection(self, event):
        self.properties['rgb'] = event.GetString()
        
    def OnRuleEnable(self, event):
        """!Rule enabled/disabled"""
        id = event.GetId()
        
        if event.IsChecked():
            value = self.FindWindowById(id+1000).GetValue()
            color = self.FindWindowById(id+2000).GetValue()
            color_str = str(color[0]) + ':' \
                + str(color[1]) + ':' + \
                str(color[2])
            
            self.ruleslines[id] = {
                'value' : value,
                'color' : color_str }
        else:
            del self.ruleslines[id]
        
    def OnRuleValue(self, event):
        """!Rule value changed"""
        num = event.GetId()
        vals = event.GetString().strip()
        
        if vals == '':
            return

        tc = self.FindWindowById(num)
        
        if self.raster:
            self.ruleslines[num-1000]['value'] = vals
        
        else:
            if self.properties['column'] == '' or self.properties['rgb'] == '':
                tc.SetValue('')
                gcmd.GMessage(parent = self,
                              message = _("Please select attribute column "
                                        "and RGB color column first"))
            else:
                try:
                    self.ruleslines[num-1000]['value'] = self.SQLConvert(vals)
                except ValueError:
                    tc.SetValue('')
                    self.ruleslines[num-1000]['value'] = ''
                    return
        
    def OnRuleColor(self, event):
        """!Rule color changed"""
        num = event.GetId()
        
        rgba_color = event.GetValue()
        
        rgb_string = str(rgba_color[0]) + ':' \
            + str(rgba_color[1]) + ':' + \
            str(rgba_color[2])
        
        self.ruleslines[num-2000]['color'] = rgb_string
        
    def SQLConvert(self, vals):
        valslist = []
        valslist = vals.split('to')
        if len(valslist) == 1:
            sqlrule = '%s=%s' % (self.properties['column'], valslist[0])
        elif len(valslist) > 1:
            sqlrule = '%s>=%s AND %s<=%s' % (self.properties['column'], valslist[0],
                                             self.properties['column'], valslist[1])
        else:
            return None
        
        return sqlrule
        
    def OnLoadTable(self, event):
        """!Load current color table (using `r.colors.out`)"""
        self.ruleslines.clear()
        self.cr_panel.DestroyChildren()
        if self.inmap:
            ctable = gcmd.RunCommand('r.colors.out',
                                     parent = self,
                                     read = True,
                                     map = self.inmap,
                                     rules = '-')
        else:
            self.OnPreview(event)
            return
        
        rulesNumber = len(ctable.splitlines())
        self.AddRules(rulesNumber)
        
        count = 0
        for line in ctable.splitlines():
            value, color = map(lambda x: x.strip(), line.split(' '))
            self.ruleslines[count]['value'] = value
            self.ruleslines[count]['color'] = color
            self.FindWindowById(count + 1000).SetValue(value)
            rgb = list()
            for c in color.split(':'):
                rgb.append(int(c))
            self.FindWindowById(count + 2000).SetColour(rgb)
            count += 1
        
        self.OnPreview(tmp = False)
        
    def OnSaveTable(self, event):
        """!Save color table to file"""
        rulestxt = ''   
        for rule in self.ruleslines.itervalues():
            if not rule['value']:
                continue
            rulestxt += rule['value'] + ' ' + rule['color'] + '\n'
        if not rulestxt:
            gcmd.GMessage(message = _("Nothing to save."),
                          parent = self)
            return
        
        dlg = wx.FileDialog(parent = self,
                            message = _("Save color table to file"),
                            defaultDir = os.getcwd(), style = wx.SAVE | wx.OVERWRITE_PROMPT)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            fd = open(path, 'w')
            fd.write(rulestxt)
            fd.close()            
        dlg.Destroy()
          
    def OnApply(self, event):
        """!Apply selected color table

        @return True on success otherwise False
        """
        ret = self.CreateColorTable()
        if not ret:
            gcmd.GMessage(parent = self, message = _("No color rules given."))
            
        if not self.nviz:
            display = self.parent.GetLayerTree().GetMapDisplay()
            if display and display.IsAutoRendered():
                display.GetWindow().UpdateMap(render = True)
        else:
            data = self.parent.GetLayerData(nvizType = 'vector')
            data['vector']['points']['thematic']['layer'] = int(self.properties['layer'])
            data['vector']['points']['thematic']['rgbcolumn'] = self.properties['rgb']
            data['vector']['points']['thematic']['update'] = None
            
            event = wxUpdateProperties(data = data)
            wx.PostEvent(self.parent.mapWindow, event)
            self.parent.mapWindow.Refresh(False)
        return ret

    def OnOK(self, event):
        """!Apply selected color table and close the dialog"""
        if self.OnApply(event):
            self.Destroy()
    
    def OnCancel(self, event):
        """!Do not apply any changes and close the dialog"""
        self.Destroy()
        
    def OnPreview(self, event = None, tmp = True):
        """!Update preview (based on computational region)"""
        if not self.inmap:
            self.preview.EraseMap()
            return
        
        # raster
        if self.raster:
            cmdlist = ['d.rast',
                       'map=%s' % self.inmap]
            ltype = 'raster'
            
            # find existing color table and copy to temp file
            try:
                name, mapset = self.inmap.split('@')
            except ValueError:
                name = self.inmap
                mapset = grass.find_file(self.inmap, element = 'cell')['mapset']
                if not mapset:
                    return
            old_colrtable = None
            if mapset == grass.gisenv()['MAPSET']:
                old_colrtable = grass.find_file(name = name, element = 'colr')['file']
            else:
                old_colrtable = grass.find_file(name = name, element = 'colr2/' + mapset)['file']
            
            if old_colrtable:
                colrtemp = utils.GetTempfile()
                shutil.copyfile(old_colrtable, colrtemp)
        # vector
        else:
            cmdlist = ['d.vect',
                        '-a',
                       'map=%s' % self.inmap,
                       'rgb_column=%s' % self.properties["rgb"],
                       'type=point,line,boundary,area']
            ltype = 'vector'
        
        if not self.layer:
            self.layer = self.Map.AddLayer(type = ltype, name = 'preview', command = cmdlist,
                                           l_active = True, l_hidden = False, l_opacity = 1.0,
                                           l_render = False) 
        else:
            self.layer.SetCmd(cmdlist)
        
        # apply new color table and display preview
        self.CreateColorTable(force = True)
        self.preview.UpdatePreview()
        
        # restore previous color table
        if self.raster and tmp:
            if old_colrtable:
                shutil.copyfile(colrtemp, old_colrtable)
                os.remove(colrtemp)
            else:
                gcmd.RunCommand('r.colors',
                                parent = self,
                                flags = 'r',
                                map = self.inmap)
        
    def OnHelp(self, event):
        """!Show GRASS manual page"""
        if self.raster:
            cmd = 'r.colors'
        else:
            cmd = 'v.colors'
        gcmd.RunCommand('g.manual',
                        quiet = True,
                        parent = self,
                        entry = cmd)
        
    def _IsNumber(self, s):
        """!Check if 's' is a number"""
        try:
            float(s)
            return True
        except ValueError:
            return False
        
    def CreateColorTable(self, force = False):
        """!Creates color table

        @return True on success
        @return False on failure
        """
        rulestxt = ''
        
        for rule in self.ruleslines.itervalues():
            if not rule['value']: # skip empty rules
                continue
            
            if self.raster:
                if rule['value'] not in ('nv', 'default') and \
                        rule['value'][-1] != '%' and \
                        not self._IsNumber(rule['value']):
                    gcmd.GError(_("Invalid rule value '%s'. Unable to apply color table.") % rule['value'],
                                parent = self)
                    return False
                
                rulestxt += rule['value'] + ' ' + rule['color'] + '\n'
            else:
                rulestxt += "UPDATE %s SET %s='%s' WHERE %s ;\n" % (self.properties['table'],
                                                                    self.properties['rgb'],
                                                                    rule['color'],
                                                                    rule['value'])
        if not rulestxt:
            return False
        
        gtemp = utils.GetTempfile()
        output = open(gtemp, "w")
        try:
            output.write(rulestxt)
        finally:
            output.close()
        
        if self.raster:
            if not force and \
                    not self.ovrwrtcheck.IsChecked():
                flags = 'w'
            else:
                flags = ''
            
            ret = gcmd.RunCommand('r.colors',
                                  flags = flags,
                                  map = self.inmap,
                                  rules = gtemp)
            if ret != 0:
                gcmd.GMessage(_("Color table already exists. "
                                "Check out 'replace existing color table' to "
                                "overwrite it."),
                              parent = self)
                return False
        
        else:
            gcmd.RunCommand('db.execute',
                            parent = self,
                            input = gtemp)
        
        return True
    
class BufferedWindow(wx.Window):
    """!A Buffered window class"""
    def __init__(self, parent, id,
                 style = wx.NO_FULL_REPAINT_ON_RESIZE,
                 Map = None, **kwargs):
        
        wx.Window.__init__(self, parent, id, style = style, **kwargs)

        self.parent = parent
        self.Map = Map
        
        # re-render the map from GRASS or just redraw image
        self.render = True
        # indicates whether or not a resize event has taken place
        self.resize = False 

        #
        # event bindings
        #
        self.Bind(wx.EVT_PAINT,        self.OnPaint)
        self.Bind(wx.EVT_IDLE,         self.OnIdle)
        self.Bind(wx.EVT_ERASE_BACKGROUND, lambda x: None)

        #
        # render output objects
        #
        # image file to be rendered
        self.mapfile = None 
        # wx.Image object (self.mapfile)
        self.img = None

        self.pdc = wx.PseudoDC()
        # will store an off screen empty bitmap for saving to file
        self._Buffer = None 

        # make sure that extents are updated at init
        self.Map.region = self.Map.GetRegion()
        self.Map.SetRegion()

    def Draw(self, pdc, img = None, pdctype = 'image'):
        """!Draws preview or clears window"""
        pdc.BeginDrawing()

        Debug.msg (3, "BufferedWindow.Draw(): pdctype=%s" % (pdctype))

        if pdctype == 'clear': # erase the display
            bg = wx.WHITE_BRUSH
            pdc.SetBackground(bg)
            pdc.Clear()
            self.Refresh()
            pdc.EndDrawing()
            return

        if pdctype == 'image' and img:
            bg = wx.TRANSPARENT_BRUSH
            pdc.SetBackground(bg)
            bitmap = wx.BitmapFromImage(img)
            w, h = bitmap.GetSize()
            pdc.DrawBitmap(bitmap, 0, 0, True) # draw the composite map
            
        pdc.EndDrawing()
        self.Refresh()

    def OnPaint(self, event):
        """!Draw pseudo DC to buffer"""
        self._Buffer = wx.EmptyBitmap(self.Map.width, self.Map.height)
        dc = wx.BufferedPaintDC(self, self._Buffer)
        
        # use PrepareDC to set position correctly
        self.PrepareDC(dc)
        
        # we need to clear the dc BEFORE calling PrepareDC
        bg = wx.Brush(self.GetBackgroundColour())
        dc.SetBackground(bg)
        dc.Clear()
        
        # create a clipping rect from our position and size
        # and the Update Region
        rgn = self.GetUpdateRegion()
        r = rgn.GetBox()
        
        # draw to the dc using the calculated clipping rect
        self.pdc.DrawToDCClipped(dc, r)
        
    def OnSize(self, event):
        """!Init image size to match window size"""
        # set size of the input image
        self.Map.width, self.Map.height = self.GetClientSize()

        # Make new off screen bitmap: this bitmap will always have the
        # current drawing in it, so it can be used to save the image to
        # a file, or whatever.
        self._Buffer = wx.EmptyBitmap(self.Map.width, self.Map.height)

        # get the image to be rendered
        self.img = self.GetImage()

        # update map display
        if self.img and self.Map.width + self.Map.height > 0: # scale image during resize
            self.img = self.img.Scale(self.Map.width, self.Map.height)
            self.render = False
            self.UpdatePreview()

        # re-render image on idle
        self.resize = True

    def OnIdle(self, event):
        """!Only re-render a preview image from GRASS during
        idle time instead of multiple times during resizing.
        """
        if self.resize:
            self.render = True
            self.UpdatePreview()
        event.Skip()

    def GetImage(self):
        """!Converts files to wx.Image"""
        if self.Map.mapfile and os.path.isfile(self.Map.mapfile) and \
                os.path.getsize(self.Map.mapfile):
            img = wx.Image(self.Map.mapfile, wx.BITMAP_TYPE_ANY)
        else:
            img = None
        
        return img
    
    def UpdatePreview(self, img = None):
        """!Update canvas if window changes geometry"""
        Debug.msg (2, "BufferedWindow.UpdatePreview(%s): render=%s" % (img, self.render))
        oldfont = ""
        oldencoding = ""
        
        if self.render:
            # extent is taken from current map display
            try:
                self.Map.region = self.parent.parent.curr_page.maptree.Map.region
            except AttributeError:
                self.Map.region = self.Map.GetRegion()
            # render new map images
            self.mapfile = self.Map.Render(force = self.render)
            self.img = self.GetImage()
            self.resize = False
        
        if not self.img:
            return
        
        # paint images to PseudoDC
        self.pdc.Clear()
        self.pdc.RemoveAll()
        # draw map image background
        self.Draw(self.pdc, self.img, pdctype = 'image')
        
        self.resize = False
        
    def EraseMap(self):
        """!Erase preview"""
        self.Draw(self.pdc, pdctype = 'clear')
    
