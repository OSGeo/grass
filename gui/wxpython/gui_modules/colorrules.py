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
    def __init__(self, parent, mapType, attributeType, properties, panelWidth = 180):
        """!Create rules panel
        
        @param mapType raster/vector
        @param attributeType color/size for choosing widget type
        @param properties properties of classes derived from ColorTable
        @param panelWidth width of scroll panel"""
        
        self.ruleslines = {}
        self.mapType = mapType
        self.attributeType = attributeType
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
        
        # (un)check all
        self.checkAll = wx.CheckBox(parent, id = wx.ID_ANY, label = _("Check all"))
        self.checkAll.SetValue(True)
        # clear button
        self.clearAll = wx.Button(parent, id = wx.ID_ANY, label = _("Clear all"))
        #  determines how many rules should be added
        self.numRules = wx.SpinCtrl(parent, id = wx.ID_ANY,
                                    min = 1, max = 1e6)
        # add rules
        self.btnAdd = wx.Button(parent, id = wx.ID_ADD)
        
        self.btnAdd.Bind(wx.EVT_BUTTON, self.OnAddRules)
        self.checkAll.Bind(wx.EVT_CHECKBOX, self.OnCheckAll)
        self.clearAll.Bind(wx.EVT_BUTTON, self.OnClearAll)
    
    
    def Clear(self):
        """!Clear and widgets and delete information"""
        self.ruleslines.clear()
        self.mainPanel.DestroyChildren()
    
    def OnCheckAll(self, event):
        """!(Un)check all rules"""
        check = event.GetInt()
        for child in self.mainPanel.GetChildren():
            if child.GetName() == 'enable':
                child.SetValue(check)
            else:
                child.Enable(check)
                
    def OnClearAll(self, event):
        """!Delete all widgets in panel"""
        self.Clear()
        
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
            enable.SetName('enable')
            enable.Bind(wx.EVT_CHECKBOX, self.OnRuleEnable)
            # value
            txt_ctrl = wx.TextCtrl(parent = self.mainPanel, id = 1000 + num,
                                   size = (80, -1),
                                   style = wx.TE_NOHIDESEL)
            if self.mapType == 'vector':
                txt_ctrl.SetToolTipString(_("Enter vector attribute values"))
            txt_ctrl.Bind(wx.EVT_TEXT, self.OnRuleValue)
            txt_ctrl.SetName('source')
            if self.attributeType == 'color':
                # color
                columnCtrl = csel.ColourSelect(self.mainPanel, id = 2000 + num,
                                               size  =  globalvar.DIALOG_COLOR_SIZE)
                columnCtrl.Bind(csel.EVT_COLOURSELECT, self.OnRuleColor)
                columnCtrl.SetName('target')
                if not start:
                    self.ruleslines[enable.GetId()] = { 'value' : '',
                                                        'color': "0:0:0" }
            else:
                # size or width
                init = 2
                if self.attributeType == 'size':
                    init = 100
                columnCtrl = wx.SpinCtrl(self.mainPanel, id = 2000 + num,
                                         size = (50, -1), min = 1, max = 1e4,
                                         initial = init)
                columnCtrl.Bind(wx.EVT_SPINCTRL, self.OnRuleSize)
                columnCtrl.Bind(wx.EVT_TEXT, self.OnRuleSize)
                columnCtrl.SetName('target')
                if not start:
                    self.ruleslines[enable.GetId()] = { 'value' : '',
                                                        self.attributeType: init }
            
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
            if self.mapType == 'vector' and not self.parent.GetParent().colorTable:
                vals = []
                vals.append(self.mainPanel.FindWindowById(id + 1000).GetValue())
                try:
                    vals.append(self.mainPanel.FindWindowById(id + 1 + 1000).GetValue())
                except AttributeError:
                    vals.append(None)
                value = self.SQLConvert(vals)
            else:
                value = self.mainPanel.FindWindowById(id + 1000).GetValue()
            color = self.mainPanel.FindWindowById(id + 2000).GetValue()
            
            if self.attributeType == 'color':
            # color
                color_str = str(color[0]) + ':' \
                          + str(color[1]) + ':' \
                          + str(color[2])
                self.ruleslines[id] = {'value' : value,
                                       'color' : color_str }
                
            else:
            # size or width
                self.ruleslines[id] = {'value' : value,
                                       self.attributeType  : float(color) }
        
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
        
        self.ruleslines[num - 2000][self.attributeType] = size
        
    def OnRuleValue(self, event):
        """!Rule value changed"""
        num = event.GetId()
        val = event.GetString().strip()
        
        if val == '':
            return
        try:
            table = self.parent.colorTable
        except AttributeError:
            # due to scrollpanel in vector dialog
            table = self.parent.GetParent().colorTable
        if table:
            self.SetRasterRule(num, val)
        else:
            self.SetVectorRule(num, val)

    def SetRasterRule(self, num, val): 
        """!Set raster rule"""       
        self.ruleslines[num - 1000]['value'] = val

    def SetVectorRule(self, num, val):
        """!Set vector rule"""
