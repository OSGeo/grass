"""
MODULE:     rules.py

CLASSES:
    * RulesText

PURPOSE:    Dialog for interactive entry of rules for r.colors,
            r.reclass, r.recode, and v.reclass

AUTHORS:    The GRASS Development Team
            Michael Barton (Arizona State University)

COPYRIGHT:  (C) 2007 by the GRASS Development Team
            This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.

"""

import os
import sys
import shutil
from debug import Debug as Debug

import wx
import wx.lib.colourselect as  csel
import wx.lib.scrolledpanel as scrolled

import dbm
import gcmd
import globalvar
import gselect
import render
import utils

class ColorTable(wx.Frame):
    def __init__(self, parent, id=wx.ID_ANY, title='',
                 pos=wx.DefaultPosition, size=(-1, -1),
                 style=wx.DEFAULT_FRAME_STYLE|wx.RESIZE_BORDER,
                 **kwargs):
        wx.Frame.__init__(self, parent, id, title, pos, size, style)

        """
        Dialog for interactively entering rules
        for map management commands

        @param cmd command (given as list)
        """
        
        self.CentreOnParent()
        self.parent = parent
        self.cmd = kwargs['cmd'] # grass command
        self.inmap = '' # input map to change
        self.rastmin = '' # min cat in raster map
        self.rastmax = '' # max cat in raster map
        self.layerchoices = ['1'] # list of database layers for vector (minimum of 1)
        self.columnchoices = [] # list of database columns for vector
        self.vlayer = 1 # vector layer for attribute table to use for setting color
        self.vtable = '' # vector attribute table used for setting color
        self.vcolumn = '' # vector attribute column for assigning colors
        self.vrgb = '' # vector attribute column to use for storing colors
        self.old_colrtable = '' # existing color table of raster map
        self.vals = '' # raster category or vector attribute values for assigning colors
        self.rgb_string = '' # r:g:b color string for assigning colors to cats
        self.ruleslines = {} # rules for creating colortable
        self.overwrite = True
        self.Map   = render.Map()  # instance of render.Map to be associated with display
        self.layer = None          # reference to layer with preview
        self.mapname = ''
                
        if self.cmd == 'r.colors':
            title = 'Create new color table for raster map'
            maplabel = 'Select raster map:'
            elem = 'cell'
            crlabel = 'Enter raster cat values or percents'
        elif self.cmd == 'vcolors':
            title = 'Create new color table for vector map'
            maplabel = 'Select vector map:'
            elem = 'vector'
            crlabel = 'Enter vector attribute values or ranges (n or n1 to n2)'

        #
        # Set the size & cursor
        #
        self.SetClientSize(size)

        # set window frame title
        self.SetTitle(title)
        
        # top controls
        self.map_label = wx.StaticText(parent=self, id=wx.ID_ANY, label=maplabel)
        self.selectionInput = gselect.Select(parent=self, id=wx.ID_ANY,
                                             size=globalvar.DIALOG_GSELECT_SIZE,
                                             type=elem)
        
        self.ovrwrtcheck = wx.CheckBox(parent=self, id=wx.ID_ANY,
                                   label=_('replace existing color table'))
        self.ovrwrtcheck.SetValue(self.overwrite)
        self.helpbtn = wx.Button(parent=self, id=wx.ID_HELP)

        if self.cmd == 'vcolors':
            self.cb_vl_label = wx.StaticText(parent=self, id=wx.ID_ANY,
                                   label='Layer:')
            self.cb_vlayer = wx.ComboBox(parent=self, id=wx.ID_ANY, size=(50, -1),
                                 style=wx.CB_SIMPLE | wx.CB_READONLY,
                                 choices=self.layerchoices)
            self.cb_vlayer.SetSelection(0)
            self.cb_vlayer.SetValue('1')
            self.cb_vc_label = wx.StaticText(parent=self, id=wx.ID_ANY,
                                   label='Attribute column:')
            self.cb_vcol = wx.ComboBox(parent=self, id=wx.ID_ANY, size=(200, -1),
                                 style=wx.CB_SIMPLE | wx.CB_READONLY,
                                 choices=self.columnchoices)
            self.cb_vrgb_label = wx.StaticText(parent=self, id=wx.ID_ANY,
                                   label='RGB color column:')
            self.cb_vrgb = wx.ComboBox(parent=self, id=wx.ID_ANY, size=(200, -1),
                                 style=wx.CB_SIMPLE | wx.CB_READONLY,
                                 choices=self.columnchoices)

        # color table and preview window
        self.cr_label = wx.StaticText(parent=self, id=wx.ID_ANY,
                    label=crlabel)
        self.cr_panel = self.__colorrulesPanel()
        self.InitDisplay() # initialize preview display
        self.preview = BufferedWindow(self, id = wx.ID_ANY, size=(400,300), Map=self.Map)
        
        # bottom controls        
        self.line = wx.StaticLine(parent=self, id=wx.ID_ANY, size=(-1,-1),
                                  style=wx.LI_HORIZONTAL)

        cancel_btn = wx.Button(self, wx.ID_CANCEL)
        apply_btn = wx.Button(self, wx.ID_APPLY) 
        ok_btn = wx.Button(self, wx.ID_OK)
        ok_btn.SetDefault()
        
        self.btnsizer = wx.StdDialogButtonSizer()
        self.btnsizer.Add(cancel_btn, flag=wx.ALL, border=5)
        self.btnsizer.Add(apply_btn, flag=wx.ALL, border=5)
        self.btnsizer.Add(ok_btn, flag=wx.ALL, border=5)
        self.btnsizer.Realize()
        
        self.preview_btn = wx.Button(self, wx.ID_ANY, _("Preview"))
        
        # bindings
        self.Bind(wx.EVT_BUTTON, self.OnHelp, self.helpbtn)
        self.Bind(wx.EVT_CHECKBOX, self.OnOverwrite,   self.ovrwrtcheck)
        self.selectionInput.Bind(wx.EVT_TEXT, self.OnSelectionInput)
        self.Bind(wx.EVT_BUTTON, self.OnCancel, cancel_btn)
        self.Bind(wx.EVT_BUTTON, self.OnApply, apply_btn)
        self.Bind(wx.EVT_BUTTON, self.OnOK, ok_btn)
        self.Bind(wx.EVT_BUTTON, self.OnPreview, self.preview_btn)
        self.Bind(wx.EVT_CLOSE,  self.OnCloseWindow)

        # additional bindings for vector color management
        if self.cmd == 'vcolors':
            self.Bind(wx.EVT_COMBOBOX, self.OnLayerSelection, self.cb_vlayer)
            self.Bind(wx.EVT_COMBOBOX, self.OnColumnSelection, self.cb_vcol)
            self.Bind(wx.EVT_COMBOBOX, self.OnRGBColSelection, self.cb_vrgb)


        # layout
        self.__doLayout()
        self.Show()
        
    def __doLayout(self):
        sizer =  wx.GridBagSizer(hgap=5, vgap=5)
        sizer.AddGrowableCol(2)

        row = 0
        sizer.Add(self.map_label, pos=(row,0),
                  flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND|wx.ALL, border=5)
        row += 1 # row 1
        sizer.Add(item=self.selectionInput, pos=(row,0), span=(1,3),
                  flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND | wx.ALL, border=5)
        row += 1 # row 2
        sizer.Add(item=self.ovrwrtcheck, pos=(row, 0), span=(1,2),
                  flag=wx.ALL, border=5)        
        sizer.Add(item=self.helpbtn, pos=(row,2), flag=wx.ALIGN_RIGHT|wx.ALL, border=5)
        if self.cmd == 'vcolors':
            row += 1
            sizer.Add(self.cb_vl_label, pos=(row,0),
                      flag=wx.ALIGN_CENTER_VERTICAL|wx.LEFT, border=10)
            sizer.Add(self.cb_vc_label, pos=(row,1),
                      flag=wx.ALIGN_CENTER_VERTICAL|wx.LEFT, border=10)
            sizer.Add(self.cb_vrgb_label, pos=(row,2),
                      flag=wx.ALIGN_CENTER_VERTICAL|wx.LEFT, border=10)
            row += 1            
            sizer.Add(self.cb_vlayer, pos=(row,0),
                      flag=wx.ALIGN_CENTER_VERTICAL|wx.LEFT, border=5)
            sizer.Add(self.cb_vcol, pos=(row,1),
                      flag=wx.ALIGN_CENTER_VERTICAL|wx.LEFT, border=5)
            sizer.Add(self.cb_vrgb, pos=(row,2),
                      flag=wx.ALIGN_CENTER_VERTICAL|wx.LEFT, border=5)

        row += 1 # row 3
        sizer.Add(item=self.cr_label, pos=(row,0), span=(1,2), flag=wx.ALL, border=5)
        row += 1 # row 4
        sizer.AddGrowableRow(row)
        sizer.Add(item=self.cr_panel, pos=(row,0),
                  flag=wx.EXPAND|wx.LEFT|wx.RIGHT, border=10)        
        sizer.Add(item=self.preview, pos=(row,1), span=(1,2),
                  flag=wx.ALIGN_LEFT|wx.EXPAND|wx.LEFT|wx.RIGHT, border=10)
        row += 1 # row 5
        sizer.Add(item=self.line, pos=(row,0), span=(1,3),
                  flag=wx.GROW|wx.EXPAND|wx.ALIGN_CENTER_VERTICAL|
                  wx.TOP|wx.BOTTOM, border=5)
        row += 1 # row 6
        sizer.Add(self.btnsizer, pos=(row,0), span=(1,2),
                  flag=wx.ALIGN_CENTER|wx.FIXED_MINSIZE)
        sizer.Add(self.preview_btn, pos=(row,2),
                  flag=wx.ALIGN_BOTTOM|wx.ALIGN_CENTER|wx.FIXED_MINSIZE|wx.ALL, border=5)

        self.SetSizer(sizer)
        sizer.Fit(self)
                    
    def __colorrulesPanel(self):
        cr_panel = scrolled.ScrolledPanel(self, -1, size=(180,300),
                                          style=wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER,
                                          name="cr_panel" )
        cr_sizer = wx.GridBagSizer(vgap=2, hgap=4)

        for num in range(100):
            txt_ctrl = wx.TextCtrl(parent=cr_panel, id=num, value='',
                                   pos=wx.DefaultPosition, size=(100,-1),
                                   style=wx.TE_NOHIDESEL)
            self.Bind(wx.EVT_TEXT, self.OnVals, txt_ctrl)
            color_ctrl = csel.ColourSelect(cr_panel, id=num)
            self.Bind(csel.EVT_COLOURSELECT, self.OnSelectColor, color_ctrl)
            self.ruleslines[num] = ["","0:0:0"]

            cr_sizer.Add(item=txt_ctrl, pos=(num,0),
                         flag=wx.ALIGN_CENTER|wx.LEFT, border=10)
            cr_sizer.Add(item=color_ctrl, pos=(num,1),
                         flag=wx.ALIGN_CENTER|wx.RIGHT, border=10)

        cr_panel.SetSizer(cr_sizer)
        cr_panel.SetAutoLayout(1)
        cr_panel.SetupScrolling()
        
        return cr_panel        

    def InitDisplay(self):
        """
        Initialize preview display, set dimensions and region
        """
        #self.width, self.height = self.GetClientSize()
        self.width = self.Map.width = 400
        self.height = self.Map.height = 300
        self.Map.geom = self.width, self.height

    def OnErase(self, event):
        """
        Erase the histogram display
        """
        self.PreviewWindow.Draw(self.HistWindow.pdc, pdctype='clear')

    def OnCloseWindow(self, event):
        """
        Window closed
        Also remove associated rendered images
        """
        #try:
        #    self.propwin.Close(True)
        #except:
        #    pass
        self.Map.Clean()
        self.Destroy()
        
    def OnSelectionInput(self, event):
        self.inmap = event.GetString()
        if self.inmap == '': return
        
        if self.cmd == 'r.colors':
            try:
                cmdlist = ['r.info', 'map=%s' % self.inmap, '-r']
                
                p = gcmd.Command(cmdlist)
                output = p.ReadStdOutput()
                for line in output:
                    line = line.strip('\n ')
                    if 'min' in line:
                        self.rastmin = line.split('=')[1].strip('\n ')
                    if 'max' in line:
                        self.rastmax = line.split('=')[1].strip('\n ')
                
                self.cr_label.SetLabel('Enter raster cat values or percents (range = %s-%s)' %
                                         (self.rastmin,self.rastmax))
            except:
                pass
        elif self.cmd == 'vcolors':
            self.mapDBInfo = dbm.VectorDBInfo(self.inmap)
            self.layerchoices = self.mapDBInfo.layers.keys()
            for n in range(len(self.layerchoices)):
                self.cb_vlayer.Insert(str(self.layerchoices[n]), n)

            # initialize column selection combox boxes for layer 1
            try:
                self.vtable = self.mapDBInfo.layers[self.vlayer]['table']
                for n in range(len(self.mapDBInfo.GetColumns(self.vtable))):
                    self.cb_vcol.Insert(self.mapDBInfo.GetColumns(self.vtable)[n], n)
                    self.cb_vrgb.Insert(self.mapDBInfo.GetColumns(self.vtable)[n], n)
                self.Update()
            except:
                pass
                
    def OnLayerSelection(self, event):
        self.vlayer = int(event.GetString())
        
        # reset choices in column selection comboboxes if layer changes
        self.vtable = self.mapDBInfo.layers[self.vlayer]['table']
        for n in range(len(self.mapDBInfo.GetColumns(self.vtable))):
            self.cb_vcol.Insert(self.mapDBInfo.GetColumns(self.vtable)[n], n)
            self.cb_vrgb.Insert(self.mapDBInfo.GetColumns(self.vtable)[n], n)
        self.Update()
        
    def OnColumnSelection(self, event):
        self.vcolumn = event.GetString()
    
    def OnRGBColSelection(self, event):
        self.vrgb = event.GetString()
                        
    def OnVals(self, event):
        num = event.GetId()
        tc = self.FindWindowById(num)
        vals = event.GetString().strip()
        if vals == '': return
        if self.cmd == 'r.colors':
            self.ruleslines[num][0] = vals
        elif self.cmd == 'vcolors':
            if self.vcolumn == '' or self.vrgb == '':
                tc.SetValue('')
                wx.MessageBox("Please select attribute column and RGB color column first")
            else:
                self.ruleslines[num][0] = self.SQLConvert(vals)

    def OnSelectColor(self, event):
        num = event.GetId()
        rgba_color = event.GetValue()
        rgb_string = str(rgba_color[0]) + ':' + str(rgba_color[1]) + ':' + str(rgba_color[2])
        self.ruleslines[num][1] = rgb_string
        
    def SQLConvert(self, vals):
        valslist = []
        valslist = vals.split('to')
        try:
            if len(valslist) == 1:
                sqlrule = '%s=%s' % (self.vcolumn, float(valslist[0]))
            elif len(valslist) > 1:
                sqlrule = '%s>=%s AND %s<=%s' % (self.vcolumn, float(valslist[0]),
                                                 self.vcolumn, float(valslist[1]))
            else:
                return ""
        except:
            return ""
        
        return sqlrule
        
    def OnApply(self, event):
        self.CreateColorTable()
    
    def OnOK(self, event):
        self.CreateColorTable()
        self.Destroy()
    
    def OnCancel(self, event):
        self.Destroy()
        
    def OnPreview(self, event):
        # Add layer to the map for preview
        #
        
        if self.cmd == 'r.colors':
            cmdlist = ['d.rast', 'map=%s' % self.inmap]
            # Find existing color table and copy to temp file
            p = gcmd.Command(['g.findfile', 'element=colr', 'file=%s' % self.inmap])
            output = p.ReadStdOutput()
            for line in output:
                if 'file=' in line:
                    old_colrtable = line.split('=')[1].strip("'")
            try:
                colrtemp = utils.GetTempfile()
                shutil.copyfile(old_colrtable,colrtemp)
            except:
                return
        elif self.cmd == 'vcolors':
            cmdlist = ['d.vect', '-a', 'map=%s' % self.inmap, 'rgb_column=%s' % self.vrgb,
                       'type=point,line,boundary,area,face']
        else:
            return
        
        self.layer = self.Map.AddLayer(type="command", name='raster', command=cmdlist,
                                       l_active=True, l_hidden=False, l_opacity=1, l_render=False)        
        
        # apply new color table and display preview
        self.CreateColorTable()
        self.preview.UpdatePreview()
        
        if self.cmd == 'r.colors':
            shutil.copyfile(colrtemp, old_colrtable)
            try:
                os.remove(colrtemp)
            except:
                pass

    def OnHelp(self, event):
        gcmd.Command(['g.manual',
                      '--quiet', 
                      '%s' % self.cmd[0]])

    def OnOverwrite(self, event):
        self.overwrite = event.IsChecked()
        
    def CreateColorTable(self):
        rulestxt = ''
        
        for num in self.ruleslines:
            if self.ruleslines[num][0] != "":
                if self.cmd == 'r.colors':
                    rulestxt += self.ruleslines[num][0] + ' ' + self.ruleslines[num][1] + '\n'
                elif self.cmd == 'vcolors':
                    rulestxt += "UPDATE %s SET %s='%s' WHERE %s ;\n" % (self.vtable,
                        self.vrgb,self.ruleslines[num][1], self.ruleslines[num][0])
                
        if rulestxt == '': return

        gtemp = utils.GetTempfile()
        output = open(gtemp, "w")
        try:
            output.write(rulestxt)
        finally:
            output.close()
        
        if self.cmd == 'r.colors':        
            cmdlist = ['r.colors', 'map=%s' % self.inmap, 'rules=%s' % gtemp]
            
            if self.overwrite == False:
                cmdlist.append('-w')
        elif self.cmd == 'vcolors':
            cmdlist = ['db.execute', 'input=%s' % gtemp]
                
        try:
            p = gcmd.Command(cmdlist)
            output = p.ReadStdOutput()
            error = p.ReadErrOutput()
        except:
            pass
        
