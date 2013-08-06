"""!
@package mapwin.graphics

@brief Map display canvas - buffered window.

Classes:
 - graphics::GraphicsSet
 - graphics::GraphicsSetItem

(C) 2006-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (handlers support, GraphicsSet)
"""


from copy import copy

import wx

from core.utils import _


class GraphicsSet:

    def __init__(self, parentMapWin, graphicsType,
                 setStatusFunc=None, drawFunc=None):
        """!Class, which contains instances of GraphicsSetItem and
            draws them For description of parameters look at method
            RegisterGraphicsToDraw in BufferedWindow class.
        """
        self.pens = {
            "default":  wx.Pen(colour=wx.BLACK, width=2, style=wx.SOLID),
            "selected":  wx.Pen(colour=wx.GREEN, width=2, style=wx.SOLID),
            "unused":  wx.Pen(colour=wx.LIGHT_GREY, width=2, style=wx.SOLID),
            "highest":  wx.Pen(colour=wx.RED, width=2, style=wx.SOLID)
        }

        # list contains instances of GraphicsSetItem
        self.itemsList = []

        self.properties = {}
        self.graphicsType = graphicsType
        self.parentMapWin = parentMapWin
        self.setStatusFunc = setStatusFunc

        if drawFunc:
            self.drawFunc = drawFunc

        elif self.graphicsType == "point":
            self.properties["size"] = 5

            self.properties["text"] = {}
            self.properties["text"]['font'] = wx.Font(pointSize=self.properties["size"],
                                                      family=wx.FONTFAMILY_DEFAULT,
                                                      style=wx.FONTSTYLE_NORMAL,
                                                      weight=wx.FONTWEIGHT_NORMAL)
            self.properties["text"]['active'] = True

            self.drawFunc = self.parentMapWin.DrawCross

        elif self.graphicsType == "line":
            self.drawFunc = self.parentMapWin.DrawLines

        elif self.graphicsType == "rectangle":
            self.drawFunc = self.parentMapWin.DrawRectangle

    def Draw(self, pdc):
        """!Draws all containing items.

        @param pdc - device context, where items are drawn
        """
        itemOrderNum = 0
        for item in self.itemsList:
            if self.setStatusFunc is not None:
                self.setStatusFunc(item, itemOrderNum)

            if item.GetPropertyVal("hide") is True:
                itemOrderNum += 1
                continue

            if self.graphicsType == "point":
                if item.GetPropertyVal("penName"):
                    self.parentMapWin.pen = self.pens[item.GetPropertyVal("penName")]
                else:
                    self.parentMapWin.pen = self.pens["default"]

                coords = self.parentMapWin.Cell2Pixel(item.GetCoords())
                size = self.properties["size"]

                self.properties["text"]['coords'] = [coords[0] + size, coords[1] + size, size, size]
                self.properties["text"]['color'] = self.parentMapWin.pen.GetColour()
                self.properties["text"]['text'] = item.GetPropertyVal("label")

                self.drawFunc(pdc=pdc,
                              coords=coords,
                              text=self.properties["text"],
                              size=self.properties["size"])

            elif self.graphicsType == "line":
                if item.GetPropertyVal("penName"):
                    self.parentMapWin.polypen = self.pens[item.GetPropertyVal("penName")]
                else:
                    self.parentMapWin.polypen = self.pens["default"]
                coords = item.GetCoords()

                self.drawFunc(pdc=pdc,
                              polycoords=coords)
             
            elif self.graphicsType == "rectangle":
                if item.GetPropertyVal("penName"):
                    pen = self.pens[item.GetPropertyVal("penName")]
                else:
                    pen = self.pens["default"]
                coords = item.GetCoords()

                self.drawFunc(pdc=pdc, pen=pen, 
                              point1=coords[0],
                              point2=coords[1])
            itemOrderNum += 1

    def AddItem(self, coords, penName=None, label=None, hide=False):
        """!Append item to the list.

        Added item is put to the last place in drawing order.
        Could be 'point' or 'line' according to graphicsType.

        @param coords - list of east, north coordinates (double) of item
                        Example: point: [1023, 122]
                                 line: [[10, 12],[20,40],[23, 2334]]
                                 rectangle: [[10, 12], [33, 45]]
        @param penName (string) the 'default' pen is used if is not defined
        @param label (string) label, which will be drawn with point. It is
        relavant just for 'point' type.
        @param hide (bool) If it is True, the item is not drawn
        when self.Draw is called. Hidden items are also counted in drawing
        order.

        @return (GraphicsSetItem) - added item reference
        """
        item = GraphicsSetItem(coords=coords, penName=penName, label=label, hide=hide)
        self.itemsList.append(item)

        return item

    def DeleteItem(self, item):
        """!Deletes item

        @param item (GraphicsSetItem) - item to remove

        @return True if item was removed
        @return False if item was not found
        """
        try:
            self.itemsList.remove(item)
        except ValueError:
            return False

        return True

    def GetAllItems(self):
        """!Returns list of all containing instances of GraphicsSetItem, in order
        as they are drawn. If you want to change order of drawing use: SetItemDrawOrder method.
        """
        # user can edit objects but not order in list, that is reason,
        # why is returned shallow copy of data list it should be used
        # SetItemDrawOrder for changing order
        return copy(self.itemsList)

    def GetItem(self, drawNum):
        """!Get given item from the list.

        @param drawNum (int) - drawing order (index) number of item

        @return instance of GraphicsSetItem which is drawn in drawNum order
        @return False if drawNum was out of range
        """
        if drawNum < len(self.itemsList) and drawNum >= 0:
            return self.itemsList[drawNum]
        else:
            return False

    def SetPropertyVal(self, propName, propVal):
        """!Set property value

        @param propName (string) - property name: "size", "text"
                                 - both properties are relevant for "point" type
        @param propVal - property value to be set

        @return True - if value was set
        @return False - if propName is not "size" or "text" or type is "line"
        """
        if propName in self.properties:
            self.properties[propName] = propVal
            return True

        return False

    def GetPropertyVal(self, propName):
        """!Get property value

        Raises KeyError if propName is not "size" or "text" or type is
        "line"

        @param propName (string) property name: "size", "text"
                                 both properties are relevant for "point" type

        @return value of property
        """
        if propName in self.properties:
            return self.properties[propName]

        raise KeyError(_("Property does not exist: %s") % (propName))

    def AddPen(self, penName, pen):
        """!Add pen

        @param penName (string) - name of added pen
        @param pen (wx.Pen) - added pen

        @return True - if pen was added
        @return False - if pen already exists
        """
        if penName in self.pens:
            return False

        self.pens[penName] = pen
        return True

    def GetPen(self, penName):
        """!Get existing pen

        @param penName (string) - name of pen

        @return wx.Pen reference if is found
        @return None if penName was not found
        """
        if penName in self.pens:
            return self.pens[penName]

        return None

    def SetItemDrawOrder(self, item, drawNum):
        """!Set draw order for item

        @param item (GraphicsSetItem)
        @param drawNum (int) - drawing order of item to be set

        @return True - if order was changed
        @return False - if drawNum is out of range or item was not found
        """
        if drawNum < len(self.itemsList) and drawNum >= 0 and \
                item in self.itemsList:
            self.itemsList.insert(drawNum, self.itemsList.pop(self.itemsList.index(item)))
            return True

        return False

    def GetItemDrawOrder(self, item):
        """!Get draw order for given item

        @param item (GraphicsSetItem)

        @return (int) - drawing order of item
        @return None - if item was not found
        """
        try:
            return self.itemsList.index(item)
        except ValueError:
            return None


