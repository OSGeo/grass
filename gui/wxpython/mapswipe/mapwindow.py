"""
@package swipe.mapwindow

@brief Map Swipe map window.

Class _MouseEvent taken from wxPython FloatCanvas source code (Christopher Barker).

Classes:
 - mapwindow::SwipeBufferedWindow
 - mapwindow::_MouseEvent

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""

import wx

from core.debug import Debug
from core.settings import UserSettings
from mapwin.buffered import BufferedMapWindow
from gui_core.wrap import Rect, NewId


EVT_MY_MOUSE_EVENTS = wx.NewEventType()
EVT_MY_MOTION = wx.NewEventType()
EVT_MOUSE_EVENTS = wx.PyEventBinder(EVT_MY_MOUSE_EVENTS)
EVT_MOTION = wx.PyEventBinder(EVT_MY_MOTION)


class SwipeBufferedWindow(BufferedMapWindow):
    """A subclass of BufferedWindow class.

    Enables to draw the image translated.
    Special mouse events with changed coordinates are used.
    """

    def __init__(self, parent, giface, Map, properties, **kwargs):
        BufferedMapWindow.__init__(
            self, parent=parent, giface=giface, Map=Map, properties=properties, **kwargs
        )
        Debug.msg(2, "SwipeBufferedWindow.__init__()")

        self.specialSize = super(SwipeBufferedWindow, self).GetClientSize()
        self.specialCoords = [0, 0]
        self.imageId = 99
        self.movingSash = False
        self._mode = "swipe"
        self.lineid = NewId()

    def _bindMouseEvents(self):
        """Binds wx mouse events and custom mouse events"""
        self.Bind(wx.EVT_MOUSE_EVENTS, self._mouseActions)
        self.Bind(wx.EVT_MOTION, self._mouseMotion)
        self.Bind(EVT_MOTION, self.OnMotion)
        self.Bind(EVT_MOUSE_EVENTS, self.MouseActions)
        self.Bind(wx.EVT_CONTEXT_MENU, self.OnContextMenu)

    def _RaiseMouseEvent(self, Event, EventType):
        """This is called in various other places to raise a Mouse Event"""
        Debug.msg(5, "SwipeBufferedWindow._RaiseMouseEvent()")

        # this computes the new coordinates from the mouse coords.
        x, y = Event.GetPosition()
        pt = x - self.GetImageCoords()[0], y - self.GetImageCoords()[1]
        evt = _MouseEvent(EventType, Event, self.GetId(), pt)
        self.GetEventHandler().ProcessEvent(evt)
        # this skip was not in the original code but is needed here
        Event.Skip()

    def _mouseActions(self, event):
        self._RaiseMouseEvent(event, EVT_MY_MOUSE_EVENTS)

    def _mouseMotion(self, event):
        self._RaiseMouseEvent(event, EVT_MY_MOTION)

    def GetClientSize(self):
        """Overridden method which returns simulated window size."""
        if self._mode == "swipe":
            return self.specialSize
        else:
            return super(SwipeBufferedWindow, self).GetClientSize()

    def SetClientSize(self, size):
        """Overridden method which sets simulated window size."""
        Debug.msg(3, "SwipeBufferedWindow.SetClientSize(): size = %s" % size)
        self.specialSize = size

    def SetMode(self, mode):
        """Sets mode of the window.

        :param mode: mode can be 'swipe' or 'mirror'
        """
        self._mode = mode

    def GetImageCoords(self):
        """Returns coordinates of rendered image"""
        if self._mode == "swipe":
            return self.specialCoords
        else:
            return (0, 0)

    def SetImageCoords(self, coords):
        """Sets coordinates of rendered image"""
        Debug.msg(
            3,
            "SwipeBufferedWindow.SetImageCoords(): coords = %s, %s"
            % (coords[0], coords[1]),
        )
        self.specialCoords = coords

    def OnSize(self, event):
        """Calls superclass's OnSize method only when needed"""
        Debug.msg(5, "SwipeBufferedWindow.OnSize()")
        if not self.movingSash:
            super(SwipeBufferedWindow, self).OnSize(event)

    def Draw(
        self,
        pdc,
        img=None,
        drawid=None,
        pdctype="image",
        coords=[0, 0, 0, 0],
        pen=None,
        brush=None,
    ):
        """Draws image (map) with translated coordinates."""
        Debug.msg(2, "SwipeBufferedWindow.Draw()")

        if pdctype == "image":
            coords = self.GetImageCoords()

        return super(SwipeBufferedWindow, self).Draw(
            pdc, img, drawid, pdctype, coords, pen, brush
        )

    def OnLeftDown(self, event):
        """Left mouse button pressed.

        In case of 'pointer' mode, coordinates must be adjusted.
        """
        if self.mouse["use"] == "pointer":
            evX, evY = event.GetPositionTuple()[:]
            imX, imY = self.GetImageCoords()
            self.lastpos = evX + imX, evY + imY
            # get decoration or text id
            self.dragid = None
            idlist = self.pdc.FindObjects(
                self.lastpos[0], self.lastpos[1], self.hitradius
            )
            if 99 in idlist:
                idlist.remove(99)
            if idlist:
                self.dragid = idlist[0]  # drag whatever is on top
        else:
            super(SwipeBufferedWindow, self).OnLeftDown(event)

    def OnDragging(self, event):
        """Mouse dragging - overlay (text) is moving.

        Coordinates must be adjusted.
        """
        if self.mouse["use"] == "pointer" and self.dragid is not None:
            evX, evY = event.GetPositionTuple()
            imX, imY = self.GetImageCoords()
            self.DragItem(self.dragid, (evX + imX, evY + imY))
        else:
            super(SwipeBufferedWindow, self).OnDragging(event)

    def TranslateImage(self, dx, dy):
        """Translate image and redraw."""
        Debug.msg(
            5, "SwipeBufferedWindow.TranslateImage(): dx = %s, dy = %s" % (dx, dy)
        )

        self.pdc.TranslateId(self.imageId, dx, dy)
        self.Refresh()

    def SetRasterNameText(self, name, textId):
        """Sets text label with map name."""
        self.textdict[textId] = {
            "bbox": Rect(),
            "coords": [10, 10],
            "font": self.GetFont(),
            "color": wx.BLACK,
            "background": wx.LIGHT_GREY,
            "rotation": 0,
            "text": name,
            "active": True,
        }

    def MouseDraw(self, pdc=None, begin=None, end=None):
        """Overridden method to recompute coordinates back to original values
        so that e.g. drawing of zoom box is done properly"""
        Debug.msg(5, "SwipeBufferedWindow.MouseDraw()")

        offsetX, offsetY = self.GetImageCoords()
        begin = (self.mouse["begin"][0] + offsetX, self.mouse["begin"][1] + offsetY)
        end = (self.mouse["end"][0] + offsetX, self.mouse["end"][1] + offsetY)
        super(SwipeBufferedWindow, self).MouseDraw(pdc, begin, end)

    def DrawMouseCursor(self, coords):
        """Draw moving cross."""
        self.pdcTmp.ClearId(self.lineid)
        color = UserSettings.Get(group="mapswipe", key="cursor", subkey="color")
        cursType = UserSettings.Get(
            group="mapswipe", key="cursor", subkey=["type", "selection"]
        )
        size = UserSettings.Get(group="mapswipe", key="cursor", subkey="size")
        width = UserSettings.Get(group="mapswipe", key="cursor", subkey="width")
        if cursType == 0:
            self.lineid = self.DrawCross(
                pdc=self.pdcTmp,
                coords=coords,
                size=size,
                pen=wx.Pen(wx.Colour(*color), width),
            )
        elif cursType == 1:
            self.lineid = self.DrawRectangle(
                pdc=self.pdcTmp,
                point1=(coords[0] - size / 2, coords[1] - size / 2),
                point2=(coords[0] + size / 2, coords[1] + size / 2),
                pen=wx.Pen(wx.Colour(*color), width),
            )
        elif cursType == 2:
            self.lineid = self.DrawCircle(
                pdc=self.pdcTmp,
                coords=coords,
                radius=size / 2,
                pen=wx.Pen(wx.Colour(*color), width),
            )


class _MouseEvent(wx.PyCommandEvent):
    """
    This event class takes a regular wxWindows mouse event as a parameter,
    and wraps it so that there is access to all the original methods. This
    is similar to subclassing, but you can't subclass a wxWindows event.

    The goal is to be able to it just like a regular mouse event.

    Difference is that it is a CommandEvent, which propagates up the
    window hierarchy until it is handled.
    """

    def __init__(self, EventType, NativeEvent, WinID, changed=None):
        Debug.msg(5, "_MouseEvent:__init__()")

        wx.PyCommandEvent.__init__(self)
        self.__dict__["_NativeEvent"] = NativeEvent
        self.__dict__["changed"] = changed
        self.SetEventType(EventType)

    def GetPosition(self):
        return wx.Point(*self.changed)

    def GetPositionTuple(self):
        return self.changed

    def GetX(self):
        return self.changed[0]

    def GetY(self):
        return self.changed[1]

    # this delegates all other attributes to the native event.
    def __getattr__(self, name):
        return getattr(self._NativeEvent, name)
