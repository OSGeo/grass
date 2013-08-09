"""!
@package gui_core.mapdisp

@brief Base classes for Map display window

Classes:
 - mapdisp::MapFrameBase
 - mapdisp::SingleMapFrame
 - mapdisp::DoubleMapFrame

(C) 2009-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Michael Barton <michael.barton@asu.edu>
@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os
import sys

import wx
import wx.aui

from core        import globalvar
from core.debug  import Debug
from core.utils import _
from gui_core.toolbars import ToolSwitcher

from grass.script import core as grass


class MapFrameBase(wx.Frame):
    """!Base class for map display window
    
    Derived class must use (create and initialize) \c statusbarManager
    or override
    GetProperty(), SetProperty() and HasProperty() methods.
    
    Several methods has to be overriden or
    \c NotImplementedError("MethodName") will be raised.
    
    If derived class enables and disables auto-rendering,
    it should override IsAutoRendered method.

    It is expected that derived class will call _setUpMapWindow().

    Derived class can has one or more map windows (and map renderes)
    but implementation of MapFrameBase expects that one window and
    one map will be current.
    Current instances of map window and map renderer should be returned
    by methods GetWindow() and GetMap() respectively.
    
    AUI manager is stored in \c self._mgr.
    """
    def __init__(self, parent = None, id = wx.ID_ANY, title = None,
                 style = wx.DEFAULT_FRAME_STYLE,
                 auimgr = None, name = None, **kwargs):
        """!

        @warning Use \a auimgr parameter only if you know what you are doing.
        
        @param parent gui parent
        @param id wx id
        @param title window title
        @param style \c wx.Frame style
        @param toolbars array of activated toolbars, e.g. ['map', 'digit']
        @param auimgr AUI manager (if \c None, wx.aui.AuiManager is used)
        @param name frame name
        @param kwargs arguments passed to \c wx.Frame
        """
        
        self.parent     = parent
        
        wx.Frame.__init__(self, parent, id, title, style = style, name = name, **kwargs)
                
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
        if auimgr == None:
            self._mgr = wx.aui.AuiManager(self)
        else:
            self._mgr = auimgr
        
        # handles switching between tools in different toolbars
        self._toolSwitcher = ToolSwitcher()
        self._toolSwitcher.toggleToolChanged.connect(self._onToggleTool)

    def _initMap(self, Map):
        """!Initialize map display, set dimensions and map region
        """
        if not grass.find_program('g.region', '--help'):
            sys.exit(_("GRASS module '%s' not found. Unable to start map "
                       "display window.") % 'g.region')
        
        Debug.msg(2, "MapFrame._initMap():")
        Map.ChangeMapSize(self.GetClientSize())
        Map.region = Map.GetRegion() # g.region -upgc
        # self.Map.SetRegion() # adjust region to match display window

    def _onToggleTool(self):
        self.GetWindow().UnregisterAllHandlers()

    def OnSize(self, event):
        """!Adjust statusbar on changing size"""
        # reposition checkbox in statusbar
        self.StatusbarReposition()
        
        # update statusbar
        self.StatusbarUpdate()

    def GetToolSwitcher(self):
        return self._toolSwitcher

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
            map = self.GetMap()
        
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
        """!Returns current map (renderer) instance"""
        raise NotImplementedError("GetMap")

    def GetWindow(self):
        """!Returns current map window"""
        raise NotImplementedError("GetWindow")

    def GetWindows(self):
        """!Returns list of map windows"""
        raise NotImplementedError("GetWindows")
        
    def GetMapToolbar(self):
        """!Returns toolbar with zooming tools"""
        raise NotImplementedError("GetMapToolbar")
       
    def GetToolbar(self, name):
        """!Returns toolbar if exists else None.
        
        Toolbars dictionary contains currently used toolbars only.
        """
        if name in self.toolbars:
            return self.toolbars[name]
        
        return None
       
    def StatusbarUpdate(self):
        """!Update statusbar content"""
        Debug.msg(5, "MapFrameBase.StatusbarUpdate()")
        self.statusbarManager.Update()
        
    def IsAutoRendered(self):
        """!Check if auto-rendering is enabled"""
        # TODO: this is now not the right place to access this attribute
        # TODO: add mapWindowProperties to init parameters
        # and pass the right object in the init of derived class?
        # or do not use this method at all, let mapwindow decide
        return self.mapWindowProperties.autoRender
        
    def CoordinatesChanged(self):
        """!Shows current coordinates on statusbar.
        """
        # assuming that the first mode is coordinates
        # probably shold not be here but good solution is not available now
        if self.statusbarManager.GetMode() == 0:
            self.statusbarManager.ShowItem('coordinates')
        
    def StatusbarReposition(self):
        """!Reposition items in statusbar"""
        self.statusbarManager.Reposition()
        
    def StatusbarEnableLongHelp(self, enable = True):
        """!Enable/disable toolbars long help"""
        for toolbar in self.toolbars.itervalues():
            toolbar.EnableLongHelp(enable)
        
    def IsStandalone(self):
        """!Check if map frame is standalone"""
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
        """!Zoom in the map."""
        self.MapWindow.SetModeZoomIn()
        
    def OnZoomOut(self, event):
        """!Zoom out the map."""
        self.MapWindow.SetModeZoomOut()

    def _setUpMapWindow(self, mapWindow):
        """Binds map windows' zoom history signals to map toolbar."""
        # enable or disable zoom history tool
        mapWindow.zoomHistoryAvailable.connect(
            lambda:
            self.GetMapToolbar().Enable('zoomBack', enable=True))
        mapWindow.zoomHistoryUnavailable.connect(
            lambda:
            self.GetMapToolbar().Enable('zoomBack', enable=False))
        mapWindow.mouseMoving.connect(self.CoordinatesChanged)

    def OnPointer(self, event):
        """!Sets mouse mode to pointer."""
        self.MapWindow.SetModePointer()

    def OnPan(self, event):
        """!Panning, set mouse to drag
        """
        self.MapWindow.SetModePan()

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


