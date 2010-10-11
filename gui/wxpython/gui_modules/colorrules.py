"""
@package colorrules.py

@brief Dialog for interactive management of raster color tables and
vector rgb_column

Classes:
 - ColorTable
 - BuferedWindow

(C) 2008, 2010 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton (Arizona State University)
@author Martin Landa <landa.martin gmail.com> (various updates)
"""

import os
import sys
import shutil

import wx
import wx.lib.colourselect as csel
import wx.lib.scrolledpanel as scrolled

from grass.script import core as grass

import dbm
import gcmd
import globalvar
import gselect
import render
import utils
from debug import Debug as Debug
from preferences import globalSettings as UserSettings

class ColorTable(wx.Frame):
    def __init__(self, parent, cmd, id=wx.ID_ANY, title = _("Set color table"),
                 style=wx.DEFAULT_FRAME_STYLE | wx.RESIZE_BORDER,
                 **kwargs):
        """!Dialog for interactively entering rules for map management
        commands

        @param cmd command (given as list)
        """
        self.parent = parent # GMFrame
        self.cmd    = cmd
        
        wx.Frame.__init__(self, parent, id, title, style = style, **kwargs)
        
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        
        # input map to change
        self.inmap = ''
        
        # raster properties
        self.rast = {
            # min cat in raster map
            'min' : None,
            # max cat in raster map
            'max' : None,
            }
        
        # vector properties
        self.vect = {
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
        
        if self.cmd == 'r.colors':
            self.SetTitle(_('Create new color table for raster map'))
            self.elem = 'cell'
            crlabel = _('Enter raster cat values or percents')
        elif self.cmd == 'vcolors':
            self.SetTitle(_('Create new color table for vector map'))
            self.elem = 'vector'
            crlabel = _('Enter vector attribute values or ranges (n or n1 to n2)')
        
        # top controls
        if self.cmd == 'r.colors':
            maplabel = _('Select raster map:')
        elif self.cmd == 'vcolors':
            maplabel = _('Select vector map:')
        inputBox = wx.StaticBox(parent=self, id=wx.ID_ANY,
                                label=" %s " % maplabel)
        self.inputSizer = wx.StaticBoxSizer(inputBox, wx.VERTICAL)
        self.selectionInput = gselect.Select(parent=self, id=wx.ID_ANY,
                                             size=globalvar.DIALOG_GSELECT_SIZE,
                                             type=self.elem)
        
        self.ovrwrtcheck = wx.CheckBox(parent=self, id=wx.ID_ANY,
                                       label=_('replace existing color table'))
        self.ovrwrtcheck.SetValue(UserSettings.Get(group='cmd', key='overwrite', subkey='enabled'))
        self.helpbtn = wx.Button(parent=self, id=wx.ID_HELP)

        if self.elem == 'vector':
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
        self.cr_panel = self.__colorrulesPanel()
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
        
        # bindings
        self.Bind(wx.EVT_BUTTON, self.OnHelp, self.helpbtn)
        self.selectionInput.Bind(wx.EVT_TEXT, self.OnSelectionInput)
        self.Bind(wx.EVT_BUTTON, self.OnCancel, self.btnCancel)
        self.Bind(wx.EVT_BUTTON, self.OnApply, self.btnApply)
        self.Bind(wx.EVT_BUTTON, self.OnOK, self.btnOK)
        self.Bind(wx.EVT_BUTTON, self.OnPreview, self.btnPreview)
        self.Bind(wx.EVT_BUTTON, self.OnAddRules, self.btnAdd)
        self.Bind(wx.EVT_CLOSE,  self.OnCloseWindow)

        # additional bindings for vector color management
        if self.cmd == 'vcolors':
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
            if (type == 'raster' and self.elem == 'cell') or \
                    (type == 'vector' and self.elem == 'vector'):
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
                         flag=wx.ALL | wx.EXPAND, border=1)
        replaceSizer.Add(item=self.helpbtn, proportion=0,
                         flag=wx.ALIGN_RIGHT | wx.ALL, border=1)

        self.inputSizer.Add(item=replaceSizer, proportion=1,
                       flag=wx.ALL | wx.EXPAND, border=1)

        #
        # body & preview
        #
        bodySizer =  wx.GridBagSizer(hgap=5, vgap=5)
        row = 0
        bodySizer.Add(item=self.cr_label, pos=(row, 0), span=(1, 3),
                      flag=wx.ALL, border=5)

        if self.cmd == 'vcolors':
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
        
    def __colorrulesPanel(self):
        cr_panel = scrolled.ScrolledPanel(parent=self, id=wx.ID_ANY,
                                          size=(180, 300),
                                          style=wx.TAB_TRAVERSAL | wx.SUNKEN_BORDER)
        
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
        for num in range(snum, snum+nrules):
            # enable
            enable = wx.CheckBox(parent=self.cr_panel, id=num)
            enable.SetValue(True)
            self.Bind(wx.EVT_CHECKBOX, self.OnRuleEnable, enable)
            # value
            txt_ctrl = wx.TextCtrl(parent=self.cr_panel, id=1000+num, value='',
                                   size=(100,-1),
                                   style=wx.TE_NOHIDESEL)
            self.Bind(wx.EVT_TEXT, self.OnRuleValue, txt_ctrl)
            # color
            color_ctrl = csel.ColourSelect(self.cr_panel, id=2000+num)
            self.Bind(csel.EVT_COLOURSELECT, self.OnRuleColor, color_ctrl)
            self.ruleslines[enable.GetId()] = { 'value' : '',
                                                'color': "0:0:0" }
            
            self.cr_sizer.Add(item=enable, pos=(num, 0),
                              flag=wx.ALIGN_CENTER_VERTICAL)
            self.cr_sizer.Add(item=txt_ctrl, pos=(num, 1),
                              flag=wx.ALIGN_CENTER | wx.RIGHT, border=5)
            self.cr_sizer.Add(item=color_ctrl, pos=(num, 2),
                              flag=wx.ALIGN_CENTER | wx.RIGHT, border=5)
        
        self.cr_panel.Layout()
        self.cr_panel.SetupScrolling()
        
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
        if event:
            self.inmap = event.GetString()
        
        if self.inmap == '':
            self.btnPreview.Enable(False)
            self.btnOK.Enable(False)
            self.btnApply.Enable(False)
            return
        
        if self.elem == 'cell':
            info = gcmd.RunCommand('r.info',
                                   parent = self,
                                   read = True,
                                   flags = 'r',
                                   map = self.inmap)

            if info:
                for line in info.splitlines():
                    if 'min' in line:
                        self.rast['min'] = float(line.split('=')[1])
                    elif 'max' in line:
                        self.rast['max'] = float(line.split('=')[1])
            else:
                self.inmap = ''
                self.rast['min'] = self.rast['max'] = None
                self.btnPreview.Enable(False)
                self.btnOK.Enable(False)
                self.btnApply.Enable(False)
                self.preview.EraseMap()
                self.cr_label.SetLabel(_('Enter raster cat values or percents'))
                return
            
            self.cr_label.SetLabel(_('Enter raster cat values or percents (range = %(min)d-%(max)d)') %
                                     { 'min' : self.rast['min'],
                                       'max' : self.rast['max'] })
        elif self.elem == 'vector':
            # initialize layer selection combobox
            self.cb_vlayer.InsertLayers(self.inmap)
            # initialize attribute table for layer=1
            layer = int(self.vect['layer'])
            self.vect['table'] = gselect.VectorDBInfo(self.inmap).layers[layer]['table']
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
        self.vect['column'] = event.GetString()
    
    def OnRGBColSelection(self, event):
        self.vect['rgb'] = event.GetString()
        
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
        
        if self.elem == 'cell':

            try:
                if vals != '-' and \
                        vals[-1] != '%':
                    float(vals)
            except (IndexError, ValueError):
                tc.SetValue('')
                self.ruleslines[num-1000]['value'] = ''
                return
            
            self.ruleslines[num-1000]['value'] = vals
            
        elif self.elem == 'vector':
            if self.vect['column'] == '' or self.vect['rgb'] == '':
                tc.SetValue('')
                wx.MessageBox(parent=self,
                              message=_("Please select attribute column "
                                        "and RGB color column first"),
                              style=wx.CENTRE)
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
            sqlrule = '%s=%s' % (self.vect['column'], valslist[0])
        elif len(valslist) > 1:
            sqlrule = '%s>=%s AND %s<=%s' % (self.vect['column'], valslist[0],
                                             self.vect['column'], valslist[1])
        else:
            return None
        
        return sqlrule
        
    def OnApply(self, event):
        self.CreateColorTable()
        display = self.parent.GetLayerTree().GetMapDisplay()
        if display:
            display.GetWindow().UpdateMap(render = True)
        
    def OnOK(self, event):
        self.OnApply(event)
        self.Destroy()
    
    def OnCancel(self, event):
        self.Destroy()
        
    def OnPreview(self, event):
        """!Update preview"""
        # raster
        if self.elem == 'cell':
            cmdlist = ['d.rast',
                       'map=%s' % self.inmap]
            ltype = 'raster'
            
            # find existing color table and copy to temp file
            try:
                name, mapset = self.inmap.split('@')
                old_colrtable = grass.find_file(name=name, element='colr2/' + mapset)['file']
            except (TypeError, ValueError):
                old_colrtable = None
            
            if old_colrtable:
                colrtemp = utils.GetTempfile()
                shutil.copyfile(old_colrtable, colrtemp)
        # vector
        elif self.elem == 'vector':
            cmdlist = ['d.vect',
                        '-a',
                       'map=%s' % self.inmap,
                       'rgb_column=%s' % self.vect["rgb"],
                       'type=point,line,boundary,area']
            ltype = 'vector'
        else:
            return
        
        if not self.layer:
            self.layer = self.Map.AddLayer(type=ltype, name='preview', command=cmdlist,
                                           l_active=True, l_hidden=False, l_opacity=1.0,
                                           l_render=False) 
        else:
            self.layer.SetCmd(cmdlist)
        
        # apply new color table and display preview
        self.CreateColorTable(force=True)
        self.preview.UpdatePreview()
        
        # restore previous color table
        if self.elem == 'cell':
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
        gcmd.RunCommand('g.manual',
                        quiet = True,
                        parent = self,
                        entry = self.cmd)
        
    def CreateColorTable(self, force=False):
        """!Creates color table"""
        rulestxt = ''
        
        for rule in self.ruleslines.itervalues():
            if not rule['value']: # skip empty rules
                continue
            
            if self.elem == 'cell':
                rulestxt += rule['value'] + ' ' + rule['color'] + '\n'
            elif self.elem == 'vector':
                rulestxt += "UPDATE %s SET %s='%s' WHERE %s ;\n" % (self.vect['table'],
                                                                    self.vect['rgb'],
                                                                    rule['color'],
                                                                    rule['value'])
        if rulestxt == '':
            return
        
        gtemp = utils.GetTempfile()
        output = open(gtemp, "w")
        try:
            output.write(rulestxt)
        finally:
            output.close()
        
        if self.elem == 'cell': 
            if not force and \
                    not self.ovrwrtcheck.IsChecked():
                flags = 'w'
            else:
                flags = ''
        
            gcmd.RunCommand('r.colors',
                            parent = self,
                            flags = flags,
                            map = self.inmap,
                            rules = gtemp)
            
        elif self.elem == 'vector':
            gcmd.RunCommand('db.execute',
                            parent = self,
                            input = gtemp)
        
class BufferedWindow(wx.Window):
    """!A Buffered window class"""
    def __init__(self, parent, id,
                 pos = wx.DefaultPosition,
                 size = wx.DefaultSize,
                 style=wx.NO_FULL_REPAINT_ON_RESIZE,
                 Map=None):

        wx.Window.__init__(self, parent, id, pos, size, style)

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
    
