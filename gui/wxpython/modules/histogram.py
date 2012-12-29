"""!
@package modules.histogram

Plotting histogram based on d.histogram

Classes:
 - histogram::BufferedWindow
 - histogram::HistogramFrame
 - histogram::HistogramToolbar

(C) 2007, 2010-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Various updates by Martin Landa <landa.martin gmail.com>
"""

import os

import wx

from core                 import globalvar
from core.render          import Map
from gui_core.forms       import GUI
from mapdisp.gprint       import PrintOptions
from core.utils           import GetLayerNameFromCmd
from gui_core.dialogs     import GetImageHandlers, ImageSizeDialog
from gui_core.preferences import DefaultFontDialog
from core.debug           import Debug
from core.gcmd            import GError
from gui_core.toolbars    import BaseToolbar, BaseIcons

class BufferedWindow(wx.Window):
    """!A Buffered window class.

    When the drawing needs to change, you app needs to call the
    UpdateHist() method. Since the drawing is stored in a bitmap, you
    can also save the drawing to file by calling the
    SaveToFile(self,file_name,file_type) method.
    """
    def __init__(self, parent, id = wx.ID_ANY,
                 style = wx.NO_FULL_REPAINT_ON_RESIZE,
                 Map = None, **kwargs):
        
        wx.Window.__init__(self, parent, id = id, style = style, **kwargs)
        
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
        self.Bind(wx.EVT_SIZE,         self.OnSize)
        self.Bind(wx.EVT_IDLE,         self.OnIdle)
        
        #
        # Render output objects
        #
        self.mapfile = None # image file to be rendered
        self.img = ""       # wx.Image object (self.mapfile)
        
        self.imagedict = {} # images and their PseudoDC ID's for painting and dragging
        
        self.pdc = wx.PseudoDC()
        self._buffer = '' # will store an off screen empty bitmap for saving to file
        
        # make sure that extents are updated at init
        self.Map.region = self.Map.GetRegion()
        self.Map.SetRegion() 
        
        self.Bind(wx.EVT_ERASE_BACKGROUND, lambda x:None)
        
    def Draw(self, pdc, img = None, drawid = None, pdctype = 'image', coords = [0,0,0,0]):
        """!Draws histogram or clears window
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
        """!Draw psuedo DC to buffer
        """
        dc = wx.BufferedPaintDC(self, self._buffer)
        
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
        
    def OnSize(self, event):
        """!Init image size to match window size
        """
        # set size of the input image
        self.Map.width, self.Map.height = self.GetClientSize()
        
        # Make new off screen bitmap: this bitmap will always have the
        # current drawing in it, so it can be used to save the image to
        # a file, or whatever.
        self._buffer = wx.EmptyBitmap(self.Map.width, self.Map.height)
        
        # get the image to be rendered
        self.img = self.GetImage()
        
        # update map display
        if self.img and self.Map.width + self.Map.height > 0: # scale image during resize
            self.img = self.img.Scale(self.Map.width, self.Map.height)
            self.render = False
            self.UpdateHist()
        
        # re-render image on idle
        self.resize = True
        
    def OnIdle(self, event):
        """!Only re-render a histogram image from GRASS during idle
        time instead of multiple times during resizing.
        """
        if self.resize:
            self.render = True
            self.UpdateHist()
        event.Skip()
        
    def SaveToFile(self, FileName, FileType, width, height):
        """!This will save the contents of the buffer to the specified
        file. See the wx.Windows docs for wx.Bitmap::SaveFile for the
        details
        """
        busy = wx.BusyInfo(message=_("Please wait, exporting image..."),
                           parent=self)
        wx.Yield()
        
        self.Map.ChangeMapSize((width, height))
        ibuffer = wx.EmptyBitmap(max(1, width), max(1, height))
        self.Map.Render(force=True, windres = True)
        img = self.GetImage()
        self.Draw(self.pdc, img, drawid = 99)
        dc = wx.BufferedPaintDC(self, ibuffer)
        dc.Clear()
        self.PrepareDC(dc)
        self.pdc.DrawToDC(dc)
        ibuffer.SaveFile(FileName, FileType)
        
        busy.Destroy()
        
    def GetImage(self):
        """!Converts files to wx.Image
        """
        if self.Map.mapfile and os.path.isfile(self.Map.mapfile) and \
                os.path.getsize(self.Map.mapfile):
            img = wx.Image(self.Map.mapfile, wx.BITMAP_TYPE_ANY)
        else:
            img = None
        
        self.imagedict[img] = 99 # set image PeudoDC ID
        return img
    
    def UpdateHist(self, img = None):
        """!Update canvas if histogram options changes or window
        changes geometry
        """
        Debug.msg (2, "BufferedWindow.UpdateHist(%s): render=%s" % (img, self.render))
        oldfont = ""
        oldencoding = ""
        
        if self.render:
            # render new map images
            # set default font and encoding environmental variables
            if "GRASS_FONT" in os.environ:
                oldfont = os.environ["GRASS_FONT"]
            if self.parent.font != "": os.environ["GRASS_FONT"] = self.parent.font
            if "GRASS_ENCODING" in os.environ:
                oldencoding = os.environ["GRASS_ENCODING"]
            if self.parent.encoding != None and self.parent.encoding != "ISO-8859-1":
                os.environ[GRASS_ENCODING] = self.parent.encoding
            
            # using active comp region
            self.Map.GetRegion(update = True)
            
            self.Map.width, self.Map.height = self.GetClientSize()
            self.mapfile = self.Map.Render(force = self.render)
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
        self.Draw(self.pdc, self.img, drawid = id) # draw map image background
        
        self.resize = False
        
        # update statusbar
        # Debug.msg (3, "BufferedWindow.UpdateHist(%s): region=%s" % self.Map.region)
        self.Map.SetRegion()
        self.parent.statusbar.SetStatusText("Image/Raster map <%s>" % self.parent.mapname)
        
        # set default font and encoding environmental variables
        if oldfont != "":
            os.environ["GRASS_FONT"] = oldfont
        if oldencoding != "":
            os.environ["GRASS_ENCODING"] = oldencoding
        
    def EraseMap(self):
        """!Erase the map display
        """
        self.Draw(self.pdc, pdctype = 'clear')
        
class HistogramFrame(wx.Frame):
    """!Main frame for hisgram display window. Uses d.histogram
    rendered onto canvas
    """
    def __init__(self, parent = None, id = wx.ID_ANY,
                 title = _("GRASS GIS Histogramming Tool (d.histogram)"),
                 size = wx.Size(500, 350),
                 style = wx.DEFAULT_FRAME_STYLE, **kwargs):
        wx.Frame.__init__(self, parent, id, title, size = size, style = style, **kwargs)
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        
        self.Map   = Map()         # instance of render.Map to be associated with display
        self.layer = None          # reference to layer with histogram
        
        # Init variables
        self.params = {}  # previously set histogram parameters
        self.propwin = '' # ID of properties dialog
        
        self.font = ""
        self.encoding = 'ISO-8859-1' # default encoding for display fonts
        
        self.toolbar = HistogramToolbar(parent = self)
        self.SetToolBar(self.toolbar)

        # find selected map
        self.mapname = None
        if parent.GetName() == "MapWindow" and not parent.IsStandalone():
            tree = parent.GetLayerManager().GetLayerTree()

            if tree.layer_selected and tree.GetLayerInfo(tree.layer_selected, key = 'type') == 'raster':
                self.mapname = tree.GetLayerInfo(tree.layer_selected, key = 'maplayer').name

        # Add statusbar
        self.statusbar = self.CreateStatusBar(number = 1, style = 0)
        # self.statusbar.SetStatusWidths([-2, -1])
        hist_frame_statusbar_fields = ["Histogramming %s" % self.mapname]
        for i in range(len(hist_frame_statusbar_fields)):
            self.statusbar.SetStatusText(hist_frame_statusbar_fields[i], i)
        
        # Init map display
        self.InitDisplay() # initialize region values
        
        # initialize buffered DC
        self.HistWindow = BufferedWindow(self, id = wx.ID_ANY, Map = self.Map) # initialize buffered DC
        
        # Bind various events
        self.Bind(wx.EVT_CLOSE,    self.OnCloseWindow)
        
        # Init print module and classes
        self.printopt = PrintOptions(self, self.HistWindow)
        
        # Add layer to the map
        self.layer = self.Map.AddLayer(ltype = "command", name = 'histogram', command = [['d.histogram']],
                                       active = False, hidden = False, opacity = 1, render = False)
        if self.mapname:
            self.SetHistLayer(self.mapname, None)
        else:
            self.OnErase(None)

    def InitDisplay(self):
        """!Initialize histogram display, set dimensions and region
        """
        self.width, self.height = self.GetClientSize()
        self.Map.geom = self.width, self.height
        
    def OnOptions(self, event):
        """!Change histogram settings"""
        cmd = ['d.histogram']
        if self.mapname != '':
            cmd.append('map=%s' % self.mapname)
        module = GUI(parent = self)
        module.ParseCommand(cmd, completed = (self.GetOptData, None, self.params))

    def GetOptData(self, dcmd, layer, params, propwin):
        """!Callback method for histogram command generated by dialog
        created in menuform.py
        """
        if dcmd:
            name, found = GetLayerNameFromCmd(dcmd, fullyQualified = True,
                                              layerType = 'raster')
            if not found:
                GError(parent = propwin,
                       message = _("Raster map <%s> not found") % name)
                return
            
            self.SetHistLayer(name, dcmd)
        self.params = params
        self.propwin = propwin
        self.HistWindow.UpdateHist()
        
    def SetHistLayer(self, name, cmd = None):
        """!Set histogram layer
        """
        self.mapname = name
        if not cmd:
            cmd = ['d.histogram',('map=%s' % self.mapname)]
        self.layer = self.Map.ChangeLayer(layer = self.layer,
                                          command = [cmd],
                                          active = True)
        
        return self.layer

    def SetHistFont(self, event):
        """!Set font for histogram. If not set, font will be default
        display font.
        """
        dlg = DefaultFontDialog(parent = self, id = wx.ID_ANY,
                                title = _('Select font for histogram text'))        
        dlg.fontlb.SetStringSelection(self.font, True)
        
        if dlg.ShowModal() == wx.ID_CANCEL:
            dlg.Destroy()
            return
        
        # set default font type, font, and encoding to whatever selected in dialog
        if dlg.font != None:
            self.font = dlg.font
        if dlg.encoding != None:
            self.encoding = dlg.encoding
        
        dlg.Destroy()
        self.HistWindow.UpdateHist()

    def OnErase(self, event):
        """!Erase the histogram display
        """
        self.HistWindow.Draw(self.HistWindow.pdc, pdctype = 'clear')
        
    def OnRender(self, event):
        """!Re-render histogram
        """
        self.HistWindow.UpdateHist()
        
    def GetWindow(self):
        """!Get buffered window"""
        return self.HistWindow
    
    def SaveToFile(self, event):
        """!Save to file
        """
        filetype, ltype = GetImageHandlers(self.HistWindow.img)
        
        # get size
        dlg = ImageSizeDialog(self)
        dlg.CentreOnParent()
        if dlg.ShowModal() != wx.ID_OK:
            dlg.Destroy()
            return
        width, height = dlg.GetValues()
        dlg.Destroy()
        
        # get filename
        dlg = wx.FileDialog(parent = self,
                            message = _("Choose a file name to save the image "
                                        "(no need to add extension)"),
                            wildcard = filetype,
                            style=wx.SAVE | wx.FD_OVERWRITE_PROMPT)
        
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            if not path:
                dlg.Destroy()
                return
            
            base, ext = os.path.splitext(path)
            fileType = ltype[dlg.GetFilterIndex()]['type']
            extType  = ltype[dlg.GetFilterIndex()]['ext']
            if ext != extType:
                path = base + '.' + extType
            
            self.HistWindow.SaveToFile(path, fileType,
                                       width, height)
        
        self.HistWindow.UpdateHist()
        dlg.Destroy()
        
    def PrintMenu(self, event):
        """!Print options and output menu
        """
        point = wx.GetMousePosition()
        printmenu = wx.Menu()
        # Add items to the menu
        setup = wx.MenuItem(printmenu, id = wx.ID_ANY, text = _('Page setup'))
        printmenu.AppendItem(setup)
        self.Bind(wx.EVT_MENU, self.printopt.OnPageSetup, setup)
        
        preview = wx.MenuItem(printmenu, id = wx.ID_ANY, text = _('Print preview'))
        printmenu.AppendItem(preview)
        self.Bind(wx.EVT_MENU, self.printopt.OnPrintPreview, preview)
        
        doprint = wx.MenuItem(printmenu, id = wx.ID_ANY, text = _('Print display'))
        printmenu.AppendItem(doprint)
        self.Bind(wx.EVT_MENU, self.printopt.OnDoPrint, doprint)
        
        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(printmenu)
        printmenu.Destroy()
        
    def OnQuit(self, event):
        self.Close(True)
        
    def OnCloseWindow(self, event):
        """!Window closed
        Also remove associated rendered images
        """
        try:
            self.propwin.Close(True)
        except:
            pass
        self.Map.Clean()
        self.Destroy()
        
class HistogramToolbar(BaseToolbar):
    """!Histogram toolbar (see histogram.py)
    """
    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()
        
    def _toolbarData(self):
        """!Toolbar data"""
        return self._getToolbarData((('histogram', BaseIcons["histogramD"],
                                      self.parent.OnOptions),
                                     ('render', BaseIcons["display"],
                                      self.parent.OnRender),
                                     ('erase', BaseIcons["erase"],
                                      self.parent.OnErase),
                                     ('font', BaseIcons["font"],
                                      self.parent.SetHistFont),
                                     (None, ),
                                     ('save', BaseIcons["saveFile"],
                                      self.parent.SaveToFile),
                                     ('hprint', BaseIcons["print"],
                                      self.parent.PrintMenu),
                                     (None, ),
                                     ('quit', BaseIcons["quit"],
                                      self.parent.OnQuit))
                                    )
