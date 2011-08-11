"""
@package colorrules.py

@brief Dialog for interactive management of raster color tables and
vector rgb_column attributes.

Classes:
 - RulesPanel
 - ColorTable
 - RasterColorTable
 - VectorColorTable
 - ThematicVectorTable
 - BuferedWindow

(C) 2008, 2010-2011 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton (Arizona State University)
@author Martin Landa <landa.martin gmail.com> (various updates)
@author Anna Kratochvilova <kratochanna gmail.com> (split to base and derived classes)
"""

import os
import sys
import shutil
import copy
import tempfile

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


class RulesPanel:
    def __init__(self, parent, mapType, columnType, properties, panelWidth = 180):
        """!Create rules panel
        
        @param mapType raster/vector
        @param columnType color/size for choosing widget type
        @param properties properties of classes derived from ColorTabel
        @param panelWidth width of scroll panel"""
        
        self.ruleslines = {}
        self.mapType = mapType
        self.columnType = columnType
        self.properties = properties
        self.parent = parent
        
        self.mainPanel = scrolled.ScrolledPanel(parent, id = wx.ID_ANY,
                                          size = (panelWidth, 300),
                                          style = wx.TAB_TRAVERSAL | wx.SUNKEN_BORDER)
        self.mainPanel.SetupScrolling(scroll_x = False)
        self.mainSizer = wx.FlexGridSizer(cols = 3, vgap = 6, hgap = 4)
        # put small border at the top of panel
        for i in range(3):
            self.mainSizer.Add(item = wx.Size(3, 3))
        
        self.mainPanel.SetSizer(self.mainSizer)
        self.mainPanel.SetAutoLayout(True)
        
        if self.mapType == 'vector' and self.columnType == 'color':
            self.label = wx.StaticText(parent, id = wx.ID_ANY, label =  _("Set color for attribute values:"))
        elif self.mapType == 'vector' and self.columnType == 'size':
            if self.parent.vectorType == 'points':
                label = label = _("Set size for attribute values:")
            else:
                label = label = _("Set width for attribute values:")                
            self.label = wx.StaticText(parent, id = wx.ID_ANY, label = label)
        
        #  determines how many rules should be added
        self.numRules = wx.SpinCtrl(parent, id = wx.ID_ANY,
                                    min = 1, max = 1e6)
        # add rules
        self.btnAdd = wx.Button(parent, id = wx.ID_ADD)
        
        self.btnAdd.Bind(wx.EVT_BUTTON, self.OnAddRules)
    
    def Clear(self):
        """!Clear and widgets and delete information"""
        self.ruleslines.clear()
        self.mainPanel.DestroyChildren()
    
    def OnAddRules(self, event):
        """!Add rules button pressed"""
        nrules = self.numRules.GetValue()
        self.AddRules(nrules)
        
    def AddRules(self, nrules, start = False):
        """!Add rules
        
        @param start set widgets (not append)"""
        
        snum = len(self.ruleslines.keys())
        if start:
            snum = 0
        for num in range(snum, snum + nrules):
            # enable
            enable = wx.CheckBox(parent = self.mainPanel, id = num)
            enable.SetValue(True)
            enable.Bind(wx.EVT_CHECKBOX, self.OnRuleEnable)
            # value
            txt_ctrl = wx.TextCtrl(parent = self.mainPanel, id = 1000 + num,
                                   size = (80, -1),
                                   style = wx.TE_NOHIDESEL)
            if self.mapType == 'vector':
                txt_ctrl.SetToolTipString(_("Enter vector attribute values (e.g. 5) "
                                            "or ranges (e.g. 5 to 10)"))
            txt_ctrl.Bind(wx.EVT_TEXT, self.OnRuleValue)
            if self.columnType == 'color':
                # color
                columnCtrl = csel.ColourSelect(self.mainPanel, id = 2000 + num,
                                               size  =  globalvar.DIALOG_COLOR_SIZE)
                columnCtrl.Bind(csel.EVT_COLOURSELECT, self.OnRuleColor)
                if not start:
                    self.ruleslines[enable.GetId()] = { 'value' : '',
                                                        'color': "0:0:0" }
            if self.columnType == 'size':
                # size
                columnCtrl = wx.SpinCtrl(self.mainPanel, id = 2000 + num,
                                         size = (50, -1), min = 1, max = 1e3,
                                         initial = 5)
                columnCtrl.Bind(wx.EVT_SPINCTRL, self.OnRuleSize)
                if not start:
                    self.ruleslines[enable.GetId()] = { 'value' : '',
                                                        'size': 5 }
            
            self.mainSizer.Add(item = enable, proportion = 0,
                              flag = wx.ALIGN_CENTER_VERTICAL)
            self.mainSizer.Add(item = txt_ctrl, proportion = 0,
                              flag = wx.ALIGN_CENTER | wx.RIGHT, border = 5)
            self.mainSizer.Add(item = columnCtrl, proportion = 0,
                              flag = wx.ALIGN_CENTER | wx.RIGHT, border = 10)
        
        self.mainPanel.Layout()
        self.mainPanel.SetupScrolling(scroll_x = False)
       
    
    def OnRuleEnable(self, event):
        """!Rule enabled/disabled"""
        id = event.GetId()
        
        if event.IsChecked():
            self.mainPanel.FindWindowById(id + 1000).Enable()
            self.mainPanel.FindWindowById(id + 2000).Enable()
            if self.mapType == 'vector':
                value = self.SQLConvert(self.mainPanel.FindWindowById(id + 1000).GetValue(), self.columnType)
            else:
                value = self.mainPanel.FindWindowById(id + 1000).GetValue()
            color = self.mainPanel.FindWindowById(id + 2000).GetValue()
            
            if self.columnType == 'color':
            # color
                color_str = str(color[0]) + ':' \
                          + str(color[1]) + ':' \
                          + str(color[2])
                self.ruleslines[id] = {'value' : value,
                                       'color' : color_str }
                
            else:
            # size
                self.ruleslines[id] = {'value' : value,
                                       'size'  : float(color) }
        
        else:
            self.mainPanel.FindWindowById(id + 1000).Disable()
            self.mainPanel.FindWindowById(id + 2000).Disable()
            del self.ruleslines[id]
        
    def OnRuleColor(self, event):
        """!Rule color changed"""
        num = event.GetId()
        
        rgba_color = event.GetValue()
        
        rgb_string = str(rgba_color[0]) + ':' \
                   + str(rgba_color[1]) + ':' \
                   + str(rgba_color[2])
        
        self.ruleslines[num-2000]['color'] = rgb_string
     
    def OnRuleSize(self, event):
        """!Rule size changed"""
        num = event.GetId()
        size = event.GetInt()
        
        self.ruleslines[num - 2000]['size'] = size
        
    def OnRuleValue(self, event):
        """!Rule value changed"""
        num = event.GetId()
        vals = event.GetString().strip()
        
        if vals == '':
            return
        
        if self.mapType == 'vector':
            self.SetVectorRule(num, vals)
        else:
            self.SetRasterRule(num, vals)

    def SetRasterRule(self, num, vals): 
        """!Set raster rule"""       
        self.ruleslines[num-1000]['value'] = vals

    def SetVectorRule(self, num, vals):
        """!Set vector rule"""
        tc = self.mainPanel.FindWindowById(num)
        if self.columnType == 'color':
            source, target = 'source_rgb', 'rgb'
        else:
            source, target = 'source_size', 'size'
        if self.properties[source] == '' or self.properties[target] == '':
            tc.SetValue('')
            gcmd.GMessage(parent = self,
                          message = _("Please select attribute column "
                                    "and RGB color column first"))
        else:
            try:
                self.ruleslines[num-1000]['value'] = self.SQLConvert(vals, self.columnType)
            except ValueError:
                tc.SetValue('')
                self.ruleslines[num-1000]['value'] = ''
                
                return
            
    def Enable(self, enable = True):
        """!Enable/Disable all widgets"""
        for child in self.mainPanel.GetChildren():
            child.Enable(enable)
        self.btnAdd.Enable(enable)
        self.numRules.Enable(enable)
        
        
    def LoadRules(self):
        """!Fill rule widgets from ruleslines"""
        message = ""
        for item in range(len(self.ruleslines)):
            self.mainPanel.FindWindowById(item + 1000).SetValue(self.ruleslines[item]['value'])
            r, g, b = (0, 0, 0) # default
            if self.ruleslines[item][self.columnType] == '':
                if self.columnType == 'color':
                    self.ruleslines[item][self.columnType] = '%d:%d:%d' % (r, g, b)
                elif self.columnType == 'size':
                    self.ruleslines[item][self.columnType] = 100
                
            if self.columnType == 'color':
                try:
                    r, g, b = map(int, self.ruleslines[item][self.columnType].split(':'))
                except ValueError, e:
                    message =  _("Bad color format. Use color format '0:0:0'")
                self.mainPanel.FindWindowById(item + 2000).SetValue((r, g, b))
            else:
                value = float(self.ruleslines[item][self.columnType])
                self.mainPanel.FindWindowById(item + 2000).SetValue(value)
                
        if message:
            gcmd.GMessage(parent = self.parent, message = message)
            return False
        
        return True    
    
    def SQLConvert(self, vals, type = 'color'):
        """!Prepare value for SQL query"""
        if type == 'color':
            source = 'source_rgb'
        elif type == 'size':
            source = 'source_size'
        valslist = []
        valslist = vals.split('to')
        if len(valslist) == 1:
            sqlrule = '%s=%s' % (self.properties[source], valslist[0])
        elif len(valslist) > 1:
            sqlrule = '%s>=%s AND %s<=%s' % (self.properties[source], valslist[0],
                                             self.properties[source], valslist[1])
        else:
            return None
        
        return sqlrule  

