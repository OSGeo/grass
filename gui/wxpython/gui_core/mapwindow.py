"""!
@package gui_core.mapwindow

@brief Map display canvas - base class for buffered window.

Classes:
 - mapwindow::MapWindow

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

from grass.script import core as grass

class MapWindow(object):
    """!Abstract map display window class
    
    Superclass for BufferedWindow class (2D display mode), and GLWindow
    (3D display mode).
    
    Subclasses have to define
     - _bindMouseEvents method which binds MouseEvent handlers
     - Pixel2Cell
     - Cell2Pixel (if it is possible)
    """
    def __init__(self, parent, id = wx.ID_ANY,
                 Map = None, tree = None, lmgr = None, **kwargs):
        self.parent = parent # MapFrame
        self.Map    = Map
        self.tree   = tree
        self.lmgr   = lmgr
        
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
                                                wx.StockCursor(wx.CURSOR_CROSS)):
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
        
        @param event one of mouse events
        @param handler function to handle event
        @param cursor cursor which temporary overrides current cursor
        
        @return True if successful
        @return False if event cannot be bind
        """
        # inserts handler into list
        for containerEv, handlers in self.handlersContainer.iteritems():
            if event == containerEv: 
                handlers.append(handler)
        
        self.mouse['useBeforeGenericEvent'] = self.mouse['use']
        self.mouse['use'] = 'genericEvent'
        
        if cursor:
            self._overriddenCursor = self.GetCursor()
            self.SetCursor(cursor)
        
        return True

    def UnregisterAllHandlers(self):
        """!Unregisters all registered handlers 

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
        
        Before handler is unregistered it is called with string value
        "unregistered" of event parameter.

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
            self.SetCursor(self._overriddenCursor)
        
        return True
    
    def Pixel2Cell(self, (x, y)):
        raise NotImplementedError()
    
    def Cell2Pixel(self, (east, north)):
        raise NotImplementedError()

    def OnMotion(self, event):
        """!Tracks mouse motion and update statusbar
        
        @see GetLastEN
        """
        try:
            self.lastEN = self.Pixel2Cell(event.GetPositionTuple())
        except (ValueError):
            self.lastEN = None
        # FIXME: special case for vdigit and access to statusbarManager
        if self.parent.statusbarManager.GetMode() == 0: # Coordinates            
            updated = False
            if hasattr(self, "digit"):
                precision = int(UserSettings.Get(group = 'projection', key = 'format',
                                             subkey = 'precision'))
                updated = self._onMotion(self.lastEN, precision)

            if not updated:
                self.parent.CoordinatesChanged()
        
        event.Skip()

    def GetLastEN(self):
        """!Returns last coordinates of mouse cursor.
        
        @see OnMotion
        """
        return self.lastEN
    
    def GetLayerByName(self, name, mapType, dataType = 'layer'):
        """!Get layer from layer tree by nam
        
        @param name layer name
        @param type 'item' / 'layer' / 'nviz'

        @return layer / map layer properties / nviz properties
        @return None
        """
        if not self.tree:
            return None
        
        try:
            mapLayer = self.Map.GetListOfLayers(l_type = mapType, l_name = name)[0]
        except IndexError:
            return None
        
        if dataType == 'layer':
            return mapLayer
        item = self.tree.FindItemByData('maplayer', mapLayer)
        if not item:
            return None
        if dataType == 'nviz':
            return self.tree.GetPyData(item)[0]['nviz']
        
        return item
        
    def GetSelectedLayer(self, type = 'layer', multi = False):
        """!Get selected layer from layer tree
        
        @param type 'item' / 'layer' / 'nviz'
        @param multi return first selected layer or all
        
        @return layer / map layer properties / nviz properties
        @return None / [] on failure
        """
        ret = []
        if not self.tree or \
                not self.tree.GetSelection():
            if multi:
                return []
            else:
                return None
        
        if multi and \
                type == 'item':
            return self.tree.GetSelections()
        
        for item in self.tree.GetSelections():
            if not item.IsChecked():
                if multi:
                    continue
                else:
                    return None

            if type == 'item': # -> multi = False
                return item
        
            try:
                if type == 'nviz':
                    layer = self.tree.GetPyData(item)[0]['nviz']
                else:
                    layer = self.tree.GetPyData(item)[0]['maplayer']
            except:
                layer = None

            if multi:
                ret.append(layer)
            else:
                return layer
            
        return ret
