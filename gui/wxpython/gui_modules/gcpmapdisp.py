"""!
@package gcpmapdisp.py

@brief display to manage ground control points with two toolbars, one for
various display management functions, one for manipulating GCPs.

Classes:
- MapFrame

(C) 2006-2010 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

Derived from mapdisp.py

@author Markus Metz
"""

import os
import sys
import glob
import math
import tempfile
import copy
import platform

import globalvar
if not os.getenv("GRASS_WXBUNDLED"):
    globalvar.CheckForWx()
import wx
import wx.aui

try:
    import subprocess
except:
    CompatPath = os.path.join(globalvar.ETCWXDIR)
    sys.path.append(CompatPath)
    from compat import subprocess

gmpath = os.path.join(globalvar.ETCWXDIR, "icons")
sys.path.append(gmpath)

grassPath = os.path.join(globalvar.ETCDIR, "python")
sys.path.append(grassPath)

import render
import toolbars
import menuform
import gselect
import disp_print
import gcmd
import dbm
import dbm_dialogs
import globalvar
import utils
import gdialogs
from grass.script import core as grass
from debug import Debug
from icon  import Icons
from preferences import globalSettings as UserSettings

from mapdisp_command import Command
from mapdisp_window import BufferedWindow

import images
imagepath = images.__path__[0]
sys.path.append(imagepath)

###
### global variables
###
# for standalone app
cmdfilename = None

