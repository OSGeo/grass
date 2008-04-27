"""
MODULE: histogram

CLASSES:
    * BufferedWindow
    * HistFrame

PURPOSE: Plotting histogram

AUTHORS: The GRASS Development Team
         Michael Barton

COPYRIGHT: (C) 2007 by the GRASS Development Team
           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import wx
import wx.aui
import os, sys, time, glob, math
from threading import Thread

import globalvar
try:
    import subprocess
except:
    CompatPath = os.path.join(globalvar.ETCWXDIR)
    sys.path.append(CompatPath)
    from compat import subprocess

gmpath = os.path.join(globalvar.ETCWXDIR, "icons")
sys.path.append(gmpath)

import render
import menuform
import disp_print
from gui_modules.preferences import SetDefaultFont as SetDefaultFont
from debug import Debug as Debug
from icon import Icons as Icons

import images
imagepath = images.__path__[0]
sys.path.append(imagepath)

os.environ["GRASS_BACKGROUNDCOLOR"] = "blue"

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
        self.Bind(wx.EVT_SIZE,         self.OnSize)
        self.Bind(wx.EVT_IDLE,         self.OnIdle)

        #
        # Render output objects
        #
        self.mapfile = None # image file to be rendered
        self.img = ""       # wx.Image object (self.mapfile)

        self.imagedict = {} # images and their PseudoDC ID's for painting and dragging

        self.pdc = wx.PseudoDC()
        self._Buffer = '' # will store an off screen empty bitmap for saving to file
        self.Map.SetRegion() # make sure that extents are updated at init

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
            self.UpdateHist()

        # re-render image on idle
        self.resize = True

    def OnIdle(self, event):
        """
        Only re-render a histogram image from GRASS during
        idle time instead of multiple times during resizing.
            """

        if self.resize:
            self.render = True
            self.UpdateHist()
        event.Skip()

    def SaveToFile(self, FileName, FileType):
        """
        This will save the contents of the buffer
        to the specified file. See the wx.Windows docs for
        wx.Bitmap::SaveFile for the details
        """
        dc = wx.BufferedPaintDC(self, self._Buffer)
        self.pdc.DrawToDC(dc)
        self._Buffer.SaveFile(FileName, FileType)

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


    def UpdateHist(self, img=None):
        """
        Update canvas if histogram options changes or window changes geometry
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

            self.Map.width, self.Map.height = self.GetClientSize()
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
        self.parent.statusbar.SetStatusText("Raster/Image map layer <%s>" % self.parent.mapname)

        # set default font and encoding environmental variables
        if oldfont != "":
            os.environ["GRASS_FONT"] = oldfont
        if oldencoding != "":
            os.environ["GRASS_ENCODING"] = oldencoding

    def EraseMap(self):
        """
        Erase the map display
        """
        self.Draw(self.pdc, pdctype='clear')

