"""
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

from gui_core.wrap import NewId


class GraphicsSet:

    def __init__(self, parentMapWin, graphicsType, pdc,
                 setStatusFunc=None, drawFunc=None, mapCoords=True):
        """Class, which contains instances of GraphicsSetItem and
            draws them For description of parameters look at method
            RegisterGraphicsToDraw in BufferedWindow class.
        """
        self.pens = {
            "default": wx.Pen(colour=wx.BLACK, width=2, style=wx.SOLID),
            "selected": wx.Pen(colour=wx.GREEN, width=2, style=wx.SOLID),
            "unused": wx.Pen(colour=wx.LIGHT_GREY, width=2, style=wx.SOLID),
            "highest": wx.Pen(colour=wx.RED, width=2, style=wx.SOLID)
        }
        self.brushes = {
            'default': wx.TRANSPARENT_BRUSH
        }

        # list contains instances of GraphicsSetItem
        self.itemsList = []

        self.properties = {}
        self.graphicsType = graphicsType
        self.parentMapWin = parentMapWin
        self.setStatusFunc = setStatusFunc
        self.mapCoords = mapCoords
        self.pdc = pdc

        if drawFunc:
            self.drawFunc = drawFunc

        elif self.graphicsType == "point":
            self.properties["size"] = 5

            self.properties["text"] = {}
            self.properties["text"]['font'] = wx.Font(
                pointSize=self.properties["size"],
                family=wx.FONTFAMILY_DEFAULT,
                style=wx.FONTSTYLE_NORMAL,
                weight=wx.FONTWEIGHT_NORMAL)
            self.properties["text"]['active'] = True

            self.drawFunc = self.parentMapWin.DrawCross

        elif self.graphicsType == "line":
            self.drawFunc = self.parentMapWin.DrawPolylines

        elif self.graphicsType == "rectangle":
            self.drawFunc = self.parentMapWin.DrawRectangle

        elif self.graphicsType == "polygon":
            self.drawFunc = self.parentMapWin.DrawPolygon

    def Draw(self):
        """Draws all containing items."""
        itemOrderNum = 0
        for item in self.itemsList:
            self._clearId(item.GetId())
            if self.setStatusFunc is not None:
                self.setStatusFunc(item, itemOrderNum)

            if item.GetPropertyVal("hide") is True:
                itemOrderNum += 1
                continue

            if self.graphicsType == "point":
                if item.GetPropertyVal("penName"):
                    self.parentMapWin.pen = self.pens[
                        item.GetPropertyVal("penName")]
                else:
                    self.parentMapWin.pen = self.pens["default"]

                if self.mapCoords:
                    coords = self.parentMapWin.Cell2Pixel(item.GetCoords())
                else:
                    coords = item.GetCoords()
                size = self.properties["size"]

                label = item.GetPropertyVal("label")
                if label is None:
                    self.properties["text"] = None
                else:
                    self.properties["text"]['coords'] = [coords[0] + size,
                                                         coords[1] + size,
                                                         size, size]
                    self.properties["text"][
                        'color'] = self.parentMapWin.pen.GetColour()
                    self.properties["text"]['text'] = label

                self.drawFunc(pdc=self.pdc, drawid=item.GetId(),
                              coords=coords,
                              text=self.properties["text"],
                              size=self.properties["size"])

            elif self.graphicsType == "line":
                if item.GetPropertyVal("penName"):
                    pen = self.pens[item.GetPropertyVal("penName")]
                else:
                    pen = self.pens["default"]

                if self.mapCoords:
                    coords = [
                        self.parentMapWin.Cell2Pixel(coords)
                        for coords in item.GetCoords()]
                else:
                    coords = item.GetCoords()

                self.drawFunc(pdc=self.pdc, pen=pen,
                              coords=coords, drawid=item.GetId())

            elif self.graphicsType == "rectangle":
                if item.GetPropertyVal("penName"):
                    pen = self.pens[item.GetPropertyVal("penName")]
                else:
                    pen = self.pens["default"]
                if item.GetPropertyVal("brushName"):
                    brush = self.brushes[item.GetPropertyVal("brushName")]
                else:
                    brush = self.brushes["default"]
                if self.mapCoords:
                    coords = [
                        self.parentMapWin.Cell2Pixel(coords)
                        for coords in item.GetCoords()]
                else:
                    coords = item.GetCoords()

                self.drawFunc(
                    pdc=self.pdc,
                    pen=pen,
                    brush=brush,
                    drawid=item.GetId(),
                    point1=coords[0],
                    point2=coords[1])

            elif self.graphicsType == "polygon":
                if item.GetPropertyVal("penName"):
                    pen = self.pens[item.GetPropertyVal("penName")]
                else:
                    pen = self.pens["default"]
                if item.GetPropertyVal("brushName"):
                    brush = self.brushes[item.GetPropertyVal("brushName")]
                else:
                    brush = self.brushes["default"]
                if self.mapCoords:
                    coords = [
                        self.parentMapWin.Cell2Pixel(coords)
                        for coords in item.GetCoords()]
                else:
                    coords = item.GetCoords()

                self.drawFunc(pdc=self.pdc, pen=pen, brush=brush,
                              coords=coords, drawid=item.GetId())
            itemOrderNum += 1

    def AddItem(self, coords, penName=None, label=None, hide=False):
        """Append item to the list.

        Added item is put to the last place in drawing order.
        Could be 'point' or 'line' according to graphicsType.

        :param coords: list of east, north coordinates (double) of item.
                       Example:

                           * point: [1023, 122]
                           * line: [[10, 12],[20,40],[23, 2334]]
                           * rectangle: [[10, 12], [33, 45]]
        :param penName: the 'default' pen is used if is not defined
        :type penName: str
        :param label: label, which will be drawn with point. It is
                      relavant just for 'point' type.
        :type label: str
        :param hide: if it is True, the item is not drawn when self.Draw
                     is called. Hidden items are also counted in drawing
                     order.
        :type hide: bool
        :return: (GraphicsSetItem) - added item reference
        """
        item = GraphicsSetItem(coords=coords, penName=penName, label=label,
                               hide=hide)
        self.itemsList.append(item)

        return item

    def DeleteItem(self, item):
        """Deletes item

        :param item: (GraphicsSetItem) - item to remove

        :return: True if item was removed
        :return: False if item was not found
        """
        try:
            self.itemsList.remove(item)
        except ValueError:
            return False

        return True

    def GetAllItems(self):
        """Returns list of all containing instances of GraphicsSetItem,
        in order as they are drawn. If you want to change order of
        drawing use: SetItemDrawOrder method.
        """
        # user can edit objects but not order in list, that is reason,
        # why is returned shallow copy of data list it should be used
        # SetItemDrawOrder for changing order
        return copy(self.itemsList)

    def GetItem(self, drawNum):
        """Get given item from the list.

        :param drawNum: drawing order (index) number of item
        :type drawNum: int

        :return: instance of GraphicsSetItem which is drawn in drawNum order
        :return: False if drawNum was out of range
        """
        return self.itemsList[drawNum]

    def SetPropertyVal(self, propName, propVal):
        """Set property value

        :param propName: - property name: "size", "text"
                         - both properties are relevant for "point" type
        :type propName: str
        :param propVal: property value to be set

        :return: True if value was set
        :return: False if propName is not "size" or "text" or type is "line"
        """
        if propName in self.properties:
            self.properties[propName] = propVal
            return True

        return False

    def GetPropertyVal(self, propName):
        """Get property value

        Raises KeyError if propName is not "size" or "text" or type is
        "line"

        :param propName: property name: "size", "text" both properties
               are relevant for "point" type
        :type propName: str

        :return: value of property
        """
        if propName in self.properties:
            return self.properties[propName]

        raise KeyError(_("Property does not exist: %s") % (propName))

    def AddPen(self, penName, pen):
        """Add pen

        :param penName: name of added pen
        :type penName: str
        :param pen: added pen
        :type pen: Wx.Pen

        :return: True - if pen was added
        :return: False - if pen already exists
        """
        if penName in self.pens:
            return False

        self.pens[penName] = pen
        return True

    def GetPen(self, penName):
        """Get existing pen

        :param penName: name of pen
        :type penName: str

        :return: wx.Pen reference if is found
        :return: None if penName was not found
        """
        if penName in self.pens:
            return self.pens[penName]

        return None

    def AddBrush(self, brushName, brush):
        """Add brush

        :param brushName: name of added brush
        :type brushName: str
        :param brush: added brush
        :type brush: wx.Brush

        :return: True - if brush was added
        :return: False - if brush already exists
        """
        if brushName in self.brushes:
            return False

        self.brushes[brushName] = brush
        return True

    def GetBrush(self, brushName):
        """Get existing brush

        :param brushName: name of brush
        :type brushName: str

        :return: wx.Brush reference if is found
        :return: None if brushName was not found
        """
        if brushName in self.brushes:
            return self.brushes[brushName]

        return None

    def SetItemDrawOrder(self, item, drawNum):
        """Set draw order for item

        :param item: (GraphicsSetItem)
        :param drawNum: drawing order of item to be set
        :type drawNum: int

        :return: True if order was changed
        :return: False if drawNum is out of range or item was not found
        """
        if drawNum < len(self.itemsList) and drawNum >= 0 and \
                item in self.itemsList:
            self.itemsList.insert(
                drawNum, self.itemsList.pop(
                    self.itemsList.index(item)))
            return True

        return False

    def GetItemDrawOrder(self, item):
        """Get draw order for given item

        :param item: (GraphicsSetItem)

        :return: (int) - drawing order of item
        :return: None - if item was not found
        """
        try:
            return self.itemsList.index(item)
        except ValueError:
            return None

    def _clearId(self, drawid):
        """Clears old object before drawing new object."""
        try:
            self.pdc.ClearId(drawid)
        except:
            pass


class GraphicsSetItem:

    def __init__(self, coords, penName=None,
                 brushName=None, label=None, hide=False):
        """Could be point or line according to graphicsType in
        GraphicsSet class

        :param coords: list of coordinates (double) of item
                       Example: point: [1023, 122]
                                line: [[10, 12],[20,40],[23, 2334]]
                                rectangle: [[10, 12], [33, 45]]
        :param penName: if it is not defined 'default' pen is used
        :type penName: str
        :param brushName: if it is not defined 'default' brush is used
        :type brushName: str
        :param label: label, which will be drawn with point. It is
                      relevant just for 'point' type
        :type label: str
        :param hide: if it is True, item is not drawn Hidden items are
                     also counted in drawing order in GraphicsSet class.
        :type hide: bool
        """
        self.coords = coords

        self.properties = {"penName": penName,
                           "brushName": brushName,
                           "hide": hide,
                           "label": label}
        self.id = NewId()

    def AddProperty(self, propName):
        """Adds new property, to set it, call SetPropertyVal afterwards.

        :param propName - name of the newly defined property
        :type propName: str
        """
        if not propName in self.properties:
            self.properties[propName] = None

    def SetPropertyVal(self, propName, propVal):
        """Set property value

        :param propName: - property name: "penName", "hide" or "label"
                         - property "label" is relevant just for 'point' type
                         - or newly defined property name
        :type propName: str
        :param propVal: property value to be set

        :return: True if value was set
        :return: False if propName is not "penName", "hide" or "label"
        """
        if propName in self.properties:
            self.properties[propName] = propVal
            return True

        return False

    def GetPropertyVal(self, propName):
        """Get property value

        Raises KeyError if propName is not "penName", "hide" or
        "label".

        :param propName: - property name: "penName", "hide" or "label"
                         - property "label" is relevant just for 'point' type
        :type propName: str

        :return: value of property
        """
        if propName in self.properties:
            return self.properties[propName]

        raise KeyError(_("Property does not exist: %s") % (propName))

    def SetCoords(self, coords):
        """Set coordinates of item

        :param coords: list of east, north coordinates (double) of item
                       Example:

                           * point: [1023, 122]
                           * line: [[10, 12],[20,40],[23, 2334]]
                           * rectangle: [[10, 12], [33, 45]]
        """
        self.coords = coords

    def GetCoords(self):
        """Get item coordinates

        :return: coordinates
        """
        return self.coords

    def GetId(self):
        """Get item id (drawing id).
        """
        return self.id
