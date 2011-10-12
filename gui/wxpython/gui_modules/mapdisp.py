"""!
@package mapdisp.py

@brief Map display with toolbar for various display management
functions, and additional toolbars (vector digitizer, 3d view).

Can be used either from Layer Manager or as d.mon backend.

Classes:
 - MapFrameBase
 - MapFrame
 - MapApp

Usage:
python mapdisp.py monitor-identifier /path/to/map/file /path/to/command/file /path/to/env/file

(C) 2006-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com> (MapFrameBase)
@author Anna Kratochvilova <kratochanna gmail.com> (MapFrameBase)
"""

import os
import sys
import glob
import math
import tempfile
import copy

import globalvar
import wx
import wx.aui

sys.path.append(os.path.join(globalvar.ETCWXDIR, "icons"))
sys.path.append(os.path.join(globalvar.ETCDIR,   "python"))

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
import mapdisp_statusbar as sb
from debug       import Debug
from icon        import Icons
from preferences import globalSettings as UserSettings

from mapdisp_window  import BufferedWindow
from histogram       import HistFrame
from wxplot          import HistFrame as HistFramePyPlot, ProfileFrame, ScatterFrame

from grass.script import core as grass

# for standalone app
monFile = { 'cmd' : None,
            'map' : None,
            'env' : None,
            }
monName = None
monSize = list(globalvar.MAP_WINDOW_SIZE)

haveCtypes = False

class MapFrameBase(wx.Frame):
    """!Base class for map display window
    
    Derived class must use statusbarManager or override
    GetProperty, SetProperty and HasProperty methods.
    If derived class enables and disables auto-rendering,
    it should override IsAutoRendered method.
    """
    def __init__(self, parent = None, id = wx.ID_ANY, title = None,
                 style = wx.DEFAULT_FRAME_STYLE, toolbars = None,
                 Map = None, auimgr = None, name = None, **kwargs):
        """!
        @param toolbars array of activated toolbars, e.g. ['map', 'digit']
        @param Map instance of render.Map
        @param auimgs AUI manager
        @param name frame name
        @param kwargs wx.Frame attributes
        """
        

        self.Map        = Map       # instance of render.Map
        self.parent     = parent
        
        wx.Frame.__init__(self, parent, id, title, style = style, name = name, **kwargs)
        
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
        
        # toolbars
        self.toolbars = {}
        
        #
        # Fancy gui
        #
        self._mgr = wx.aui.AuiManager(self)
        
    def _initMap(self, map):
        """!Initialize map display, set dimensions and map region
        """
        if not grass.find_program('g.region', ['--help']):
            sys.exit(_("GRASS module '%s' not found. Unable to start map "
                       "display window.") % 'g.region')
        
        self.width, self.height = self.GetClientSize()
        
        Debug.msg(2, "MapFrame._initMap():")
        map.ChangeMapSize(self.GetClientSize())
        map.region = map.GetRegion() # g.region -upgc
        # self.Map.SetRegion() # adjust region to match display window
        
    def SetProperty(self, name, value):
        """!Sets property"""
        self.statusbarManager.SetProperty(name, value)
        
    def GetProperty(self, name):
        """!Returns property"""
        return self.statusbarManager.GetProperty(name)
        
    def HasProperty(self, name):
        """!Checks whether object has property"""
        return self.statusbarManager.HasProperty(name)
    
    def GetPPM(self):
        """! Get pixel per meter
        
        @todo now computed every time, is it necessary?
        @todo enable user to specify ppm (and store it in UserSettings)
        """
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
        ppm = ((ppi[0] / 2.54) * 100, # pixel per meter
                    (ppi[1] / 2.54) * 100)
        
        Debug.msg(4, "MapFrameBase.GetPPM(): size: px=%d,%d mm=%f,%f "
                  "in=%f,%f ppi: sys=%d,%d com=%d,%d; ppm=%f,%f" % \
                  (dpSizePx[0], dpSizePx[1], dpSizeMM[0], dpSizeMM[1],
                   dpSizeIn[0], dpSizeIn[1],
                   sysPpi[0], sysPpi[1], comPpi[0], comPpi[1],
                   ppm[0], ppm[1]))
        
        return ppm
    
    def SetMapScale(self, value, map = None):
        """! Set current map scale
        
        @param value scale value (n if scale is 1:n)
        @param map Map instance (if none self.Map is used)
        """
        if not map:
            map = self.Map
        
        region = self.Map.region
        dEW = value * (region['cols'] / self.GetPPM()[0])
        dNS = value * (region['rows'] / self.GetPPM()[1])
        region['n'] = region['center_northing'] + dNS / 2.
        region['s'] = region['center_northing'] - dNS / 2.
        region['w'] = region['center_easting']  - dEW / 2.
        region['e'] = region['center_easting']  + dEW / 2.
        
        # add to zoom history
        self.GetWindow().ZoomHistory(region['n'], region['s'],
                                   region['e'], region['w'])
    
    def GetMapScale(self, map = None):
        """! Get current map scale
        
        @param map Map instance (if none self.Map is used)
        """
        if not map:
            map = self.Map
        
        region = map.region
        ppm = self.GetPPM()

        heightCm = region['rows'] / ppm[1] * 100
        widthCm  = region['cols'] / ppm[0] * 100

        Debug.msg(4, "MapFrame.GetMapScale(): width_cm=%f, height_cm=%f" %
                  (widthCm, heightCm))

        xscale = (region['e'] - region['w']) / (region['cols'] / ppm[0])
        yscale = (region['n'] - region['s']) / (region['rows'] / ppm[1])
        scale = (xscale + yscale) / 2.
        
        Debug.msg(3, "MapFrame.GetMapScale(): xscale=%f, yscale=%f -> scale=%f" % \
                      (xscale, yscale, scale))
        
        return scale
        
    def GetProgressBar(self):
        """!Returns progress bar
        
        Progress bar can be used by other classes.
        """
        return self.statusbarManager.GetProgressBar()
        
    def GetRender(self):
        """!Returns current instance of render.Map()
        
        @todo make this method obsolate (name GetMap is better)
        """
        return self.Map

    def GetMap(self):
        """!Returns current Map instance
        """
        return self.Map

    def GetWindow(self):
        """!Get map window"""
        return self.MapWindow
        
    def GetMapToolbar(self):
       """!Returns toolbar with zooming tools"""
       raise NotImplementedError()
       
    def GetToolbar(self, name):
        """!Returns toolbar if exists else None.
        
        Toolbars dictionary contains currently used toolbars only.
        """
        if name in self.toolbars:
            return self.toolbars[name]
        
        return None
       
    def StatusbarUpdate(self):
        """!Update statusbar content"""
        self.statusbarManager.Update()
        
    def IsAutoRendered(self):
        """!Check if auto-rendering is enabled"""
        return self.GetProperty('render')
        
    def CoordinatesChanged(self):
        """!Shows current coordinates on statusbar.
        
        Used in BufferedWindow to report change of map coordinates (under mouse cursor).
        """
        self.statusbarManager.ShowItem('coordinates')
        
    def StatusbarReposition(self):
        """!Reposition items in statusbar"""
        self.statusbarManager.Reposition()
        
    def StatusbarEnableLongHelp(self, enable = True):
        """!Enable/disable toolbars long help"""
        for toolbar in self.toolbars.itervalues():
            toolbar.EnableLongHelp(enable)
        
    def IsStandalone(self):
        """!Check if Map display is standalone"""
        raise NotImplementedError("IsStandalone")
   
    def OnRender(self, event):
        """!Re-render map composition (each map layer)
        """
        raise NotImplementedError("OnRender")
        
    def OnDraw(self, event):
        """!Re-display current map composition
        """
        self.MapWindow.UpdateMap(render = False)
        
    def OnErase(self, event):
        """!Erase the canvas
        """
        self.MapWindow.EraseMap()
        
    def OnZoomIn(self, event):
        """!Zoom in the map.
        Set mouse cursor, zoombox attributes, and zoom direction
        """
        toolbar = self.GetMapToolbar()
        self._switchTool(toolbar, event)
        
        win = self.GetWindow()
        self._prepareZoom(mapWindow = win, zoomType = 1)
        
    def OnZoomOut(self, event):
        """!Zoom out the map.
        Set mouse cursor, zoombox attributes, and zoom direction
        """
        toolbar = self.GetMapToolbar()
        self._switchTool(toolbar, event)
        
        win = self.GetWindow()
        self._prepareZoom(mapWindow = win, zoomType = -1)
        
    def _prepareZoom(self, mapWindow, zoomType):
        """!Prepares MapWindow for zoom, toggles toolbar
        
        @param mapWindow MapWindow to prepare
        @param zoomType 1 for zoom in, -1 for zoom out
        """
        mapWindow.mouse['use'] = "zoom"
        mapWindow.mouse['box'] = "box"
        mapWindow.zoomtype = zoomType
        mapWindow.pen = wx.Pen(colour = 'Red', width = 2, style = wx.SHORT_DASH)
        
        # change the cursor
        mapWindow.SetCursor(self.cursors["cross"])
    
    def _switchTool(self, toolbar, event):
        """!Helper function to switch tools"""
        if toolbar:
            toolbar.OnTool(event)
            toolbar.action['desc'] = ''
            
    def OnPan(self, event):
        """!Panning, set mouse to drag
        """
        toolbar = self.GetMapToolbar()
        self._switchTool(toolbar, event)
        
        win = self.GetWindow()
        self._preparePan(mapWindow = win)
    
    def _preparePan(self, mapWindow):
        """!Prepares MapWindow for pan, toggles toolbar
        
        @param mapWindow MapWindow to prepare
        """
        mapWindow.mouse['use'] = "pan"
        mapWindow.mouse['box'] = "pan"
        mapWindow.zoomtype = 0
        
        # change the cursor
        mapWindow.SetCursor(self.cursors["hand"])
        
    def OnZoomBack(self, event):
        """!Zoom last (previously stored position)
        """
        self.MapWindow.ZoomBack()
        
    def OnZoomToMap(self, event):
        """!
        Set display extents to match selected raster (including NULLs)
        or vector map.
        """
        self.MapWindow.ZoomToMap(layers = self.Map.GetListOfLayers())
    
    def OnZoomToWind(self, event):
        """!Set display geometry to match computational region
        settings (set with g.region)
        """
        self.MapWindow.ZoomToWind()
        
    def OnZoomToDefault(self, event):
        """!Set display geometry to match default region settings
        """
        self.MapWindow.ZoomToDefault()
        