class SingleMapFrame(MapFrameBase):
    """! Frame with one map window.
    
    It is base class for frames which needs only one map.
    
    Derived class should have \c self.MapWindow or
    it has to override GetWindow() methods.
    
    @note To access maps use getters only
    (when using class or when writing class itself).
    """
    def __init__(self, parent = None, giface = None, id = wx.ID_ANY, title = None,
                 style = wx.DEFAULT_FRAME_STYLE,
                 Map = None,
                 auimgr = None, name = None, **kwargs):
        """!
        
        @param parent gui parent
        @param id wx id
        @param title window title
        @param style \c wx.Frame style
        @param Map instance of render.Map
        @param name frame name
        @param kwargs arguments passed to MapFrameBase
        """

        MapFrameBase.__init__(self, parent = parent, id = id, title = title,
                              style = style,
                              auimgr = auimgr, name = name, **kwargs)

        self.Map = Map       # instance of render.Map

        #
        # initialize region values
        #
        self._initMap(Map = self.Map)
        
    def GetMap(self):
        """!Returns map (renderer) instance"""
        return self.Map

    def GetWindow(self):
        """!Returns map window"""
        return self.MapWindow
        
    def GetWindows(self):
        """!Returns list of map windows"""
        return [self.MapWindow]

    def OnRender(self, event):
        """!Re-render map composition (each map layer)
        """
        self.GetWindow().UpdateMap(render = True, renderVector = True)
        
        # update statusbar
        self.StatusbarUpdate()
        