class BufferedWindow(wx.Window):
    """
    A Buffered window class.

    When the drawing needs to change, you app needs to call the
    UpdateHist() method. Since the drawing is stored in a bitmap, you
    can also save the drawing to file by calling the
    SaveToFile(self,file_name,file_type) method.
    """

    def __init__(self, parent, id,
                 pos = wx.DefaultPosition,
                 size = wx.DefaultSize,
                 style=wx.NO_FULL_REPAINT_ON_RESIZE,
                 Map=None):

        wx.Window.__init__(self, parent, id, pos, size, style)

        self.parent = parent
        self.Map = Map
        self.mapname = self.parent.mapname

        #
        # Flags
        #
        self.render = True  # re-render the map from GRASS or just redraw image
        self.resize = False # indicates whether or not a resize event has taken place
        self.dragimg = None # initialize variable for map panning
        self.pen = None     # pen for drawing zoom boxes, etc.

        #
        # Event bindings
        #
        self.Bind(wx.EVT_PAINT,        self.OnPaint)
        #self.Bind(wx.EVT_SIZE,         self.OnSize)
        self.Bind(wx.EVT_IDLE,         self.OnIdle)

        #
        # Render output objects
        #
        self.mapfile = None # image file to be rendered
        self.img = ""       # wx.Image object (self.mapfile)

        self.imagedict = {} # images and their PseudoDC ID's for painting and dragging

        self.pdc = wx.PseudoDC()
        self._Buffer = '' # will store an off screen empty bitmap for saving to file

        # make sure that extents are updated at init
        self.Map.region = self.Map.GetRegion()
        self.Map.SetRegion() 

        self.Bind(wx.EVT_ERASE_BACKGROUND, lambda x:None)

    def Draw(self, pdc, img=None, drawid=None, pdctype='image', coords=[0,0,0,0]):
        """
        Draws histogram or clears window
        """

        if drawid == None:
            if pdctype == 'image' :
                drawid = imagedict[img]
            elif pdctype == 'clear':
                drawid == None
            else:
                drawid = wx.NewId()
        else:
            pdc.SetId(drawid)

        pdc.BeginDrawing()

        Debug.msg (3, "BufferedWindow.Draw(): id=%s, pdctype=%s, coord=%s" % (drawid, pdctype, coords))

        if pdctype == 'clear': # erase the display
            bg = wx.WHITE_BRUSH
            pdc.SetBackground(bg)
            pdc.Clear()
            self.Refresh()
            pdc.EndDrawing()
            return

        if pdctype == 'image':
            bg = wx.TRANSPARENT_BRUSH
            pdc.SetBackground(bg)
            bitmap = wx.BitmapFromImage(img)
            w,h = bitmap.GetSize()
            pdc.DrawBitmap(bitmap, coords[0], coords[1], True) # draw the composite map
            pdc.SetIdBounds(drawid, (coords[0],coords[1],w,h))

        pdc.EndDrawing()
        self.Refresh()

    def OnPaint(self, event):
        """
        Draw psuedo DC to buffer
        """
    
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
        self.pdc.DrawToDCClipped(dc,r)

        #self.pdc.DrawToDC(dc)

    def OnSize(self, event):
        """
         Init image size to match window size
        """

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
        """
        Only re-render a histogram image from GRASS during
        idle time instead of multiple times during resizing.
            """

        if self.resize:
            self.render = True
            self.UpdatePreview()
        event.Skip()

    def GetImage(self):
        """
        Converts files to wx.Image
        """
        if self.Map.mapfile and os.path.isfile(self.Map.mapfile) and \
                os.path.getsize(self.Map.mapfile):
            img = wx.Image(self.Map.mapfile, wx.BITMAP_TYPE_ANY)
        else:
            img = None

        self.imagedict[img] = 99 # set image PeudoDC ID
        return img


    def UpdatePreview(self, img=None):
        """
        Update canvas if window changes geometry
        """

        Debug.msg (2, "BufferedWindow.UpdatePreview(%s): render=%s" % (img, self.render))
        oldfont = ""
        oldencoding = ""

        if self.render:
            # render new map images
            self.mapfile = self.Map.Render(force=self.render)
            self.img = self.GetImage()
            self.resize = False

        if not self.img: return
        try:
            id = self.imagedict[self.img]
        except:
            return

        # paint images to PseudoDC
        self.pdc.Clear()
        self.pdc.RemoveAll()
        self.Draw(self.pdc, self.img, drawid=id) # draw map image background

        self.resize = False

        # update statusbar
        # Debug.msg (3, "BufferedWindow.UpdateHist(%s): region=%s" % self.Map.region)
        self.Map.SetRegion()

    def EraseMap(self):
        """
        Erase the map display
        """
        self.Draw(self.pdc, pdctype='clear')

