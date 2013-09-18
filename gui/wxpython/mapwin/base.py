"""!
@package mapwin.mapwindow

@brief Map display canvas basic functionality - base class and properties.

Classes:
 - mapwindow::MapWindowProperties
 - mapwindow::MapWindowBase

(C) 2006-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Michael Barton
@author Jachym Cepicky
@author Vaclav Petras <wenzeslaus gmail.com> (handlers support)
@author Stepan Turek <stepan.turek seznam.cz> (handlers support)
"""

import wx

from core.settings import UserSettings
from core.gcmd     import GError
from core.utils import _

from grass.script import core as grass
from grass.pydispatch.signal import Signal


class MapWindowProperties(object):
    def __init__(self):
        self._resolution = None
        self.resolutionChanged = Signal('MapWindowProperties.resolutionChanged')
        self._autoRender = None
        self.autoRenderChanged = Signal('MapWindowProperties.autoRenderChanged')
        self._showRegion = None
        self.showRegionChanged = Signal('MapWindowProperties.showRegionChanged')
        self._alignExtent = None
        self.alignExtentChanged = Signal('MapWindowProperties.alignExtentChanged')

    def setValuesFromUserSettings(self):
        """Convenient function to get values from user settings into this object."""
        self._resolution = UserSettings.Get(group='display',
                                            key='compResolution',
                                            subkey='enabled')
        self._autoRender = UserSettings.Get(group='display',
                                            key='autoRendering',
                                            subkey='enabled')
        self._showRegion = False  # in statusbar.py was not from settings
        self._alignExtent = UserSettings.Get(group='display',
                                             key='alignExtent',
                                             subkey='enabled')
    @property
    def resolution(self):
        return self._resolution

    @resolution.setter
    def resolution(self, value):
        if value != self._resolution:
            self._resolution = value
            self.resolutionChanged.emit(value=value)

    @property
    def autoRender(self):
        return self._autoRender

    @autoRender.setter
    def autoRender(self, value):
        if value != self._autoRender:
            self._autoRender = value
            self.autoRenderChanged.emit(value=value)

    @property
    def showRegion(self):
        return self._showRegion

    @showRegion.setter
    def showRegion(self, value):
        if value != self._showRegion:
            self._showRegion = value
            self.showRegionChanged.emit(value=value)

    @property
    def alignExtent(self):
        return self._alignExtent

    @alignExtent.setter
    def alignExtent(self, value):
        if value != self._alignExtent:
            self._alignExtent = value
            self.alignExtentChanged.emit(value=value)


