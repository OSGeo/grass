"""!
@package psmap.utils

@brief utilities for wxpsmap

Classes:
 - utils::Rect2D
 - utils::Rect2DPP
 - utils::Rect2DPS

(C) 2012 by Anna Kratochvilova, and the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""

import wx

class Rect2D(wx.Rect2D):
    """!Class representing rectangle with floating point values.

    Overrides wx.Rect2D to unify Rect access methods, which are
    different (e.g. wx.Rect.GetTopLeft() x wx.Rect2D.GetLeftTop()).
    More methods can be added depending on needs.
    """
    def __init__(self, x = 0, y = 0, width = 0, height = 0):
        wx.Rect2D.__init__(self, x = x, y = y, w = width, h = height)

    def GetX(self):
        return self.x
        
    def GetY(self):
        return self.y

    def GetWidth(self):
        return self.width

    def SetWidth(self, width):
        self.width = width
        
    def GetHeight(self):
        return self.height

    def SetHeight(self, height):
        self.height = height

class Rect2DPP(Rect2D):
    """!Rectangle specified by 2 points (with floating point values).

    @see Rect2D, Rect2DPS
    """
    def __init__(self, topLeft = wx.Point2D(), bottomRight = wx.Point2D()):
        Rect2D.__init__(self, x = 0, y = 0, width = 0, height = 0)

        x1, y1 = topLeft[0], topLeft[1]
        x2, y2 = bottomRight[0], bottomRight[1]

        self.SetLeft(min(x1, x2))
        self.SetTop(min(y1, y2))
        self.SetRight(max(x1, x2))
        self.SetBottom(max(y1, y2))

class Rect2DPS(Rect2D):
    """!Rectangle specified by point and size (with floating point values).

    @see Rect2D, Rect2DPP
    """
    def __init__(self, pos = wx.Point2D(), size = (0, 0)):
        Rect2D.__init__(self, x = pos[0], y = pos[1], width = size[0], height = size[1])