class MapFrame(MapFrameBase):
    """!Main frame for map display window. Drawing takes place in
    child double buffered drawing window.
    """
    def __init__(self, parent = None, title = _("GRASS GIS - Map display"),
                 toolbars = ["map"], tree = None, notebook = None, lmgr = None,
                 page = None, Map = None, auimgr = None, name = 'MapWindow', **kwargs):
        """!Main map display window with toolbars, statusbar and
        BufferedWindow (map canvas)
        
        @param toolbars array of activated toolbars, e.g. ['map', 'digit']
        @param tree reference to layer tree
        @param notebook control book ID in Layer Manager
        @param lmgr Layer Manager
        @param page notebook page with layer tree
        @param Map instance of render.Map
        @param auimgs AUI manager
        @param name frame name
        @param kwargs wx.Frame attributes
        """
        MapFrameBase.__init__(self, parent = parent, title = title, toolbars = toolbars,
                              Map = Map, auimgr = auimgr, name = name, **kwargs)
        
        self._layerManager = lmgr   # Layer Manager object
        self.tree       = tree      # Layer Manager layer tree object
        self.page       = page      # Notebook page holding the layer tree
        self.layerbook  = notebook  # Layer Manager layer tree notebook
        #
        # Add toolbars
        #
        for toolb in toolbars:
            self.AddToolbar(toolb)
        
        #
        # Add statusbar
        #
        
        # items for choice
        self.statusbarItems = [sb.SbCoordinates,
                               sb.SbRegionExtent,
                               sb.SbCompRegionExtent,
                               sb.SbShowRegion,
                               sb.SbAlignExtent,
                               sb.SbResolution,
                               sb.SbDisplayGeometry,
                               sb.SbMapScale,
                               sb.SbGoTo,
                               sb.SbProjection]
                            
        self.statusbarItemsHiddenInNviz = (sb.SbAlignExtent,
                                           sb.SbDisplayGeometry,
                                           sb.SbShowRegion,
                                           sb.SbResolution,
                                           sb.SbMapScale)
        
        # create statusbar and its manager
        statusbar = self.CreateStatusBar(number = 4, style = 0)
        statusbar.SetStatusWidths([-5, -2, -1, -1])
        self.statusbarManager = sb.SbManager(mapframe = self, statusbar = statusbar)
        
        # fill statusbar manager
        self.statusbarManager.AddStatusbarItemsByClass(self.statusbarItems, mapframe = self, statusbar = statusbar)
        self.statusbarManager.AddStatusbarItem(sb.SbMask(self, statusbar = statusbar, position = 2))
        self.statusbarManager.AddStatusbarItem(sb.SbRender(self, statusbar = statusbar, position = 3))
        
        self.statusbarManager.Update()

        #
        # Init map display (buffered DC & set default cursor)
        #
        self.MapWindow2D = BufferedWindow(self, id = wx.ID_ANY,
                                          Map = self.Map, tree = self.tree, lmgr = self._layerManager)
        # default is 2D display mode
        self.MapWindow = self.MapWindow2D
        self.MapWindow.SetCursor(self.cursors["default"])
        # used by vector digitizer
        self.MapWindowVDigit = None
        # used by Nviz (3D display mode)
        self.MapWindow3D = None 

        #
        # initialize region values
        #
        self._initMap(map = self.Map) 

        #
        # Bind various events
        #
        self.Bind(wx.EVT_ACTIVATE, self.OnFocus)
        self.Bind(wx.EVT_CLOSE,    self.OnCloseWindow)
        self.Bind(render.EVT_UPDATE_PRGBAR, self.OnUpdateProgress)
        
        #
        # Update fancy gui style
        #
        self._mgr.AddPane(self.MapWindow, wx.aui.AuiPaneInfo().CentrePane().
                          Dockable(False).BestSize((-1,-1)).Name('2d').
                          CloseButton(False).DestroyOnClose(True).
                          Layer(0))
        self._mgr.Update()

        #
        # Init print module and classes
        #
        self.printopt = disp_print.PrintOptions(self, self.MapWindow)
        
        #
        # Init zoom history
        #
        self.MapWindow.ZoomHistory(self.Map.region['n'],
                                   self.Map.region['s'],
                                   self.Map.region['e'],
                                   self.Map.region['w'])

        #
        # Re-use dialogs
        #
        self.dialogs = {}
        self.dialogs['attributes'] = None
        self.dialogs['category'] = None
        self.dialogs['barscale'] = None
        self.dialogs['legend'] = None

        self.decorationDialog = None # decoration/overlays
        
        
    def _addToolbarVDigit(self):
        """!Add vector digitizer toolbar
        """
        from vdigit import haveVDigit
        
        if not haveVDigit:
            from vdigit import errorMsg
            msg = _("Unable to start wxGUI vector digitizer.\nDo you want to start "
                    "TCL/TK digitizer (v.digit) instead?\n\n"
                    "Details: %s" % errorMsg)
            
            self.toolbars['map'].combo.SetValue(_("2D view"))
            dlg = wx.MessageDialog(parent = self,
                                   message = msg,
                                   caption=_("Vector digitizer failed"),
                                   style = wx.YES_NO | wx.CENTRE)
            if dlg.ShowModal() == wx.ID_YES:
                mapName = self.tree.GetPyData(self.tree.layer_selected)[0]['maplayer'].GetName()
                self._layerManager.goutput.RunCmd(['v.digit', 'map=%s' % mapName],
                                                  switchPage = False)
            dlg.Destroy()
            
            self.toolbars['map'].combo.SetValue(_("2D view"))
            return
        
        if self._layerManager:
            log = self._layerManager.goutput
        else:
            log = None
        
        if not self.MapWindowVDigit:
            from mapdisp_vdigit import VDigitWindow
            self.MapWindowVDigit = VDigitWindow(self, id = wx.ID_ANY,
                                                Map = self.Map, tree = self.tree,
                                                lmgr = self._layerManager)
            self.MapWindowVDigit.Show()
            self._mgr.AddPane(self.MapWindowVDigit, wx.aui.AuiPaneInfo().CentrePane().
                          Dockable(False).BestSize((-1,-1)).Name('vdigit').
                          CloseButton(False).DestroyOnClose(True).
                          Layer(0))
        
        self.MapWindow = self.MapWindowVDigit
        
        if self._mgr.GetPane('2d').IsShown():
            self._mgr.GetPane('2d').Hide()
        elif self._mgr.GetPane('3d').IsShown():
            self._mgr.GetPane('3d').Hide()
        self._mgr.GetPane('vdigit').Show()
        self.toolbars['vdigit'] = toolbars.VDigitToolbar(parent = self, mapcontent = self.Map,
                                                         layerTree = self.tree,
                                                         log = log)
        self.MapWindowVDigit.SetToolbar(self.toolbars['vdigit'])
        
        self._mgr.AddPane(self.toolbars['vdigit'],
                          wx.aui.AuiPaneInfo().
                          Name("vdigittoolbar").Caption(_("Vector Digitizer Toolbar")).
                          ToolbarPane().Top().Row(1).
                          LeftDockable(False).RightDockable(False).
                          BottomDockable(False).TopDockable(True).
                          CloseButton(False).Layer(2).
                          BestSize((self.toolbars['vdigit'].GetBestSize())))
        # change mouse to draw digitized line
        self.MapWindow.mouse['box'] = "point"
        self.MapWindow.zoomtype     = 0
        self.MapWindow.pen          = wx.Pen(colour = 'red',   width = 2, style = wx.SOLID)
        self.MapWindow.polypen      = wx.Pen(colour = 'green', width = 2, style = wx.SOLID)

    def AddNviz(self):
        """!Add 3D view mode window
        """
        import nviz
        
        # check for GLCanvas and OpenGL
        if not nviz.haveNviz:
            self.toolbars['map'].combo.SetValue(_("2D view"))
            gcmd.GError(parent = self,
                        message = _("Unable to switch to 3D display mode.\nThe Nviz python extension "
                                    "was not found or loaded properly.\n"
                                    "Switching back to 2D display mode.\n\nDetails: %s" % nviz.errorMsg))
            return
        
        # disable 3D mode for other displays
        for page in range(0, self._layerManager.gm_cb.GetPageCount()):
            if self._layerManager.gm_cb.GetPage(page) != self._layerManager.curr_page:
                if '3D' in self._layerManager.gm_cb.GetPage(page).maptree.mapdisplay.toolbars['map'].combo.GetString(1):
                    self._layerManager.gm_cb.GetPage(page).maptree.mapdisplay.toolbars['map'].combo.Delete(1)
        self.toolbars['map'].Enable2D(False)
        # add rotate tool to map toolbar
        self.toolbars['map'].InsertTool((('rotate', Icons['nviz']['rotate'],
                                          self.OnRotate, wx.ITEM_CHECK,7),)) # 7 is position
        self.toolbars['map'].ChangeToolsDesc(mode2d = False)
        # update status bar
        
        self.statusbarManager.HideStatusbarChoiceItemsByClass(self.statusbarItemsHiddenInNviz)
        self.statusbarManager.SetMode(0)
        
        # erase map window
        self.MapWindow.EraseMap()
        
        self._layerManager.goutput.WriteCmdLog(_("Starting 3D view mode..."),
                                               switchPage = False)
        self.SetStatusText(_("Please wait, loading data..."), 0)
        
        # create GL window
        if not self.MapWindow3D:
            self.MapWindow3D = nviz.GLWindow(self, id = wx.ID_ANY,
                                             Map = self.Map, tree = self.tree, lmgr = self._layerManager)
            self.MapWindow = self.MapWindow3D
            self.MapWindow.SetCursor(self.cursors["default"])
            
            # add Nviz notebookpage
            self._layerManager.AddNvizTools()
            
            # switch from MapWindow to MapWindowGL
            self._mgr.GetPane('2d').Hide()
            self._mgr.AddPane(self.MapWindow3D, wx.aui.AuiPaneInfo().CentrePane().
                              Dockable(False).BestSize((-1,-1)).Name('3d').
                              CloseButton(False).DestroyOnClose(True).
                              Layer(0))
            
            self.MapWindow3D.OnPaint(None) # -> LoadData
            self.MapWindow3D.Show()
            self.MapWindow3D.ResetViewHistory()            
            self.MapWindow3D.UpdateView(None)
        else:
            self.MapWindow = self.MapWindow3D
            os.environ['GRASS_REGION'] = self.Map.SetRegion(windres = True)
            self.MapWindow3D.GetDisplay().Init()
            del os.environ['GRASS_REGION']
            
            # switch from MapWindow to MapWindowGL
            self._mgr.GetPane('2d').Hide()
            self._mgr.GetPane('3d').Show()
            
            # add Nviz notebookpage
            self._layerManager.AddNvizTools()
            self.MapWindow3D.ResetViewHistory()
            for page in ('view', 'light', 'fringe', 'constant', 'cplane'):
                self._layerManager.nviz.UpdatePage(page)
        
        self.SetStatusText("", 0)
        self._mgr.Update()
    
    def RemoveNviz(self):
        """!Restore 2D view"""
        self.toolbars['map'].RemoveTool(self.toolbars['map'].rotate)
        # update status bar
        self.statusbarManager.ShowStatusbarChoiceItemsByClass(self.statusbarItemsHiddenInNviz)
        self.statusbarManager.SetMode(UserSettings.Get(group = 'display',
                                                       key = 'statusbarMode',
                                                       subkey = 'selection'))
        self.SetStatusText(_("Please wait, unloading data..."), 0)
        self._layerManager.goutput.WriteCmdLog(_("Switching back to 2D view mode..."),
                                               switchPage = False)
        self.MapWindow3D.UnloadDataLayers(force = True)
        # switch from MapWindowGL to MapWindow
        self._mgr.GetPane('2d').Show()
        self._mgr.GetPane('3d').Hide()

        self.MapWindow = self.MapWindow2D
        # remove nviz notebook page
        self._layerManager.RemoveNvizTools()
        
        self.MapWindow.UpdateMap()
        self._mgr.Update()
        
    def AddToolbar(self, name):
        """!Add defined toolbar to the window
        
        Currently known toolbars are:
         - 'map'     - basic map toolbar
         - 'vdigit'  - vector digitizer
         - 'gcpdisp' - GCP Manager Display
        """
        # default toolbar
        if name == "map":
            self.toolbars['map'] = toolbars.MapToolbar(self, self.Map)
            
            self._mgr.AddPane(self.toolbars['map'],
                              wx.aui.AuiPaneInfo().
                              Name("maptoolbar").Caption(_("Map Toolbar")).
                              ToolbarPane().Top().Name('mapToolbar').
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(False).TopDockable(True).
                              CloseButton(False).Layer(2).
                              BestSize((self.toolbars['map'].GetBestSize())))
            
        # vector digitizer
        elif name == "vdigit":
            self._addToolbarVDigit()
        
        self._mgr.Update()
        
    def RemoveToolbar (self, name):
        """!Removes defined toolbar from the window

        @todo Only hide, activate by calling AddToolbar()
        """
        # cannot hide main toolbar
        if name == "map":
            return
        
        self._mgr.DetachPane(self.toolbars[name])
        self.toolbars[name].Destroy()
        self.toolbars.pop(name)
        
        if name == 'vdigit':
            self._mgr.GetPane('vdigit').Hide()
            self._mgr.GetPane('2d').Show()
            self.MapWindow = self.MapWindow2D
            
        self.toolbars['map'].combo.SetValue(_("2D view"))
        self.toolbars['map'].Enable2D(True)
        
        self._mgr.Update()
    
    def IsPaneShown(self, name):
        """!Check if pane (toolbar, mapWindow ...) of given name is currently shown"""
        if self._mgr.GetPane(name).IsOk():
            return self._mgr.GetPane(name).IsShown()
        return False
        
    def OnUpdateProgress(self, event):
        """!Update progress bar info
        """
        self.GetProgressBar().SetValue(event.value)
        
        event.Skip()
        
    def OnFocus(self, event):
        """!Change choicebook page to match display.
        """
        # change bookcontrol page to page associated with display
        if self.page:
            pgnum = self.layerbook.GetPageIndex(self.page)
            if pgnum > -1:
                self.layerbook.SetSelection(pgnum)
                self._layerManager.curr_page = self.layerbook.GetCurrentPage()
        
        event.Skip()
        
    def OnRender(self, event):
        """!Re-render map composition (each map layer)
        """
        # delete tmp map layers (queries)
        qlayer = self.Map.GetListOfLayers(l_name = globalvar.QUERYLAYER)
        for layer in qlayer:
            self.Map.DeleteLayer(layer)
        
        # delete tmp lines
        if self.MapWindow.mouse["use"] in ("measure",
                                           "profile"):
            self.MapWindow.polycoords = []
            self.MapWindow.ClearLines()
        
        # deselect features in vdigit
        if self.GetToolbar('vdigit'):
            self.MapWindow.digit.GetDisplay().SetSelected([])
            self.MapWindow.UpdateMap(render = True, renderVector = True)
        else:
            self.MapWindow.UpdateMap(render = True)
        
        # update statusbar
        self.StatusbarUpdate()

    def OnPointer(self, event):
        """!Pointer button clicked
        """
        if self.GetMapToolbar():
            if event:
                self.toolbars['map'].OnTool(event)
            self.toolbars['map'].action['desc'] = ''
        
        self.MapWindow.mouse['use'] = "pointer"
        self.MapWindow.mouse['box'] = "point"

        # change the cursor
        if self.GetToolbar('vdigit'):
            # digitization tool activated
            self.MapWindow.SetCursor(self.cursors["cross"])

            # reset mouse['box'] if needed
            if self.toolbars['vdigit'].GetAction() in ['addLine']:
                if self.toolbars['vdigit'].GetAction('type') in ['point', 'centroid']:
                    self.MapWindow.mouse['box'] = 'point'
                else: # line, boundary
                    self.MapWindow.mouse['box'] = 'line'
            elif self.toolbars['vdigit'].GetAction() in ['addVertex', 'removeVertex', 'splitLine',
                                                         'editLine', 'displayCats', 'queryMap',
                                                         'copyCats']:
                self.MapWindow.mouse['box'] = 'point'
            else: # moveLine, deleteLine
                self.MapWindow.mouse['box'] = 'box'
        
        else:
            self.MapWindow.SetCursor(self.cursors["default"])

    def OnRotate(self, event):
        """!Rotate 3D view
        """
        if self.GetMapToolbar():
            self.toolbars['map'].OnTool(event)
            self.toolbars['map'].action['desc'] = ''
        
        self.MapWindow.mouse['use'] = "rotate"
        
        # change the cursor
        self.MapWindow.SetCursor(self.cursors["hand"])

    def OnZoomRegion(self, event):
        """!Zoom to region
        """
        self.Map.getRegion()
        self.Map.getResolution()
        self.UpdateMap()
        # event.Skip()

    def OnAlignRegion(self, event):
        """!Align region
        """
        if not self.Map.alignRegion:
            self.Map.alignRegion = True
        else:
            self.Map.alignRegion = False
        # event.Skip()        
        
    def SaveToFile(self, event):
        """!Save map to image
        """
        if self.IsPaneShown('3d'):
            filetype = "PPM file (*.ppm)|*.ppm|TIF file (*.tif)|*.tif"
            ltype = [{ 'ext' : 'ppm', 'type' : 'ppm' },
                     { 'ext' : 'tif', 'type' : 'tif' }]
        else:
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
                            style = wx.SAVE | wx.FD_OVERWRITE_PROMPT)
        
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

    def OnCloseWindow(self, event):
        """!Window closed.
        Also close associated layer tree page
        """
        pgnum = None
        self.Map.Clean()
        
        # close edited map and 3D tools properly
        if self.GetToolbar('vdigit'):
            maplayer = self.toolbars['vdigit'].GetLayer()
            if maplayer:
                self.toolbars['vdigit'].OnExit()
        if self.IsPaneShown('3d'):
            self.RemoveNviz()
        
        if not self._layerManager:
            self.Destroy()
        elif self.page:
            pgnum = self.layerbook.GetPageIndex(self.page)
            if pgnum > -1:
                self.layerbook.DeletePage(pgnum)
    
    
    def QueryMap(self, x, y):
        """!Query raster or vector map layers by r/v.what
        
        @param x,y coordinates
        """
        # set query snap distance for v.what at map unit equivalent of 10 pixels
        qdist = 10.0 * ((self.Map.region['e'] - self.Map.region['w']) / self.Map.width)
        east, north = self.MapWindow.Pixel2Cell((x, y))
        
        if not self.IsStandalone():
            num = 0
            for layer in self.tree.GetSelections():
                ltype = self.tree.GetPyData(layer)[0]['maplayer'].GetType()
                if ltype in ('raster', 'rgb', 'his',
                             'vector', 'thememap', 'themechart'):
                    num += 1
            
            if num < 1:
                gcmd.GMessage(parent = self,
                              message = _('No raster or vector map layer selected for querying.'))
                return
        
        rast = list()
        vect = list()
        rcmd = ['r.what', '--v']
        vcmd = ['v.what', '--v']
        
        if self.IsStandalone():
            pass
        else:
            for layer in self.tree.GetSelections():
                ltype = self.tree.GetPyData(layer)[0]['maplayer'].GetType()
                dcmd = self.tree.GetPyData(layer)[0]['cmd']
                name, found = utils.GetLayerNameFromCmd(dcmd)
                
                if not found:
                    continue
                if ltype == 'raster':
                    rast.append(name)
                elif ltype in ('rgb', 'his'):
                    for iname in name.split('\n'):
                        rast.append(iname)
                elif ltype in ('vector', 'thememap', 'themechart'):
                    vect.append(name)
        # rasters are not queried this way in 3D, we don't want them now
        if self.IsPaneShown('3d'):
            rast = list()
        # use display region settings instead of computation region settings
        self.tmpreg = os.getenv("GRASS_REGION")
        os.environ["GRASS_REGION"] = self.Map.SetRegion(windres = False)
        
        # build query commands for any selected rasters and vectors
        if rast:
            rcmd.append('-f')
            rcmd.append('-n')
            rcmd.append('input=%s' % ','.join(rast))
            rcmd.append('east_north=%f,%f' % (float(east), float(north)))
        
        if vect:
            # check for vector maps open to be edited
            digitToolbar = self.toolbars['vdigit']
            if digitToolbar:
                lmap = digitToolbar.GetLayer().GetName()
                for name in vect:
                    if lmap == name:
                        self._layerManager.goutput.WriteWarning(_("Vector map <%s> "
                                                                  "opened for editing - skipped.") % map)
                        vect.remove(name)
            
            if len(vect) < 1:
                self._layerManager.goutput.WriteCmdLog(_("Nothing to query."))
                return
            
            vcmd.append('-a')
            vcmd.append('map=%s' % ','.join(vect))
            vcmd.append('layer=%s' % ','.join(['-1'] * len(vect)))
            vcmd.append('east_north=%f,%f' % (float(east), float(north)))
            vcmd.append('distance=%f' % float(qdist))
        
        Debug.msg(1, "QueryMap(): raster=%s vector=%s" % (','.join(rast),
                                                          ','.join(vect)))
        # parse query command(s)
        if not self.IsStandalone():
            if rast:
                self._layerManager.goutput.RunCmd(rcmd,
                                                  compReg = False,
                                                  onDone  =  self._QueryMapDone)
            if vect:
                self._layerManager.goutput.RunCmd(vcmd,
                                                  onDone = self._QueryMapDone)
        else:
            if rast:
                gcmd.RunCommand(rcmd)
            if vect:
                gcmd.RunCommand(vcmd)
        
    def _QueryMapDone(self, cmd, returncode):
        """!Restore settings after querying (restore GRASS_REGION)
        
        @param returncode command return code
        """
        if hasattr(self, "tmpreg"):
            if self.tmpreg:
                os.environ["GRASS_REGION"] = self.tmpreg
            elif 'GRASS_REGION' in os.environ:
                del os.environ["GRASS_REGION"]
        elif 'GRASS_REGION' in os.environ:
            del os.environ["GRASS_REGION"]
        
        if hasattr(self, "tmpreg"):
            del self.tmpreg
        
    def QueryVector(self, x, y):
        """!Query vector map layer features

        Attribute data of selected vector object are displayed in GUI dialog.
        Data can be modified (On Submit)
        """
        if not self.tree.layer_selected or \
                self.tree.GetPyData(self.tree.layer_selected)[0]['type'] != 'vector':
            gcmd.GMessage(parent = self,
                          message = _("No map layer selected for querying."))
            return
        
        posWindow = self.ClientToScreen((x + self.MapWindow.dialogOffset,
                                         y + self.MapWindow.dialogOffset))
        
        qdist = 10.0 * ((self.Map.region['e'] - self.Map.region['w']) /
                        self.Map.width)
        
        east, north = self.MapWindow.Pixel2Cell((x, y))
        
        mapName = self.tree.GetPyData(self.tree.layer_selected)[0]['maplayer'].name
        
        if self.tree.GetPyData(self.tree.layer_selected)[0]['maplayer'].GetMapset() != \
                grass.gisenv()['MAPSET']:
            mode = 'display'
        else:
            mode = 'update'
        
        if self.dialogs['attributes'] is None:
            dlg = dbm_dialogs.DisplayAttributesDialog(parent = self.MapWindow,
                                                      map = mapName,
                                                      query = ((east, north), qdist),
                                                      pos = posWindow,
                                                      action = mode)
            self.dialogs['attributes'] = dlg
        
        else:
            # selection changed?
            if not self.dialogs['attributes'].mapDBInfo or \
                    self.dialogs['attributes'].mapDBInfo.map != mapName:
                self.dialogs['attributes'].UpdateDialog(map = mapName, query = ((east, north), qdist),
                                                        action = mode)
            else:
                self.dialogs['attributes'].UpdateDialog(query = ((east, north), qdist),
                                                        action = mode)
        if not self.dialogs['attributes'].IsFound():
            self._layerManager.goutput.WriteLog(_('Nothing found.'))
        
        cats = self.dialogs['attributes'].GetCats()
        
        qlayer = None
        if not self.IsPaneShown('3d'):
            try:
                qlayer = self.Map.GetListOfLayers(l_name = globalvar.QUERYLAYER)[0]
            except IndexError:
                pass
        
        if self.dialogs['attributes'].mapDBInfo and cats:
            if not self.IsPaneShown('3d'):
                # highlight feature & re-draw map
                if qlayer:
                    qlayer.SetCmd(self.AddTmpVectorMapLayer(mapName, cats,
                                                            useId = False,
                                                            addLayer = False))
                else:
                    qlayer = self.AddTmpVectorMapLayer(mapName, cats, useId = False)
                
                # set opacity based on queried layer
                opacity = self.tree.GetPyData(self.tree.layer_selected)[0]['maplayer'].GetOpacity(float = True)
                qlayer.SetOpacity(opacity)
                
                self.MapWindow.UpdateMap(render = False, renderVector = False)
            if not self.dialogs['attributes'].IsShown():
                self.dialogs['attributes'].Show()
        else:
            if qlayer:
                self.Map.DeleteLayer(qlayer)
                self.MapWindow.UpdateMap(render = False, renderVector = False)
            if self.dialogs['attributes'].IsShown():
                self.dialogs['attributes'].Hide()
        
    def OnQuery(self, event):
        """!Query tools menu"""
        if self.GetMapToolbar():
            self.toolbars['map'].OnTool(event)
            action = self.toolbars['map'].GetAction()
            
        self.toolbars['map'].action['desc'] = 'queryMap'
        self.MapWindow.mouse['use'] = "query"
        
        if not self.IsStandalone():
            # switch to output console to show query results
            self._layerManager.notebook.SetSelectionByName('output')
        
        self.MapWindow.mouse['box'] = "point"
        self.MapWindow.zoomtype = 0
        
        # change the cursor
        self.MapWindow.SetCursor(self.cursors["cross"])
        
    def AddTmpVectorMapLayer(self, name, cats, useId = False, addLayer = True):
        """!Add temporal vector map layer to map composition

        @param name name of map layer
        @param useId use feature id instead of category 
        """
        # color settings from ATM
        color = UserSettings.Get(group = 'atm', key = 'highlight', subkey = 'color')
        colorStr = str(color[0]) + ":" + \
            str(color[1]) + ":" + \
            str(color[2])

        # icon used in vector display and its size
        icon = ''
        size = 0
        vparam = self.tree.GetPyData(self.tree.layer_selected)[0]['cmd']
        for p in vparam:
            if '=' in p:
                parg,pval = p.split('=')
                if parg == 'icon': icon = pval
                elif parg == 'size': size = int(pval)

        pattern = ["d.vect",
                   "map=%s" % name,
                   "color=%s" % colorStr,
                   "fcolor=%s" % colorStr,
                   "width=%d"  % UserSettings.Get(group = 'atm', key = 'highlight', subkey = 'width')]
        if icon != '':
            pattern.append('icon=%s' % icon)
        if size > 0:
            pattern.append('size=%i' % size)
        
        if useId:
            cmd = pattern
            cmd.append('-i')
            cmd.append('cats=%s' % str(cats))
        else:
            cmd = []
            for layer in cats.keys():
                cmd.append(copy.copy(pattern))
                lcats = cats[layer]
                cmd[-1].append("layer=%d" % layer)
                cmd[-1].append("cats=%s" % utils.ListOfCatsToRange(lcats))
        
        if addLayer:
            if useId:
                return self.Map.AddLayer(type = 'vector', name = globalvar.QUERYLAYER, command = cmd,
                                         l_active = True, l_hidden = True, l_opacity = 1.0)
            else:
                return self.Map.AddLayer(type = 'command', name = globalvar.QUERYLAYER, command = cmd,
                                         l_active = True, l_hidden = True, l_opacity = 1.0)
        else:
            return cmd

    def OnAnalyze(self, event):
        """!Analysis tools menu
        """
        point = wx.GetMousePosition()
        toolsmenu = wx.Menu()
        icons = Icons['displayWindow']
        
        # Add items to the menu
        measure = wx.MenuItem(toolsmenu, wx.ID_ANY, icons["measure"].GetLabel())
        measure.SetBitmap(icons["measure"].GetBitmap(self.iconsize))
        toolsmenu.AppendItem(measure)
        self.Bind(wx.EVT_MENU, self.OnMeasure, measure)
        
        profile = wx.MenuItem(toolsmenu, wx.ID_ANY, icons["profile"].GetLabel())
        profile.SetBitmap(icons["profile"].GetBitmap(self.iconsize))
        toolsmenu.AppendItem(profile)
        self.Bind(wx.EVT_MENU, self.OnProfile, profile)

        scatterplot = wx.MenuItem(toolsmenu, wx.ID_ANY, _("Create bivariate scatterplot of raster maps"))
        scatterplot.SetBitmap(icons["profile"].GetBitmap(self.iconsize))
        toolsmenu.AppendItem(scatterplot)
        self.Bind(wx.EVT_MENU, self.OnScatterplot, scatterplot)

        histogram2 = wx.MenuItem(toolsmenu, wx.ID_ANY, icons["histogram"].GetLabel())
        histogram2.SetBitmap(icons["histogram"].GetBitmap(self.iconsize))
        toolsmenu.AppendItem(histogram2)
        self.Bind(wx.EVT_MENU, self.OnHistogramPyPlot, histogram2)

        histogram = wx.MenuItem(toolsmenu, wx.ID_ANY, _("Create histogram with d.histogram"))
        histogram.SetBitmap(icons["histogram"].GetBitmap(self.iconsize))
        toolsmenu.AppendItem(histogram)
        self.Bind(wx.EVT_MENU, self.OnHistogram, histogram)

        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(toolsmenu)
        toolsmenu.Destroy()

    def OnMeasure(self, event):
        """!Init measurement routine that calculates map distance
        along transect drawn on map display
        """
        self.totaldist = 0.0 # total measured distance
        
        # switch Layer Manager to output console to show measure results
        self._layerManager.notebook.SetSelectionByName('output')
        
        # change mouse to draw line for measurement
        self.MapWindow.mouse['use'] = "measure"
        self.MapWindow.mouse['box'] = "line"
        self.MapWindow.zoomtype = 0
        self.MapWindow.pen     = wx.Pen(colour = 'red', width = 2, style = wx.SHORT_DASH)
        self.MapWindow.polypen = wx.Pen(colour = 'green', width = 2, style = wx.SHORT_DASH)
        
        # change the cursor
        self.MapWindow.SetCursor(self.cursors["pencil"])
        
        # initiating output
        style = self._layerManager.goutput.cmd_output.StyleWarning
        self._layerManager.goutput.WriteLog(_('Click and drag with left mouse button '
                                              'to measure.%s'
                                              'Double click with left button to clear.') % \
                                                (os.linesep), style)
        if self.Map.projinfo['proj'] != 'xy':
            units = self.Map.projinfo['units']
            self._layerManager.goutput.WriteCmdLog(_('Measuring distance') + ' ('
                                                   + units + '):')
        else:
            self._layerManager.goutput.WriteCmdLog(_('Measuring distance:'))
        
        if self.Map.projinfo['proj'] == 'll':
            try:
                import grass.lib.gis as gislib
                global haveCtypes
                haveCtypes = True

                gislib.G_begin_distance_calculations()
            except ImportError, e:
                self._layerManager.goutput.WriteWarning(_('Geodesic distance is not yet '
                                                          'supported by this tool.\n'
                                                          'Reason: %s' % e))
        
    def MeasureDist(self, beginpt, endpt):
        """!Calculate map distance from screen distance
        and print to output window
        """
        self._layerManager.notebook.SetSelectionByName('output')
        
        dist, (north, east) = self.MapWindow.Distance(beginpt, endpt)
        
        dist = round(dist, 3)
        d, dunits = self.FormatDist(dist)
        
        self.totaldist += dist
        td, tdunits = self.FormatDist(self.totaldist)
        
        strdist = str(d)
        strtotdist = str(td)
        
        if self.Map.projinfo['proj'] == 'xy' or 'degree' not in self.Map.projinfo['unit']:
            angle = int(math.degrees(math.atan2(north,east)) + 0.5)
            angle = 180 - angle
            if angle < 0:
                angle = 360 + angle
            
            mstring = '%s = %s %s\n%s = %s %s\n%s = %d %s\n%s' \
                % (_('segment'), strdist, dunits,
                   _('total distance'), strtotdist, tdunits,
                   _('bearing'), angle, _('deg'),
                   '-' * 60)
        else:
            mstring = '%s = %s %s\n%s = %s %s\n%s' \
                % (_('segment'), strdist, dunits,
                   _('total distance'), strtotdist, tdunits,
                   '-' * 60)
        
        self._layerManager.goutput.WriteLog(mstring)
        
        return dist

    def OnProfile(self, event):
        """!Init profile canvas and tools
        """
        raster = []
        if self.tree.layer_selected and \
                self.tree.GetPyData(self.tree.layer_selected)[0]['type'] == 'raster':
            raster.append(self.tree.GetPyData(self.tree.layer_selected)[0]['maplayer'].name)

        self.profile = ProfileFrame(self,
                                            id = wx.ID_ANY, pos = wx.DefaultPosition, size = (700,300),
                                            style = wx.DEFAULT_FRAME_STYLE, 
                                            rasterList = raster)
        self.profile.Show()
        # Open raster select dialog to make sure that a raster (and the desired raster)
        # is selected to be profiled
        self.profile.OnSelectRaster(None)

    def FormatDist(self, dist):
        """!Format length numbers and units in a nice way,
        as a function of length. From code by Hamish Bowman
        Grass Development Team 2006"""
        
        mapunits = self.Map.projinfo['units']
        if mapunits == 'metres':
            mapunits = 'meters'
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
        elif 'degree' in mapunits and \
                not haveCtypes:
            if dist < 1:
                outunits = 'min'
                divisor = (1/60.0)
            else:
                outunits = 'deg'
        else:
            outunits = 'meters'
        
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
    

    def OnHistogramPyPlot(self, event):
        """!Init PyPlot histogram display canvas and tools
        """
        raster = []

        for layer in self.tree.GetSelections():
            if self.tree.GetPyData(layer)[0]['maplayer'].GetType() != 'raster':
                continue
            raster.append(self.tree.GetPyData(layer)[0]['maplayer'].GetName())

        self.histogramPyPlot = HistFramePyPlot(self, id = wx.ID_ANY, 
                                                pos = wx.DefaultPosition, size = (700,300),
                                                style = wx.DEFAULT_FRAME_STYLE, 
                                                rasterList = raster)
        self.histogramPyPlot.Show()
        # Open raster select dialog to make sure that a raster (and the desired raster)
        # is selected to be histogrammed
        self.histogramPyPlot.OnSelectRaster(None)
        
    def OnScatterplot(self, event):
        """!Init PyPlot scatterplot display canvas and tools
        """
        raster = []

        for layer in self.tree.GetSelections():
            if self.tree.GetPyData(layer)[0]['maplayer'].GetType() != 'raster':
                continue
            raster.append(self.tree.GetPyData(layer)[0]['maplayer'].GetName())

        self.scatterplot = ScatterFrame(self, id = wx.ID_ANY, 
                                                pos = wx.DefaultPosition, size = (700,300),
                                                style = wx.DEFAULT_FRAME_STYLE, 
                                                rasterList = raster)
        self.scatterplot.Show()
        # Open raster select dialog to make sure that at least 2 rasters (and the desired rasters)
        # are selected to be plotted
        self.scatterplot.OnSelectRaster(None)

    def OnHistogram(self, event):
        """!Init histogram display canvas and tools
        """
        self.histogram = HistFrame(parent = self, id = wx.ID_ANY, size = globalvar.HIST_WINDOW_SIZE,
                                   style = wx.DEFAULT_FRAME_STYLE)
        
        # show new display
        self.histogram.Show()
        self.histogram.Refresh()
        self.histogram.Update()
       
    def OnDecoration(self, event):
        """!Decorations overlay menu
        """
        point = wx.GetMousePosition()
        decmenu = wx.Menu()
        icons = Icons['displayWindow']
        
        # Add items to the menu
        AddScale = wx.MenuItem(decmenu, wx.ID_ANY, icons["addBarscale"].GetLabel())
        AddScale.SetBitmap(icons["addBarscale"].GetBitmap(self.iconsize))
        decmenu.AppendItem(AddScale)
        self.Bind(wx.EVT_MENU, self.OnAddBarscale, AddScale)
        # temporary
        if self.IsPaneShown('3d'):
            AddScale.Enable(False)
            AddArrow = wx.MenuItem(decmenu, wx.ID_ANY, _("Add north arrow"))
            AddArrow.SetBitmap(icons["addBarscale"].GetBitmap(self.iconsize))
            decmenu.AppendItem(AddArrow)
            self.Bind(wx.EVT_MENU, self.OnAddArrow, AddArrow)
        
        AddLegend = wx.MenuItem(decmenu, wx.ID_ANY, icons["addLegend"].GetLabel())
        AddLegend.SetBitmap(icons["addLegend"].GetBitmap(self.iconsize))
        decmenu.AppendItem(AddLegend)
        self.Bind(wx.EVT_MENU, self.OnAddLegend, AddLegend)
        
        AddText = wx.MenuItem(decmenu, wx.ID_ANY, icons["addText"].GetLabel())
        AddText.SetBitmap(icons["addText"].GetBitmap(self.iconsize))
        decmenu.AppendItem(AddText)
        self.Bind(wx.EVT_MENU, self.OnAddText, AddText)
        
        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(decmenu)
        decmenu.Destroy()
        
    def OnAddBarscale(self, event):
        """!Handler for scale/arrow map decoration menu selection.
        """
        if self.dialogs['barscale']:
            return
        
        id = 0 # unique index for overlay layer

        # If location is latlon, only display north arrow (scale won't work)
        #        proj = self.Map.projinfo['proj']
        #        if proj == 'll':
        #            barcmd = 'd.barscale -n'
        #        else:
        #            barcmd = 'd.barscale'

        # decoration overlay control dialog
        self.dialogs['barscale'] = \
            gdialogs.DecorationDialog(parent = self, title = _('Scale and North arrow'),
                                      size = (350, 200),
                                      style = wx.DEFAULT_DIALOG_STYLE | wx.CENTRE,
                                      cmd = ['d.barscale', 'at=0,95'],
                                      ovlId = id,
                                      name = 'barscale',
                                      checktxt = _("Show/hide scale and North arrow"),
                                      ctrltxt = _("scale object"))

        self.dialogs['barscale'].CentreOnParent()
        ### dialog cannot be show as modal - in the result d.barscale is not selectable
        ### self.dialogs['barscale'].ShowModal()
        self.dialogs['barscale'].Show()
        self.MapWindow.mouse['use'] = 'pointer'

    def OnAddLegend(self, event):
        """!Handler for legend map decoration menu selection.
        """
        if self.dialogs['legend']:
            return
        
        id = 1 # index for overlay layer in render

        cmd = ['d.legend', 'at=5,50,2,5']
        if self.tree.layer_selected and \
                self.tree.GetPyData(self.tree.layer_selected)[0]['type'] == 'raster':
            cmd.append('map=%s' % self.tree.GetPyData(self.tree.layer_selected)[0]['maplayer'].name)

        # Decoration overlay control dialog
        self.dialogs['legend'] = \
            gdialogs.DecorationDialog(parent = self, title = ('Legend'),
                                      size = (350, 200),
                                      style = wx.DEFAULT_DIALOG_STYLE | wx.CENTRE,
                                      cmd = cmd,
                                      ovlId = id,
                                      name = 'legend',
                                      checktxt = _("Show/hide legend"),
                                      ctrltxt = _("legend object")) 

        self.dialogs['legend'].CentreOnParent() 
        ### dialog cannot be show as modal - in the result d.legend is not selectable
        ### self.dialogs['legend'].ShowModal()
        self.dialogs['legend'].Show()
        self.MapWindow.mouse['use'] = 'pointer'

    def OnAddText(self, event):
        """!Handler for text decoration menu selection.
        """
        if self.MapWindow.dragid > -1:
            id = self.MapWindow.dragid
            self.MapWindow.dragid = -1
        else:
            # index for overlay layer in render
            if len(self.MapWindow.textdict.keys()) > 0:
                id = max(self.MapWindow.textdict.keys()) + 1
            else:
                id = 101
        
        self.dialogs['text'] = gdialogs.TextLayerDialog(parent = self, ovlId = id, 
                                                        title = _('Add text layer'),
                                                        size = (400, 200))
        self.dialogs['text'].CenterOnParent()

        # If OK button pressed in decoration control dialog
        if self.dialogs['text'].ShowModal() == wx.ID_OK:
            text = self.dialogs['text'].GetValues()['text']
            active = self.dialogs['text'].GetValues()['active']
        
            # delete object if it has no text or is not active
            if text == '' or active == False:
                try:
                    self.MapWindow2D.pdc.ClearId(id)
                    self.MapWindow2D.pdc.RemoveId(id)
                    del self.MapWindow.textdict[id]
                    if self.IsPaneShown('3d'):
                        self.MapWindow3D.UpdateOverlays()
                        self.MapWindow.UpdateMap()
                    else:
                        self.MapWindow2D.UpdateMap(render = False, renderVector = False)
                except:
                    pass
                return

            
            self.MapWindow.textdict[id] = self.dialogs['text'].GetValues()
            
            if self.IsPaneShown('3d'):
                self.MapWindow3D.UpdateOverlays()
                self.MapWindow3D.UpdateMap()
            else:
                self.MapWindow2D.pdc.ClearId(id)
                self.MapWindow2D.pdc.SetId(id)
                self.MapWindow2D.UpdateMap(render = False, renderVector = False)
            
        self.MapWindow.mouse['use'] = 'pointer'
    
    def OnAddArrow(self, event):
        """!Handler for north arrow menu selection.
            Opens Appearance page of nviz notebook.
        """
        
        self._layerManager.nviz.SetPage('decoration')
        self.MapWindow3D.SetDrawArrow((70, 70))
        
    def GetOptData(self, dcmd, type, params, propwin):
        """!Callback method for decoration overlay command generated by
        dialog created in menuform.py
        """
        # Reset comand and rendering options in render.Map. Always render decoration.
        # Showing/hiding handled by PseudoDC
        self.Map.ChangeOverlay(ovltype = type, type = 'overlay', name = '', command = dcmd,
                               l_active = True, l_render = False)
        self.params[type] = params
        self.propwin[type] = propwin

    def OnZoomToMap(self, event):
        """!Set display extents to match selected raster (including
        NULLs) or vector map.
        """
        self.MapWindow.ZoomToMap()

    def OnZoomToRaster(self, event):
        """!Set display extents to match selected raster map (ignore NULLs)
        """
        self.MapWindow.ZoomToMap(ignoreNulls = True)
        
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

        zoomwind = wx.MenuItem(zoommenu, wx.ID_ANY, _('Zoom to computational region'))
        zoommenu.AppendItem(zoomwind)
        self.Bind(wx.EVT_MENU, self.OnZoomToWind, zoomwind)

        zoomdefault = wx.MenuItem(zoommenu, wx.ID_ANY, _('Zoom to default region'))
        zoommenu.AppendItem(zoomdefault)
        self.Bind(wx.EVT_MENU, self.OnZoomToDefault, zoomdefault)

        zoomsaved = wx.MenuItem(zoommenu, wx.ID_ANY, _('Zoom to saved region'))
        zoommenu.AppendItem(zoomsaved)
        self.Bind(wx.EVT_MENU, self.OnZoomToSaved, zoomsaved)

        savewind = wx.MenuItem(zoommenu, wx.ID_ANY, _('Set computational region from display extent'))
        zoommenu.AppendItem(savewind)
        self.Bind(wx.EVT_MENU, self.OnDisplayToWind, savewind)

        savezoom = wx.MenuItem(zoommenu, wx.ID_ANY, _('Save display geometry to named region'))
        zoommenu.AppendItem(savezoom)
        self.Bind(wx.EVT_MENU, self.SaveDisplayRegion, savezoom)

        # Popup the menu. If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(zoommenu)
        zoommenu.Destroy()

    def SetProperties(self, render = False, mode = 0, showCompExtent = False,
                      constrainRes = False, projection = False, alignExtent = True):
        """!Set properies of map display window"""
        self.SetProperty('render', render)
        self.statusbarManager.SetMode(mode)
        self.StatusbarUpdate()
        self.SetProperty('region', showCompExtent)
        self.SetProperty('alignExtent', alignExtent)
        self.SetProperty('resolution', constrainRes)
        self.SetProperty('projection', projection)
        
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
    
    def GetMapToolbar(self):
        """!Returns toolbar with zooming tools"""
        return self.toolbars['map']
    