class MapFrame(wx.Frame):
    """!Main frame for map display window. Drawing takes place in
    child double buffered drawing window.
    """
    def __init__(self, parent=None, id=wx.ID_ANY, title=_("GRASS GIS Manage Ground Control Points"),
                 style=wx.DEFAULT_FRAME_STYLE, toolbars=["gcpdisp"],
                 tree=None, notebook=None, lmgr=None, page=None,
                 Map=None, auimgr=None, **kwargs):
        """!Main map display window with toolbars, statusbar and
        DrawWindow

        @param toolbars array of activated toolbars, e.g. ['map', 'digit']
        @param tree reference to layer tree
        @param notebook control book ID in Layer Manager
        @param lmgr Layer Manager
        @param page notebook page with layer tree
        @param Map instance of render.Map
        @param auimgs AUI manager
        @param kwargs wx.Frame attribures
        """
        self._layerManager = lmgr   # Layer Manager object
        self.Map        = Map       # instance of render.Map
        self.tree       = tree      # Layer Manager layer tree object
        self.page       = page      # Notebook page holding the layer tree
        self.layerbook  = notebook  # Layer Manager layer tree notebook
        self.parent     = parent
        
        if not kwargs.has_key('name'):
            kwargs['name'] = 'GCPMapWindow'
        wx.Frame.__init__(self, parent, id, title, style = style, **kwargs)
        
        # available cursors
        self.cursors = {
            # default: cross
            # "default" : wx.StockCursor(wx.CURSOR_DEFAULT),
            "default" : wx.StockCursor(wx.CURSOR_ARROW),
            "cross"   : wx.StockCursor(wx.CURSOR_CROSS),
            "hand"    : wx.StockCursor(wx.CURSOR_HAND),
            "pencil"  : wx.StockCursor(wx.CURSOR_PENCIL),
            "sizenwse": wx.StockCursor(wx.CURSOR_SIZENWSE)
            }
        
        #
        # set the size & system icon
        #
        self.SetClientSize(self.GetSize())
        self.iconsize = (16, 16)

        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass_map.ico'), wx.BITMAP_TYPE_ICO))

        #
        # Fancy gui
        #
        self._mgr = wx.aui.AuiManager(self)

        #
        # Add toolbars
        #
        self.toolbars = { 'map' : None,
                          'vdigit' : None,
                          'georect' : None, 
                          'gcpdisp' : None, 
                          'gcpman' : None, 
                          'nviz' : None }

        for toolb in toolbars:
            self.AddToolbar(toolb)

        self.activemap = self.toolbars['gcpdisp'].togglemap
        self.activemap.SetSelection(0)
        self.SrcMap        = self.grwiz.SrcMap       # instance of render.Map
        self.TgtMap        = self.grwiz.TgtMap       # instance of render.Map
        self._mgr.SetDockSizeConstraint(0.5, 0.5)

        #
        # Add statusbar
        #
        self.statusbar = self.CreateStatusBar(number=4, style=0)
        self.statusbar.SetStatusWidths([-5, -2, -1, -1])
        self.statusbarWin = dict()
        self.statusbarWin['toggle'] = wx.Choice(self.statusbar, wx.ID_ANY,
                                  choices = [_("Coordinates"),
                                             _("Extent"),
                                             _("Comp. region"),
                                             _("Show comp. extent"),
                                             _("Display mode"),
                                             _("Display geometry"),
                                             _("Map scale"),
                                             _("Go to GCP No."),
                                             _("RMS error")])
        # set StatusBar to Go to GCP No.
        self.statusbarWin['toggle'].SetSelection(7)

        self.statusbar.Bind(wx.EVT_CHOICE, self.OnToggleStatus, self.statusbarWin['toggle'])
        # auto-rendering checkbox
        self.statusbarWin['render'] = wx.CheckBox(parent=self.statusbar, id=wx.ID_ANY,
                                                  label=_("Render"))
        self.statusbar.Bind(wx.EVT_CHECKBOX, self.OnToggleRender, self.statusbarWin['render'])
        self.statusbarWin['render'].SetValue(UserSettings.Get(group='display',
                                                              key='autoRendering',
                                                              subkey='enabled'))
        self.statusbarWin['render'].SetToolTip(wx.ToolTip (_("Enable/disable auto-rendering")))
        # show region
        self.statusbarWin['region'] = wx.CheckBox(parent=self.statusbar, id=wx.ID_ANY,
                                                  label=_("Show computational extent"))
        self.statusbar.Bind(wx.EVT_CHECKBOX, self.OnToggleShowRegion, self.statusbarWin['region'])
        
        self.statusbarWin['region'].SetValue(False)
        self.statusbarWin['region'].Hide()
        self.statusbarWin['region'].SetToolTip(wx.ToolTip (_("Show/hide computational "
                                                             "region extent (set with g.region). "
                                                             "Display region drawn as a blue box inside the "
                                                             "computational region, "
                                                             "computational region inside a display region "
                                                             "as a red box).")))
        # set resolution
        self.statusbarWin['resolution'] = wx.CheckBox(parent=self.statusbar, id=wx.ID_ANY,
                                                      label=_("Constrain display resolution to computational settings"))
        self.statusbar.Bind(wx.EVT_CHECKBOX, self.OnToggleResolution, self.statusbarWin['resolution'])
        self.statusbarWin['resolution'].SetValue(UserSettings.Get(group='display', key='compResolution', subkey='enabled'))
        self.statusbarWin['resolution'].Hide()
        self.statusbarWin['resolution'].SetToolTip(wx.ToolTip (_("Constrain display resolution "
                                                                 "to computational region settings. "
                                                                 "Default value for new map displays can "
                                                                 "be set up in 'User GUI settings' dialog.")))
        # map scale
        self.statusbarWin['mapscale'] = wx.ComboBox(parent = self.statusbar, id = wx.ID_ANY,
                                                    style = wx.TE_PROCESS_ENTER,
                                                    size=(150, -1))
        self.statusbarWin['mapscale'].SetItems(['1:1000',
                                                '1:5000',
                                                '1:10000',
                                                '1:25000',
                                                '1:50000',
                                                '1:100000',
                                                '1:1000000'])
        self.statusbarWin['mapscale'].Hide()
        self.statusbar.Bind(wx.EVT_TEXT_ENTER, self.OnChangeMapScale, self.statusbarWin['mapscale'])
        self.statusbar.Bind(wx.EVT_COMBOBOX, self.OnChangeMapScale, self.statusbarWin['mapscale'])

        # go to
        self.statusbarWin['goto'] = wx.SpinCtrl(parent=self.statusbar, id=wx.ID_ANY,
                             min=0)
        self.statusbar.Bind(wx.EVT_SPINCTRL, self.OnGoTo, self.statusbarWin['goto'])
        self.statusbarWin['goto'].Hide()
        self.statusbar.Bind(wx.EVT_TEXT_ENTER, self.OnGoTo, self.statusbarWin['goto'])

        # projection, unused but BufferedWindow checks for it
        self.statusbarWin['projection'] = wx.CheckBox(parent=self.statusbar, id=wx.ID_ANY,
                                                      label=_("Use defined projection"))
        self.statusbarWin['projection'].SetValue(False)
        size = self.statusbarWin['projection'].GetSize()
        self.statusbarWin['projection'].SetMinSize((size[0] + 150, size[1]))
        self.statusbarWin['projection'].SetToolTip(wx.ToolTip (_("Reproject coordinates displayed "
                                                                 "in the statusbar. Projection can be "
                                                                 "defined in GUI preferences dialog "
                                                                 "(tab 'Display')")))
        self.statusbarWin['projection'].Hide()

        # mask
        self.statusbarWin['mask'] = wx.StaticText(parent = self.statusbar, id = wx.ID_ANY,
                                                  label = '')
        self.statusbarWin['mask'].SetForegroundColour(wx.Colour(255, 0, 0))
        
        # on-render gauge
        self.statusbarWin['progress'] = wx.Gauge(parent=self.statusbar, id=wx.ID_ANY,
                                      range=0, style=wx.GA_HORIZONTAL)
        self.statusbarWin['progress'].Hide()
        
        self.StatusbarReposition() # reposition statusbar

        #
        # Init map display (buffered DC & set default cursor)
        #
        self.grwiz.SwitchEnv('source')
        self.SrcMapWindow = BufferedWindow(self, id=wx.ID_ANY,
                                          Map=self.SrcMap, tree=self.tree, lmgr=self._layerManager)

        self.grwiz.SwitchEnv('target')
        self.TgtMapWindow = BufferedWindow(self, id=wx.ID_ANY,
                                          Map=self.TgtMap, tree=self.tree, lmgr=self._layerManager)
        self.MapWindow = self.SrcMapWindow
        self.Map = self.SrcMap
        self.SrcMapWindow.SetCursor(self.cursors["cross"])
        self.TgtMapWindow.SetCursor(self.cursors["cross"])

        #
        # initialize region values
        #
        self.__InitDisplay() 

        #
        # Bind various events
        #
        self.Bind(wx.EVT_ACTIVATE, self.OnFocus)
        self.Bind(render.EVT_UPDATE_PRGBAR, self.OnUpdateProgress)
        self.Bind(wx.EVT_SIZE,     self.OnDispResize)
        self.activemap.Bind(wx.EVT_CHOICE, self.OnUpdateActive)
        
        #
        # Update fancy gui style
        #
        # AuiManager wants a CentrePane, workaround to get two equally sized windows
        self.list = self.CreateGCPList()

        #self.SrcMapWindow.SetSize((300, 300))
        #self.TgtMapWindow.SetSize((300, 300))
        self.list.SetSize((100, 150))
        self._mgr.AddPane(self.list, wx.aui.AuiPaneInfo().
                  Name("gcplist").Caption(_("GCP List")).LeftDockable(False).
                  RightDockable(False).PinButton().FloatingSize((600,200)).
                  CloseButton(False).DestroyOnClose(True).
                  Top().Layer(1).MinSize((200,100)))
        self._mgr.AddPane(self.SrcMapWindow, wx.aui.AuiPaneInfo().
                  Name("source").Caption(_("Source Display")).Dockable(False).
                  CloseButton(False).DestroyOnClose(True).Floatable(False).
                  Centre())
        self._mgr.AddPane(self.TgtMapWindow, wx.aui.AuiPaneInfo().
                  Name("target").Caption(_("Target Display")).Dockable(False).
                  CloseButton(False).DestroyOnClose(True).Floatable(False).
                  Right().Layer(0))

        srcwidth, srcheight = self.SrcMapWindow.GetSize()
        tgtwidth, tgtheight = self.TgtMapWindow.GetSize()
        srcwidth = (srcwidth + tgtwidth) / 2
        self._mgr.GetPane("target").Hide()
        self._mgr.Update()
        self._mgr.GetPane("source").BestSize((srcwidth, srcheight))
        self._mgr.GetPane("target").BestSize((srcwidth, srcheight))
        if self.show_target:
            self._mgr.GetPane("target").Show()
        else:
            self.activemap.Enable(False)
        # needed by Mac OS, does not harm on Linux, breaks display on Windows
        if platform.system() != 'Windows':
            self._mgr.Update()

        #
        # Init print module and classes
        #
        self.printopt = disp_print.PrintOptions(self, self.MapWindow)
        
        #
        # Initialization of digitization tool
        #
        self.digit = None

        # set active map
        self.MapWindow = self.SrcMapWindow
        self.Map = self.SrcMap
        
        # do not init zoom history here, that happens when zooming to map(s)

        #
        # Re-use dialogs
        #
        self.dialogs = {}
        self.dialogs['attributes'] = None
        self.dialogs['category'] = None
        self.dialogs['barscale'] = None
        self.dialogs['legend'] = None

        self.decorationDialog = None # decoration/overlays

    def AddToolbar(self, name):
        """!Add defined toolbar to the window
        
        Currently known toolbars are:
         - 'map'     - basic map toolbar
         - 'vdigit'  - vector digitizer
         - 'gcpdisp' - GCP Manager, Display
         - 'gcpman'  - GCP Manager, points management
         - 'georect' - georectifier
         - 'nviz'    - 3D view mode
        """
        # default toolbar
        if name == "map":
            self.toolbars['map'] = toolbars.MapToolbar(self, self.Map)

            self._mgr.AddPane(self.toolbars['map'],
                              wx.aui.AuiPaneInfo().
                              Name("maptoolbar").Caption(_("Map Toolbar")).
                              ToolbarPane().Top().
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(False).TopDockable(True).
                              CloseButton(False).Layer(2).
                              BestSize((self.toolbars['map'].GetSize())))

        # GCP display
        elif name == "gcpdisp":
            self.toolbars['gcpdisp'] = toolbars.GCPDisplayToolbar(self)

            self._mgr.AddPane(self.toolbars['gcpdisp'],
                              wx.aui.AuiPaneInfo().
                              Name("gcpdisplaytoolbar").Caption(_("GCP Display toolbar")).
                              ToolbarPane().Top().
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(False).TopDockable(True).
                              CloseButton(False).Layer(2))

            if self.show_target == False:
                self.toolbars['gcpdisp'].Enable('zoommenu', enable = False)

            self.toolbars['gcpman'] = toolbars.GCPManToolbar(self)

            self._mgr.AddPane(self.toolbars['gcpman'],
                              wx.aui.AuiPaneInfo().
                              Name("gcpmanagertoolbar").Caption(_("GCP Manager toolbar")).
                              ToolbarPane().Top().Row(1).
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(False).TopDockable(True).
                              CloseButton(False).Layer(2))
            
        self._mgr.Update()

    def __InitDisplay(self):
        """
        Initialize map display, set dimensions and map region
        """
        self.width, self.height = self.GetClientSize()

        Debug.msg(2, "MapFrame.__InitDisplay():")
        self.grwiz.SwitchEnv('source')
        self.SrcMap.ChangeMapSize(self.GetClientSize())
        self.SrcMap.region = self.SrcMap.GetRegion() # g.region -upgc
        self.grwiz.SwitchEnv('target')
        self.TgtMap.ChangeMapSize(self.GetClientSize())
        self.TgtMap.region = self.TgtMap.GetRegion() # g.region -upgc
        # self.SrcMap.SetRegion() # adjust region to match display window
        # self.TgtMap.SetRegion() # adjust region to match display window

    def OnUpdateProgress(self, event):
        """
        Update progress bar info
        """
        self.statusbarWin['progress'].SetValue(event.value)
        
        event.Skip()
        
    def OnFocus(self, event):
        """
        Change choicebook page to match display.
        Or set display for georectifying
        """
        if self._layerManager and \
                self._layerManager.gcpmanagement:
            # in GCP Management, set focus to current MapWindow for mouse actions
            self.OnPointer(event)
            self.MapWindow.SetFocus()
        else:
            # change bookcontrol page to page associated with display
            # GCP Manager: use bookcontrol?
            if self.page:
                pgnum = self.layerbook.GetPageIndex(self.page)
                if pgnum > -1:
                    self.layerbook.SetSelection(pgnum)
        
        event.Skip()

    def OnDraw(self, event):
        """!Re-display current map composition
        """
        self.MapWindow.UpdateMap(render = False)
        
    def OnRender(self, event):
        """!Re-render map composition (each map layer)
        """
        # delete tmp map layers (queries)
        qlayer = self.Map.GetListOfLayers(l_name=globalvar.QUERYLAYER)
        for layer in qlayer:
            self.Map.DeleteLayer(layer)

        self.SrcMapWindow.UpdateMap(render=True)
        if self.show_target:
            self.TgtMapWindow.UpdateMap(render=True)
        
        # update statusbar
        self.StatusbarUpdate()

    def OnPointer(self, event):
        """!Pointer button clicked
        """
        # change the cursor
        self.SrcMapWindow.SetCursor(self.cursors["cross"])
        self.SrcMapWindow.mouse['use'] = "pointer"
        self.SrcMapWindow.mouse['box'] = "point"
        self.TgtMapWindow.SetCursor(self.cursors["cross"])
        self.TgtMapWindow.mouse['use'] = "pointer"
        self.TgtMapWindow.mouse['box'] = "point"

    def OnZoomIn(self, event):
        """
        Zoom in the map.
        Set mouse cursor, zoombox attributes, and zoom direction
        """
        if self.toolbars['map']:
            self.toolbars['map'].OnTool(event)
            self.toolbars['map'].action['desc'] = ''
        
        self.MapWindow.mouse['use'] = "zoom"
        self.MapWindow.mouse['box'] = "box"
        self.MapWindow.zoomtype = 1
        self.MapWindow.pen = wx.Pen(colour='Red', width=2, style=wx.SHORT_DASH)
        
        # change the cursor
        self.MapWindow.SetCursor(self.cursors["cross"])

        if self.MapWindow == self.SrcMapWindow:
            win = self.TgtMapWindow
        elif self.MapWindow == self.TgtMapWindow:
            win = self.SrcMapWindow

        win.mouse['use'] = "zoom"
        win.mouse['box'] = "box"
        win.zoomtype = 1
        win.pen = wx.Pen(colour='Red', width=2, style=wx.SHORT_DASH)
        
        # change the cursor
        win.SetCursor(self.cursors["cross"])

    def OnZoomOut(self, event):
        """
        Zoom out the map.
        Set mouse cursor, zoombox attributes, and zoom direction
        """
        if self.toolbars['map']:
            self.toolbars['map'].OnTool(event)
            self.toolbars['map'].action['desc'] = ''
        
        self.MapWindow.mouse['use'] = "zoom"
        self.MapWindow.mouse['box'] = "box"
        self.MapWindow.zoomtype = -1
        self.MapWindow.pen = wx.Pen(colour='Red', width=2, style=wx.SHORT_DASH)
        
        # change the cursor
        self.MapWindow.SetCursor(self.cursors["cross"])

        if self.MapWindow == self.SrcMapWindow:
            win = self.TgtMapWindow
        elif self.MapWindow == self.TgtMapWindow:
            win = self.SrcMapWindow

        win.mouse['use'] = "zoom"
        win.mouse['box'] = "box"
        win.zoomtype = -1
        win.pen = wx.Pen(colour='Red', width=2, style=wx.SHORT_DASH)
        
        # change the cursor
        win.SetCursor(self.cursors["cross"])

    def OnZoomBack(self, event):
        """
        Zoom last (previously stored position)
        """
        self.MapWindow.ZoomBack()

    def OnPan(self, event):
        """
        Panning, set mouse to drag
        """
        if self.toolbars['map']:
            self.toolbars['map'].OnTool(event)
            self.toolbars['map'].action['desc'] = ''
        
        self.MapWindow.mouse['use'] = "pan"
        self.MapWindow.mouse['box'] = "pan"
        self.MapWindow.zoomtype = 0
        
        # change the cursor
        self.MapWindow.SetCursor(self.cursors["hand"])

        if self.MapWindow == self.SrcMapWindow:
            win = self.TgtMapWindow
        elif self.MapWindow == self.TgtMapWindow:
            win = self.SrcMapWindow

        win.mouse['use'] = "pan"
        win.mouse['box'] = "pan"
        win.zoomtype = 0
        
        # change the cursor
        win.SetCursor(self.cursors["hand"])

    def OnErase(self, event):
        """
        Erase the canvas
        """
        self.MapWindow.EraseMap()

        if self.MapWindow == self.SrcMapWindow:
            win = self.TgtMapWindow
        elif self.MapWindow == self.TgtMapWindow:
            win = self.SrcMapWindow

        win.EraseMap()

    def OnZoomRegion(self, event):
        """
        Zoom to region
        """
        self.Map.getRegion()
        self.Map.getResolution()
        self.UpdateMap()
        # event.Skip()

    def OnAlignRegion(self, event):
        """
        Align region
        """
        if not self.Map.alignRegion:
            self.Map.alignRegion = True
        else:
            self.Map.alignRegion = False
        # event.Skip()

    def OnToggleRender(self, event):
        """
        Enable/disable auto-rendering
        """
        if self.statusbarWin['render'].GetValue():
            self.OnRender(None)

    def OnToggleShowRegion(self, event):
        """
        Show/Hide extent in map canvas
        """
        if self.statusbarWin['region'].GetValue():
            # show extent
            self.MapWindow.regionCoords = []
        else:
            del self.MapWindow.regionCoords

        # redraw map if auto-rendering is enabled
        if self.statusbarWin['render'].GetValue():
            self.OnRender(None)

    def OnToggleResolution(self, event):
        """
        Use resolution of computation region settings
        for redering image instead of display resolution
        """
        # redraw map if auto-rendering is enabled
        if self.statusbarWin['render'].GetValue():
            self.OnRender(None)
        
    def OnToggleStatus(self, event):
        """
        Toggle status text
        """
        self.StatusbarUpdate()

    def OnChangeMapScale(self, event):
        """
        Map scale changed by user
        """
        scale = event.GetString()

        try:
            if scale[:2] != '1:':
                raise ValueError
            value = int(scale[2:])
        except ValueError:
            self.statusbarWin['mapscale'].SetValue('1:%ld' % int(self.mapScaleValue))
            return

        dEW = value * (self.Map.region['cols'] / self.ppm[0])
        dNS = value * (self.Map.region['rows'] / self.ppm[1])
        self.Map.region['n'] = self.Map.region['center_northing'] + dNS / 2.
        self.Map.region['s'] = self.Map.region['center_northing'] - dNS / 2.
        self.Map.region['w'] = self.Map.region['center_easting']  - dEW / 2.
        self.Map.region['e'] = self.Map.region['center_easting']  + dEW / 2.
        
        # add to zoom history
        self.MapWindow.ZoomHistory(self.Map.region['n'], self.Map.region['s'],
                                   self.Map.region['e'], self.Map.region['w'])
        
        # redraw a map
        self.MapWindow.UpdateMap()
        self.statusbarWin['mapscale'].SetFocus()
        
    def OnGoTo(self, event):
        """
        Go to position
        """
        #GCPNo = int(event.GetString())
        GCPNo = self.statusbarWin['goto'].GetValue()

        if GCPNo < 0 or GCPNo > len(self.mapcoordlist):
            wx.MessageBox(parent=self,
                  message="%s 1 - %s." % (_("Valid Range:"),
                                 len(self.mapcoordlist)),
                  caption=_("Invalid GCP Number"), style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
            return

        if GCPNo == 0:
            return

        self.list.selectedkey = GCPNo
        self.list.selected = self.list.FindItemData(-1, GCPNo)
        self.list.render = False
        self.list.SetItemState(self.list.selected,
                          wx.LIST_STATE_SELECTED,
                          wx.LIST_STATE_SELECTED)
        self.list.render = True
        
        # Source MapWindow:
        begin = (self.mapcoordlist[GCPNo][1], self.mapcoordlist[GCPNo][2])
        begin = self.SrcMapWindow.Cell2Pixel(begin)
        end = begin
        self.SrcMapWindow.Zoom(begin, end, 0)

        # redraw map
        self.SrcMapWindow.UpdateMap()

        if self.show_target:
            # Target MapWindow:
            begin = (self.mapcoordlist[GCPNo][3], self.mapcoordlist[GCPNo][4])
            begin = self.TgtMapWindow.Cell2Pixel(begin)
            end = begin
            self.TgtMapWindow.Zoom(begin, end, 0)

            # redraw map
            self.TgtMapWindow.UpdateMap()

        self.statusbarWin['goto'].SetFocus()
        
    def StatusbarUpdate(self):
        """!Update statusbar content"""

        self.statusbarWin['region'].Hide()
        self.statusbarWin['resolution'].Hide()
        self.statusbarWin['mapscale'].Hide()
        self.statusbarWin['goto'].Hide()
        self.mapScaleValue = self.ppm = None

        if self.statusbarWin['toggle'].GetSelection() == 0: # Coordinates
            self.statusbar.SetStatusText("", 0)
            # enable long help
            self.StatusbarEnableLongHelp()

        elif self.statusbarWin['toggle'].GetSelection() in (1, 2): # Extent
            sel = self.statusbarWin['toggle'].GetSelection()
            if sel == 1:
                region = self.Map.region
            else:
                region = self.Map.GetRegion() # computation region

            precision = int(UserSettings.Get(group = 'projection', key = 'format',
                                             subkey = 'precision'))
            format = UserSettings.Get(group = 'projection', key = 'format',
                                      subkey = 'll')
            
            if self.Map.projinfo['proj'] == 'll' and format == 'DMS':
                w, s = utils.Deg2DMS(region["w"], region["s"],
                                     string = False, precision = precision)
                e, n = utils.Deg2DMS(region["e"], region["n"],
                                     string = False, precision = precision)
                if sel == 1:
                    self.statusbar.SetStatusText("%s - %s, %s - %s" %
                                                 (w, e, s, n), 0)
                else:
                    ewres, nsres = utils.Deg2DMS(region['ewres'], region['nsres'],
                                                 string = False, precision = precision)
                    self.statusbar.SetStatusText("%s - %s, %s - %s (%s, %s)" %
                                                 (w, e, s, n, ewres, nsres), 0)
            else:
                w, s = region["w"], region["s"]
                e, n = region["e"], region["n"]
                if sel == 1:
                    self.statusbar.SetStatusText("%.*f - %.*f, %.*f - %.*f" %
                                                 (precision, w, precision, e,
                                                  precision, s, precision, n), 0)
                else:
                    ewres, nsres = region['ewres'], region['nsres']
                    self.statusbar.SetStatusText("%.*f - %.*f, %.*f - %.*f (%.*f, %.*f)" %
                                                 (precision, w, precision, e,
                                                  precision, s, precision, n,
                                                  precision, ewres, precision, nsres), 0)
            # enable long help
            self.StatusbarEnableLongHelp()

        elif self.statusbarWin['toggle'].GetSelection() == 3: # Show comp. extent
            self.statusbar.SetStatusText("", 0)
            self.statusbarWin['region'].Show()
            # disable long help
            self.StatusbarEnableLongHelp(False)

        elif self.statusbarWin['toggle'].GetSelection() == 4: # Display mode
            self.statusbar.SetStatusText("", 0)
            self.statusbarWin['resolution'].Show()
            # disable long help
            self.StatusbarEnableLongHelp(False)

        elif self.statusbarWin['toggle'].GetSelection() == 5: # Display geometry
            self.statusbar.SetStatusText("rows=%d; cols=%d; nsres=%.2f; ewres=%.2f" %
                                         (self.Map.region["rows"], self.Map.region["cols"],
                                          self.Map.region["nsres"], self.Map.region["ewres"]), 0)
            # enable long help
            self.StatusbarEnableLongHelp()

        elif self.statusbarWin['toggle'].GetSelection() == 6: # Map scale
            # TODO: need to be fixed...
            ### screen X region problem
            ### user should specify ppm
            dc = wx.ScreenDC()
            dpSizePx = wx.DisplaySize()   # display size in pixels
            dpSizeMM = wx.DisplaySizeMM() # display size in mm (system)
            dpSizeIn = (dpSizeMM[0] / 25.4, dpSizeMM[1] / 25.4) # inches
            sysPpi  = dc.GetPPI()
            comPpi = (dpSizePx[0] / dpSizeIn[0],
                      dpSizePx[1] / dpSizeIn[1])

            ppi = comPpi                  # pixel per inch
            self.ppm = ((ppi[0] / 2.54) * 100, # pixel per meter
                        (ppi[1] / 2.54) * 100)

            Debug.msg(4, "MapFrame.StatusbarUpdate(mapscale): size: px=%d,%d mm=%f,%f "
                      "in=%f,%f ppi: sys=%d,%d com=%d,%d; ppm=%f,%f" % \
                          (dpSizePx[0], dpSizePx[1], dpSizeMM[0], dpSizeMM[1],
                           dpSizeIn[0], dpSizeIn[1],
                           sysPpi[0], sysPpi[1], comPpi[0], comPpi[1],
                           self.ppm[0], self.ppm[1]))

            region = self.Map.region

            heightCm = region['rows'] / self.ppm[1] * 100
            widthCm  = region['cols'] / self.ppm[0] * 100

            Debug.msg(4, "MapFrame.StatusbarUpdate(mapscale): width_cm=%f, height_cm=%f" %
                      (widthCm, heightCm))

            xscale = (region['e'] - region['w']) / (region['cols'] / self.ppm[0])
            yscale = (region['n'] - region['s']) / (region['rows'] / self.ppm[1])
            scale = (xscale + yscale) / 2.
            
            Debug.msg(3, "MapFrame.StatusbarUpdate(mapscale): xscale=%f, yscale=%f -> scale=%f" % \
                          (xscale, yscale, scale))

            self.statusbar.SetStatusText("")
            try:
                self.statusbarWin['mapscale'].SetValue("1:%ld" % (scale + 0.5))
            except TypeError:
                pass
            self.mapScaleValue = scale
            self.statusbarWin['mapscale'].Show()

            # disable long help
            self.StatusbarEnableLongHelp(False)

        elif self.statusbarWin['toggle'].GetSelection() == 7: # go to

            self.statusbar.SetStatusText("")
            max = self.list.GetItemCount()
            if max < 1:
                max = 1
            self.statusbarWin['goto'].SetRange(0, max)

            self.statusbarWin['goto'].Show()

            # disable long help
            self.StatusbarEnableLongHelp(False)
        
        elif self.statusbarWin['toggle'].GetSelection() == 8: # RMS error
            self.statusbar.SetStatusText(_("Forward: %(forw)s, Backward: %(back)s") %
                                         { 'forw' : self.fwd_rmserror,
                                           'back' : self.bkw_rmserror })
            # disable long help
            # self.StatusbarEnableLongHelp(False)
            
        else:
            self.statusbar.SetStatusText("", 1)

    def StatusbarEnableLongHelp(self, enable=True):
        """!Enable/disable toolbars long help"""
        for toolbar in self.toolbars.itervalues():
            if toolbar:
                toolbar.EnableLongHelp(enable)
                
    def StatusbarReposition(self):
        """!Reposition checkbox in statusbar"""
        # reposition checkbox
        widgets = [(0, self.statusbarWin['region']),
                   (0, self.statusbarWin['resolution']),
                   (0, self.statusbarWin['mapscale']),
                   (0, self.statusbarWin['progress']),
                   (0, self.statusbarWin['goto']),
                   (1, self.statusbarWin['toggle']),
                   (2, self.statusbarWin['mask']),
                   (3, self.statusbarWin['render'])]
        for idx, win in widgets:
            rect = self.statusbar.GetFieldRect(idx)
            wWin, hWin = win.GetBestSize()
            if idx == 0: # show region / mapscale / process bar
                # -> size
                if win == self.statusbarWin['progress']:
                    wWin = rect.width - 6
                # -> position
                # if win == self.statusbarWin['region']:
                # x, y = rect.x + rect.width - wWin, rect.y - 1
                # align left
                # else:
                x, y = rect.x + 3, rect.y - 1
                w, h = wWin, rect.height + 2
            else: # choice || auto-rendering
                x, y = rect.x, rect.y - 1
                w, h = rect.width, rect.height + 2
                if idx == 1: # choice
                    h = hWin
                elif idx == 2: # mask
                    x += 5
                    y += 4
                elif idx == 3: # render
                    x += 5

            win.SetPosition((x, y))
            win.SetSize((w, h))

    def SaveToFile(self, event):
        """!Save map to image
        """
        img = self.MapWindow.img
        if not img:
            gcmd.GMessage(parent = self,
                          message = _("Nothing to render (empty map). Operation canceled."))
            return
        filetype, ltype = gdialogs.GetImageHandlers(img)

        # get size
        dlg = gdialogs.ImageSizeDialog(self)
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
            
            self.MapWindow.SaveToFile(path, fileType,
                                      width, height)
            
        dlg.Destroy()

    def PrintMenu(self, event):
        """
        Print options and output menu for map display
        """
        point = wx.GetMousePosition()
        printmenu = wx.Menu()
        # Add items to the menu
        setup = wx.MenuItem(printmenu, wx.ID_ANY, _('Page setup'))
        printmenu.AppendItem(setup)
        self.Bind(wx.EVT_MENU, self.printopt.OnPageSetup, setup)

        preview = wx.MenuItem(printmenu, wx.ID_ANY, _('Print preview'))
        printmenu.AppendItem(preview)
        self.Bind(wx.EVT_MENU, self.printopt.OnPrintPreview, preview)

        doprint = wx.MenuItem(printmenu, wx.ID_ANY, _('Print display'))
        printmenu.AppendItem(doprint)
        self.Bind(wx.EVT_MENU, self.printopt.OnDoPrint, doprint)

        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(printmenu)
        printmenu.Destroy()

    def GetRender(self):
        """!Returns current instance of render.Map()
        """
        return self.Map

    def GetWindow(self):
        """!Get map window"""
        return self.MapWindow
    
    def FormatDist(self, dist):
        """!Format length numbers and units in a nice way,
        as a function of length. From code by Hamish Bowman
        Grass Development Team 2006"""

        mapunits = self.Map.projinfo['units']
        if mapunits == 'metres': mapunits = 'meters'
        outunits = mapunits
        dist = float(dist)
        divisor = 1.0

        # figure out which units to use
        if mapunits == 'meters':
            if dist > 2500.0:
                outunits = 'km'
                divisor = 1000.0
            else: outunits = 'm'
        elif mapunits == 'feet':
            # nano-bug: we match any "feet", but US Survey feet is really
            #  5279.9894 per statute mile, or 10.6' per 1000 miles. As >1000
            #  miles the tick markers are rounded to the nearest 10th of a
            #  mile (528'), the difference in foot flavours is ignored.
            if dist > 5280.0:
                outunits = 'miles'
                divisor = 5280.0
            else:
                outunits = 'ft'
        elif 'degree' in mapunits:
            if dist < 1:
                outunits = 'min'
                divisor = (1/60.0)
            else:
                outunits = 'deg'

        # format numbers in a nice way
        if (dist/divisor) >= 2500.0:
            outdist = round(dist/divisor)
        elif (dist/divisor) >= 1000.0:
            outdist = round(dist/divisor,1)
        elif (dist/divisor) > 0.0:
            outdist = round(dist/divisor,int(math.ceil(3-math.log10(dist/divisor))))
        else:
            outdist = float(dist/divisor)

        return (outdist, outunits)

    def OnZoomToMap(self, event):
        """!
        Set display extents to match selected raster (including NULLs)
        or vector map.
        """
        self.MapWindow.ZoomToMap(layers = self.Map.GetListOfLayers())

    def OnZoomToRaster(self, event):
        """!
        Set display extents to match selected raster map (ignore NULLs)
        """
        self.MapWindow.ZoomToMap(ignoreNulls = True)

    def OnZoomToWind(self, event):
        """!Set display geometry to match computational region
        settings (set with g.region)
        """
        self.MapWindow.ZoomToWind()
        
    def OnZoomToDefault(self, event):
        """!Set display geometry to match default region settings
        """
        self.MapWindow.ZoomToDefault()
        
    def OnZoomToSaved(self, event):
        """!Set display geometry to match extents in
        saved region file
        """
        self.MapWindow.ZoomToSaved()
        
    def OnDisplayToWind(self, event):
        """!Set computational region (WIND file) to match display
        extents
        """
        self.MapWindow.DisplayToWind()
 
    def SaveDisplayRegion(self, event):
        """!Save display extents to named region file.
        """
        self.MapWindow.SaveDisplayRegion()
        
    def OnZoomMenu(self, event):
        """!Popup Zoom menu
        """
        point = wx.GetMousePosition()
        zoommenu = wx.Menu()
        # Add items to the menu

        zoomwind = wx.MenuItem(zoommenu, wx.ID_ANY, _('Zoom to computational region (set with g.region)'))
        zoommenu.AppendItem(zoomwind)
        self.Bind(wx.EVT_MENU, self.OnZoomToWind, zoomwind)

        zoomdefault = wx.MenuItem(zoommenu, wx.ID_ANY, _('Zoom to default region'))
        zoommenu.AppendItem(zoomdefault)
        self.Bind(wx.EVT_MENU, self.OnZoomToDefault, zoomdefault)

        zoomsaved = wx.MenuItem(zoommenu, wx.ID_ANY, _('Zoom to saved region'))
        zoommenu.AppendItem(zoomsaved)
        self.Bind(wx.EVT_MENU, self.OnZoomToSaved, zoomsaved)

        savewind = wx.MenuItem(zoommenu, wx.ID_ANY, _('Set computational region from display'))
        zoommenu.AppendItem(savewind)
        self.Bind(wx.EVT_MENU, self.OnDisplayToWind, savewind)

        savezoom = wx.MenuItem(zoommenu, wx.ID_ANY, _('Save display geometry to named region'))
        zoommenu.AppendItem(savezoom)
        self.Bind(wx.EVT_MENU, self.SaveDisplayRegion, savezoom)

        # Popup the menu. If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(zoommenu)
        zoommenu.Destroy()
        
    def SetProperties(self, render=False, mode=0, showCompExtent=False,
                      constrainRes=False, projection=False):
        """!Set properies of map display window"""
        self.statusbarWin['render'].SetValue(render)
        self.statusbarWin['toggle'].SetSelection(mode)
        self.StatusbarUpdate()
        self.statusbarWin['region'].SetValue(showCompExtent)
        self.statusbarWin['resolution'].SetValue(constrainRes)
        if showCompExtent:
            self.MapWindow.regionCoords = []
        
    def IsStandalone(self):
        """!Check if Map display is standalone"""
        if self._layerManager:
            return False
        
        return True
    
    def GetLayerManager(self):
        """!Get reference to Layer Manager

        @return window reference
        @return None (if standalone)
        """
        return self._layerManager
    
# end of class MapFrame