class GraphicsSetItem:

    def __init__(self, coords, penName=None, label=None, hide=False):
        """!Could be point or line according to graphicsType in
        GraphicsSet class

        @param coords - list of coordinates (double) of item
                        Example: point: [1023, 122]
                                 line: [[10, 12],[20,40],[23, 2334]]
                                 rectangle: [[10, 12], [33, 45]]
        @param penName (string) if it is not defined 'default' pen is used
        @param label (string) label, which will be drawn with point. It is
        relevant just for 'point' type
        @param hide (bool) if it is True, item is not drawn
                           Hidden items are also counted in drawing order in
                           GraphicsSet class.
        """
        self.coords = coords

        self.properties = {"penName": penName,
                           "hide": hide,
                           "label": label}

    def SetPropertyVal(self, propName, propVal):
        """!Set property value

        @param propName (string) - property name: "penName", "hide" or "label"
                                 - property "label" is relevant just for 'point' type
        @param propVal - property value to be set

        @return True - if value was set
        @return False - if propName is not "penName", "hide" or "label"
        """
        if propName in self.properties:
            self.properties[propName] = propVal
            return True

        return False

    def GetPropertyVal(self, propName):
        """!Get property value

        Raises KeyError if propName is not "penName", "hide" or
        "label".

        @param propName (string) - property name: "penName", "hide" or "label"
                                 - property "label" is relevant just for 'point' type

        @return value of property
        """
        if propName in self.properties:
            return self.properties[propName]

        raise KeyError(_("Property does not exist: %s") % (propName))

    def SetCoords(self, coords):
        """!Set coordinates of item

        @param coords - list of east, north coordinates (double) of item
                        Example: point: [1023, 122]
                                 line: [[10, 12],[20,40],[23, 2334]]
                                 rectangle: [[10, 12], [33, 45]]
        """
        self.coords = coords

    def GetCoords(self):
        """!Get item coordinates

        @returns coordinates
        """
        return self.coords
