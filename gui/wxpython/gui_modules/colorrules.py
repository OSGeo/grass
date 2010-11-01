"""
@package colorrules.py

@brief Dialog for interactive management of raster color tables and
vector rgb_column attributes.

Classes:
 - ColorTable
 - BuferedWindow

(C) 2008, 2010 by the GRASS Development Team
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
from debug import Debug as Debug
from preferences import globalSettings as UserSettings

class ColorTable(wx.Frame):
    def __init__(self, parent, raster, id=wx.ID_ANY, title = _("Set color table"),
                 style=wx.DEFAULT_FRAME_STYLE | wx.RESIZE_BORDER,
                 **kwargs):
        """!Dialog for interactively entering rules for map management
        commands

        @param raster True to raster otherwise vector
        """
        self.parent = parent # GMFrame
        self.raster = raster
        
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
                'layers' : ['1'],
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
        
        if self.raster:
            self.SetTitle(_('Create new color table for raster map'))
            crlabel = _('Enter raster category values or percents')
        else:
            self.SetTitle(_('Create new color table for vector map'))
            crlabel = _('Enter vector attribute values or ranges (n or n1 to n2)')
        
        # top controls
        if self.raster:
            maplabel = _('Select raster map:')
        else:
            maplabel = _('Select vector map:')
        inputBox = wx.StaticBox(parent=self, id=wx.ID_ANY,
                                label=" %s " % maplabel)
        self.inputSizer = wx.StaticBoxSizer(inputBox, wx.VERTICAL)
        if self.raster:
            elem = 'cell'
        else:
            elem = 'vector'
        self.selectionInput = gselect.Select(parent=self, id=wx.ID_ANY,
                                             size=globalvar.DIALOG_GSELECT_SIZE,
                                             type=elem)
        
        self.ovrwrtcheck = wx.CheckBox(parent=self, id=wx.ID_ANY,
                                       label=_('replace existing color table'))
        self.ovrwrtcheck.SetValue(UserSettings.Get(group='cmd', key='overwrite', subkey='enabled'))
        
        if self.raster:
            self.btnSave = wx.Button(parent=self, id=wx.ID_SAVE)
            self.btnSave.SetToolTipString(_('Save color table to file'))
        
        if not self.raster:
            self.cb_vl_label = wx.StaticText(parent=self, id=wx.ID_ANY,
                                             label=_('Layer:'))
            self.cb_vc_label = wx.StaticText(parent=self, id=wx.ID_ANY,
                                             label=_('Attribute column:'))
            self.cb_vrgb_label = wx.StaticText(parent=self, id=wx.ID_ANY,
                                               label=_('RGB color column:'))
            self.cb_vlayer = gselect.LayerSelect(self)
            self.cb_vcol = gselect.ColumnSelect(self)
            self.cb_vrgb = gselect.ColumnSelect(self)
        
        # color table and preview window
        self.cr_label = wx.StaticText(parent=self, id=wx.ID_ANY,
                                      label=crlabel)
        self.cr_panel = self._colorRulesPanel()
        # add two rules as default
        self.AddRules(2)
        
        self.numRules = wx.SpinCtrl(parent=self, id=wx.ID_ANY,
                                    min=1, max=1e6)
        
        # initialize preview display
        self.InitDisplay()
        self.preview = BufferedWindow(self, id=wx.ID_ANY, size=(400, 300),
                                      Map=self.Map)
        self.preview.EraseMap()
        
        self.btnCancel = wx.Button(parent=self, id=wx.ID_CANCEL)
        self.btnApply = wx.Button(parent=self, id=wx.ID_APPLY) 
        self.btnOK = wx.Button(parent=self, id=wx.ID_OK)
        self.btnOK.SetDefault()
        self.btnOK.Enable(False)
        self.btnApply.Enable(False)
        
        self.btnPreview = wx.Button(parent=self, id=wx.ID_ANY,
                                    label=_("Preview"))
        self.btnPreview.Enable(False)
        self.btnAdd = wx.Button(parent=self, id=wx.ID_ADD)
        self.helpbtn = wx.Button(parent=self, id=wx.ID_HELP)
            
        
        # bindings
        self.Bind(wx.EVT_BUTTON, self.OnHelp, self.helpbtn)
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

        # set map layer from layer tree
        try:
            layer = self.parent.curr_page.maptree.layer_selected
        except:
            layer = None
        if layer:
            mapLayer = self.parent.curr_page.maptree.GetPyData(layer)[0]['maplayer']
            name = mapLayer.GetName()
            type = mapLayer.GetType()
            self.selectionInput.SetValue(name)
            self.inmap = name
            self.OnSelectionInput(None)
        
        # layout
        self.__doLayout()
        self.SetMinSize(self.GetSize())
        
        self.CentreOnScreen()
        self.Show()
        
    def __doLayout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        #
        # input
        #
        self.inputSizer.Add(item=self.selectionInput,
                       flag=wx.ALIGN_CENTER_VERTICAL | wx.ALL | wx.EXPAND, border=5)
        replaceSizer = wx.BoxSizer(wx.HORIZONTAL)
        replaceSizer.Add(item=self.ovrwrtcheck, proportion=1,
                         flag=wx.ALL | wx.ALIGN_CENTER_VERTICAL, border=1)
        if self.raster:
            replaceSizer.Add(item=self.btnSave, proportion=0,
                            flag=wx.ALIGN_RIGHT | wx.ALL, border=5)
        
        self.inputSizer.Add(item=replaceSizer, proportion=1,
                       flag=wx.ALL | wx.EXPAND, border=0)

        #
        # body & preview
        #
        bodySizer =  wx.GridBagSizer(hgap=5, vgap=5)
        row = 0
        bodySizer.Add(item=self.cr_label, pos=(row, 0), span=(1, 3),
                      flag=wx.ALL, border=5)
        
        if not self.raster:
            vSizer = wx.GridBagSizer(hgap=5, vgap=5)
            vSizer.Add(self.cb_vl_label, pos=(0, 0),
                       flag=wx.ALIGN_CENTER_VERTICAL)
            vSizer.Add(self.cb_vlayer,  pos=(0, 1),
                       flag=wx.ALIGN_CENTER_VERTICAL)
            vSizer.Add(self.cb_vc_label, pos=(0, 2),
                       flag=wx.ALIGN_CENTER_VERTICAL)
            vSizer.Add(self.cb_vcol, pos=(0, 3),
                       flag=wx.ALIGN_CENTER_VERTICAL)
            vSizer.Add(self.cb_vrgb_label, pos=(1, 2),
                      flag=wx.ALIGN_CENTER_VERTICAL)
            vSizer.Add(self.cb_vrgb, pos=(1, 3),
                       flag=wx.ALIGN_CENTER_VERTICAL)
            row += 1
            bodySizer.Add(item=vSizer, pos=(row, 0), span=(1, 3))
        
        row += 1
        bodySizer.Add(item=self.cr_panel, pos=(row, 0), span=(1, 2))
        
        bodySizer.Add(item=self.preview, pos=(row, 2),
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT, border=10)
        bodySizer.AddGrowableRow(row)
        bodySizer.AddGrowableCol(2)
        
        row += 1
        bodySizer.Add(item=self.numRules, pos=(row, 0),
                      flag=wx.ALIGN_CENTER_VERTICAL)
        
        bodySizer.Add(item=self.btnAdd, pos=(row, 1))
        bodySizer.Add(item=self.btnPreview, pos=(row, 2),
                      flag=wx.ALIGN_RIGHT)
        
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(self.helpbtn,
                     flag=wx.LEFT | wx.RIGHT, border=5)
        btnSizer.Add(self.btnCancel,
                     flag=wx.LEFT | wx.RIGHT, border=5)
        btnSizer.Add(self.btnApply,
                     flag=wx.LEFT | wx.RIGHT, border=5)
        btnSizer.Add(self.btnOK,
                     flag=wx.LEFT | wx.RIGHT, border=5)
        
        sizer.Add(item=self.inputSizer, proportion=0,
                  flag=wx.ALL | wx.EXPAND, border=5)
        
        sizer.Add(item=bodySizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=5)
        
        sizer.Add(item=wx.StaticLine(parent=self, id=wx.ID_ANY,
                                     style=wx.LI_HORIZONTAL),
                  proportion=0,
                  flag=wx.EXPAND | wx.ALL, border=5) 
        
        sizer.Add(item=btnSizer, proportion=0,
                  flag=wx.ALL | wx.ALIGN_RIGHT, border=5)
        
        self.SetSizer(sizer)
        sizer.Fit(self)
        self.Layout()
        
    def _colorRulesPanel(self):
        """!Create rules panel"""
        cr_panel = scrolled.ScrolledPanel(parent=self, id=wx.ID_ANY,
                                          size=(180, 300),
                                          style=wx.TAB_TRAVERSAL | wx.SUNKEN_BORDER)
        cr_panel.SetupScrolling(scroll_x = False)
        self.cr_sizer = wx.GridBagSizer(vgap=2, hgap=4)
        
        cr_panel.SetSizer(self.cr_sizer)
        cr_panel.SetAutoLayout(True)
        
        return cr_panel        

    def OnAddRules(self, event):
        """!Add rules button pressed"""
        nrules = self.numRules.GetValue()
        self.AddRules(nrules)
        
    def AddRules(self, nrules):
        """!Add rules"""
        snum = len(self.ruleslines.keys())
        for num in range(snum, snum + nrules):
            # enable
            enable = wx.CheckBox(parent=self.cr_panel, id=num)
            enable.SetValue(True)
            self.Bind(wx.EVT_CHECKBOX, self.OnRuleEnable, enable)
            # value
            txt_ctrl = wx.TextCtrl(parent=self.cr_panel, id=1000 + num,
                                   size=(90, -1),
                                   style=wx.TE_NOHIDESEL)
            self.Bind(wx.EVT_TEXT, self.OnRuleValue, txt_ctrl)
            # color
            color_ctrl = csel.ColourSelect(self.cr_panel, id=2000 + num,
                                           size = globalvar.DIALOG_COLOR_SIZE)
            self.Bind(csel.EVT_COLOURSELECT, self.OnRuleColor, color_ctrl)
            self.ruleslines[enable.GetId()] = { 'value' : '',
                                                'color': "0:0:0" }
            
            self.cr_sizer.Add(item=enable, pos=(num, 0),
                              flag=wx.ALIGN_CENTER_VERTICAL)
            self.cr_sizer.Add(item=txt_ctrl, pos=(num, 1),
                              flag=wx.ALIGN_CENTER | wx.RIGHT, border=5)
            self.cr_sizer.Add(item=color_ctrl, pos=(num, 2),
                              flag=wx.ALIGN_CENTER | wx.RIGHT, border=10)
        
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
        self.PreviewWindow.Draw(self.HistWindow.pdc, pdctype='clear')

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
        
        else:
            # initialize layer selection combobox
            self.cb_vlayer.InsertLayers(self.inmap)
            # initialize attribute table for layer=1
            layer = int(self.properties['layer'])
            self.properties['table'] = gselect.VectorDBInfo(self.inmap).layers[layer]['table']
            # initialize column selection comboboxes 
            self.cb_vcol.InsertColumns(vector=self.inmap, layer=layer)
            self.cb_vrgb.InsertColumns(vector=self.inmap, layer=layer)
            self.Update()
    
        self.btnPreview.Enable(True)
        self.btnOK.Enable(True)
        self.btnApply.Enable(True)
        
    def OnLayerSelection(self, event):
        # reset choices in column selection comboboxes if layer changes
        self.vlayer = int(event.GetString())
        self.vtable = gselect.VectorDBInfo(self.inmap).layers[str(self.vlayer)]
        self.cb_vcol.InsertColumns(vector=self.inmap, layer=self.vlayer)
        self.cb_vrgb.InsertColumns(vector=self.inmap, layer=self.vlayer)
        self.Update()
        
    def OnColumnSelection(self, event):
        self.properties['column'] = event.GetString()
    
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
                gcmd.GMessage(parent=self,
                              message=_("Please select attribute column "
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
                old_colrtable = grass.find_file(name=name, element='colr')['file']
            else:
                old_colrtable = grass.find_file(name=name, element='colr2/' + mapset)['file']
            
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
            self.layer = self.Map.AddLayer(type=ltype, name='preview', command=cmdlist,
                                           l_active=True, l_hidden=False, l_opacity=1.0,
                                           l_render=False) 
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
            cmd = 'vcolors'
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
            if rule['value'] not in ('nv', 'default') and \
                    rule['value'][-1] != '%' and \
                    not self._IsNumber(rule['value']):
                gcmd.GError(_("Invalid rule value '%s'. Unable to apply color table.") % rule['value'],
                            parent = self)
                return False
            
            if self.raster:
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
                 style=wx.NO_FULL_REPAINT_ON_RESIZE,
                 Map=None, **kwargs):
        
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

    def Draw(self, pdc, img=None, pdctype='image'):
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
    
    def UpdatePreview(self, img=None):
        """!Update canvas if window changes geometry"""
        Debug.msg (2, "BufferedWindow.UpdatePreview(%s): render=%s" % (img, self.render))
        oldfont = ""
        oldencoding = ""
        
        if self.render:
            # make sure that extents are updated
            self.Map.region = self.Map.GetRegion()
            self.Map.SetRegion()
            
            # render new map images
            self.mapfile = self.Map.Render(force=self.render)
            self.img = self.GetImage()
            self.resize = False
        
        if not self.img:
            return
        
        # paint images to PseudoDC
        self.pdc.Clear()
        self.pdc.RemoveAll()
        # draw map image background
        self.Draw(self.pdc, self.img, pdctype='image')
        
        self.resize = False
        
    def EraseMap(self):
        """!Erase preview"""
        self.Draw(self.pdc, pdctype='clear')
    