class MapApp(wx.App):
    def OnInit(self):
        wx.InitAllImageHandlers()
        if __name__ == "__main__":
            self.cmdTimeStamp = os.path.getmtime(monFile['cmd'])
            Map = render.Map(cmdfile = monFile['cmd'], mapfile = monFile['map'],
                             envfile = monFile['env'], monitor = monName)
        else:
            Map = None
        
        self.mapFrm = MapFrame(parent = None, id = wx.ID_ANY, Map = Map,
                               size = monSize)
        # self.SetTopWindow(Map)
        self.mapFrm.Show()
        
        if __name__ == "__main__":
            self.timer = wx.PyTimer(self.watcher)
            #check each 0.5s
            global mtime
            mtime = 500
            self.timer.Start(mtime)
            
        return True
    
    def OnExit(self):
        if __name__ == "__main__":
            # stop the timer
            # self.timer.Stop()
            # terminate thread
            for f in monFile.itervalues():
                grass.try_remove(f)
            
    def watcher(self):
        """!Redraw, if new layer appears (check's timestamp of
        cmdfile)
        """
        # todo: events
        if os.path.getmtime(monFile['cmd']) > self.cmdTimeStamp:
            self.timer.Stop()
            self.cmdTimeStamp = os.path.getmtime(monFile['cmd'])
            self.mapFrm.OnDraw(None)
            self.timer.Start(mtime)
        