class DoubleMapFrame(MapFrameBase):
    """! Frame with two map windows.
    
    It is base class for frames which needs two maps.
    There is no primary and secondary map. Both maps are equal.
    However, one map is current.
    
    It is expected that derived class will call _bindWindowsActivation()
    when both map windows will be initialized.
    
    Drived class should have method GetMapToolbar() returns toolbar
    which has methods SetActiveMap() and Enable().
    
    @note To access maps use getters only
    (when using class or when writing class itself).
    
    @todo Use it in GCP manager
    (probably changes to both DoubleMapFrame and GCP MapFrame will be neccessary).
    """
    def __init__(self, parent = None, id = wx.ID_ANY, title = None,
                 style = wx.DEFAULT_FRAME_STYLE,
                 firstMap = None, secondMap = None,
                 auimgr = None, name = None, **kwargs):
        """!
        
        \a firstMap is set as active (by assign it to \c self.Map).
        Derived class should assging to \c self.MapWindow to make one
        map window current by dafault.
        
        @param parent gui parent
        @param id wx id
        @param title window title
        @param style \c wx.Frame style
        @param name frame name
        @param kwargs arguments passed to MapFrameBase
        """
        
        MapFrameBase.__init__(self, parent = parent, id = id, title = title,
                              style = style,
                              auimgr = auimgr, name = name, **kwargs)
        
        self.firstMap = firstMap
        self.secondMap = secondMap
        self.Map = firstMap

        #
        # initialize region values
        #
        self._initMap(Map = self.firstMap)
        self._initMap(Map = self.secondMap)
        self._bindRegions = False
    
    def _bindWindowsActivation(self):
        self.GetFirstWindow().Bind(wx.EVT_ENTER_WINDOW, self.ActivateFirstMap)
        self.GetSecondWindow().Bind(wx.EVT_ENTER_WINDOW, self.ActivateSecondMap)
    
    def _onToggleTool(self):
        self.GetFirstWindow().UnregisterAllHandlers()
        self.GetSecondWindow().UnregisterAllHandlers()

    def GetFirstMap(self):
        """!Returns first Map instance
        """
        return self.firstMap
        
    def GetSecondMap(self):
        """!Returns second Map instance
        """
        return self.secondMap
        
    def GetFirstWindow(self):
        """!Get first map window"""
        return self.firstMapWindow
    
    def GetSecondWindow(self):
        """!Get second map window"""
        return self.secondMapWindow
    
    def GetMap(self):
        """!Returns current map (renderer) instance
        
        @note Use this method to access current map renderer.
        (It is not guarented that current map will be stored in
        \c self.Map in future versions.)
        """
        return self.Map
    
    def GetWindow(self):
        """!Returns current map window
        
        @see GetMap()
        """
        return self.MapWindow
    
    def GetWindows(self):
        """!Return list of all windows"""
        return [self.firstMapWindow, self.secondMapWindow]
    
    def ActivateFirstMap(self, event = None):
        """!Make first Map and MapWindow active and (un)bind regions of the two Maps."""
        if self.MapWindow == self.firstMapWindow:
            return

        self.Map = self.firstMap
        self.MapWindow = self.firstMapWindow
        self.GetMapToolbar().SetActiveMap(0)

        # bind/unbind regions
        if self._bindRegions:
            self.firstMapWindow.zoomChanged.connect(self.OnZoomChangedFirstMap)
            self.secondMapWindow.zoomChanged.disconnect(self.OnZoomChangedSecondMap)

    def ActivateSecondMap(self, event = None):
        """!Make second Map and MapWindow active and (un)bind regions of the two Maps."""
        if self.MapWindow == self.secondMapWindow:
            return        

        self.Map = self.secondMap
        self.MapWindow = self.secondMapWindow
        self.GetMapToolbar().SetActiveMap(1)

        if self._bindRegions:
            self.secondMapWindow.zoomChanged.connect(self.OnZoomChangedSecondMap)
            self.firstMapWindow.zoomChanged.disconnect(self.OnZoomChangedFirstMap)

    def SetBindRegions(self, on):
        """!Set or unset binding display regions."""
        self._bindRegions = on

        if on:
            if self.MapWindow == self.firstMapWindow:
                self.firstMapWindow.zoomChanged.connect(self.OnZoomChangedFirstMap)
            else:
                self.secondMapWindow.zoomChanged.connect(self.OnZoomChangedSecondMap)
        else:
            if self.MapWindow == self.firstMapWindow:
                self.firstMapWindow.zoomChanged.disconnect(self.OnZoomChangedFirstMap)
            else:
                self.secondMapWindow.zoomChanged.disconnect(self.OnZoomChangedSecondMap)

    def OnZoomChangedFirstMap(self):
        """!Display region of the first window (Map) changed.

        Synchronize the region of the second map and re-render it.
        This is the default implementation which can be overridden.
        """
        region = self.GetFirstMap().GetCurrentRegion()
        self.GetSecondMap().region.update(region)
        self.Render(mapToRender = self.GetSecondWindow())

    def OnZoomChangedSecondMap(self):
        """!Display region of the second window (Map) changed.

        Synchronize the region of the second map and re-render it.
        This is the default implementation which can be overridden.
        """
        region = self.GetSecondMap().GetCurrentRegion()
        self.GetFirstMap().region.update(region)
        self.Render(mapToRender = self.GetFirstWindow())

    def OnZoomIn(self, event):
        """!Zoom in the map."""
        self.GetFirstWindow().SetModeZoomIn()
        self.GetSecondWindow().SetModeZoomIn()

    def OnZoomOut(self, event):
        """!Zoom out the map."""
        self.GetFirstWindow().SetModeZoomOut()
        self.GetSecondWindow().SetModeZoomOut()
        
    def OnPan(self, event):
        """!Panning, set mouse to pan"""
        self.GetFirstWindow().SetModePan()
        self.GetSecondWindow().SetModePan()

    def OnPointer(self, event):
        """!Set pointer mode (dragging overlays)"""
        self.GetFirstWindow().SetModePointer()
        self.GetSecondWindow().SetModePointer()

    def OnRender(self, event):
        """!Re-render map composition (each map layer)
        """
        self.Render(mapToRender = self.GetFirstWindow())
        self.Render(mapToRender = self.GetSecondWindow())
    
    def Render(self, mapToRender):
        """!Re-render map composition"""
        mapToRender.UpdateMap(render = True,
                              renderVector = mapToRender == self.GetFirstWindow())
        
        # update statusbar
        self.StatusbarUpdate()
        
    def OnErase(self, event):
        """!Erase the canvas
        """
        self.Erase(mapToErase = self.GetFirstWindow())
        self.Erase(mapToErase = self.GetSecondWindow())
        
    def Erase(self, mapToErase):
        """!Erase the canvas
        """
        mapToErase.EraseMap()
        
    def OnDraw(self, event):
        """!Re-display current map composition
        """
        self.Draw(mapToDraw = self.GetFirstWindow())
        self.Draw(mapToDraw = self.GetSecondWindow())
        
    def Draw(self, mapToDraw):
        """!Re-display current map composition
        """
        mapToDraw.UpdateMap(render = False)