class MapWindowBase(object):
    """!Abstract map display window class
    
    Superclass for BufferedWindow class (2D display mode), and GLWindow
    (3D display mode).
    
    Subclasses have to define
     - _bindMouseEvents method which binds MouseEvent handlers
     - Pixel2Cell
     - Cell2Pixel (if it is possible)
    """
    def __init__(self, parent, giface, Map):
        self.parent = parent
        self.Map = Map
        self._giface = giface

        # Emitted when someone registers as mouse event handler
        self.mouseHandlerRegistered = Signal('MapWindow.mouseHandlerRegistered')
        # Emitted when mouse event handler is unregistered
        self.mouseHandlerUnregistered = Signal('MapWindow.mouseHandlerUnregistered')
        # emitted after double click in pointer mode on legend, text, scalebar
        self.overlayActivated = Signal('MapWindow.overlayActivated')
        # emitted when overlay should be hidden
        self.overlayHidden = Signal('MapWindow.overlayHidden')

        # mouse attributes -- position on the screen, begin and end of
        # dragging, and type of drawing
        self.mouse = {
            'begin': [0, 0], # screen coordinates
            'end'  : [0, 0],
            'use'  : "pointer",
            'box'  : "point"
            }
        # last east, north coordinates, changes on mouse motion
        self.lastEN = None 
        
        # stores overridden cursor
        self._overriddenCursor = None

        # dictionary where event types are stored as keys and lists of
        # handlers for these types as values
        self.handlersContainer = {
            wx.EVT_LEFT_DOWN : [],
            wx.EVT_LEFT_UP : [],
            wx.EVT_LEFT_DCLICK : [],
            wx.EVT_MIDDLE_DOWN : [],
            wx.EVT_MIDDLE_UP : [],
            wx.EVT_MIDDLE_DCLICK : [],
            wx.EVT_RIGHT_DOWN : [],
            wx.EVT_RIGHT_UP : [],
            wx.EVT_RIGHT_DCLICK : [],
            wx.EVT_MOTION : [],
            wx.EVT_ENTER_WINDOW : [],
            wx.EVT_LEAVE_WINDOW : [],
            wx.EVT_MOUSEWHEEL : [],
            wx.EVT_MOUSE_EVENTS : []
            }

        # available cursors
        self._cursors = {
            "default": wx.StockCursor(wx.CURSOR_ARROW),
            "cross": wx.StockCursor(wx.CURSOR_CROSS),
            "hand": wx.StockCursor(wx.CURSOR_HAND),
            "pencil": wx.StockCursor(wx.CURSOR_PENCIL),
            "sizenwse": wx.StockCursor(wx.CURSOR_SIZENWSE)
            }

        # default cursor for window is arrow (at least we rely on it here)
        # but we need to define attribute here
        # cannot call SetNamedCursor since it expects the instance
        # to be a wx window, so setting only the attribute
        self._cursor = 'default'

        wx.CallAfter(self.InitBinding)

    def __del__(self):
        self.UnregisterAllHandlers()

    def InitBinding(self):
        """!Binds helper functions, which calls all handlers
           registered to events with the events
        """
        for ev, handlers in self.handlersContainer.iteritems():
            self.Bind(ev, self.EventTypeHandler(handlers))
    
    def EventTypeHandler(self, evHandlers):
        return lambda event:self.HandlersCaller(event, evHandlers)  
    
    def HandlersCaller(self, event, handlers):
        """!Hepler function which calls all handlers registered for
        event
        """
        for handler in handlers:
            try:
                handler(event)
            except:
                handlers.remove(handler)
                GError(parent = self,
                       message=_("Error occured during calling of handler: %s \n"
                                 "Handler was unregistered.") % handler.__name__)
        
        event.Skip() 

    def RegisterMouseEventHandler(self, event, handler, cursor = None):
        """!Binds event handler

        @depreciated This method is depreciated. Use Signals or drawing API instead.
        Signals do not cover all events but new Signals can be added when needed
        consider also adding generic signal. However, more interesing and useful
        is higher level API to create objects, graphics etc.

        Call event.Skip() in handler to allow default processing in MapWindow.

        If any error occures inside of handler, the handler is removed.

        Before handler is unregistered it is called with 
        string value "unregistered" of event parameter.

        @code
        # your class methods
        def OnButton(self, event):
            # current map display's map window
            # expects LayerManager to be the parent
            self.mapwin = self.parent.GetLayerTree().GetMapDisplay().GetWindow()
            if self.mapwin.RegisterEventHandler(wx.EVT_LEFT_DOWN, self.OnMouseAction,
                                                'cross'):
                self.parent.GetLayerTree().GetMapDisplay().Raise()
            else:
                # handle that you cannot get coordinates
        
        def OnMouseAction(self, event):
            # get real world coordinates of mouse click
            coor = self.mapwin.Pixel2Cell(event.GetPositionTuple()[:])
            self.text.SetLabel('Coor: ' + str(coor))
            self.mapwin.UnregisterMouseEventHandler(wx.EVT_LEFT_DOWN, self.OnMouseAction)
            event.Skip()
        @endcode

        Emits mouseHandlerRegistered signal before handler is registered.        

        @param event one of mouse events
        @param handler function to handle event
        @param cursor cursor which temporary overrides current cursor
        
        @return True if successful
        @return False if event cannot be bind
        """
        self.mouseHandlerRegistered.emit()
        # inserts handler into list
        for containerEv, handlers in self.handlersContainer.iteritems():
            if event == containerEv: 
                handlers.append(handler)
        
        self.mouse['useBeforeGenericEvent'] = self.mouse['use']
        self.mouse['use'] = 'genericEvent'
        
        if cursor:
            self._overriddenCursor = self.GetNamedCursor()
            self.SetNamedCursor(cursor)
        
        return True

    def UnregisterAllHandlers(self):
        """!Unregisters all registered handlers 

        @depreciated This method is depreciated. Use Signals or drawing API instead.

        Before each handler is unregistered it is called with string
        value "unregistered" of event parameter.
        """
        for containerEv, handlers in self.handlersContainer.iteritems():
            for handler in handlers:
                try:
                    handler("unregistered")
                    handlers.remove(handler)
                except:
                    GError(parent = self,
                           message = _("Error occured during unregistration of handler: %s \n \
                                       Handler was unregistered.") % handler.__name__)
                    handlers.remove(handler)
        
    def UnregisterMouseEventHandler(self, event, handler):
        """!Unbinds event handler for event

        @depreciated This method is depreciated. Use Signals or drawing API instead.

        Before handler is unregistered it is called with string value
        "unregistered" of event parameter.

        Emits mouseHandlerUnregistered signal after handler is unregistered.

        @param handler handler to unbind
        @param event event from which handler will be unbinded
        
        @return True if successful
        @return False if event cannot be unbind
        """
        # removes handler from list 
        for containerEv, handlers in self.handlersContainer.iteritems():
            if event != containerEv:
                continue
            try:
                handler("unregistered")
                if handler in handlers:
                    handlers.remove(handler)
                else:
                    grass.warning(_("Handler: %s was not registered") \
                                      % handler.__name__)
            except:
                GError(parent = self,
                       message = _("Error occured during unregistration of handler: %s \n \
                                       Handler was unregistered") % handler.__name__)
                handlers.remove(handler) 
        
        # restore mouse use (previous state)
        self.mouse['use'] = self.mouse['useBeforeGenericEvent']
        
        # restore overridden cursor
        if self._overriddenCursor:
            self.SetNamedCursor(self._overriddenCursor)

        self.mouseHandlerUnregistered.emit()
        return True
    
    def Pixel2Cell(self, xyCoords):
        raise NotImplementedError()
    
    def Cell2Pixel(self, enCoords):
        raise NotImplementedError()

    def OnMotion(self, event):
        """!Tracks mouse motion and update statusbar

        @todo remove this method when lastEN is not used

        @see GetLastEN
        """
        try:
            self.lastEN = self.Pixel2Cell(event.GetPositionTuple())
        except (ValueError):
            self.lastEN = None

        event.Skip()

    def GetLastEN(self):
        """!Returns last coordinates of mouse cursor.

        @depreciated This method is depreciated. Use Signal with coordinates as parameters.

        @see OnMotion
        """
        return self.lastEN

    def SetNamedCursor(self, cursorName):
        """!Sets cursor defined by name."""
        cursor = self._cursors[cursorName]
        self.SetCursor(cursor)
        self._cursor = cursorName

    def GetNamedCursor(self):
        """!Returns current cursor name."""
        return self._cursor

    cursor = property(fget=GetNamedCursor, fset=SetNamedCursor)

    def SetModePointer(self):
        """!Sets mouse mode to pointer."""
        self.mouse['use'] = 'pointer'
        self.mouse['box'] = 'point'
        self.SetNamedCursor('default')

    def SetModePan(self):
        """!Sets mouse mode to pan."""
        self.mouse['use'] = "pan"
        self.mouse['box'] = "box"
        self.zoomtype = 0
        self.SetNamedCursor('hand')

    def SetModeZoomIn(self):
        self._setModeZoom(zoomType=1)

    def SetModeZoomOut(self):
        self._setModeZoom(zoomType=-1)

    def _setModeZoom(self, zoomType):
        self.mouse['use'] = "zoom"
        self.mouse['box'] = "box"
        self.zoomtype = zoomType
        self.pen = wx.Pen(colour='Red', width=2, style=wx.SHORT_DASH)
        self.SetNamedCursor('cross')