##        tc = self.mainPanel.FindWindowById(num)
##        source, target = 'sourceColumn', 'storeColumn'
##        if self.properties[source] == '' or self.properties[target] == '':
##            tc.SetValue('')
##            gcmd.GMessage(parent = self.parent,
##                          message = _("Please select attribute column "
##                                    "and RGB color column first"))
##        else:
        vals = []
        vals.append(val)
        try:
            vals.append(self.mainPanel.FindWindowById(num + 1).GetValue())
        except AttributeError:
            vals.append(None)
        self.ruleslines[num - 1000]['value'] = self.SQLConvert(vals)
##            except ValueError:
##                tc.SetValue('')
##                self.ruleslines[num - 1000]['value'] = ''
            
    def Enable(self, enable = True):
        """!Enable/Disable all widgets"""
        for child in self.mainPanel.GetChildren():
            child.Enable(enable)
        sql = True
        self.LoadRulesline(sql)# todo
        self.btnAdd.Enable(enable)
        self.numRules.Enable(enable)
        self.checkAll.Enable(enable)
        self.clearAll.Enable(enable)
        
        
    def LoadRules(self):
        message = ""
        for item in range(len(self.ruleslines)):
            self.mainPanel.FindWindowById(item + 1000).SetValue(self.ruleslines[item]['value'])
            r, g, b = (0, 0, 0) # default
            if not self.ruleslines[item][self.attributeType]:
                if self.attributeType == 'color':
                    self.ruleslines[item][self.attributeType] = '%d:%d:%d' % (r, g, b)
                elif self.attributeType == 'size':
                    self.ruleslines[item][self.attributeType] = 100                
                elif self.attributeType == 'width':
                    self.ruleslines[item][self.attributeType] = 2
                
            if self.attributeType == 'color':
                try:
                    r, g, b = map(int, self.ruleslines[item][self.attributeType].split(':'))
                except ValueError, e:
                    message =  _("Bad color format. Use color format '0:0:0'")
                self.mainPanel.FindWindowById(item + 2000).SetValue((r, g, b))
            else:
                value = float(self.ruleslines[item][self.attributeType])
                self.mainPanel.FindWindowById(item + 2000).SetValue(value)
                
        if message:
            gcmd.GMessage(parent = self.parent, message = message)
            return False
        
        return True
                
    def SQLConvert(self, vals):
        """!Prepare value for SQL query"""
        if vals[0].isdigit():
            sqlrule = '%s>=%s' % (self.properties['sourceColumn'], vals[0])
            if vals[1]:
                sqlrule += ' AND %s<%s' % (self.properties['sourceColumn'], vals[1])
        else:
            sqlrule = '%s=%s' % (self.properties['sourceColumn'], vals[0])
        
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
            if sel and self.parent.curr_page.maptree.GetPyData(sel)[0]['type'] == self.mapType:
                layer = sel
            else:
                layer = self.parent.curr_page.maptree.FindItemByData(key = 'type', value = self.mapType)
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
        btnSizer.Add(wx.Size(-1, -1), proportion = 1)
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
        self.rulesPanel = RulesPanel(parent = parent, mapType = self.mapType,
                                          attributeType = self.attributeType, properties = self.properties)
        
        bodySizer.Add(item = self.rulesPanel.mainPanel, pos = (row, 0), span = (1, 2),
                      flag = wx.ALL, border = 5)
        # add two rules as default
        self.rulesPanel.AddRules(2)
        
        # preview window
        self._createPreview(parent = parent)
        bodySizer.Add(item = self.preview, pos = (row, 2),
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT, border = 5)
        row += 1
        # add ckeck all and clear all
        bodySizer.Add(item = self.rulesPanel.checkAll, flag = wx.ALIGN_CENTER_VERTICAL, 
                      pos = (row, 0))
        bodySizer.Add(item = self.rulesPanel.clearAll, pos = (row, 1))
        
        # preview button
        self.btnPreview = wx.Button(parent, id = wx.ID_ANY,
                                    label = _("Preview"))
        bodySizer.Add(item = self.btnPreview, pos = (row, 2),
                      flag = wx.ALIGN_RIGHT)
        self.btnPreview.Enable(False)
        self.btnPreview.SetToolTipString(_("Show preview of map "
                                           "(current Map Display extent is used)."))
        row +=1
        # add rules button and spin to sizer
        bodySizer.Add(item = self.rulesPanel.numRules, pos = (row, 0),
                      flag = wx.ALIGN_CENTER_VERTICAL)
        bodySizer.Add(item = self.rulesPanel.btnAdd, pos = (row, 1))
        

       
                
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
        ret = self.CreateColorTable(mapType = self.mapType)
        if not self.colorTable:
            self.UseAttrColumn()
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
    
    def LoadTable(self, mapType = 'raster'):
        """!Load current color table (using `r.colors.out`)"""
        
        self.rulesPanel.Clear()
        if mapType == 'raster':
            cmd = ['r.colors.out',
                   'read=True',
                   'map=%s' % self.inmap,
                   'rules=-']
        else:
            cmd = ['v.colors.out',
                   'read=True',
                   'map=%s' % self.inmap,
                   'rules=-']
            
            if self.properties['sourceColumn'] != 'cat':
                cmd.append('column=%s' % self.properties['sourceColumn'])
            
        cmd = utils.CmdToTuple(cmd)
        
        if self.inmap:
            ctable = gcmd.RunCommand(cmd[0], **cmd[1])
        else:
            self.OnPreview()
            return
        
        rulesNumber = len(ctable.splitlines())
        self.rulesPanel.AddRules(rulesNumber)
        
        count = 0
        for line in ctable.splitlines():
            value, color = map(lambda x: x.strip(), line.split(' '))
            self.rulesPanel.ruleslines[count]['value'] = value
            self.rulesPanel.ruleslines[count]['color'] = color
            self.rulesPanel.mainPanel.FindWindowById(count + 1000).SetValue(value)
            rgb = list()
            for c in color.split(':'):
                rgb.append(int(c))
            self.rulesPanel.mainPanel.FindWindowById(count + 2000).SetColour(rgb)
            count += 1
        
        self.OnPreview(tmp = True)    
        
    def CreateColorTable(self, force = False, mapType = 'raster'):
        """!Creates color table

        @return True on success
        @return False on failure
        """
        rulestxt = ''
        
        for rule in self.rulesPanel.ruleslines.itervalues():
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
        
        flags = ''
        if mapType == 'raster':
            if not force and not self.ovrwrtcheck.IsChecked():
                flags = 'w'
                
        cmd = ['%s.colors' % mapType[0],#r.colors/v.colors
                'flags=%s' % flags,
                'map=%s' % self.inmap,
                'rules=%s' % gtemp]
        if mapType == 'vector' and self.properties['sourceColumn']:
            cmd.append('column=%s' % self.properties['sourceColumn'])
        cmd = utils.CmdToTuple(cmd)
        ret = gcmd.RunCommand(cmd[0], **cmd[1])
                        
        if ret != 0 and mapType == 'raster':
            gcmd.GMessage(_("Color table already exists. "
                            "Check out 'replace existing color table' to "
                            "overwrite it."),
                          parent = self)
            return False
        
        return True
    
    def DoPreview(self, ltype, cmdlist):
        """!Update preview (based on computational region)"""
        
        if not self.layer:
            self.layer = self.Map.AddLayer(type = ltype, name = 'preview', command = cmdlist,
                                           l_active = True, l_hidden = False, l_opacity = 1.0,
                                           l_render = False) 
        else:
            self.layer.SetCmd(cmdlist)
        
        # apply new color table and display preview
        self.CreateColorTable(force = True, mapType = self.mapType)
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

        self.mapType = 'raster'
        self.attributeType = 'color' 
        self.colorTable = True 
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
            self.LoadTable()
            return
        
        info = grass.raster_info(map = self.inmap)
        
        if info:
            self.properties['min'] = info['min']
            self.properties['max'] = info['max']
            self.LoadTable()
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
            
          
    def OnPreview(self, tmp = True):
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
        for rule in self.rulesPanel.ruleslines.itervalues():
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
    

                      
class VectorColorTable(ColorTable):
    def __init__(self, parent, attributeType, **kwargs):
        """!Dialog for interactively entering color rules for vector maps"""
        # dialog attributes
        self.mapType = 'vector'
        self.attributeType = attributeType # color, size, width
        # in version 7 v.colors used, otherwise color column only
        self.version7 = int(grass.version()['version'].split('.')[0]) >= 7
        self.colorTable = False
        # vector properties
        self.properties = {
            # vector layer for attribute table to use for setting color
            'layer' : 1, 
            # vector attribute table used for setting color         
            'table' : '',
            # vector attribute column for assigning colors
            'sourceColumn' : '', 
            # vector attribute column to use for loading colors
            'loadColumn' : '',
            # vector attribute column to use for storing colors
            'storeColumn' : '',    
            # vector attribute column for temporary storing colors   
            'tmpColumn' : ''
            }     
        self.columnsProp = {'color': {'name': 'GRASSRGB', 'type1': 'varchar(11)', 'type2': ['character']},
                            'size' : {'name': 'GRASSSIZE', 'type1': 'integer', 'type2': ['integer']},
                            'width': {'name': 'GRASSWIDTH', 'type1': 'integer', 'type2': ['integer']}}
        ColorTable.__init__(self, parent = parent,
                            title = _('Create new color rules for vector map'), **kwargs)   
        
        # additional bindings for vector color management
        self.Bind(wx.EVT_COMBOBOX, self.OnLayerSelection, self.layerSelect)
        self.Bind(wx.EVT_COMBOBOX, self.OnSourceColumnSelection, self.sourceColumn)
        self.Bind(wx.EVT_COMBOBOX, self.OnFromColSelection, self.fromColumn)
        self.Bind(wx.EVT_COMBOBOX, self.OnToColSelection, self.toColumn)
        self.Bind(wx.EVT_BUTTON, self.OnAddColumn, self.addColumn)    
        
        self._initLayer()
        if self.colorTable:
            self.cr_label.SetLabel(_("Enter vector attribute values or percents:"))
        else:
            self.cr_label.SetLabel(_("Enter vector attribute values:"))
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
        inputSizer.Add(item = self.selectionInput, proportion = 1,
                       flag = wx.ALIGN_CENTER_VERTICAL | wx.ALL | wx.EXPAND, border = 5)
                    
        return inputSizer
    
    def _createVectorAttrb(self, parent):
        """!Create part of dialog with layer/column selection"""
        inputBox = wx.StaticBox(parent = parent, id = wx.ID_ANY,
                                label = " %s " % _("Select vector columns"))
        cb_vl_label = wx.StaticText(parent, id = wx.ID_ANY,
                                             label = _('Layer:'))
        cb_vc_label = wx.StaticText(parent, id = wx.ID_ANY,
                                         label = _('Attribute column:'))
                                        
        if self.attributeType == 'color':
            labels =  [_("Load color from column:"), _("Save color to column:")]
        elif self.attributeType == 'size':
            labels =  [_("Load size from column:"), _("Save size to column:")]
        elif self.attributeType == 'width':
            labels =  [_("Load width from column:"), _("Save width to column:")]
            
        if self.version7 and self.attributeType == 'color':
            self.useColumn = wx.CheckBox(parent, id = wx.ID_ANY,
                                  label = _("Use color column instead of color table:"))
            self.useColumn.Bind(wx.EVT_CHECKBOX, self.OnCheckColumn)
        
        fromColumnLabel = wx.StaticText(parent, id = wx.ID_ANY,
                                            label = labels[0])
        toColumnLabel = wx.StaticText(parent, id = wx.ID_ANY,
                                            label = labels[1])
                                                
        self.rgb_range_label = wx.StaticText(parent, id = wx.ID_ANY)
        self.layerSelect = gselect.LayerSelect(parent)
        self.sourceColumn = gselect.ColumnSelect(parent)
        self.fromColumn = gselect.ColumnSelect(parent)
        self.toColumn = gselect.ColumnSelect(parent)
        self.addColumn = wx.Button(parent, id = wx.ID_ANY,
                                             label = _('Add column'))
        self.addColumn.SetToolTipString(_("Add GRASSRGB column to current attribute table."))
        
        # layout
        inputSizer = wx.StaticBoxSizer(inputBox, wx.VERTICAL)
        vSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        row = 0
        vSizer.Add(cb_vl_label, pos = (row, 0),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.layerSelect,  pos = (row, 1),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        row += 1
        vSizer.Add(cb_vc_label, pos = (row, 0),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.sourceColumn, pos = (row, 1),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.rgb_range_label, pos = (row, 2),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        row += 1   
        if self.version7 and self.attributeType == 'color':
            vSizer.Add(self.useColumn, pos = (row, 0), span = (1, 2),
                       flag = wx.ALIGN_CENTER_VERTICAL)
            row += 1
            
        vSizer.Add(fromColumnLabel, pos = (row, 0),
                  flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.fromColumn, pos = (row, 1),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        row += 1
        vSizer.Add(toColumnLabel, pos = (row, 0),
                  flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.toColumn, pos = (row, 1),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        vSizer.Add(self.addColumn, pos = (row, 2),
                   flag = wx.ALIGN_CENTER_VERTICAL)
        inputSizer.Add(item = vSizer,
                       flag = wx.ALIGN_CENTER_VERTICAL | wx.ALL | wx.EXPAND, border = 5)
        self.colorColumnSizer = vSizer        
        return inputSizer 
       
    def _doLayout(self):
        """!Do main layout"""
        scrollPanel = scrolled.ScrolledPanel(self, id = wx.ID_ANY, size = (650, 500),
                                             style = wx.TAB_TRAVERSAL)
        scrollPanel.SetupScrolling()
        sizer = wx.BoxSizer(wx.VERTICAL)
        #
        # map selection
        #
        mapSelection = self._createMapSelection(parent = scrollPanel)
        sizer.Add(item = mapSelection, proportion = 0,
                  flag = wx.ALL | wx.EXPAND, border = 5)
        #
        # set vector attributes
        #
        vectorAttrb = self._createVectorAttrb(parent = scrollPanel)
        sizer.Add(item = vectorAttrb, proportion = 0,
                  flag = wx.ALL | wx.EXPAND, border = 5)
        #
        # body & preview
        #
        bodySizer = self._createBody(parent = scrollPanel)
        sizer.Add(item = bodySizer, proportion = 0,
                  flag = wx.ALL, border = 5)
                
                
        scrollPanel.SetSizer(sizer)
        scrollPanel.Fit()        
        
        #
        # buttons
        #
        btnSizer = self._createButtons()
        
        mainsizer = wx.BoxSizer(wx.VERTICAL)
        mainsizer.Add(scrollPanel, proportion = 1, flag = wx.EXPAND | wx.ALL, border = 5)
        mainsizer.Add(item = wx.StaticLine(parent = self, id = wx.ID_ANY,
                                       style = wx.LI_HORIZONTAL), proportion = 0,
                                       flag = wx.EXPAND | wx.ALL, border = 5) 
        mainsizer.Add(item = btnSizer, proportion = 0,
                  flag = wx.ALL | wx.ALIGN_RIGHT | wx.EXPAND, border = 5)
        
        self.SetSizer(mainsizer)
        mainsizer.Layout()
        mainsizer.Fit(self)       
    
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
                                                      completed = (self.CreateAttrTable, self.inmap, ''))
        else:
            dlg.Destroy()
  
    def OnCheckColumn(self, event):
        """!Use color column instead of color table"""
        if self.useColumn.GetValue():
            self.properties['loadColumn'] = self.fromColumn.GetStringSelection()
            self.properties['storeColumn'] = self.toColumn.GetStringSelection()
            self.fromColumn.Enable(True)
            self.toColumn.Enable(True)
            self.colorTable = False
            
            if self.properties['loadColumn']:
                self.LoadTable()
            else:
                self.rulesPanel.Clear()
        else:
            self.properties['loadColumn'] = ''
            self.properties['storeColumn'] = ''
            self.fromColumn.Enable(False)
            self.toColumn.Enable(False)
            self.colorTable = True
            
            self.LoadTable()
        
            
    def EnableVectorAttributes(self, enable):
        """!Enable/disable part of dialog connected with db"""
        for child in self.colorColumnSizer.GetChildren():
            child.GetWindow().Enable(enable)
    
    def DisableClearAll(self):
        """!Enable, disable the whole dialog"""
        self.rulesPanel.Clear()
        self.EnableVectorAttributes(False)
        self.btnPreview.Enable(False)
        self.btnOK.Enable(False)
        self.btnApply.Enable(False)
        self.preview.EraseMap()
        
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
            self.DisableClearAll()
            return
        
        if self.inmap and not self.CheckMapset():
            # currently v.colors need the map to be in current mapset
            if self.version7 and self.attributeType == 'color':
                message = _("Selected map <%s> is not in current mapset <%s>. "
                            "Color rules cannot be edited.") % \
                            (self.inmap, grass.gisenv()['MAPSET'])
            else:
                message = _("Selected map <%s> is not in current mapset <%s>. "
                            "Attribute table cannot be edited.") % \
                            (self.inmap, grass.gisenv()['MAPSET'])
            wx.CallAfter(gcmd.GMessage, parent = self, message = message)
            self.DisableClearAll()
            return
              
        # check for db connection
        self.dbInfo = gselect.VectorDBInfo(self.inmap)
        enable = True
        if not len(self.dbInfo.layers): # no connection
            if not (self.version7 and self.attributeType == 'color'): # otherwise it doesn't matter
                wx.CallAfter(self.NoConnection, self.inmap)
                enable = False
            for combo in (self.layerSelect, self.sourceColumn, self.fromColumn, self.toColumn):
                combo.SetValue("")
                combo.Clear()
            self.EnableVectorAttributes(False)
              
        else: # db connection exist
        # initialize layer selection combobox
            self.EnableVectorAttributes(True)
            self.layerSelect.InsertLayers(self.inmap)
            # initialize attribute table for layer=1
            self.properties['layer'] = self.layerSelect.GetString(0)
            self.layerSelect.SetStringSelection(self.properties['layer'])
            layer = int(self.properties['layer'])
            self.properties['table'] = self.dbInfo.layers[layer]['table']
            
            if self.attributeType == 'color':
                self.AddTemporaryColumn(name = 'tmpColumn', type = 'varchar(20)')
            else:
                self.AddTemporaryColumn(name = 'tmpColumn', type = 'integer')
            
            # initialize column selection comboboxes 
            
            self.OnLayerSelection(event = None)
        if self.version7 and self.attributeType == 'color':
            self.useColumn.SetValue(False)
            self.OnCheckColumn(event = None) 
        
        self.LoadTable()
            
        self.btnPreview.Enable(enable)
        self.btnOK.Enable(enable)
        self.btnApply.Enable(enable)   
    
    def AddTemporaryColumn(self, name, type):
        """!Add temporary column to not overwrite the original values,
            need to be deleted when closing dialog"""
        ret = gcmd.RunCommand('v.db.addcolumn',
                              map = self.inmap,
                              layer = self.properties['layer'],
                              column = '%s %s' % (name, type))
        self.properties[name] = name
        
    def DeleteTemporaryColumn(self, name):
        """!Delete temporary column"""
        ret = gcmd.RunCommand('v.db.dropcolumn',
                              map = self.inmap,
                              layer = self.properties['layer'],
                              column = name)
                       
    def OnLayerSelection(self, event):
        # reset choices in column selection comboboxes if layer changes
        vlayer = int(self.layerSelect.GetStringSelection())
        self.sourceColumn.InsertColumns(vector = self.inmap, layer = vlayer,
                                        type = ['integer', 'double precision'], dbInfo = self.dbInfo,
                                        excludeCols = ['tmpColumn'])
        self.sourceColumn.SetStringSelection('cat')
        self.properties['sourceColumn'] = self.sourceColumn.GetString(0)
        
        if self.attributeType == 'color':
            type = ['character']
        else:
            type = ['integer']
        self.fromColumn.InsertColumns(vector = self.inmap, layer = vlayer, type = type,
                                      dbInfo = self.dbInfo, excludeCols = ['tmpColumn'])
        self.toColumn.InsertColumns(vector = self.inmap, layer = vlayer, type = type,
                                    dbInfo = self.dbInfo, excludeCols = ['tmpColumn'])
        
        found = self.fromColumn.FindString(self.columnsProp[self.attributeType]['name'])
        if found != wx.NOT_FOUND:
            self.fromColumn.SetSelection(found)
            self.toColumn.SetSelection(found)
            self.properties['loadColumn'] = self.fromColumn.GetString(found)
            self.properties['storeColumn'] = self.toColumn.GetString(found)
        else:
            self.properties['loadColumn'] = ''
            self.properties['storeColumn'] = ''
        
        if event:
            self.LoadTable()
        self.Update()
        
    def OnSourceColumnSelection(self, event):
        self.properties['sourceColumn'] = event.GetString()
        
        self.LoadTable()
    
    def OnAddColumn(self, event):
        """!Add GRASS(RGB,SIZE,WIDTH) column if it doesn't exist"""
        if self.columnsProp[self.attributeType]['name'] not in self.fromColumn.GetItems():
            ret = gcmd.RunCommand('v.db.addcolumn',
                                  map = self.inmap,
                                  layer = self.properties['layer'],
                                  columns = '%s %s' % (self.columnsProp[self.attributeType]['name'],
                                            self.columnsProp[self.attributeType]['type1']))
            self.toColumn.InsertColumns(self.inmap, self.properties['layer'],
                                        type = self.columnsProp[self.attributeType]['type2'])
            self.toColumn.SetStringSelection(self.columnsProp[self.attributeType]['name'])
            self.properties['storeColumn'] = self.toColumn.GetStringSelection()
            
            self.LoadTable()
        else:
            gcmd.GMessage(parent = self,
                          message = _("%s column already exists.") % \
                                    self.columnsProp[self.attributeType]['name'])
                        
    def CreateAttrTable(self, dcmd, layer, params, propwin):
        """!Create attribute table"""
        if dcmd:
            cmd = utils.CmdToTuple(dcmd)
            ret = gcmd.RunCommand(cmd[0], **cmd[1])
            if ret == 0:
                self.OnSelectionInput(None)
                return True
            
        for combo in (self.layerSelect, self.sourceColumn, self.fromColumn, self.toColumn):
            combo.SetValue("")
            combo.Disable()
        return False    
    
    def LoadTable(self):
        """!Load table"""
        if self.colorTable:
            ColorTable.LoadTable(self, mapType = 'vector')
        else:
            self.LoadRulesFromColumn(sourceColumn = self.properties['sourceColumn'],
                                     loadColumn = self.properties['loadColumn'],
                                     rulesPanel = self.rulesPanel, 
                                     type = self.attributeType)
            
    def LoadRulesFromColumn(self, sourceColumn, loadColumn, rulesPanel, type):
        """!Load current column (GRASSRGB, size column)"""
        
        rulesPanel.Clear()
        if not sourceColumn:
            self.preview.EraseMap()
            return
        
        busy = wx.BusyInfo(message = _("Please wait, loading data from attribute table..."),
                           parent = self)
        wx.Yield()
        
        columns = sourceColumn
        if loadColumn:
            columns += ',' + loadColumn
            
        if self.inmap:
            outFile = tempfile.NamedTemporaryFile(mode = 'w+b')
            sep = '|'
            ret = gcmd.RunCommand('v.db.select',
                                  quiet = True,
                                  flags = 'c',
                                  map = self.inmap,
                                  layer = self.properties['layer'],
                                  columns = columns,
                                  fs = sep,
                                  stdout = outFile)
        else:
            self.preview.EraseMap()
            busy.Destroy()
            return
        
##        if type == 'color':
##            ctype = self.dbInfo.GetTableDesc(self.properties['table'])\
##                                            [sourceColumn]['ctype']
        outFile.seek(0)
        i = 0
        minim = maxim = 0.0
        while True:
            # os.linesep doesn't work here (MSYS)
            record = outFile.readline().replace('\n', '')
            if not record:
                break
            rulesPanel.ruleslines[i] = {}
            
            if not loadColumn:
                col1 = record
                col2 = None
            else:
                col1, col2 = record.split(sep)
            
##            if ctype not in (int, float):
##                col1 = "'" + col1 + "'"
##            else:
            if float(col1) < minim:
                minim = float(col1)
            if float(col1) > maxim:
                maxim = float(col1)
                    
            rulesPanel.ruleslines[i]['value'] = col1
            rulesPanel.ruleslines[i][type] = col2
            i += 1
        
        rulesPanel.AddRules(i, start = True)
        ret = rulesPanel.LoadRules()
##        self.SetRangeLabel(type, ctype, minim, maxim)
        
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
                
    def OnFromColSelection(self, event):
        """!Selection in combobox (for loading values) changed"""
        self.properties['loadColumn'] = event.GetString()
        
        self.LoadTable()
    
    def OnToColSelection(self, event):
        """!Selection in combobox (for storing values) changed"""
        self.properties['storeColumn'] = event.GetString()
    
    def OnPreview(self, event = None, tmp = True):
        """!Update preview (based on computational region)"""
        if self.colorTable:
            self.OnTablePreview(tmp)
        else:
            self.OnColumnPreview() 
                                 
    def OnTablePreview(self, tmp):
        """!Update preview (based on computational region)"""
        if not self.inmap:
            self.preview.EraseMap()
            return
        
        ltype = 'vector'
        cmdlist = ['d.vect',
                   'map=%s' % self.inmap]
        
        # find existing color table and copy to temp file
        old_colrtable = None
        path = grass.find_file(name = self.inmap, element = 'vector')['file']
        
        if os.path.exists(os.path.join(path, 'colr')):
            old_colrtable = os.path.join(path, 'colr')
            colrtemp = utils.GetTempfile()
            shutil.copyfile(old_colrtable, colrtemp)
            
        ColorTable.DoPreview(self, ltype, cmdlist)  
        
        # restore previous color table
        if tmp:
            if old_colrtable:
                shutil.copyfile(colrtemp, old_colrtable)
                os.remove(colrtemp)
            else:
                gcmd.RunCommand('v.colors',
                                parent = self,
                                flags = 'r',
                                map = self.inmap)
    def OnColumnPreview(self):
        """!Update preview (based on computational region)"""
        if not self.inmap or not self.properties['tmpColumn']:
            self.preview.EraseMap()
            return
        
        cmdlist = ['d.vect',
                    '-a',
                   'map=%s' % self.inmap,
                   'type=point,line,boundary,area']
                
        if self.attributeType == 'color':
            cmdlist.append('rgb_column=%s' % self.properties['tmpColumn'])
        elif self.attributeType == 'size':
            cmdlist.append('size_column=%s' % self.properties['tmpColumn'])
        elif self.attributeType == 'width':
            cmdlist.append('width_column=%s' % self.properties['tmpColumn'])
            
        ltype = 'vector'
        
        ColorTable.DoPreview(self, ltype, cmdlist)
        
    def OnHelp(self, event):
        """!Show GRASS manual page"""
        cmd = 'v.colors'
        ColorTable.RunHelp(self, cmd = cmd)
        
    def UseAttrColumn(self, useAttrColumn = True):
        """!Use attribute column to render map"""
        # TODO switch back to table, not only selected map
        layer = self.parent.curr_page.maptree.layer_selected
        if layer and self.parent.curr_page.maptree.GetPyData(layer)[0]['maplayer'].GetName() == self.inmap:
##            cmd = self.parent.curr_page.maptree.GetPyData(layer)[0]['maplayer'].GetCmd()
##            if useAttrColumn:
##                cmd[1].update({'flags': 'a', })
            cmdlist = ['d.vect',
                        '-a',
                       'map=%s' % self.inmap,
                       'type=point,line,boundary,area']
                
            if self.attributeType == 'color':
                cmdlist.append('rgb_column=%s' % self.properties['storeColumn'])
            elif self.attributeType == 'size':
                cmdlist.append('size_column=%s' % self.properties['storeColumn'])
            elif self.attributeType == 'width':
                cmdlist.append('width_column=%s' % self.properties['storeColumn'])
            
            self.parent.curr_page.maptree.GetPyData(layer)[0]['maplayer'].SetCmd(cmdlist)
        
    def CreateColorTable(self, force = False, mapType = 'vector'):
        """!Create color rules (color table or color column)"""
        if self.colorTable:
            ret = ColorTable.CreateColorTable(self, force, mapType)
        else:
            ret = self.UpdateColorColumn(force)
            
        return ret
        
    def UpdateColorColumn(self, force):
        """!Creates color table

        @return True on success
        @return False on failure
        """
        rulestxt = ''
        
        for rule in self.rulesPanel.ruleslines.itervalues():
            if not rule['value']: # skip empty rules
                continue
            if force:
                rgb_col = self.properties['tmpColumn']
            else:
                rgb_col = self.properties['storeColumn']
                if not self.properties['storeColumn']:
                    gcmd.GMessage(self.parent, message = _("Please select column to save values to."))
                    
            rulestxt += "UPDATE %s SET %s='%s' WHERE %s ;\n" % (self.properties['table'],
                                                                rgb_col,
                                                                rule[self.attributeType],
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
    
    def OnOK(self, event):
        """!Apply selected color table, close the dialog
            and delete temporary column"""
        if self.OnApply(event):
            self.Destroy()
            self.DeleteTemporaryColumn(name = self.properties['tmpColumn'])
    
    def OnCancel(self, event):
        """!Do not apply any changes and close the dialog"""
        self.Destroy()
        self.DeleteTemporaryColumn(name = self.properties['tmpColumn'])
        
class ThematicVectorTable(VectorColorTable):
    def __init__(self, parent, vectorType, **kwargs):
        """!Dialog for interactively entering color/size rules
            for vector maps for thematic mapping in nviz"""
        self.vectorType = vectorType
        VectorColorTable.__init__(self, parent = parent, **kwargs)
        
        
        self.SetTitle(_("Thematic mapping for vector map in 3D view"))
        
                    
    def _initLayer(self):
        """!Set initial layer when opening dialog"""
        self.inmap = self.parent.GetLayerData(nvizType = 'vector', nameOnly = True)
        self.selectionInput.SetValue(self.inmap)
        self.selectionInput.Disable()
        
    def OnApply(self, event):
        """!Apply selected color table

        @return True on success otherwise False
        """
        
        ret = self.CreateColorTable()
        if not ret:
            gcmd.GMessage(parent = self, message = _("No rules given."))
        
        data = self.parent.GetLayerData(nvizType = 'vector')
        data['vector']['points']['thematic']['layer'] = int(self.properties['layer'])
        
        value = None
        if self.properties['storeColumn']:
            value = self.properties['storeColumn']
            
        if not self.colorTable:
            if self.attributeType == 'color':
                data['vector'][self.vectorType]['thematic']['rgbcolumn'] = value
            else:
                data['vector'][self.vectorType]['thematic']['sizecolumn'] = value
        else:
            if self.attributeType == 'color':
                data['vector'][self.vectorType]['thematic']['rgbcolumn'] = None
            else:
                data['vector'][self.vectorType]['thematic']['sizecolumn'] = None
        
        data['vector'][self.vectorType]['thematic']['update'] = None
        
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self.parent.mapWindow, event)
        self.parent.mapWindow.Refresh(False)
        
        return ret
           
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
    