class ColorTable(wx.Frame):
    def __init__(self, parent, title, id = wx.ID_ANY,
                 style = wx.DEFAULT_FRAME_STYLE | wx.RESIZE_BORDER,
                 **kwargs):
        """!Dialog for interactively entering rules for map management
        commands

        @param raster True to raster otherwise vector
        @param nviz True if ColorTable is called from nviz thematic mapping
        """
        self.parent = parent # GMFrame
        
        wx.Frame.__init__(self, parent, id, title, style = style, **kwargs)
        
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        
        # instance of render.Map to be associated with display
        self.Map  = render.Map() 
        
        # input map to change
        self.inmap = ''
        # reference to layer with preview
        self.layer = None          
           
        # layout
        self._doLayout()
        
        # bindings
        self.Bind(wx.EVT_BUTTON, self.OnHelp, self.btnHelp)
        self.selectionInput.Bind(wx.EVT_TEXT, self.OnSelectionInput)
        self.Bind(wx.EVT_BUTTON, self.OnCancel, self.btnCancel)
        self.Bind(wx.EVT_BUTTON, self.OnApply, self.btnApply)
        self.Bind(wx.EVT_BUTTON, self.OnOK, self.btnOK)
        self.Bind(wx.EVT_CLOSE,  self.OnCloseWindow)
       
        self.Bind(wx.EVT_BUTTON, self.OnPreview, self.btnPreview)
        
    def _initLayer(self):
        """!Set initial layer when opening dialog"""
        # set map layer from layer tree, first selected,
        # if not the right type, than select another
        try:
            sel = self.parent.curr_page.maptree.layer_selected
            if sel and self.parent.curr_page.maptree.GetPyData(sel)[0]['type'] == self.type:
                layer = sel
            else:
                layer = self.parent.curr_page.maptree.FindItemByData(key = 'type', value = self.type)
        except:
            layer = None
        if layer:
            mapLayer = self.parent.curr_page.maptree.GetPyData(layer)[0]['maplayer']
            name = mapLayer.GetName()
            type = mapLayer.GetType()
            self.selectionInput.SetValue(name)
            self.inmap = name
            
    def _createPreview(self, parent):
        """!Create preview"""
        # initialize preview display
        self.InitDisplay()
        self.preview = BufferedWindow(parent, id = wx.ID_ANY, size = (400, 300),
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
    
    def _createBody(self, parent):
        """!Create dialog body consisting of rules and preview"""
        bodySizer =  wx.GridBagSizer(hgap = 5, vgap = 5)
        row = 0
        
        # label with range
        self.cr_label = wx.StaticText(parent, id = wx.ID_ANY, label = '')
        bodySizer.Add(item = self.cr_label, pos = (row, 0), span = (1, 3),
                      flag = wx.ALL, border = 5)
        row += 1
        
        # color table
        self.colorRulesPanel = RulesPanel(parent = parent, mapType = self.type,
                                          columnType = 'color', properties = self.properties)
        if self.type == 'vector':
            bodySizer.Add(item = self.colorRulesPanel.label, pos = (row, 0), span = (1, 2),
                          flag = wx.ALL, border = 5)
            row += 1
        
        bodySizer.Add(item = self.colorRulesPanel.mainPanel, pos = (row, 0), span = (1, 2),
                      flag = wx.ALL, border = 5)
        # add two rules as default
        self.colorRulesPanel.AddRules(2)
        
        # preview window
        self._createPreview(parent = parent)
        bodySizer.Add(item = self.preview, pos = (row, 2),
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT, border = 10)
        bodySizer.AddGrowableRow(row)
        bodySizer.AddGrowableCol(2)
        row += 1
        
        # add rules button and spin to sizer
        bodySizer.Add(item = self.colorRulesPanel.numRules, pos = (row, 0),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        bodySizer.Add(item = self.colorRulesPanel.btnAdd, pos = (row, 1))
        
        # preview button
        self.btnPreview = wx.Button(parent, id = wx.ID_ANY,
                                    label = _("Preview"))
        bodySizer.Add(item = self.btnPreview, pos = (row, 2),
                      flag = wx.ALIGN_RIGHT)
        self.btnPreview.Enable(False)
        self.btnPreview.SetToolTipString(_("Show preview of map "
                                           "(current Map Display extent is used)."))
                
        return bodySizer
    
        
    def InitDisplay(self):
        """!Initialize preview display, set dimensions and region
        """
        self.width = self.Map.width = 400
        self.height = self.Map.height = 300
        self.Map.geom = self.width, self.height

    def OnCloseWindow(self, event):
        """!Window closed
        Also remove associated rendered images
        """
        self.Map.Clean()
        self.Destroy()
          
    def OnApply(self, event):
        """!Apply selected color table

        @return True on success otherwise False
        """
        ret = self.CreateColorTable()
        if ret:
            display = self.parent.GetLayerTree().GetMapDisplay()
            if display and display.IsAutoRendered():
                display.GetWindow().UpdateMap(render = True)
            
        return ret

    def OnOK(self, event):
        """!Apply selected color table and close the dialog"""
        if self.OnApply(event):
            self.Destroy()
    
    def OnCancel(self, event):
        """!Do not apply any changes and close the dialog"""
        self.Destroy()
        
    def DoPreview(self, ltype, cmdlist):
        """!Update preview (based on computational region)"""
        
        if not self.layer:
            self.layer = self.Map.AddLayer(type = ltype, name = 'preview', command = cmdlist,
                                           l_active = True, l_hidden = False, l_opacity = 1.0,
                                           l_render = False) 
        else:
            self.layer.SetCmd(cmdlist)
        
        # apply new color table and display preview
        self.CreateColorTable(force = True)
        self.preview.UpdatePreview()
        
    def RunHelp(self, cmd):
        """!Show GRASS manual page"""
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
        

class RasterColorTable(ColorTable):
    def __init__(self, parent, **kwargs):
        """!Dialog for interactively entering color rules for raster maps"""

        self.type = 'raster'
        
        # raster properties
        self.properties = {
            # min cat in raster map
            'min' : None,
            # max cat in raster map
            'max' : None,
            }        
        
        ColorTable.__init__(self, parent,
                            title = _('Create new color table for raster map'), **kwargs)
       
       
        # additional bindings for raster color management
        self.Bind(wx.EVT_BUTTON, self.OnSaveTable, self.btnSave)
        
        self._initLayer()
        
        self.SetMinSize(self.GetSize()) 
        self.CentreOnScreen()
        self.Show()
    
    def _createMapSelection(self, parent):
        """!Create map selection part of dialog"""
        # top controls
        maplabel = _('Select raster map:')
        inputBox = wx.StaticBox(parent, id = wx.ID_ANY,
                                label = " %s " % maplabel)
        inputSizer = wx.StaticBoxSizer(inputBox, wx.VERTICAL)

        self.selectionInput = gselect.Select(parent, id = wx.ID_ANY,
                                             size = globalvar.DIALOG_GSELECT_SIZE,
                                             type = 'cell')
        self.ovrwrtcheck = wx.CheckBox(parent, id = wx.ID_ANY,
                                       label = _('replace existing color table'))
        self.ovrwrtcheck.SetValue(UserSettings.Get(group = 'cmd', key = 'overwrite', subkey = 'enabled'))
        
        self.btnSave = wx.Button(parent, id = wx.ID_SAVE)
        self.btnSave.SetToolTipString(_('Save color table to file'))
            
        # layout
        inputSizer.Add(item = self.selectionInput,
                       flag = wx.ALIGN_CENTER_VERTICAL | wx.ALL | wx.EXPAND, border = 5)
        replaceSizer = wx.BoxSizer(wx.HORIZONTAL)
        replaceSizer.Add(item = self.ovrwrtcheck, proportion = 1,
                         flag = wx.ALL | wx.ALIGN_CENTER_VERTICAL, border = 1)
    
        replaceSizer.Add(item = self.btnSave, proportion = 0,
                        flag = wx.ALIGN_RIGHT | wx.ALL, border = 5)
        
        inputSizer.Add(item = replaceSizer, proportion = 1,
                       flag = wx.ALL | wx.EXPAND, border = 0)
    
        return inputSizer
    
    def _doLayout(self):
        """!Do main layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        #
        # map selection
        #
        mapSelection = self._createMapSelection(parent = self)
        sizer.Add(item = mapSelection, proportion = 0,
                  flag = wx.ALL | wx.EXPAND, border = 5)
        #
        # body & preview
        #
        bodySizer = self._createBody(parent = self)
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
    
    def OnSelectionInput(self, event):
        """!Raster map selected"""
        if event:
            self.inmap = event.GetString()

        if self.inmap:
            if not grass.find_file(name = self.inmap, element = 'cell')['file']:
                self.inmap = None
        
        if not self.inmap:
            self.btnPreview.Enable(False)
            self.btnOK.Enable(False)
            self.btnApply.Enable(False)
            self.OnLoadTable(event)
            return
        
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
            
        self.btnPreview.Enable()
        self.btnOK.Enable()
        self.btnApply.Enable()
    
    def OnLoadTable(self, event):
        """!Load current color table (using `r.colors.out`)"""
        
        self.colorRulesPanel.Clear()
        
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
        self.colorRulesPanel.AddRules(rulesNumber)
        
        count = 0
        for line in ctable.splitlines():
            value, color = map(lambda x: x.strip(), line.split(' '))
            self.colorRulesPanel.ruleslines[count]['value'] = value
            self.colorRulesPanel.ruleslines[count]['color'] = color
            self.colorRulesPanel.mainPanel.FindWindowById(count + 1000).SetValue(value)
            rgb = list()
            for c in color.split(':'):
                rgb.append(int(c))
            self.colorRulesPanel.mainPanel.FindWindowById(count + 2000).SetColour(rgb)
            count += 1
        
        self.OnPreview(tmp = False)
            
          
    def OnPreview(self, event = None, tmp = True):
        """!Update preview (based on computational region)"""
        if not self.inmap:
            self.preview.EraseMap()
            return
        
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
            
        ColorTable.DoPreview(self, ltype, cmdlist)  
        
        # restore previous color table
        if tmp:
            if old_colrtable:
                shutil.copyfile(colrtemp, old_colrtable)
                os.remove(colrtemp)
            else:
                gcmd.RunCommand('r.colors',
                                parent = self,
                                flags = 'r',
                                map = self.inmap)
    
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
        
    def OnHelp(self, event):
        """!Show GRASS manual page"""
        cmd = 'r.colors'
        ColorTable.RunHelp(self, cmd = cmd)
    
    def CreateColorTable(self, force = False):
        """!Creates color table

        @return True on success
        @return False on failure
        """
        rulestxt = ''
        
        for rule in self.colorRulesPanel.ruleslines.itervalues():
            if not rule['value']: # skip empty rules
                continue
            
            if rule['value'] not in ('nv', 'default') and \
                    rule['value'][-1] != '%' and \
                    not self._IsNumber(rule['value']):
                gcmd.GError(_("Invalid rule value '%s'. Unable to apply color table.") % rule['value'],
                            parent = self)
                return False
            
            rulestxt += rule['value'] + ' ' + rule['color'] + '\n'
           
        if not rulestxt:
            gcmd.GMessage(parent = self, message = _("No color rules given."))
            return False
        
        gtemp = utils.GetTempfile()
        output = open(gtemp, "w")
        try:
            output.write(rulestxt)
        finally:
            output.close()
        
        if not force and not self.ovrwrtcheck.IsChecked():
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
        
        return True
                      
class VectorColorTable(ColorTable):
    def __init__(self, parent, **kwargs):
        """!Dialog for interactively entering color rules for vector maps"""
        
        self.type = 'vector'
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
            'source_rgb' : '', 
            # vector attribute column to use for storing colors
            'rgb' : '',       
            # vector attribute column for assigning size
            'source_size' : '', 
            # vector attribute column to use for storing size
            'size' : '',
            }         
        
        ColorTable.__init__(self, parent = parent,
                            title = _('Create new color table for vector map'), **kwargs)   
        
        # additional bindings for vector color management
        self.Bind(wx.EVT_COMBOBOX, self.OnLayerSelection, self.cb_vlayer)
        self.Bind(wx.EVT_COMBOBOX, self.OnColumnSelection, self.cb_color_att)
        self.Bind(wx.EVT_COMBOBOX, self.OnRGBColSelection, self.cb_rgb_col)
        self.Bind(wx.EVT_BUTTON, self.OnAddColumn, self.btn_add_RGB)    
        
        self._initLayer()
        self.cr_label.SetLabel(_("Enter vector attribute values (e.g. 5) "
                                            "or ranges (e.g. 5 to 10)"))
        self.SetMinSize(self.GetSize()) 
        self.CentreOnScreen()
        self.Show()

    def _createMapSelection(self, parent):
        """!Create map selection part of dialog"""
        # top controls
        maplabel = _('Select vector map:')
        inputBox = wx.StaticBox(parent, id = wx.ID_ANY,
                                label = " %s " % maplabel)
        inputSizer = wx.StaticBoxSizer(inputBox, wx.VERTICAL)
        
        self.selectionInput = gselect.Select(parent, id = wx.ID_ANY,
                                             size = globalvar.DIALOG_GSELECT_SIZE,
                                             type = 'vector')
            
        # layout
        inputSizer.Add(item = self.selectionInput,
                       flag = wx.ALIGN_CENTER_VERTICAL | wx.ALL | wx.EXPAND, border = 5)
        replaceSizer = wx.BoxSizer(wx.HORIZONTAL)
        
        inputSizer.Add(item = replaceSizer, proportion = 1,
                       flag = wx.ALL | wx.EXPAND, border = 0)
    
        return inputSizer
    
    def _createVectorAttrb(self, parent):
        """!Create part of dialog with layer/column selection"""
        inputBox = wx.StaticBox(parent = parent, id = wx.ID_ANY,
                                label = " %s " % _("Select vector columns"))
        cb_vl_label = wx.StaticText(parent, id = wx.ID_ANY,
                                             label = _('Layer:'))
        cb_vc_label = wx.StaticText(parent, id = wx.ID_ANY,
                                         label = _('Attribute column:'))
        cb_vrgb_label = wx.StaticText(parent, id = wx.ID_ANY,
                                           label = _('RGB color column:'))
        self.rgb_range_label = wx.StaticText(parent, id = wx.ID_ANY)
        self.cb_vlayer = gselect.LayerSelect(parent)
        self.cb_color_att = gselect.ColumnSelect(parent)
        self.cb_rgb_col = gselect.ColumnSelect(parent)
        self.btn_add_RGB = wx.Button(parent, id = wx.ID_ANY,
                                             label = _('Add column'))
        self.btn_add_RGB.SetToolTipString(_("Add GRASSRGB column to current attribute table."))
        
        # layout
        inputSizer = wx.StaticBoxSizer(inputBox, wx.VERTICAL)
        vSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        vSizer.Add(cb_vl_label, pos = (0, 0),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.cb_vlayer,  pos = (0, 1),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(cb_vc_label, pos = (1, 0),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.cb_color_att, pos = (1, 1),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.rgb_range_label, pos = (1, 2),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(cb_vrgb_label, pos = (2, 0),
                  flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.cb_rgb_col, pos = (2, 1),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.btn_add_RGB, pos = (2, 2),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        inputSizer.Add(item = vSizer,
                       flag = wx.ALIGN_CENTER_VERTICAL | wx.ALL | wx.EXPAND, border = 5)
                
        return inputSizer 
       
    def _doLayout(self):
        """!Do main layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        #
        # map selection
        #
        mapSelection = self._createMapSelection(parent = self)
        sizer.Add(item = mapSelection, proportion = 0,
                  flag = wx.ALL | wx.EXPAND, border = 5)
        #
        # set vector attributes
        #
        vectorAttrb = self._createVectorAttrb(parent = self)
        sizer.Add(item = vectorAttrb, proportion = 0,
                  flag = wx.ALL | wx.EXPAND, border = 5)
        #
        # body & preview
        #
        bodySizer = self._createBody(parent = self)
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
                 
    def SetInfoString(self):
        """!Show information about vector map column type/range"""
        driver, db = self.dbInfo.GetDbSettings(int(self.properties['layer']))
        nrows = grass.db_describe(table = self.properties['table'], driver = driver, database = db)['nrows']
        self.properties['min'] = self.properties['max'] = ''
        type = self.dbInfo.GetTableDesc(self.properties['table'])\
                                                    [self.properties['source_rgb']]['type']
        ctype = self.dbInfo.GetTableDesc(self.properties['table'])\
                                                    [self.properties['source_rgb']]['ctype']
        if ctype == int or ctype == float:  
            if nrows < 500: # not too large
                ret = gcmd.RunCommand('v.db.select',
                                      quiet = True,
                                      read  = True,
                                      flags = 'c',
                                      map = self.inmap,
                                      layer = self.properties['layer'],
                                      columns = self.properties['source_rgb']).strip('\n')
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
               
    
    def CheckMapset(self):
        """!Check if current vector is in current mapset"""
        if grass.find_file(name = self.inmap,
                           element = 'vector')['mapset'] == grass.gisenv()['MAPSET']:
            return True
        else:
            return False 
         
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
            
    def OnSelectionInput(self, event):
        """!Vector map selected"""
        if event:
            self.inmap = event.GetString()

        if self.inmap:
            if not grass.find_file(name = self.inmap, element = 'vector')['file']:
                self.inmap = None
        
        self.UpdateDialog()
        
    def UpdateDialog(self):
        """!Update dialog after map selection"""    
        if not self.inmap:
            self.colorRulesPanel.ruleslines.Clear()
            self.btnPreview.Enable(False)
            self.btnOK.Enable(False)
            self.btnApply.Enable(False)
            self.preview.EraseMap()
            return
                               
        # check for db connection
        self.dbInfo = gselect.VectorDBInfo(self.inmap)
        if not len(self.dbInfo.layers):
            wx.CallAfter(self.NoConnection, self.inmap)
            for combo in (self.cb_vlayer, self.cb_color_att, self.cb_rgb_col):
                combo.SetValue("")
                combo.Disable()
                combo.Clear()
            enable = False   
        else:
        # initialize layer selection combobox
            for combo in (self.cb_vlayer, self.cb_color_att, self.cb_rgb_col):
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
                self.btn_add_RGB.Enable(True)
            else:
                enable = False
                wx.CallAfter(gcmd.GMessage, parent = self, 
                             message = _("Selected map <%s> is not in current mapset <%s>. "
                                        "Attribute table cannot be edited. "
                                        "Color rules will not be saved.") % 
                                        (self.inmap, grass.gisenv()['MAPSET']))
                              
                self.btn_add_RGB.Enable(False)
            
        self.btnPreview.Enable(enable)
        self.btnOK.Enable(enable)
        self.btnApply.Enable(enable)   
        
    def OnLayerSelection(self, event):
        # reset choices in column selection comboboxes if layer changes
        vlayer = int(self.cb_vlayer.GetStringSelection())
        self.cb_color_att.InsertColumns(vector = self.inmap, layer = vlayer, dbInfo = self.dbInfo)
        self.cb_color_att.SetSelection(0)
        self.properties['source_rgb'] = self.cb_color_att.GetString(0)
        self.cb_rgb_col.InsertColumns(vector = self.inmap, layer = vlayer, type = ["character"], dbInfo = self.dbInfo)
        self.cb_rgb_col.Delete(self.cb_rgb_col.FindString(self.properties['source_rgb']))
        found = self.cb_rgb_col.FindString('GRASSRGB')
        if found != wx.NOT_FOUND:
            self.cb_rgb_col.SetSelection(found)
            self.properties['rgb'] = self.cb_rgb_col.GetString(found)
        else:
            self.properties['rgb'] = ''
##        self.SetInfoString()
        
        self.LoadTable(attColumn = self.properties['source_rgb'],
                       rgbColumn = self.properties['rgb'], rulesPanel = self.colorRulesPanel)
        self.Update()
        
    def OnColumnSelection(self, event):
        self.properties['source_rgb'] = event.GetString()
        
        self.LoadTable(attColumn = self.properties['source_rgb'],
                       rgbColumn = self.properties['rgb'], rulesPanel = self.colorRulesPanel)
    
    def OnAddColumn(self, event):
        """!Add GRASSRGB column if it doesn't exist"""
        if 'GRASSRGB' not in self.cb_rgb_col.GetItems():
            ret = gcmd.RunCommand('v.db.addcolumn',
                                   map = self.inmap,
                                  layer = self.properties['layer'],
                                  columns = 'GRASSRGB varchar(20)')
            self.cb_rgb_col.InsertColumns(self.inmap, self.properties['layer'], type = ["character"])
            self.cb_rgb_col.SetStringSelection('GRASSRGB')
            self.properties['rgb'] = self.cb_rgb_col.GetStringSelection()
            
            self.LoadTable(attColumn = self.properties['source_rgb'], rgbColumn = self.properties['rgb'],
                       rulesPanel = self.colorRulesPanel, type = 'color')
        else:
            gcmd.GMessage(parent = self,
                          message = _("GRASSRGB column already exists."))
                        
    def CreateTable(self, dcmd, layer, params, propwin):
        """!Create attribute table"""
        if dcmd:
            cmd = utils.CmdToTuple(dcmd)
            ret = gcmd.RunCommand(cmd[0], **cmd[1])
            if ret == 0:
                self.OnSelectionInput(None)
                return True
            
        for combo in (self.cb_vlayer, self.cb_color_att, self.cb_rgb_col):
            combo.SetValue("")
            combo.Disable()
        return False    
    
    def LoadTable(self, attColumn, rgbColumn, rulesPanel, type = 'color'):
        """!Load current column (GRASSRGB, size column)"""
        
        rulesPanel.Clear()
        if not attColumn or not rgbColumn:
            self.preview.EraseMap()
            return
        
        busy = wx.BusyInfo(message = _("Please wait, loading data from attribute table..."),
                           parent = self)
        wx.Yield()
        if self.inmap:
            outFile = tempfile.NamedTemporaryFile(mode='w+b')
            sep = '|'
            ret = gcmd.RunCommand('v.db.select',
                                      quiet = True,
                                      flags = 'c',
                                      map = self.inmap,
                                      layer = self.properties['layer'],
                                      columns = attColumn + ',' + rgbColumn,
                                      fs = sep,
                                      stdout = outFile)
            
        else:
            self.OnPreview(event)
            busy.Destroy()
            return
        if type == 'color':
            ctype = self.dbInfo.GetTableDesc(self.properties['table'])\
                                            [self.properties['source_rgb']]['ctype']
        elif type == 'size':
            ctype = self.dbInfo.GetTableDesc(self.properties['table'])\
                                            [self.properties['source_size']]['ctype']
        outFile.seek(0)
        i = 0
        minim = maxim = 0.0
        while True:
            # os.linesep doesn't work here (MSYS)
            record = outFile.readline().replace('\n', '')
            
            if not record:
                break
            rulesPanel.ruleslines[i] = {}
            
            value = record.split(sep)[0]
            if ctype not in (int, float):
                value = "'" + value + "'"
            else:
                if float(value) < minim:
                    minim = float(value)
                if float(value) > maxim:
                    maxim = float(value)
            rulesPanel.ruleslines[i]['value'] = value
            rulesPanel.ruleslines[i][type] = record.split(sep)[1]
            i += 1
        
        rulesPanel.AddRules(i, start = True)
        ret = rulesPanel.LoadRules()
        self.SetRangeLabel(type, ctype, minim, maxim)
        
        if ret:
            self.OnPreview()    
        else:
            rulesPanel.Clear()
    
        busy.Destroy()
        
    def SetRangeLabel(self, type, ctype, minim, maxim):
        """!Set labels with info about attribute column range"""
        if type == 'color':
            if minim or maxim:
                if ctype == int:
                    self.rgb_range_label.SetLabel(_("range: %.1f to %.1f") % (minim, maxim))
                elif ctype == float:
                    self.rgb_range_label.SetLabel(_("range: %d to %d") % (minim, maxim))
            else:
                self.rgb_range_label.SetLabel('')
        elif type == 'size':
            if minim or maxim:
                if ctype == int:
                    self.size_range_label.SetLabel(_("range: %.1f to %.1f") % (minim, maxim))
                elif ctype == float:
                    self.size_range_label.SetLabel(_("range: %d to %d") % (minim, maxim))
            else:
                self.size_range_label.SetLabel('')
                
                
    def OnRGBColSelection(self, event):
        self.properties['rgb'] = event.GetString()
        
        self.LoadTable(attColumn = self.properties['source_rgb'],
                       rgbColumn = self.properties['rgb'], rulesPanel = self.colorRulesPanel)
            
    def OnPreview(self, event = None):
        """!Update preview (based on computational region)"""
        if not self.inmap or not self.properties["rgb"]:
            self.preview.EraseMap()
            return
        
        cmdlist = ['d.vect',
                    '-a',
                   'map=%s' % self.inmap,
                   'rgb_column=%s' % self.properties["rgb"],
                   'type=point,line,boundary,area']
        ltype = 'vector'
        
        ColorTable.DoPreview(self, ltype, cmdlist)
        
    def OnHelp(self, event):
        """!Show GRASS manual page"""
        cmd = 'v.colors'
        ColorTable.RunHelp(self, cmd = cmd)
        
    def CreateColorTable(self, force = False):
        """!Creates color table

        @return True on success
        @return False on failure
        """
        rulestxt = ''
        
        for rule in self.colorRulesPanel.ruleslines.itervalues():
            if not rule['value']: # skip empty rules
                continue
            
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
        
        gcmd.RunCommand('db.execute',
                        parent = self,
                        input = gtemp)
        return True
    
##    def ColorFromString(self, rgb):
##        """!Convert color string '255:255:255' to tuple"""
##        try:
##            r, g, b = rgb.split(':')
##            return (r, g, b)
##        
##        except ValueError:
##            return False
        
class ThematicVectorTable(VectorColorTable):
    def __init__(self, parent, vectorType, **kwargs):
        """!Dialog for interactively entering color/size rules
            for vector maps for thematic mapping in nviz"""
        self.vectorType = vectorType
        VectorColorTable.__init__(self, parent = parent, **kwargs)
        
        # additional bingings
        self.Bind(wx.EVT_COMBOBOX, self.OnSizeSourceSelection, self.cb_size_att)
        self.Bind(wx.EVT_COMBOBOX, self.OnSizeSelection, self.cb_size_col)
        self.Bind(wx.EVT_BUTTON, self.OnAddSizeColumn, self.btn_add_size)
        self.Bind(wx.EVT_CHECKBOX, self.OnColorChecked,  self.rgb_check)
        self.Bind(wx.EVT_CHECKBOX, self.OnSizeChecked,  self.size_check)
        
        self.SetTitle(_("Thematic mapping for vector map in 3D view"))
        
        
    def UpdateDialog(self):
        """!Update dialog according to selected map"""
        VectorColorTable.UpdateDialog(self)
                               
        if not len(self.dbInfo.layers):
            for combo in (self.cb_size_att, self.cb_size_col):
                combo.SetValue("")
                combo.Disable()
                combo.Clear()
            enable = False   
        else:
        # initialize layer selection combobox
            for combo in (self.cb_vlayer, self.cb_size_att, self.cb_size_col):
                combo.Enable()
            
            if self.CheckMapset():
                self.btn_add_size.Enable(True)
            else:
                self.btn_add_RGB.Enable(False)
                
    def OnLayerSelection(self, event):
        VectorColorTable.OnLayerSelection(self, event)
        # reset choices in column selection comboboxes if layer changes
        vlayer = int(self.cb_vlayer.GetStringSelection())
        self.cb_size_att.InsertColumns(vector = self.inmap, layer = vlayer, dbInfo = self.dbInfo)
        self.cb_size_att.SetSelection(0)
        self.properties['source_size'] = self.cb_size_att.GetString(0)
        self.cb_size_col.InsertColumns(vector = self.inmap, layer = vlayer,
                                       type = ["integer", "double precision"], dbInfo = self.dbInfo)
        for item in self.cb_size_col.GetItems():
            if item.lower().find('size') >= 0:
                self.cb_size_col.SetStringSelection(item)
                self.properties['size'] = item
            else:
                self.properties['size'] = ''
##        self.SetInfoString()
        self.LoadTable(attColumn = self.properties['source_size'], rgbColumn = self.properties['size'],
                       rulesPanel = self.sizeRulesPanel, type = 'size')
        self.Update()  
        
    def OnSizeSelection(self, event):
        self.properties['size'] = event.GetString()
        
        self.LoadTable(attColumn = self.properties['source_size'], rgbColumn = self.properties['size'],
                       rulesPanel = self.sizeRulesPanel, type = 'size')
    
    def OnSizeSourceSelection(self, event):
        self.properties['source_size'] = event.GetString()
        
##        self.SetInfoString()
        
        self.LoadTable(attColumn = self.properties['source_size'], rgbColumn = self.properties['size'],
                       rulesPanel = self.sizeRulesPanel, type = 'size') 
                    
    def _initLayer(self):
        """!Set initial layer when opening dialog"""
        self.inmap = self.parent.GetLayerData(nvizType = 'vector', nameOnly = True)
        self.selectionInput.SetValue(self.inmap)
        self.selectionInput.Disable()
        
    def _doLayout(self):
        """!Do main layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        #
        # map selection
        #
        mapSelection = self._createMapSelection(parent = self)
        sizer.Add(item = mapSelection, proportion = 0,
                  flag = wx.ALL | wx.EXPAND, border = 5)
        #
        # set vector attributes
        #
        vectorAttrb = self._createVectorAttrb(parent = self)
        sizer.Add(item = vectorAttrb, proportion = 0,
                  flag = wx.ALL | wx.EXPAND, border = 5)
        #
        # body & preview
        #
        bodySizer = self._createBody(parent = self)
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
    
    def _createBody(self, parent):
        """!Create dialog body consisting of rules and preview"""
        bodySizer =  wx.GridBagSizer(hgap = 5, vgap = 5)
        row = 0
        
        # label with instructions - don't want it now
        self.cr_label = wx.StaticText(parent, id = wx.ID_ANY)
        self.cr_label.Hide()

        # color table
        self.colorRulesPanel = RulesPanel(parent = parent, mapType = self.type,
                                          columnType = 'color', properties = self.properties,
                                          panelWidth = 200)
        # size table        
        self.sizeRulesPanel = RulesPanel(parent = parent, mapType = self.type,
                                         columnType = 'size', properties = self.properties,
                                         panelWidth = 200)
                                        
        bodySizer.Add(item = self.colorRulesPanel.label, pos = (row, 0), span = (1, 2))
        bodySizer.Add(item = self.sizeRulesPanel.label, pos = (row, 2), span = (1, 2))
        row += 1
        
        bodySizer.Add(item = self.colorRulesPanel.mainPanel, pos = (row, 0), span = (1, 2))
        # add two rules as default
        self.colorRulesPanel.AddRules(2)
        bodySizer.Add(item = self.sizeRulesPanel.mainPanel, pos = (row, 2), span = (1, 2))
        # add two rules as default
        self.sizeRulesPanel.AddRules(2)
        
        # preview window
        self._createPreview(parent = parent)
        bodySizer.Add(item = self.preview, pos = (row, 4),
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT, border = 10)
        bodySizer.AddGrowableRow(row)
        bodySizer.AddGrowableCol(4)
        row += 1
        
        # add rules button and spin to sizer
        bodySizer.Add(item = self.colorRulesPanel.numRules, pos = (row, 0),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        bodySizer.Add(item = self.colorRulesPanel.btnAdd, pos = (row, 1))
        bodySizer.Add(item = self.sizeRulesPanel.numRules, pos = (row, 2),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        bodySizer.Add(item = self.sizeRulesPanel.btnAdd, pos = (row, 3), 
                        flag = wx.ALIGN_LEFT)

        # preview button
        self.btnPreview = wx.Button(parent, id = wx.ID_ANY,
                                    label = _("Preview"))
        bodySizer.Add(item = self.btnPreview, pos = (row, 4),
                      flag = wx.ALIGN_RIGHT)
        self.btnPreview.Enable(False)
        self.btnPreview.SetToolTipString(_("Show preview of map "
                                           "(current Map Display extent is used)."))
                
        return bodySizer
    
    def _createVectorAttrb(self, parent):
        """!Create part of dialog with layer/column selection"""
        inputBox = wx.StaticBox(parent = parent, id = wx.ID_ANY,
                                label = " %s " % _("Select vector columns"))
        layer_label = wx.StaticText(parent, id = wx.ID_ANY, label = _('Layer:'))
        self.rgb_check = wx.CheckBox(parent, id = wx.ID_ANY, label = _('Use color for thematic mapping:'))
        if self.vectorType == 'points':
            label = _('Use symbol size for thematic mapping:')
        else:
            label = _('Use line width for thematic mapping:')
        self.size_check = wx.CheckBox(parent, id = wx.ID_ANY, label = label)
        
        self.rgb_check.SetValue(True)
        self.size_check.SetValue(True)
                                            
        color_att_label = wx.StaticText(parent, id = wx.ID_ANY,
                                         label = _('Attribute column:'))
        size_att_label = wx.StaticText(parent, id = wx.ID_ANY,
                                         label = _('Attribute column:'))
        rgb_col_label = wx.StaticText(parent, id = wx.ID_ANY,
                                           label = _('RGB color column:'))
        if self.vectorType == 'points':
            label = _('Symbol size column:')
        else:
            label = _('Line with column:')
        size_col_label = wx.StaticText(parent, id = wx.ID_ANY, label = label)
        
        self.rgb_range_label = wx.StaticText(parent, id = wx.ID_ANY)                                        
        self.size_range_label = wx.StaticText(parent, id = wx.ID_ANY)
        cb_size = (150, -1)
        self.cb_vlayer = gselect.LayerSelect(parent, size = cb_size)
        self.cb_color_att = gselect.ColumnSelect(parent, size = cb_size)
        self.cb_size_att = gselect.ColumnSelect(parent, size = cb_size)
        self.cb_rgb_col = gselect.ColumnSelect(parent, size = cb_size)
        self.cb_size_col = gselect.ColumnSelect(parent, size = cb_size)
        self.btn_add_RGB = wx.Button(parent, id = wx.ID_ANY,
                                             label = _('Add column'))
        self.btn_add_size = wx.Button(parent, id = wx.ID_ANY,
                                             label = _('Add column'))
        self.btn_add_RGB.SetToolTipString(_("Add GRASSRGB column to current attribute table."))
        
        if self.vectorType == 'points':
            label = _("Add size column to current attribute table.")
        else:
            label = _("Add width column to current attribute table.")
        self.btn_add_size.SetToolTipString(label)
        
        # layout
        inputSizer = wx.StaticBoxSizer(inputBox, wx.VERTICAL)
        vSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        vSizer.AddGrowableCol(2)
        
        vSizer.Add(layer_label, pos = (0, 0),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.cb_vlayer,  pos = (0, 1),
                   flag = wx.ALIGN_CENTER_VERTICAL)
                
        vSizer.Add(self.rgb_check,  pos = (1, 0), span = (1, 3),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.size_check,  pos = (1, 3), span = (1, 3),
                   flag = wx.ALIGN_CENTER_VERTICAL)
                
        vSizer.Add(color_att_label, pos = (2, 0),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(size_att_label, pos = (2, 3),
                   flag = wx.ALIGN_CENTER_VERTICAL)
                
        vSizer.Add(rgb_col_label, pos = (4, 0),
                  flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(size_col_label, pos = (4, 3),
                  flag = wx.ALIGN_CENTER_VERTICAL)
                
        vSizer.Add(self.cb_color_att, pos = (2, 1),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.cb_size_att, pos = (2, 4),
                   flag = wx.ALIGN_CENTER_VERTICAL)
                
        vSizer.Add(self.rgb_range_label, pos = (3, 1),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.size_range_label, pos = (3, 4),
                   flag = wx.ALIGN_CENTER_VERTICAL)
                
        vSizer.Add(self.cb_rgb_col, pos = (4, 1),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.cb_size_col, pos = (4, 4),
                   flag = wx.ALIGN_CENTER_VERTICAL)
                
        vSizer.Add(self.btn_add_RGB, pos = (4, 2),
                  flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.btn_add_size, pos = (4, 5),
                  flag = wx.ALIGN_CENTER_VERTICAL)
        inputSizer.Add(item = vSizer,
                       flag = wx.ALIGN_CENTER_VERTICAL | wx.ALL | wx.EXPAND, border = 5)
                
        return inputSizer 
    
    def OnAddSizeColumn(self, event):
        """!Add size column if it doesn't exist"""
        if self.vectorType == 'points':
            name = 'GRASSSIZE'
        else:
            name = 'GRASSWIDTH'
        
        ret = gcmd.RunCommand('v.db.addcolumn',
                               map = self.inmap,
                              layer = self.properties['layer'],
                              columns = '%s integer' % name)
        self.cb_size_col.InsertColumns(self.inmap, self.properties['layer'], type = ["integer"])
        self.cb_size_col.SetStringSelection(name)
        self.properties['size'] = name
        
        self.LoadTable(attColumn = self.properties['source_size'], rgbColumn = self.properties['size'],
                       rulesPanel = self.sizeRulesPanel, type = 'size')

    def OnColorChecked(self, event):
        """!Use color for thematic mapping"""
        if self.rgb_check.IsChecked():
            self.colorRulesPanel.Enable(True)
        else:
            self.colorRulesPanel.Enable(False)
        
    def OnSizeChecked(self, event):
        """!Use size for thematic mapping"""
        if self.size_check.IsChecked():
            self.sizeRulesPanel.Enable(True)
        else:
            self.sizeRulesPanel.Enable(False)
        
    def OnPreview(self, event = None):
        """!Update preview (based on computational region)"""
        if not self.inmap:
            self.preview.EraseMap()
            return
        
        cmdlist = ['d.vect',
                    '-a',
                   'map=%s' % self.inmap,
                   'type=point,line,boundary,area']
                
        if self.size_check.IsChecked() and self.properties["size"]:
            if self.vectorType == 'points':
                cmdlist.append('size_column=%s' % self.properties["size"])
            else:
                cmdlist.append('width_column=%s' % self.properties["size"])
            
        if self.rgb_check.IsChecked() and self.properties["rgb"]:
            cmdlist.append('rgb_column=%s' % self.properties["rgb"])
        ltype = 'vector'
        ColorTable.DoPreview(self, ltype, cmdlist)
    
    def OnApply(self, event):
        """!Apply selected color table

        @return True on success otherwise False
        """
        ret = self.CreateColorTable()
        if not ret:
            GMessage(parent = self, message = _("No color rules given."))
        
        data = self.parent.GetLayerData(nvizType = 'vector')
        data['vector']['points']['thematic']['layer'] = int(self.properties['layer'])
        
        if self.size_check.IsChecked() and self.properties['size']:
            data['vector'][self.vectorType]['thematic']['sizecolumn'] = self.properties['size']
        else:
            data['vector'][self.vectorType]['thematic']['sizecolumn'] = None
            
        if self.rgb_check.IsChecked() and self.properties['rgb']:
            data['vector'][self.vectorType]['thematic']['rgbcolumn'] = self.properties['rgb']
        else:
            data['vector'][self.vectorType]['thematic']['rgbcolumn'] = None
        
        data['vector'][self.vectorType]['thematic']['update'] = None
        
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.parent.mapWindow, event)
        self.parent.mapWindow.Refresh(False)
        
        return ret
    
    def CreateColorTable(self, force = False):
        """!Creates color table

        @return True on success
        @return False on failure
        """
        VectorColorTable.CreateColorTable(self)
        rulestxt = ''
        
        for rule in self.sizeRulesPanel.ruleslines.itervalues():
            if not rule['value']: # skip empty rules
                continue
            
            rulestxt += "UPDATE %s SET %s='%s' WHERE %s ;\n" % (self.properties['table'],
                                                                self.properties['size'],
                                                                rule['size'],
                                                                rule['value'])
        if not rulestxt:
            return False
        
        gtemp = utils.GetTempfile()
        output = open(gtemp, "w")
        try:
            output.write(rulestxt)
        finally:
            output.close()
        
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
                self.Map.region = copy.deepcopy(self.parent.parent.curr_page.maptree.Map.region)
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
    
