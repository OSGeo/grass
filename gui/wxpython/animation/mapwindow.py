"""!
@package animation.mapwindow

@brief Animation window

Classes:
 - mapwindow::BufferedWindow
 - mapwindow::AnimationWindow

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""

import wx
from core.debug import Debug


class BufferedWindow(wx.Window):
    """
    A Buffered window class (http://wiki.wxpython.org/DoubleBufferedDrawing).

    To use it, subclass it and define a Draw(DC) method that takes a DC
    to draw to. In that method, put the code needed to draw the picture
    you want. The window will automatically be double buffered, and the
    screen will be automatically updated when a Paint event is received.

    When the drawing needs to change, you app needs to call the
    UpdateDrawing() method. Since the drawing is stored in a bitmap, you
    can also save the drawing to file by calling the
    SaveToFile(self, file_name, file_type) method.

    """
    def __init__(self, *args, **kwargs):
        # make sure the NO_FULL_REPAINT_ON_RESIZE style flag is set.
        kwargs['style'] = kwargs.setdefault('style', wx.NO_FULL_REPAINT_ON_RESIZE) | \
            wx.NO_FULL_REPAINT_ON_RESIZE
        wx.Window.__init__(self, *args, **kwargs)

        Debug.msg(2, "BufferedWindow.__init__()")
        wx.EVT_PAINT(self, self.OnPaint)
        wx.EVT_SIZE(self, self.OnSize)
        # OnSize called to make sure the buffer is initialized.
        # This might result in OnSize getting called twice on some
        # platforms at initialization, but little harm done.
        self.OnSize(None)

    def Draw(self, dc):
        ## just here as a place holder.
        ## This method should be over-ridden when subclassed
        pass

    def OnPaint(self, event):
        Debug.msg(5, "BufferedWindow.OnPaint()")
        # All that is needed here is to draw the buffer to screen
        dc = wx.BufferedPaintDC(self, self._Buffer)

    def OnSize(self, event):
        Debug.msg(5, "BufferedWindow.OnSize()")
        # The Buffer init is done here, to make sure the buffer is always
        # the same size as the Window
        #Size  = self.GetClientSizeTuple()
        size = self.GetClientSize()

        # Make new offscreen bitmap: this bitmap will always have the
        # current drawing in it, so it can be used to save the image to
        # a file, or whatever.
        self._Buffer = wx.EmptyBitmap(*size)
        self.UpdateDrawing()
        # event.Skip()

    def SaveToFile(self, FileName, FileType=wx.BITMAP_TYPE_PNG):
        ## This will save the contents of the buffer
        ## to the specified file. See the wxWindows docs for
        ## wx.Bitmap::SaveFile for the details
        self._Buffer.SaveFile(FileName, FileType)

    def UpdateDrawing(self):
        """
        This would get called if the drawing needed to change, for whatever reason.

        The idea here is that the drawing is based on some data generated
        elsewhere in the system. If that data changes, the drawing needs to
        be updated.

        This code re-draws the buffer, then calls Update, which forces a paint event.
        """
        dc = wx.MemoryDC()
        dc.SelectObject(self._Buffer)
        self.Draw(dc)
        del dc  # need to get rid of the MemoryDC before Update() is called.
        self.Refresh()
        self.Update()


class AnimationWindow(BufferedWindow):
    def __init__(self, parent, id=wx.ID_ANY,
                 style=wx.DEFAULT_FRAME_STYLE | wx.FULL_REPAINT_ON_RESIZE |
                 wx.BORDER_RAISED):
        Debug.msg(2, "AnimationWindow.__init__()")

        self.bitmap = wx.EmptyBitmap(1, 1)
        self.parent = parent
        self._pdc = wx.PseudoDC()
        self._overlay = None
        self._tmpMousePos = None

        BufferedWindow.__init__(self, parent=parent, id=id, style=style)
        self.SetBackgroundColour(wx.BLACK)
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Bind(wx.EVT_MOUSE_EVENTS, self.OnMouseEvents)

    def Draw(self, dc):
        """!Draws bitmap."""
        Debug.msg(5, "AnimationWindow.Draw()")

        dc.Clear()  # make sure you clear the bitmap!
        dc.DrawBitmap(self.bitmap, x=0, y=0)

    def OnSize(self, event):
        Debug.msg(5, "AnimationWindow.OnSize()")

        self.DrawBitmap(self.bitmap)

        BufferedWindow.OnSize(self, event)
        if event:
            event.Skip()

    def DrawBitmap(self, bitmap):
        """!Draws bitmap.
        Does not draw the bitmap if it is the same one as last time.
        """
        if self.bitmap == bitmap:
            return

        self.bitmap = bitmap
        self.UpdateDrawing()

    def DrawOverlay(self, x, y):
        self._pdc.BeginDrawing()
        self._pdc.SetId(1)
        self._pdc.DrawBitmap(bmp=self._overlay, x=x, y=y)
        self._pdc.SetIdBounds(1, wx.Rect(x, y, self._overlay.GetWidth(),
                                         self._overlay.GetHeight()))
        self._pdc.EndDrawing()

    def SetOverlay(self, bitmap, xperc, yperc):
        """!Sets overlay bitmap (legend)

        @param bitmap instance of wx.Bitmap
        @param xperc x coordinate of bitmap top left corner in % of screen
        @param yperc y coordinate of bitmap top left corner in % of screen
        """
        Debug.msg(3, "AnimationWindow.SetOverlay()")
        if bitmap:
            if self._overlay:
                self._pdc.RemoveAll()
            self._overlay = bitmap
            size = self.GetClientSize()
            x = xperc * size[0]
            y = yperc * size[1]
            self.DrawOverlay(x, y)
        else:
            self._overlay = None
            self._pdc.RemoveAll()
        self.UpdateDrawing()

    def ClearOverlay(self):
        """!Clear overlay (legend) """
        Debug.msg(3, "AnimationWindow.ClearOverlay()")
        self._overlay = None
        self._pdc.RemoveAll()
        self.UpdateDrawing()

    def OnPaint(self, event):
        Debug.msg(5, "AnimationWindow.OnPaint()")
        # All that is needed here is to draw the buffer to screen
        dc = wx.BufferedPaintDC(self, self._Buffer)
        if self._overlay:
            self._pdc.DrawToDC(dc)

    def OnMouseEvents(self, event):
        """!Handle mouse events."""
        # If it grows larger, split it.
        current = event.GetPosition()
        if event.LeftDown():
            self._dragid = None
            idlist = self._pdc.FindObjects(current[0], current[1],
                                           radius=10)
            if 1 in idlist:
                self._dragid = 1
            self._tmpMousePos = current

        elif event.LeftUp():
            self._dragid = None
            self._tmpMousePos = None

        elif event.Dragging():
            if self._dragid is None:
                return
            dx = current[0] - self._tmpMousePos[0]
            dy = current[1] - self._tmpMousePos[1]
            self._pdc.TranslateId(self._dragid, dx, dy)
            self.UpdateDrawing()
            self._tmpMousePos = current

    def GetOverlayPos(self):
        """!Returns x, y position in pixels"""
        rect = self._pdc.GetIdBounds(1)
        return rect.GetX(), rect.GetY()