class HistFrame(wx.Frame):
    """
    Main frame for hisgram display window.
    Uses d.histogram rendered onto canvas
    """

    def __init__(self, parent=None, id = wx.ID_ANY, title="Histogram of image or raster map",
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_FRAME_STYLE):

        wx.Frame.__init__(self, parent, id, title, pos, size, style)

        toolbar = self.__createToolBar()

        self.Map   = render.Map()  # instance of render.Map to be associated with display
        self.layer = None          # reference to layer with histogram
        #
        # Set the size & cursor
        #
        self.SetClientSize(size)
        self.iconsize = (16, 16)

        # Init variables
        self.params = {} # previously set histogram parameters
        self.propwin = '' # ID of properties dialog

        self.font = ""
        self.encoding = 'ISO-8859-1' # default encoding for display fonts

        #
        # Add statusbar
        #
        self.mapname = ''
        self.statusbar = self.CreateStatusBar(number=1, style=0)
        # self.statusbar.SetStatusWidths([-2, -1])
        hist_frame_statusbar_fields = ["Histogramming %s" % self.mapname]
        for i in range(len(hist_frame_statusbar_fields)):
            self.statusbar.SetStatusText(hist_frame_statusbar_fields[i], i)

        #
        # Init map display
        #
        self.InitDisplay() # initialize region values

        # initialize buffered DC
        self.HistWindow = BufferedWindow(self, id = wx.ID_ANY, Map=self.Map) # initialize buffered DC

        #
        # Bind various events
        #
        self.Bind(wx.EVT_CLOSE,    self.OnCloseWindow)

        #
        # Init print module and classes
        #
        self.printopt = disp_print.PrintOptions(self, self.HistWindow)

        #
        # Add layer to the map
        #
        self.layer = self.Map.AddLayer(type="command", name='', command=['d.histogram'],
                                       l_active=False, l_hidden=False, l_opacity=1, l_render=False)


    def __createToolBar(self):
        """Creates toolbar"""

        toolbar = self.CreateToolBar()
        for each in self.toolbarData():
            self.AddToolbarButton(toolbar, *each)
        toolbar.Realize()

    def AddToolbarButton(self, toolbar, label, icon, help, handler):
        """Adds buttons to the toolbar"""

        if not label:
            toolbar.AddSeparator()
            return
        tool = toolbar.AddLabelTool(id=wx.ID_ANY, label=label, bitmap=icon, shortHelp=help)
        self.Bind(wx.EVT_TOOL, handler, tool)

    def toolbarData(self):

        return   (
                 ('histogram', Icons["histogram"].GetBitmap(), Icons["histogram"].GetLabel(), self.OnOptions),
                 ('erase', Icons["erase"].GetBitmap(), Icons["erase"].GetLabel(), self.OnErase),
                 ('font', Icons["font"].GetBitmap(), Icons["font"].GetLabel(), self.SetHistFont),
                 ('', '', '', ''),
                 ('save',  Icons["savefile"].GetBitmap(),  Icons["savefile"].GetLabel(),  self.SaveToFile),
                 ('print',  Icons["printmap"].GetBitmap(),  Icons["printmap"].GetLabel(),  self.PrintMenu),
                 ('quit',  wx.ArtProvider.GetBitmap(wx.ART_QUIT, wx.ART_TOOLBAR, (16,16)),  Icons["quit"].GetLabel(), self.OnQuit),                  )

    def InitDisplay(self):
        """
        Initialize histogram display, set dimensions and region
        """
        self.width, self.height = self.GetClientSize()
        self.Map.geom = self.width, self.height

    def OnOptions(self, event):
        global gmpath
        completed = ''

        menuform.GUI().ParseCommand(['d.histogram'], gmpath,
                                    completed=(self.GetOptData, "hist", self.params),
                                    parentframe=None)


    def GetOptData(self, dcmd, layer, params, propwin):
        """
        Callback method for histogram command generated by
        dialog created in menuform.py
        """

        # Reset comand and rendering options in render.Map. Always render decoration.
        # Showing/hiding handled by PseudoDC

        self.SetHistLayer(dcmd)
        self.params = params
        self.propwin = propwin

        self.HistWindow.UpdateHist()

    def SetHistLayer(self, cmd):
        """
        Set histogram layer
        """

        for item in cmd:
            if 'map=' in item:
                self.mapname = item.split('=')[1]

        self.layer = self.Map.ChangeLayer(layer=self.layer, type="command", name='histogram',
                                          command=cmd,
                                          l_active=True, l_hidden=False, l_opacity=1.0)

        return self.layer

    def SetHistFont(self, event):
        """
        Set font for histogram. If not
        set, font will be default display font.
        """

        dlg = SetDefaultFont(self, wx.ID_ANY, 'Select font for histogram text',
                             pos=wx.DefaultPosition, size=wx.DefaultSize,
                             style=wx.DEFAULT_DIALOG_STYLE,
                             encoding=self.encoding)
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
        """
        Erase the histogram display
        """
        self.HistWindow.Draw(self.HistWindow.pdc, pdctype='clear')

    def SaveToFile(self, event):
        """
        Save to file
        """
        filetype =  "PNG file (*.png)|*.png|"\
                    "TIF file (*.tif)|*.tif|"\
                    "GIF file (*.gif)|*.gif"

        dlg = wx.FileDialog(self, "Choose a file name to save the image as a PNG to",
            defaultDir = "",
            defaultFile = "",
            wildcard = filetype,
            style=wx.SAVE|wx.FD_OVERWRITE_PROMPT)
        if dlg.ShowModal() == wx.ID_OK:
            base = os.path.splitext(dlg.GetPath())[0]
            ext = os.path.splitext(dlg.GetPath())[1]
            if dlg.GetFilterIndex() == 0:
                type = wx.BITMAP_TYPE_PNG
                path = dlg.GetPath()
                if ext != '.png': path = base+'.png'
            elif dlg.GetFilterIndex() == 1:
                type = wx.BITMAP_TYPE_TIF
                if ext != '.tif': path = base+'.tif'
            elif dlg.GetFilterIndex() == 2:
                type = wx.BITMAP_TYPE_TIF
                if ext != '.gif': path = base+'.gif'
            self.HistWindow.SaveToFile(path, type)
        dlg.Destroy()

    def PrintMenu(self, event):
        """
        Print options and output menu
        """
        point = wx.GetMousePosition()
        printmenu = wx.Menu()
        # Add items to the menu
        setup = wx.MenuItem(printmenu, -1,'Page setup')
        printmenu.AppendItem(setup)
        self.Bind(wx.EVT_MENU, self.printopt.OnPageSetup, setup)

        preview = wx.MenuItem(printmenu, -1,'Print preview')
        printmenu.AppendItem(preview)
        self.Bind(wx.EVT_MENU, self.printopt.OnPrintPreview, preview)

        doprint = wx.MenuItem(printmenu, -1,'Print display')
        printmenu.AppendItem(doprint)
        self.Bind(wx.EVT_MENU, self.printopt.OnDoPrint, doprint)

        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(printmenu)
        printmenu.Destroy()

    def OnQuit(self, event):
        self.Close(True)

    def OnCloseWindow(self, event):
        """
        Window closed
        Also remove associated rendered images
        """
        try:
            self.propwin.Close(True)
        except:
            pass
        self.Map.Clean()
        self.Destroy()