if __name__ == "__main__":
    # set command variable
    if len(sys.argv) < 5:
        print __doc__
        sys.exit(1)
    
    monName = sys.argv[1]
    monFile = { 'map' : sys.argv[2],
                'cmd' : sys.argv[3],
                'env' : sys.argv[4],
                }
    if len(sys.argv) >= 6:
        try:
            monSize[0] = int(sys.argv[5])
        except ValueError:
            pass
    
    if len(sys.argv) == 7:
        try:
            monSize[1] = int(sys.argv[6])
        except ValueError:
            pass
    
    import gettext
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)
    
    grass.verbose(_("Starting map display <%s>...") % (monName))

    gcmd.RunCommand('g.gisenv',
                    set = 'MONITOR_%s_PID=%d' % (monName, os.getpid()))
    
    gm_map = MapApp(0)
    # set title
    gm_map.mapFrm.SetTitle(_("GRASS GIS Map Display: " +
                             monName + 
                             " - Location: " + grass.gisenv()["LOCATION_NAME"]))
    
    gm_map.MainLoop()
    
    grass.verbose(_("Stopping map display <%s>...") % (monName))

    # clean up GRASS env variables
    env = grass.gisenv()
    env_name = 'MONITOR_%s' % monName
    for key in env.keys():
        if key.find(env_name) == 0:
            gcmd.RunCommand('g.gisenv',
                              set = '%s=' % key)
        if key == 'MONITOR' and env[key] == monName:
            gcmd.RunCommand('g.gisenv',
                            set = '%s=' % key)
    
    sys.exit(0)
