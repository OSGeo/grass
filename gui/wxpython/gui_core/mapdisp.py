"""!
@package gui_core.mapdisp

@brief Base classes for Map display window

Classes:
 - mapdisp::MapFrameBase

(C) 2009-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Michael Barton <michael.barton@asu.edu>
"""

import os

import wx

from core       import globalvar
from core.debug import Debug

from grass.script import core as grass

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
