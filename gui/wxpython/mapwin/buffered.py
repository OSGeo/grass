"""
@package mapwin.mapwindow

@brief Map display canvas - buffered window.

Classes:
 - mapwindow::BufferedWindow
 - mapwindow::GraphicsSet
 - mapwindow::GraphicsSetItem

(C) 2006-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Michael Barton
@author Jachym Cepicky
@author Stepan Turek <stepan.turek seznam.cz> (handlers support, GraphicsSet)
@author Anna Petrasova <kratochanna gmail.com> (refactoring)
@author Vaclav Petras <wenzeslaus gmail.com> (refactoring)
"""

import os
import time
import math
import sys

import wx

from operator import itemgetter

from grass.pydispatch.signal import Signal

from core.globalvar import wxPythonPhoenix
import grass.script as gs

from gui_core.dialogs import SavedRegion
from gui_core.wrap import (
    DragImage,
    PseudoDC,
    EmptyBitmap,
    BitmapFromImage,
    Window,
    Menu,
    Rect,
    NewId,
)
from core.gcmd import RunCommand, GException, GError
from core.debug import Debug
from core.settings import UserSettings
from mapwin.base import MapWindowBase
from core import utils
from mapwin.graphics import GraphicsSet
from core.gthread import gThread

try:
    import grass.lib.gis as gislib

    haveCtypes = True
except (ImportError, TypeError):
    haveCtypes = False


class BufferedMapWindow(MapWindowBase, Window):
    """A Buffered window class (2D view mode)

    Superclass for VDigitWindow (vector digitizer).

    When the drawing needs to change, you app needs to call the
    UpdateMap() method. Since the drawing is stored in a bitmap, you
    can also save the drawing to file by calling the
    SaveToFile() method.
    """

    def __init__(
        self,
        parent,
        giface,
        Map,
        properties,
        id=wx.ID_ANY,
        overlays=None,
        style=wx.NO_FULL_REPAINT_ON_RESIZE,
        **kwargs,
    ):
        """
        :param parent: parent window
        :param giface: grass interface instance
        :param map: map instance
        :param properties: instance of MapWindowProperties
        :param id: wx window id
        :param style: wx window style
        :param kwargs: keyword arguments passed to MapWindow and wx.Window
        """
        MapWindowBase.__init__(self, parent=parent, giface=giface, Map=Map)
        wx.Window.__init__(self, parent=parent, id=id, style=style, **kwargs)
        # This is applied when no layers are rendered and thus the background
        # color is not applied in rendering itself (it would be applied always
        # if rendering would use transparent background).
        self.SetBackgroundColour(
            wx.Colour(*UserSettings.Get(group="display", key="bgcolor", subkey="color"))
        )

        self._properties = properties
        # this class should not ask for digit, this is a hack
        self.digit = None

        # flags
        self.resize = False  # indicates whether or not a resize event has taken place
        self.dragimg = None  # initialize variable for map panning
        self.alwaysRender = (
            False  # if it always sets render to True in self.UpdateMap()
        )

        # variables for drawing on DC
        self.pen = None  # pen for drawing zoom boxes, etc.
        # pen for drawing polylines (measurements, profiles, etc)
        self.polypen = None
        # List of wx.Point tuples defining a polyline (geographical
        # coordinates)
        self.polycoords = []
        # ID of rubber band line
        self.lineid = None
        # ID of poly line resulting from cumulative rubber band lines (e.g.
        # measurement)
        self.plineid = None

        # following class members deals with merging more updateMap request
        # into one UpdateMap process

        # thread where timer for measuring delay limit
        self.renderTimingThr = gThread()
        # relevant timer id given by the thread
        self.timerRunId = None
        # time, of last updateMap request
        self.lastUpdateMapReq = None
        # defines time limit for waiting for another update request
        self.updDelay = 0
        # holds information about level of rendering during the delay limit
        self.render = self.renderVector = False

        # Emitted when zoom of a window is changed
        self.zoomChanged = Signal("BufferedWindow.zoomChanged")

        # Emitted when map was queried, parameters x, y are mouse coordinates
        # TODO: change pixel coordinates to map coordinates (using Pixel2Cell)
        self.mapQueried = Signal("BufferedWindow.mapQueried")

        # Emitted when the zoom history stack is emptied
        self.zoomHistoryUnavailable = Signal("BufferedWindow.zoomHistoryUnavailable")
        # Emitted when the zoom history stack is not empty
        self.zoomHistoryAvailable = Signal("BufferedWindow.zoomHistoryAvailable")

        # Emitted when map enters the window
        self.mouseEntered = Signal("BufferedWindow.mouseEntered")
        # Emitted when left mouse button is released and mouse use is 'pointer'
        # Parameters are x and y of the mouse click in map (cell) units
        # new and experimental, if the concept would be used widely,
        # it could replace register and unregister mechanism
        # and partially maybe also internal mouse use dictionary
        self.mouseLeftUpPointer = Signal("BufferedWindow.mouseLeftUpPointer")
        # Emitted when left mouse button is released
        self.mouseLeftUp = Signal("BufferedWindow.mouseLeftUp")
        # Emitted when right mouse button is released
        self.mouseRightUp = Signal("BufferedWindow.mouseRightUp")
        # Emitted when left mouse button was pressed
        self.mouseLeftDown = Signal("BufferedWindow.mouseLeftDown")
        # Emitted after double-click
        self.mouseDClick = Signal("BufferedWindow.mouseDClick")
        # Emitted when mouse us moving (mouse motion event)
        # Parameters are x and y of the mouse position in map (cell) units
        self.mouseMoving = Signal("BufferedWindow.mouseMoving")

        # event bindings
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Bind(wx.EVT_IDLE, self.OnIdle)

        self._bindMouseEvents()

        self.processMouse = True

        # render output objects
        self.img = None  # wx.Image object (self.mapfile)
        # decoration overlays
        self.overlays = overlays
        # images and their PseudoDC ID's for painting and dragging
        self.imagedict = {}
        self.select = {}  # selecting/unselecting decorations for dragging
        self.textdict = {}  # text, font, and color indexed by id

        # zoom objects
        self.zoomhistory = []  # list of past zoom extents
        self.currzoom = 0  # current set of extents in zoom history being used
        self.zoomtype = 1  # 1 zoom in, 0 no zoom, -1 zoom out
        self.hitradius = 10  # distance for selecting map decorations
        # offset for dialog (e.g. DisplayAttributesDialog)
        self.dialogOffset = 5

        # OnSize called to make sure the buffer is initialized.
        # This might result in OnSize getting called twice on some
        # platforms at initialization, but little harm done.
        # self.OnSize(None)

        self._definePseudoDC()
        # redraw all pdc's, pdcTmp layer is redrawn always (speed issue)
        self.redrawAll = True

        # will store an off screen empty bitmap for saving to file
        self._buffer = EmptyBitmap(max(1, self.Map.width), max(1, self.Map.height))

        self.Bind(wx.EVT_ERASE_BACKGROUND, lambda x: None)

        # rerender when Map reports change
        self.Map.layerChanged.connect(self.OnUpdateMap)
        self.Map.GetRenderMgr().renderDone.connect(self._updateMFinished)

        # vars for handling mouse clicks
        self.dragid = None
        self.lastpos = (0, 0)

        # list for registration of graphics to draw
        self.graphicsSetList = []

        # dict for registration of context menu actions
        self._extraContextActions = {}

    def OnUpdateMap(self):
        # before lambda func was used, however it was problem
        # to disconnect it from signal
        self.UpdateMap()

    def DisactivateWin(self):
        """Use when the class instance is hidden in MapFrame."""
        self.Map.layerChanged.disconnect(self.OnUpdateMap)

    def ActivateWin(self):
        """Used when the class instance is activated in MapFrame."""
        self.Map.layerChanged.connect(self.OnUpdateMap)

    def _definePseudoDC(self):
        """Define PseudoDC objects to use"""
        # create PseudoDC used for background map, map decorations like scales
        # and legends
        self.pdc = PseudoDC()
        # used for digitization tool
        self.pdcVector = None
        # transparent objects (region box, raster digitizer)
        self.pdcTransparent = PseudoDC()
        # pseudoDC for temporal objects (select box, measurement tool, etc.)
        self.pdcTmp = PseudoDC()

    def _bindMouseEvents(self):
        self.Bind(wx.EVT_MOUSE_EVENTS, self.MouseActions)
        self.Bind(wx.EVT_MOTION, self.OnMotion)
        self.Bind(wx.EVT_CONTEXT_MENU, self.OnContextMenu)

    def RegisterContextAction(self, name, label, action):
        """Register context menu item.

        :param name: action name
        :param label: callback function returning label
        :param action: handler
        """
        self._extraContextActions[name] = {"label": label, "action": action}

    def OnContextMenu(self, event):
        """Show Map Display context menu"""
        if self.digit:
            event.Skip()
            return

        # generate popup-menu
        menu = Menu()

        if not hasattr(self, "popupCopyCoordinates"):
            self.popupCopyCoordinates = NewId()
            self.Bind(wx.EVT_MENU, self.OnCopyCoordinates, id=self.popupCopyCoordinates)
        menu.Append(self.popupCopyCoordinates, _("Copy coordinates to clipboard"))
        if self._extraContextActions:
            menu.AppendSeparator()
        for key, action_dict in self._extraContextActions.items():
            if not hasattr(self, key):
                aid = NewId()
                setattr(self, key, aid)
                self.Bind(wx.EVT_MENU, action_dict["action"], id=aid)
            menu.Append(getattr(self, key), action_dict["label"]())

        pos = self.ScreenToClient(event.GetPosition())
        idlist = self.pdc.FindObjects(pos[0], pos[1], self.hitradius)
        if (
            self.overlays
            and idlist
            and [i for i in idlist if i in list(self.overlays.keys())]
        ):  # legend, scale bar, north arrow, dtext
            menu.AppendSeparator()
            removeId = NewId()
            self.Bind(
                wx.EVT_MENU,
                lambda evt: self.overlayRemoved.emit(overlayId=idlist[0]),
                id=removeId,
            )
            menu.Append(removeId, self.overlays[idlist[0]].removeLabel)

            # raster legend can be resized
            if self.overlays[idlist[0]].name == "legend":
                resizeLegendId = NewId()
                self.Bind(
                    wx.EVT_MENU,
                    lambda evt: self.overlays[idlist[0]].StartResizing(),
                    id=resizeLegendId,
                )
                menu.Append(resizeLegendId, _("Resize and move legend"))

            activateId = NewId()
            self.Bind(
                wx.EVT_MENU,
                lambda evt: self.overlayActivated.emit(overlayId=idlist[0]),
                id=activateId,
            )
            menu.Append(activateId, self.overlays[idlist[0]].activateLabel)
        self.PopupMenu(menu)
        menu.Destroy()

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
        """Draws map and overlay decorations"""
        if drawid is None:
            if pdctype == "image" and img:
                drawid = self.imagedict[img]
            elif pdctype == "clear":
                drawid = None
            else:
                drawid = NewId()

        # TODO: find better solution
        if not pen:
            pen = self.polypen if pdctype == "polyline" else self.pen

        if img and pdctype == "image":
            # self.imagedict[img]['coords'] = coords
            self.select[self.imagedict[img]["id"]] = False  # ?

        pdc.BeginDrawing()

        if drawid != 99:
            bg = wx.TRANSPARENT_BRUSH
        else:
            bg = wx.Brush(self.GetBackgroundColour())

        pdc.SetBackground(bg)

        Debug.msg(
            5,
            "BufferedWindow.Draw(): id=%s, pdctype = %s, coord=%s"
            % (drawid, pdctype, coords),
        )

        # set PseudoDC id
        if drawid is not None:
            pdc.SetId(drawid)

        if pdctype == "clear":  # erase the display
            bg = wx.WHITE_BRUSH
            # bg = wx.Brush(self.GetBackgroundColour())
            pdc.SetBackground(bg)
            pdc.RemoveAll()
            pdc.Clear()
            pdc.EndDrawing()

            self.Refresh()
            return None

        if pdctype == "image":  # draw selected image
            bitmap = BitmapFromImage(img)
            w, h = bitmap.GetSize()
            pdc.DrawBitmap(bitmap, coords[0], coords[1], True)  # draw the composite map
            pdc.SetIdBounds(drawid, Rect(coords[0], coords[1], w, h))

        elif pdctype == "box":  # draw a box on top of the map
            if pen:
                if not brush:
                    brush = wx.Brush(wx.CYAN, wx.TRANSPARENT)
                pdc.SetBrush(brush)
                pdc.SetPen(pen)
                x2 = max(coords[0], coords[2])
                x1 = min(coords[0], coords[2])
                y2 = max(coords[1], coords[3])
                y1 = min(coords[1], coords[3])
                rwidth = x2 - x1
                rheight = y2 - y1
                rect = Rect(x1, y1, rwidth, rheight)
                pdc.DrawRectangleRect(rect)
                pdc.SetIdBounds(drawid, rect)

        elif pdctype == "line":  # draw a line on top of the map
            if pen:
                pdc.SetBrush(wx.Brush(wx.CYAN, wx.TRANSPARENT))
                pdc.SetPen(pen)
                pdc.DrawLinePoint(
                    wx.Point(coords[0], coords[1]), wx.Point(coords[2], coords[3])
                )
                pdc.SetIdBounds(
                    drawid, Rect(coords[0], coords[1], coords[2], coords[3])
                )

        # polyline is a series of connected lines defined as sequence of points
        # lines are individual, not connected lines which must be drawn as 1
        # object (e.g. cross)
        elif pdctype in {"polyline", "lines"}:
            if pen:
                pdc.SetBrush(wx.Brush(wx.CYAN, wx.TRANSPARENT))
                pdc.SetPen(pen)
                if len(coords) < 2:
                    return None
                if pdctype == "polyline":
                    i = 1
                    while i < len(coords):
                        pdc.DrawLinePoint(
                            wx.Point(coords[i - 1][0], coords[i - 1][1]),
                            wx.Point(coords[i][0], coords[i][1]),
                        )
                        i += 1
                else:
                    for line in coords:
                        pdc.DrawLine(line[0], line[1], line[2], line[3])

                # get bounding rectangle for polyline/lines
                xlist = []
                ylist = []
                if len(coords) > 0:
                    if pdctype == "polyline":
                        for point in coords:
                            x, y = point
                            xlist.append(x)
                            ylist.append(y)
                    else:
                        for line in coords:
                            x1, y1, x2, y2 = line
                            xlist.extend([x1, x2])
                            ylist.extend([y1, y2])
                    x1 = min(xlist)
                    x2 = max(xlist)
                    y1 = min(ylist)
                    y2 = max(ylist)
                    pdc.SetIdBounds(drawid, Rect(x1, y1, x2, y2))

        elif pdctype == "polygon":
            if pen:
                pdc.SetPen(pen)
                if not brush:
                    brush = wx.TRANSPARENT_BRUSH
                pdc.SetBrush(brush)
                pdc.DrawPolygon(points=coords)
                x = min(coords, key=itemgetter(0))[0]
                y = min(coords, key=itemgetter(1))[1]
                w = max(coords, key=itemgetter(0))[0] - x
                h = max(coords, key=itemgetter(1))[1] - y
                pdc.SetIdBounds(drawid, Rect(x, y, w, h))

        elif pdctype == "circle":  # draw circle
            if pen:
                pdc.SetPen(pen)
                if not brush:
                    brush = wx.TRANSPARENT_BRUSH
                pdc.SetBrush(brush)
                radius = abs(coords[2] - coords[0]) / 2
                pdc.DrawCircle(
                    max(coords[0], coords[2]) - radius,
                    max(coords[1], coords[3]) - radius,
                    radius=radius,
                )
                pdc.SetIdBounds(
                    drawid, Rect(coords[0], coords[1], coords[2], coords[3])
                )

        elif pdctype == "point":  # draw point
            if pen:
                pdc.SetPen(pen)
                pdc.DrawPoint(coords[0], coords[1])
                coordsBound = (
                    coords[0] - 5,
                    coords[1] - 5,
                    coords[0] + 5,
                    coords[1] + 5,
                )
                pdc.SetIdBounds(drawid, Rect(coordsBound))

        elif pdctype == "text":  # draw text on top of map
            if not img["active"]:
                return None  # only draw active text
            rotation = float(img["rotation"]) if "rotation" in img else 0.0
            w, h = self.GetFullTextExtent(img["text"])[0:2]
            pdc.SetFont(img["font"])
            pdc.SetTextForeground(img["color"])
            if "background" in img:
                pdc.SetBackgroundMode(wx.SOLID)
                pdc.SetTextBackground(img["background"])
            coords, bbox = self.TextBounds(img)
            if rotation == 0:
                pdc.DrawText(img["text"], int(coords[0]), int(coords[1]))
            else:
                pdc.DrawRotatedText(
                    img["text"], int(coords[0]), int(coords[1]), rotation
                )
            pdc.SetIdBounds(drawid, bbox)

        pdc.EndDrawing()

        self.Refresh()

        return drawid

    def TextBounds(self, textinfo, relcoords=False):
        """Return text boundary data

        :param textinfo: text metadata (text, font, color, rotation)
        :param coords: reference point

        :return: coords of nonrotated text bbox (TL corner)
        :return: bbox of rotated text bbox (wx.Rect)
        :return: relCoords are text coord inside bbox
        """
        rotation = float(textinfo["rotation"]) if "rotation" in textinfo else 0.0

        coords = textinfo["coords"]
        bbox = Rect(coords[0], coords[1], 0, 0)
        relCoords = (0, 0)
        Debug.msg(
            4,
            "BufferedWindow.TextBounds(): text=%s, rotation=%f"
            % (textinfo["text"], rotation),
        )

        self.Update()

        self.SetFont(textinfo["font"])

        w, h = self.GetTextExtent(textinfo["text"])

        if rotation == 0:
            bbox[2], bbox[3] = w, h
            if relcoords:
                return coords, bbox, relCoords
            return coords, bbox

        boxh = math.fabs(math.sin(math.radians(rotation)) * w) + h
        boxw = math.fabs(math.cos(math.radians(rotation)) * w) + h
        if 0 < rotation < 90:
            bbox[1] -= boxh
            relCoords = (0, boxh)
        elif 90 <= rotation < 180:
            bbox[0] -= boxw
            bbox[1] -= boxh
            relCoords = (boxw, boxh)
        elif 180 <= rotation < 270:
            bbox[0] -= boxw
            relCoords = (boxw, 0)
        bbox[2] = boxw
        bbox[3] = boxh
        bbox.Inflate(h, h)
        if relcoords:
            return coords, bbox, relCoords
        return coords, bbox

    def OnPaint(self, event):
        """Draw PseudoDC's to buffered paint DC

        If self.redrawAll is False on self.pdcTmp content is re-drawn
        """
        Debug.msg(5, "BufferedWindow.OnPaint(): redrawAll=%s" % self.redrawAll)
        dc = wx.BufferedPaintDC(self, self._buffer)
        dc.Clear()

        # use PrepareDC to set position correctly
        # probably does nothing, removed from wxPython 2.9
        # self.PrepareDC(dc)

        # create a clipping rect from our position and size
        # and update region
        rgn = self.GetUpdateRegion().GetBox()
        if wxPythonPhoenix:
            dc.SetClippingRegion(rgn)
        else:
            dc.SetClippingRect(rgn)

        switchDraw = False
        if self.redrawAll is None:
            self.redrawAll = True
            switchDraw = True

        if self.redrawAll:  # redraw pdc and pdcVector
            # draw to the dc using the calculated clipping rect
            self.pdc.DrawToDCClipped(dc, rgn)

            # draw vector map layer
            if self.digit:
                # decorate with GDDC (transparency)
                try:
                    gcdc = wx.GCDC(dc)
                    if self.pdcVector:
                        self.pdcVector.DrawToDCClipped(gcdc, rgn)
                except NotImplementedError as e:
                    print(e, file=sys.stderr)
                    self.pdcVector.DrawToDCClipped(dc, rgn)

            self.bufferLast = None
        else:  # do not redraw pdc and pdcVector
            if self.bufferLast is None:
                # draw to the dc
                self.pdc.DrawToDC(dc)

                if self.digit:
                    # decorate with GDDC (transparency)
                    try:
                        gcdc = wx.GCDC(dc)
                        self.pdcVector.DrawToDC(gcdc)
                    except NotImplementedError as e:
                        print(e, file=sys.stderr)
                        self.pdcVector.DrawToDC(dc)

                # store buffered image
                # self.bufferLast = wx.BitmapFromImage(self.buffer.ConvertToImage())
                self.bufferLast = dc.GetAsBitmap(
                    Rect(0, 0, self.Map.width, self.Map.height)
                )

            self.pdc.DrawBitmap(self.bufferLast, 0, 0, False)
            self.pdc.DrawToDC(dc)

        # draw semitransparent objects (e.g. region box, raster digitizer
        # objects)
        try:
            gcdc = wx.GCDC(dc)
            self.pdcTransparent.DrawToDC(gcdc)
        except NotImplementedError as e:
            print(e, file=sys.stderr)
            self.pdcTransparent.DrawToDC(dc)

        # draw temporary object on the foreground
        self.pdcTmp.DrawToDC(dc)

        if switchDraw:
            self.redrawAll = False

    def OnSize(self, event):
        """Scale map image so that it is the same size as the Window"""
        # re-render image on idle
        self.resize = gs.clock()

    def OnIdle(self, event):
        """Only re-render a composite map image from GRASS during
        idle time instead of multiple times during resizing.
        """

        # use OnInternalIdle() instead ?

        if self.resize and self.resize + 0.2 < gs.clock():
            Debug.msg(3, "BufferedWindow.OnSize():")

            # set size of the input image
            self.Map.ChangeMapSize(self.GetClientSize())

            # Make new off screen bitmap: this bitmap will always have the
            # current drawing in it, so it can be used to save the image to
            # a file, or whatever.
            self._buffer.Destroy()
            self._buffer = EmptyBitmap(max(1, self.Map.width), max(1, self.Map.height))

            # get the image to be rendered
            self.img = self.GetImage()

            # update map display
            updatemap = True
            if (
                self.img and self.Map.width + self.Map.height > 0
            ):  # scale image after resize
                self.img = self.img.Scale(self.Map.width, self.Map.height)
                if len(self.Map.GetListOfLayers()) > 0:
                    self.UpdateMap()
                    updatemap = False

            if updatemap:
                self.UpdateMap(render=True)
            self.resize = False
        elif self.resize:
            event.RequestMore()

        event.Skip()

    def SaveToFile(self, FileName, FileType, width, height, callback=None):
        """This draws the pseudo DC to a buffer that can be saved to
        a file.

        :param filename: file name
        :param FileType: type of bitmap
        :param width: image width
        :param height: image height
        """
        Debug.msg(1, "MapWindow.SaveToFile(): %s (%dx%d)", FileName, width, height)

        self._fileName = FileName
        self._fileType = FileType
        self._saveToFileCallback = callback

        self._busy = wx.BusyInfo(_("Please wait, exporting image..."), parent=self)
        wx.GetApp().Yield()

        self.Map.ChangeMapSize((width, height))
        renderMgr = self.Map.GetRenderMgr()
        # this seems wrong, rendering should have callback
        # when callback present, rendering does not emit signal
        # just calls callback
        renderMgr.renderDone.disconnect(self._updateMFinished)
        renderMgr.renderDone.connect(self._saveToFileDone)
        self.Map.Render(force=True, windres=self._properties.resolution)

    def _saveToFileDone(self, callback=None):
        renderMgr = self.Map.GetRenderMgr()
        renderMgr.renderDone.disconnect(self._saveToFileDone)

        ibuffer = EmptyBitmap(max(1, self.Map.width), max(1, self.Map.height))

        img = self.GetImage()
        self.pdc.RemoveAll()
        self.Draw(self.pdc, img, drawid=99)

        # compute size ratio to move overlay accordingly
        cSize = self.GetClientSize()
        ratio = float(self.Map.width) / cSize[0], float(self.Map.height) / cSize[1]

        # redraw legend, scalebar
        for img in self.GetOverlay():
            # draw any active and defined overlays
            if self.imagedict[img]["layer"].IsActive():
                id = self.imagedict[img]["id"]
                coords = (
                    int(ratio[0] * self.overlays[id].coords[0]),
                    int(ratio[1] * self.overlays[id].coords[1]),
                )
                self.Draw(
                    self.pdc,
                    img=img,
                    drawid=id,
                    pdctype=self.overlays[id].pdcType,
                    coords=coords,
                )

        # redraw text labels
        for id in list(self.textdict.keys()):
            textinfo = self.textdict[id]
            oldCoords = textinfo["coords"]
            textinfo["coords"] = (
                ratio[0] * textinfo["coords"][0],
                ratio[1] * textinfo["coords"][1],
            )
            self.Draw(self.pdc, img=self.textdict[id], drawid=id, pdctype="text")
            # set back old coordinates
            textinfo["coords"] = oldCoords

        dc = wx.BufferedDC(None, ibuffer)
        dc.Clear()
        # probably does nothing, removed from wxPython 2.9
        # self.PrepareDC(dc)
        self.pdc.DrawToDC(dc)
        if self.digit:
            self.pdcVector.DrawToDC(dc)
        ibuffer.SaveFile(self._fileName, self._fileType)

        del self._busy
        del self._fileName
        del self._fileType

        renderMgr.renderDone.connect(self._updateMFinished)

        self.UpdateMap(render=True)
        self.Refresh()
        if self._saveToFileCallback:
            self._saveToFileCallback()

    def GetOverlay(self):
        """Converts rendered overlay files to wx.Image

        Updates self.imagedict

        :return: list of images
        """
        imgs = []
        for overlay in self.Map.GetListOfLayers(ltype="overlay", active=True):
            if (
                overlay.mapfile is None
                or not os.path.isfile(overlay.mapfile)
                or (not os.path.getsize(overlay.mapfile))
            ):
                continue
            img = utils.autoCropImageFromFile(overlay.mapfile)

            for key in list(self.imagedict.keys()):
                if self.imagedict[key]["id"] == overlay.id:
                    del self.imagedict[key]

            self.imagedict[img] = {"id": overlay.id, "layer": overlay}
            imgs.append(img)

        return imgs

    def GetImage(self):
        """Converts rendered map files to wx.Image

        Updates self.imagedict (id=99)

        :return: wx.Image instance (map composition)
        """
        imgId = 99
        if (
            self.Map.mapfile
            and os.path.isfile(self.Map.mapfile)
            and os.path.getsize(self.Map.mapfile)
        ):
            img = wx.Image(self.Map.mapfile, wx.BITMAP_TYPE_ANY)
        else:
            img = None

        for key in list(self.imagedict.keys()):
            if self.imagedict[key]["id"] == imgId:
                del self.imagedict[key]

        self.imagedict[img] = {"id": imgId}

        return img

    def SetAlwaysRenderEnabled(self, alwaysRender=True):
        self.alwaysRender = alwaysRender

    def IsAlwaysRenderEnabled(self):
        return self.alwaysRender

    def UpdateMap(
        self,
        render=True,
        renderVector=True,
        delay=0.0,
        reRenderTool=False,
    ):
        """Updates the canvas anytime there is a change to the
        underlying images or to the geometry of the canvas.

        This method should not be called directly.

        .. todo::
            change direct calling of UpdateMap method to emitting grass
            interface updateMap signal

        .. todo::
            consider using strong/weak signal instead of delay limit in
            giface

        :param render: re-render map composition
        :param renderVector: re-render vector map layer enabled for editing (used for
                             digitizer)
        :param delay: defines time threshold  in seconds for postponing
                      rendering to merge more update requests.
        :param reRenderTool bool: enable re-render map if True, when
                                  auto re-render map is disabled and
                                  Display map or Render map tool is
                                  activated from the Map Display toolbar

        If another request comes within the limit, rendering is delayed
        again. Next delay limit is chosen according to the smallest
        delay value of all requests which have come during waiting period.

        Let say that first UpdateMap request come with 5 second delay
        limit. After 4  seconds of waiting another UpdateMap request
        come with delay limit of 2.5 seconds. New waiting period is set
        to 2.5 seconds, because limit of the second request is the
        smallest. If no other request comes rendering will be done
        after 6.5 seconds from the first request.

        Arguments 'render' and 'renderVector' have priority for True.
        It means that if more UpdateMap requests come within waiting
        period and at least one request has argument set for True, map
        will be updated with the True value of the argument.
        """

        if not self._properties.autoRender and not reRenderTool:
            return

        if self.timerRunId is None or delay < self.updDelay:
            self.updDelay = delay

        if render:
            self.render = render
        if renderVector:
            self.renderVector = renderVector

        updTime = time.time()
        self.lastUpdateMapReq = updTime

        if self.updDelay < 0.0:
            self._runUpdateMap()
        else:
            self.timerRunId = self.renderTimingThr.GetId()
            self.renderTimingThr.Run(
                callable=self._timingFunction,
                ondone=self._onUpdateMap,
                pid=self.timerRunId,
            )

    def _timingFunction(self, pid):
        """Timer measuring elapsed time, since last update request.

        It terminates, when delay limit is exceeded.

        :param pid: id which defines whether it is newest timer, or
                    there is another one (representing newer Update map
                    request). If it is not the newest, it is terminated.
        """
        while True:
            updTime = time.time()
            time.sleep(0.01)
            if (
                updTime > self.lastUpdateMapReq + self.updDelay
                or pid != self.timerRunId
            ):
                return

    def _onUpdateMap(self, event):
        if self and self.timerRunId == event.pid:
            self._runUpdateMap()

    def _runUpdateMap(self):
        """Update map when delay limit is over."""
        self.timerRunId = None
        self._updateM(self.render, self.renderVector)
        self.render = self.renderVector = False

    def _updateM(self, render=True, renderVector=True):
        """
        :func:`UpdateMap` for arguments description.
        """
        Debug.msg(
            1,
            "BufferedWindow.UpdateMap(): started "
            "(render=%s, renderVector=%s)" % (render, renderVector),
        )

        # was if self.Map.cmdfile and ...
        if self.IsAlwaysRenderEnabled() and self.img is None:
            render = True

        try:
            if render:
                # update display size
                self.Map.ChangeMapSize(self.GetClientSize())

            self.Map.Render(force=render, windres=self._properties.resolution)
        except GException as e:
            GError(message=e.value)

    def _updateMFinished(self, renderVector=True):
        Debug.msg(1, "BufferedWindow.UpdateMap(): finished")
        self.img = self.GetImage()  # id=99

        #
        # clear pseudoDcs
        #
        for pdc in (self.pdc, self.pdcTransparent, self.pdcTmp):
            pdc.Clear()
            pdc.RemoveAll()

        #
        # draw background map image to PseudoDC
        #
        if not self.img:
            self.Draw(self.pdc, pdctype="clear")
        else:
            try:
                id = self.imagedict[self.img]["id"]
            except Exception as e:
                Debug.mgs(1, "UpdateMap() failed: %s", e)
                return False
            self.Draw(self.pdc, self.img, drawid=id)

        #
        # render vector map layer
        #
        if renderVector and self.digit:
            self._updateMap()

        #
        # render overlays
        #
        for img in self.GetOverlay():
            # draw any active and defined overlays
            if self.imagedict[img]["layer"].IsActive():
                id = self.imagedict[img]["id"]
                self.Draw(
                    self.pdc,
                    img=img,
                    drawid=id,
                    pdctype=self.overlays[id].pdcType,
                    coords=self.overlays[id].coords,
                )

        for id in list(self.textdict.keys()):
            self.Draw(
                self.pdc,
                img=self.textdict[id],
                drawid=id,
                pdctype="text",
                coords=[10, 10, 10, 10],
            )

        # optionally draw computational extent box
        self.DrawCompRegionExtent()

        #
        # redraw pdcTmp if needed
        #

        # draw registered graphics
        if len(self.graphicsSetList) > 0:
            penOrig = self.pen
            polypenOrig = self.polypen

            for item in self.graphicsSetList:
                try:
                    item.Draw()
                except Exception:
                    GError(
                        parent=self,
                        message=_(
                            "Unable to draw registered graphics. "
                            "The graphics was unregistered."
                        ),
                    )
                    self.UnregisterGraphicsToDraw(item)

            self.pen = penOrig
            self.polypen = polypenOrig

        if len(self.polycoords) > 0:
            self.DrawLines(self.pdcTmp)

        return True

    def DrawCompRegionExtent(self):
        """Draw computational region extent in the display

        Display region is drawn as a blue box inside the computational region,
        computational region inside a display region as a red box).
        """
        if self._properties.showRegion:
            compReg = self.Map.GetRegion()
            dispReg = self.Map.GetCurrentRegion()
            reg = dispReg if utils.isInRegion(dispReg, compReg) else compReg

            regionCoords = [
                (reg["w"], reg["n"]),
                (reg["e"], reg["n"]),
                (reg["e"], reg["s"]),
                (reg["w"], reg["s"]),
                (reg["w"], reg["n"]),
            ]

            # draw region extent
            self.polypen = wx.Pen(
                colour=wx.Colour(255, 0, 0, 128), width=3, style=wx.SOLID
            )
            self.DrawLines(pdc=self.pdcTransparent, polycoords=regionCoords)

    def EraseMap(self):
        """Erase map canvas"""
        self.Draw(self.pdc, pdctype="clear")

        if self.digit:
            self.Draw(self.pdcVector, pdctype="clear")

        self.Draw(self.pdcTransparent, pdctype="clear")
        self.Draw(self.pdcTmp, pdctype="clear")

        self.Map.AbortAllThreads()

    def DragMap(self, moveto):
        """Drag the entire map image for panning.

        :param moveto: dx,dy
        """
        # Prevent drag map error if auto re-render map is disabled
        # wx._core.wxAssertionError: C++ assertion
        # "!wxMouseCapture::IsInCaptureStack(this) failed at
        # /tmp/pip-req-build-oxkmg2wi/ext/wxWidgets/src/common/wincmn.cpp(3270) in
        # CaptureMouse(): Recapturing the mouse in the same window?
        if not self._properties.autoRender:
            return

        dc = wx.BufferedDC(wx.ClientDC(self))
        dc.SetBackground(wx.Brush("White"))
        dc.Clear()

        self.dragimg = DragImage(self._buffer)
        self.dragimg.BeginDrag((0, 0), self)
        self.dragimg.GetImageRect(moveto)
        self.dragimg.Move(moveto)

        self.dragimg.DoDrawImage(dc, moveto)
        self.dragimg.EndDrag()

    def DragItem(self, id, coords):
        """Drag an overlay decoration item"""
        if id in (99, "") or id is None:
            return
        Debug.msg(5, "BufferedWindow.DragItem(): id=%d" % id)
        x, y = self.lastpos
        dx = coords[0] - x
        dy = coords[1] - y
        self.pdc.SetBackground(wx.Brush(self.GetBackgroundColour()))
        r = self.pdc.GetIdBounds(id)

        if isinstance(r, list):
            r = Rect(r[0], r[1], r[2], r[3])
        if id in self.textdict:  # text dragging
            rtop = (r[0], r[1] - r[3], r[2], r[3])
            r = r.Union(rtop)
            rleft = (r[0] - r[2], r[1], r[2], r[3])
            r = r.Union(rleft)

        self.pdc.TranslateId(id, dx, dy)

        r2 = self.pdc.GetIdBounds(id)
        if isinstance(r2, list):
            r2 = Rect(r[0], r[1], r[2], r[3])
        if id in self.textdict:  # text
            self.textdict[id]["bbox"] = r2
            self.textdict[id]["coords"][0] += dx
            self.textdict[id]["coords"][1] += dy

        r = r.Union(r2)
        r.Inflate(4, 4)
        self.RefreshRect(r, False)
        self.lastpos = (coords[0], coords[1])

    def MouseDraw(self, pdc=None, begin=None, end=None):
        """Mouse box or line from 'begin' to 'end'

        If not given from self.mouse['begin'] to self.mouse['end'].
        """
        if not pdc:
            return

        if begin is None:
            begin = self.mouse["begin"]
        if end is None:
            end = self.mouse["end"]

        Debug.msg(
            5,
            "BufferedWindow.MouseDraw(): use=%s, box=%s, begin=%f,%f, end=%f,%f"
            % (
                self.mouse["use"],
                self.mouse["box"],
                begin[0],
                begin[1],
                end[0],
                end[1],
            ),
        )

        if self.mouse["box"] == "box":
            boxid = wx.ID_NEW
            mousecoords = [begin[0], begin[1], end[0], end[1]]
            r = pdc.GetIdBounds(boxid)
            if isinstance(r, list):
                r = Rect(r[0], r[1], r[2], r[3])
            r.Inflate(4, 4)
            pdc.ClearId(boxid)
            self.RefreshRect(r, False)
            pdc.SetId(boxid)
            self.Draw(pdc, drawid=boxid, pdctype="box", coords=mousecoords)

        elif self.mouse["box"] == "line":
            self.lineid = wx.ID_NEW
            mousecoords = [begin[0], begin[1], end[0], end[1]]
            x1 = min(begin[0], end[0])
            x2 = max(begin[0], end[0])
            y1 = min(begin[1], end[1])
            y2 = max(begin[1], end[1])
            r = Rect(x1, y1, x2 - x1, y2 - y1)
            r.Inflate(4, 4)
            pdc.ClearId(self.lineid)
            self.RefreshRect(r, False)
            pdc.SetId(self.lineid)
            self.Draw(pdc, drawid=self.lineid, pdctype="line", coords=mousecoords)

    def DrawLines(self, pdc=None, polycoords=None):
        """Draw polyline in PseudoDC

        Set self.pline to wx.NEW_ID + 1

        :param polycoords: list of polyline vertices, geographical
                           coordinates (if not given, self.polycoords is used)
        """
        if not pdc:
            pdc = self.pdcTmp

        if not polycoords:
            polycoords = self.polycoords

        if len(polycoords) <= 0:
            return -1
        self.plineid = wx.ID_NEW + 1
        # convert from EN to XY
        coords = [self.Cell2Pixel(p) for p in polycoords]
        self.Draw(pdc, drawid=self.plineid, pdctype="polyline", coords=coords)
        Debug.msg(
            4,
            "BufferedWindow.DrawLines(): coords=%s, id=%s" % (coords, self.plineid),
        )
        return self.plineid

    def DrawPolylines(self, pdc, coords, pen, drawid=None):
        """Draw polyline in PseudoDC.

        This is similar to DrawLines but this is used with GraphicsSet,
        coordinates should be always in pixels.

        :param pdc: PseudoDC
        :param coords: list of coordinates (pixel coordinates)
        :param pen: pen to be used
        :param drawid: id of the drawn object (used by PseudoDC)
        """
        Debug.msg(4, "BufferedWindow.DrawPolylines(): coords=%s" % coords)
        self.lineId = self.Draw(
            pdc, drawid=None, pdctype="polyline", coords=coords, pen=pen
        )

        return self.lineid

    def DrawCross(
        self,
        pdc,
        coords,
        size,
        rotation=0,
        pen=None,
        text=None,
        textAlign="lr",
        textOffset=(5, 5),
        drawid=None,
    ):
        """Draw cross in PseudoDC

        .. todo::
            implement rotation

        :param pdc: PseudoDC
        :param coords: center coordinates (pixel coordinates)
        :param rotation: rotate symbol
        :param text: draw also text (text, font, color, rotation)
        :param textAlign: alignment (default 'lower-right')
        :param textOffset: offset for text (from center point)
        :param drawid: id of the drawn object (used by PseudoDC)
        """
        Debug.msg(
            4,
            "BufferedWindow.DrawCross(): pdc=%s, coords=%s, size=%d"
            % (pdc, coords, size),
        )
        coordsCross = (
            (coords[0], coords[1] - size, coords[0], coords[1] + size),
            (coords[0] - size, coords[1], coords[0] + size, coords[1]),
        )

        self.lineid = self.Draw(
            pdc, drawid=drawid, pdctype="lines", coords=coordsCross, pen=pen
        )

        if not text:
            return self.lineid

        if textAlign == "ul":
            coord = [coords[0] - textOffset[0], coords[1] - textOffset[1], 0, 0]
        elif textAlign == "ur":
            coord = [coords[0] + textOffset[0], coords[1] - textOffset[1], 0, 0]
        elif textAlign == "lr":
            coord = [coords[0] + textOffset[0], coords[1] + textOffset[1], 0, 0]
        else:
            coord = [coords[0] - textOffset[0], coords[1] + textOffset[1], 0, 0]

        self.Draw(pdc, img=text, pdctype="text", coords=coord, pen=pen)

        return self.lineid

    def DrawRectangle(self, pdc, point1, point2, pen, brush=None, drawid=None):
        """Draw rectangle (not filled) in PseudoDC

        :param pdc: PseudoDC
        :param point1: top left corner (pixel coordinates)
        :param point2: bottom right corner (pixel coordinates)
        :param pen: pen
        :param drawid: id of the drawn object (used by PseudoDC)
        """
        Debug.msg(
            4,
            "BufferedWindow.DrawRectangle(): pdc=%s, point1=%s, point2=%s"
            % (pdc, point1, point2),
        )
        coords = [point1[0], point1[1], point2[0], point2[1]]
        self.lineid = self.Draw(
            pdc, drawid=drawid, pdctype="box", coords=coords, pen=pen, brush=brush
        )
        return self.lineid

    def DrawCircle(self, pdc, coords, radius, pen, brush=None, drawid=None):
        """Draw circle (not filled) in PseudoDC

        :param pdc: PseudoDC
        :param coords: center (pixel coordinates)
        :param radius: radius
        :param pen: pen
        :param drawid: id of the drawn object (used by PseudoDC)
        """
        Debug.msg(
            4,
            "BufferedWindow.DrawCircle(): pdc=%s, coords=%s, radius=%s"
            % (pdc, coords, radius),
        )
        newcoords = [
            coords[0] - radius,
            coords[1] - radius,
            coords[0] + radius,
            coords[1] + radius,
        ]
        self.lineid = self.Draw(
            pdc, drawid=drawid, pdctype="circle", coords=newcoords, pen=pen, brush=brush
        )
        return self.lineid

    def DrawPolygon(self, pdc, coords, pen, brush=None, drawid=None):
        """Draws polygon from a list of points (do not append the first point)

        :param pdc: PseudoDC
        :param coords: list of coordinates (pixel coordinates)
        :param pen: pen
        :param drawid: id of the drawn object (used by PseudoDC)
        """
        # avid wx.GCDC assert
        if len(coords) <= 1:
            return None
        self.lineid = self.Draw(
            pdc, drawid=drawid, pdctype="polygon", coords=coords, pen=pen, brush=brush
        )
        return self.lineid

    def _computeZoomToPointAndRecenter(self, position, zoomtype):
        """Computes zoom parameters for recenter mode.

        Computes begin and end parameters for Zoom() method.
        Used for zooming by single click (not box)
        and mouse wheel zooming (zoom and recenter mode).
        """
        if zoomtype > 0:
            begin = (
                position[0] - self.Map.width / 4,
                position[1] - self.Map.height / 4,
            )
            end = (position[0] + self.Map.width / 4, position[1] + self.Map.height / 4)
        else:
            begin = (
                (self.Map.width - position[0]) / 2,
                (self.Map.height - position[1]) / 2,
            )
            end = (begin[0] + self.Map.width / 2, begin[1] + self.Map.height / 2)
        return begin, end

    def MouseActions(self, event):
        """Mouse motion and button click notifier"""
        if not self.processMouse:
            return

        # zoom with mouse wheel
        if event.GetWheelRotation() != 0:
            self.OnMouseWheel(event)

        # left mouse button pressed
        elif event.LeftDown():
            self.OnLeftDown(event)

        # left mouse button released
        elif event.LeftUp():
            self.OnLeftUp(event)

        # dragging
        elif event.Dragging():
            self.OnDragging(event)

        # double click
        elif event.ButtonDClick():
            self.OnButtonDClick(event)

        # middle mouse button pressed
        elif event.MiddleDown():
            self.OnMiddleDown(event)

        # middle mouse button released
        elif event.MiddleUp():
            self.OnMiddleUp(event)

        # right mouse button pressed
        elif event.RightDown():
            self.OnRightDown(event)

        # right mouse button released
        elif event.RightUp():
            self.OnRightUp(event)

        elif event.Entering():
            self.OnMouseEnter(event)

        elif event.Moving():
            pixelCoordinates = event.GetPosition()
            coordinates = self.Pixel2Cell(pixelCoordinates)
            self.mouseMoving.emit(x=coordinates[0], y=coordinates[1])
            self.OnMouseMoving(event)

    def OnMouseWheel(self, event):
        """Mouse wheel moved"""
        zoomBehaviour = UserSettings.Get(
            group="display", key="mouseWheelZoom", subkey="selection"
        )
        if zoomBehaviour == 2:
            event.Skip()
            return

        self.processMouse = False
        current = event.GetPosition()
        wheel = event.GetWheelRotation()
        Debug.msg(5, "BufferedWindow.MouseAction(): wheel=%d" % wheel)

        zoomtype = 1 if wheel > 0 else -1
        if UserSettings.Get(group="display", key="scrollDirection", subkey="selection"):
            zoomtype *= -1
        # zoom 1/2 of the screen (TODO: settings)
        if zoomBehaviour == 0:  # zoom and recenter
            begin, end = self._computeZoomToPointAndRecenter(
                position=current, zoomtype=zoomtype
            )

        elif zoomBehaviour == 1:  # zoom to current cursor position
            begin = (current[0] / 2, current[1] / 2)
            end = (
                (self.Map.width - current[0]) / 2 + current[0],
                (self.Map.height - current[1]) / 2 + current[1],
            )

        # zoom
        self.Zoom(begin, end, zoomtype)

        # redraw map
        self.UpdateMap(delay=0.2)

        self.Refresh()
        self.processMouse = True

    def OnDragging(self, event):
        """Mouse dragging"""
        Debug.msg(5, "BufferedWindow.MouseAction(): Dragging")
        current = event.GetPosition()
        previous = self.mouse["begin"]
        move = (current[0] - previous[0], current[1] - previous[1])

        digitToolbar = self.toolbar if self.digit else None

        # dragging or drawing box with left button
        if self.mouse["use"] == "pan" or event.MiddleIsDown():
            self.DragMap(move)

        # dragging decoration overlay item
        elif (
            self.mouse["use"] == "pointer"
            and not digitToolbar
            and self.dragid is not None
        ):
            coords = event.GetPosition()
            self.DragItem(self.dragid, coords)

        # dragging anything else - rubber band box or line
        else:
            if self.mouse["use"] == "pointer" and not digitToolbar:
                return

            self.mouse["end"] = event.GetPosition()
            if event.LeftIsDown() and not (
                digitToolbar
                and digitToolbar.GetAction() == "moveLine"
                and len(self.digit.GetDisplay().GetSelected()) > 0
            ):
                self.MouseDraw(pdc=self.pdcTmp)

    def OnLeftDown(self, event):
        """Left mouse button pressed"""
        Debug.msg(5, "BufferedWindow.OnLeftDown(): use=%s" % self.mouse["use"])

        self.mouse["begin"] = event.GetPosition()

        # vector digizer
        if self.mouse["use"] == "pointer" and self.digit:
            if event.ControlDown():
                self.OnLeftDownUndo(event)
            else:
                self._onLeftDown(event)

        elif self.mouse["use"] == "pointer":
            # get decoration or text id
            idlist = []
            self.dragid = ""
            self.lastpos = self.mouse["begin"]
            idlist = self.pdc.FindObjects(
                self.lastpos[0], self.lastpos[1], self.hitradius
            )
            if 99 in idlist:
                idlist.remove(99)
            if idlist != []:
                self.dragid = idlist[0]  # drag whatever is on top

        coords = self.Pixel2Cell(self.mouse["begin"])
        self.mouseLeftDown.emit(x=coords[0], y=coords[1])

        event.Skip()

    def OnLeftUp(self, event):
        """Left mouse button released

        Emits mapQueried signal when mouse use is 'query'.
        """
        Debug.msg(5, "BufferedWindow.OnLeftUp(): use=%s" % self.mouse["use"])

        self.mouse["end"] = event.GetPosition()
        coordinates = self.Pixel2Cell(self.mouse["end"])

        if self.mouse["use"] in {"zoom", "pan"}:
            # set region in zoom or pan
            begin = self.mouse["begin"]
            end = self.mouse["end"]

            if self.mouse["use"] == "zoom":
                # set region for click (zero-width box)
                if begin[0] - end[0] == 0 or begin[1] - end[1] == 0:
                    begin, end = self._computeZoomToPointAndRecenter(
                        position=end, zoomtype=self.zoomtype
                    )
            self.Zoom(begin, end, self.zoomtype)

            # redraw map
            self.UpdateMap(render=True)

        elif self.mouse["use"] == "query":
            self.mapQueried.emit(x=self.mouse["end"][0], y=self.mouse["end"][1])

        elif self.mouse["use"] == "pointer" and self.digit:
            self._onLeftUp(event)

        elif self.mouse["use"] == "pointer":
            if self.dragid:
                # end drag of overlay decoration
                if self.overlays and self.dragid in self.overlays:
                    self.overlays[self.dragid].coords = self.pdc.GetIdBounds(
                        self.dragid
                    )
                elif self.dragid in self.textdict:
                    self.textdict[self.dragid]["bbox"] = self.pdc.GetIdBounds(
                        self.dragid
                    )
                self.dragid = None

            self.mouseLeftUpPointer.emit(x=coordinates[0], y=coordinates[1])

        elif self.mouse["use"] == "drawRegion":
            coordinatesBegin = self.Pixel2Cell(self.mouse["begin"])

            if coordinatesBegin[0] < coordinates[0]:
                west = coordinatesBegin[0]
                east = coordinates[0]
            else:
                west = coordinates[0]
                east = coordinatesBegin[0]
            if coordinatesBegin[1] < coordinates[1]:
                south = coordinatesBegin[1]
                north = coordinates[1]
            else:
                south = coordinates[1]
                north = coordinatesBegin[1]

            region = self.Map.GetRegion()
            RunCommand(
                "g.region",
                parent=self,
                flags="a",
                nsres=region["nsres"],
                ewres=region["ewres"],
                n=north,
                s=south,
                e=east,
                w=west,
            )

            # redraw map
            self.UpdateMap(render=False)

        # TODO: decide which coordinates to send (e, n, mouse['begin'],
        # mouse['end'])
        self.mouseLeftUp.emit(x=coordinates[0], y=coordinates[1])

    def OnButtonDClick(self, event):
        """Mouse button double click"""
        Debug.msg(5, "BufferedWindow.OnButtonDClick(): use=%s" % self.mouse["use"])

        screenCoords = event.GetPosition()

        if self.mouse["use"] == "pointer":
            # select overlay decoration options dialog
            idlist = self.pdc.FindObjects(
                screenCoords[0], screenCoords[1], self.hitradius
            )
            if idlist and idlist[0] != 99:
                self.dragid = idlist[0]
                self.overlayActivated.emit(overlayId=self.dragid)

        coords = self.Pixel2Cell(screenCoords)
        self.mouseDClick.emit(x=coords[0], y=coords[1])

    def OnRightDown(self, event):
        """Right mouse button pressed"""
        Debug.msg(5, "BufferedWindow.OnRightDown(): use=%s" % self.mouse["use"])

        if self.digit:
            self._onRightDown(event)

        event.Skip()

    def OnRightUp(self, event):
        """Right mouse button released"""
        Debug.msg(5, "BufferedWindow.OnRightUp(): use=%s" % self.mouse["use"])

        if self.digit:
            self._onRightUp(event)

        self.redrawAll = True
        self.Refresh()

        coords = self.Pixel2Cell(event.GetPosition())
        self.mouseRightUp.emit(x=coords[0], y=coords[1])

        event.Skip()

    def OnMiddleDown(self, event):
        """Middle mouse button pressed"""
        if not event:
            return

        self.mouse["begin"] = event.GetPosition()

    def OnMiddleUp(self, event):
        """Middle mouse button released"""
        self.mouse["end"] = event.GetPosition()

        # set region in zoom or pan
        begin = self.mouse["begin"]
        end = self.mouse["end"]

        self.Zoom(begin, end, 0)  # no zoom

        # redraw map
        self.UpdateMap(render=True)

    def OnMouseEnter(self, event):
        """Mouse entered window and no mouse buttons were pressed

        Emits the mouseEntered signal.
        """
        self.mouseEntered.emit()
        event.Skip()

    def OnMouseMoving(self, event):
        """Motion event and no mouse buttons were pressed"""
        if self.mouse["use"] == "pointer" and self.digit:
            self._onMouseMoving(event)

        pos = event.GetPosition()
        idlist = self.pdc.FindObjects(pos[0], pos[1], self.hitradius)
        if (
            self.overlays
            and idlist
            and [i for i in idlist if i in list(self.overlays.keys())]
        ):  # legend, scale bar, north arrow, dtext
            self.SetToolTip("Right click to modify or remove")
        else:
            self.SetToolTip(None)
        event.Skip()

    def OnCopyCoordinates(self, event):
        """Copy coordinates to clipboard"""
        e, n = self.GetLastEN()
        if wx.TheClipboard.Open():
            do = wx.TextDataObject()
            # TODO: put delimiter in settings and apply also for Go to in
            # statusbar
            delim = ","
            do.SetText(str(e) + delim + str(n))
            wx.TheClipboard.SetData(do)
            wx.TheClipboard.Close()

    def ClearLines(self, pdc=None):
        """Clears temporary drawn lines from PseudoDC"""
        if not pdc:
            pdc = self.pdcTmp
        try:
            pdc.ClearId(self.lineid)
            pdc.RemoveId(self.lineid)
        except (KeyError, TypeError):
            pass

        try:
            pdc.ClearId(self.plineid)
            pdc.RemoveId(self.plineid)
        except (KeyError, TypeError):
            pass

        Debug.msg(
            4,
            "BufferedWindow.ClearLines(): lineid=%s, plineid=%s"
            % (self.lineid, self.plineid),
        )

        return True

    def Pixel2Cell(self, xyCoords):
        """Convert image coordinates to real word coordinates

        :param xyCoords: image coordinates

        :return: easting, northing
        :return: None on error
        """
        try:
            x = int(xyCoords[0])
            y = int(xyCoords[1])
        except (TypeError, ValueError, IndexError):
            return None

        if self.Map.region["ewres"] > self.Map.region["nsres"]:
            res = self.Map.region["ewres"]
        else:
            res = self.Map.region["nsres"]

        w = self.Map.region["center_easting"] - (self.Map.width / 2) * res
        n = self.Map.region["center_northing"] + (self.Map.height / 2) * res

        east = w + x * res
        north = n - y * res

        return (east, north)

    def Cell2Pixel(self, enCoords):
        """Convert real word coordinates to image coordinates"""
        try:
            east = float(enCoords[0])
            north = float(enCoords[1])
        except (TypeError, ValueError, IndexError):
            return None

        if self.Map.region["ewres"] > self.Map.region["nsres"]:
            res = self.Map.region["ewres"]
        else:
            res = self.Map.region["nsres"]

        w = self.Map.region["center_easting"] - (self.Map.width / 2) * res
        n = self.Map.region["center_northing"] + (self.Map.height / 2) * res

        x = round((east - w) / res)
        y = round((n - north) / res)

        return (x, y)

    def Zoom(self, begin, end, zoomtype):
        """Calculates new region while (un)zoom/pan-ing"""
        x1, y1 = begin
        x2, y2 = end
        newreg = {}

        # threshold - too small squares do not make sense
        # can only zoom to windows of > 5x5 screen pixels
        if abs(x2 - x1) > 5 and abs(y2 - y1) > 5 and zoomtype != 0:
            if x1 > x2:
                x1, x2 = x2, x1
            if y1 > y2:
                y1, y2 = y2, y1

            # zoom in
            if zoomtype > 0:
                newreg["w"], newreg["n"] = self.Pixel2Cell((x1, y1))
                newreg["e"], newreg["s"] = self.Pixel2Cell((x2, y2))

            # zoom out
            elif zoomtype < 0:
                newreg["w"], newreg["n"] = self.Pixel2Cell((-x1 * 2, -y1 * 2))
                newreg["e"], newreg["s"] = self.Pixel2Cell(
                    (
                        self.Map.width + 2 * (self.Map.width - x2),
                        self.Map.height + 2 * (self.Map.height - y2),
                    )
                )
        # pan
        elif zoomtype == 0:
            dx = x1 - x2
            dy = y1 - y2
            if dx == 0 and dy == 0:
                dx = x1 - self.Map.width / 2
                dy = y1 - self.Map.height / 2
            newreg["w"], newreg["n"] = self.Pixel2Cell((dx, dy))
            newreg["e"], newreg["s"] = self.Pixel2Cell(
                (self.Map.width + dx, self.Map.height + dy)
            )

        # if new region has been calculated, set the values
        if newreg != {}:
            # LL locations
            if self.Map.projinfo["proj"] == "ll":
                self.Map.region["n"] = min(self.Map.region["n"], 90.0)
                self.Map.region["s"] = max(self.Map.region["s"], -90.0)

            ce = newreg["w"] + (newreg["e"] - newreg["w"]) / 2
            cn = newreg["s"] + (newreg["n"] - newreg["s"]) / 2

            # calculate new center point and display resolution
            self.Map.region["center_easting"] = ce
            self.Map.region["center_northing"] = cn
            self.Map.region["ewres"] = (newreg["e"] - newreg["w"]) / self.Map.width
            self.Map.region["nsres"] = (newreg["n"] - newreg["s"]) / self.Map.height
            if self._properties.alignExtent:
                self.Map.AlignExtentFromDisplay()
            else:
                for k in ("n", "s", "e", "w"):
                    self.Map.region[k] = newreg[k]

            if self.digit and hasattr(self, "moveInfo"):
                self._zoom(None)

            self.ZoomHistory(
                self.Map.region["n"],
                self.Map.region["s"],
                self.Map.region["e"],
                self.Map.region["w"],
            )

        if self.redrawAll is False:
            self.redrawAll = True

    def ZoomBack(self):
        """Zoom to previous extents in zoomhistory list

        Emits zoomChanged signal.
        Emits zoomHistoryUnavailable signal when stack is empty.
        """
        Debug.msg(4, "BufferedWindow.ZoomBack(): hist)=%s" % self.zoomhistory)

        zoom = []

        if len(self.zoomhistory) > 1:
            self.zoomhistory.pop()
            zoom = self.zoomhistory[-1]

        if len(self.zoomhistory) < 2:
            self.zoomHistoryUnavailable.emit()

        # zoom to selected region
        self.Map.GetRegion(n=zoom[0], s=zoom[1], e=zoom[2], w=zoom[3], update=True)
        # update map
        self.UpdateMap()

        self.zoomChanged.emit()

    def ZoomHistory(self, n, s, e, w):
        """Manages a list of last 10 zoom extents

        Emits zoomChanged signal.
        Emits zoomHistoryAvailable signal when stack is not empty.
        Emits zoomHistoryUnavailable signal when stack is empty.

        All methods which are changing zoom should call this method
        to make a record in the history. The signal zoomChanged will be
        then emitted automatically.

        :param n,s,e,w: north, south, east, west

        :return: removed history item if exists (or None)
        """
        removed = None
        self.zoomhistory.append((n, s, e, w))

        if len(self.zoomhistory) > 10:
            removed = self.zoomhistory.pop(0)

        if removed:
            Debug.msg(
                4,
                "BufferedWindow.ZoomHistory(): hist=%s, removed=%s"
                % (self.zoomhistory, removed),
            )
        else:
            Debug.msg(4, "BufferedWindow.ZoomHistory(): hist=%s" % (self.zoomhistory))

        # update toolbar
        if len(self.zoomhistory) > 1:
            self.zoomHistoryAvailable.emit()
        else:
            self.zoomHistoryUnavailable.emit()

        self.zoomChanged.emit()

        return removed

    def InitZoomHistory(self):
        """Initializes zoom history.

        .. todo::
            First item is handled in some special way. Improve the
            documentation or fix the code.

        It does not emits any signals.

        This method can be possibly removed when the history will solve the
        fist item in different way or when GCP manager (and possibly others)
        will handle Map variable in the way that it will be prepared for
        MapWindow/BufferedWindow and thus usable to initialize history.
        """
        self.zoomhistory.append(
            (
                self.Map.region["n"],
                self.Map.region["s"],
                self.Map.region["e"],
                self.Map.region["w"],
            )
        )
        Debug.msg(4, "BufferedWindow.InitZoomHistory(): hist=%s" % (self.zoomhistory))

    def ResetZoomHistory(self):
        """Reset zoom history"""
        self.zoomhistory = []

    def ZoomToMap(self, layers=None, ignoreNulls=False, render=True):
        """Set display extents to match selected raster
        or vector map(s).

        :param layers: list of layers to be zoom to
        :param ignoreNulls: True to ignore null-values (valid only for rasters)
        :param render: True to re-render display
        """
        if not layers:
            layers = self._giface.GetLayerList().GetSelectedLayers(checkedOnly=False)
            layers = [layer.maplayer for layer in layers]

        if not layers:
            return

        rast = []
        rast3d = None
        vect = []
        updated = False
        for layer in layers:
            # only one raster is used: g.region does not support multiple
            if layer.type == "raster":
                rast.append(layer.GetName())
            elif layer.type == "raster_3d":
                rast3d = layer.GetName()
            elif layer.type == "vector":
                if self.digit and self.toolbar.GetLayer() == layer:
                    w, s, b, e, n, t = self.digit.GetDisplay().GetMapBoundingBox()
                    self.Map.GetRegion(n=n, s=s, w=w, e=e, update=True)
                    updated = True
                else:
                    vect.append(layer.name)
            elif layer.type == "rgb":
                for rname in layer.GetName().splitlines():
                    rast.append(rname)

        if not updated:
            self.Map.GetRegion(
                rast=rast, rast3d=rast3d, vect=vect, zoom=ignoreNulls, update=True
            )

        self.ZoomHistory(
            self.Map.region["n"],
            self.Map.region["s"],
            self.Map.region["e"],
            self.Map.region["w"],
        )

        if render:
            self.UpdateMap()

    def ZoomToWind(self):
        """Set display geometry to match computational region
        settings (set with g.region)
        """
        self.Map.region = self.Map.GetRegion()

        self.ZoomHistory(
            self.Map.region["n"],
            self.Map.region["s"],
            self.Map.region["e"],
            self.Map.region["w"],
        )

        self.UpdateMap()

    def ZoomToDefault(self):
        """Set display geometry to match default region settings"""
        self.Map.region = self.Map.GetRegion(default=True)
        self.Map.AdjustRegion()  # align region extent to the display

        self.ZoomHistory(
            self.Map.region["n"],
            self.Map.region["s"],
            self.Map.region["e"],
            self.Map.region["w"],
        )

        self.UpdateMap()

    def GoTo(self, e, n):
        region = self.Map.GetCurrentRegion()

        region["center_easting"], region["center_northing"] = e, n

        dn = (region["nsres"] * region["rows"]) / 2.0
        region["n"] = region["center_northing"] + dn
        region["s"] = region["center_northing"] - dn
        de = (region["ewres"] * region["cols"]) / 2.0
        region["e"] = region["center_easting"] + de
        region["w"] = region["center_easting"] - de

        self.Map.AdjustRegion()

        # add to zoom history
        self.ZoomHistory(region["n"], region["s"], region["e"], region["w"])
        self.UpdateMap()

    def DisplayToWind(self):
        """Set computational region (WIND file) to match display
        extents
        """
        tmpreg = os.getenv("GRASS_REGION")
        if tmpreg:
            del os.environ["GRASS_REGION"]

        # We ONLY want to set extents here. Don't mess with resolution. Leave that
        # for user to set explicitly with g.region
        new = self.Map.AlignResolution()
        RunCommand(
            "g.region",
            parent=self,
            overwrite=True,
            n=new["n"],
            s=new["s"],
            e=new["e"],
            w=new["w"],
            rows=int(new["rows"]),
            cols=int(new["cols"]),
        )

        if tmpreg:
            os.environ["GRASS_REGION"] = tmpreg

        self.UpdateMap(render=False)

    def SetRegion(self, zoomOnly=True):
        """Set display extents/computational region from named region
        file.

        :param zoomOnly: zoom to named region only (computational region is not saved)
        """
        if zoomOnly:
            label = _("Zoom to saved region extents")
        else:
            label = _("Set computational region from named region")
        dlg = SavedRegion(parent=self, title=label, loadsave="load")

        if dlg.ShowModal() == wx.ID_CANCEL or not dlg.GetName():
            dlg.Destroy()
            return

        region = dlg.GetName()
        if not gs.find_file(name=region, element="windows")["name"]:
            GError(
                parent=self,
                message=_("Region <%s> not found. Operation canceled.") % region,
            )
            dlg.Destroy()
            return

        dlg.Destroy()

        if zoomOnly:
            self.Map.GetRegion(regionName=region, update=True)

            self.ZoomHistory(
                self.Map.region["n"],
                self.Map.region["s"],
                self.Map.region["e"],
                self.Map.region["w"],
            )
        else:
            # set computation region from named region file
            RunCommand("g.region", parent=self, region=region)

        self.UpdateMap()

    def SaveRegion(self, display=True):
        """Save display extents/computational region to named region
        file.

        :param display: True for display extends otherwise computational region
        """
        if display:
            title = _("Save display extents to region file")
        else:
            title = _("Save computational region to region file")

        dlg = SavedRegion(parent=self, title=title, loadsave="save")
        if dlg.ShowModal() == wx.ID_CANCEL or not dlg.GetName():
            dlg.Destroy()
            return

        # test to see if it already exists and ask permission to overwrite
        if gs.find_file(name=dlg.GetName(), element="windows")["name"]:
            overwrite = wx.MessageBox(
                parent=self,
                message=_(
                    "Region file <%s> already exists. Do you want to overwrite it?"
                )
                % (dlg.GetName()),
                caption=_("Warning"),
                style=wx.YES_NO | wx.CENTRE,
            )
            if overwrite != wx.YES:
                dlg.Destroy()
                return

        if display:
            self._saveDisplayRegion(dlg.GetName())
        else:
            self._saveCompRegion(dlg.GetName())

        dlg.Destroy()

    def _saveCompRegion(self, name):
        """Save region settings to region file

        :param name: region name
        """
        RunCommand("g.region", overwrite=True, parent=self, flags="u", save=name)

    def _saveDisplayRegion(self, name):
        """Save display extents to region file

        :param name: region name
        """
        new = self.Map.GetCurrentRegion()

        tmpreg = os.getenv("GRASS_REGION")
        if tmpreg:
            del os.environ["GRASS_REGION"]

        RunCommand(
            "g.region",
            overwrite=True,
            parent=self,
            flags="u",
            n=new["n"],
            s=new["s"],
            e=new["e"],
            w=new["w"],
            rows=int(new["rows"]),
            cols=int(new["cols"]),
            save=name,
        )

        if tmpreg:
            os.environ["GRASS_REGION"] = tmpreg

    def Distance(self, beginpt, endpt, screen=True):
        """Calculates distance

        Ctypes required for LL-locations

        :param beginpt: first point
        :param endpt: second point
        :param screen: True for screen coordinates otherwise EN
        """
        if screen:
            e1, n1 = self.Pixel2Cell(beginpt)
            e2, n2 = self.Pixel2Cell(endpt)
        else:
            e1, n1 = beginpt
            e2, n2 = endpt

        dEast = e2 - e1
        dNorth = n2 - n1

        if self.Map.projinfo["proj"] == "ll" and haveCtypes:
            dist = gislib.G_distance(e1, n1, e2, n2)
        else:
            dist = math.sqrt(math.pow((dEast), 2) + math.pow((dNorth), 2))

        return (dist, (dEast, dNorth))

    def GetMap(self):
        """Get render.Map() instance"""
        return self.Map

    def RegisterGraphicsToDraw(
        self, graphicsType, pdc=None, setStatusFunc=None, drawFunc=None, mapCoords=True
    ):
        """This method registers graphics to draw.

        :param type: (string) - graphics type: "point", "line" or "rectangle"
        :param pdc: PseudoDC object, default is pdcTmp
        :param setStatusFunc: function called before drawing each item
                              Status function should be in this form:
                              setStatusFunc(item, itemOrderNum)
                              item passes instance of GraphicsSetItem
                              which will be drawn itemOrderNum number of item
                              in drawing order (from O)
                              Hidden items are also counted in drawing order.
        :type setStatusFunc: function
        :param drawFunc: defines own function for drawing, if function
                         is not defined DrawCross method is used for
                         type "point", DrawLines method for type "line",
                         DrawRectangle for "rectangle".
        :param mapCoords: True if map coordinates should be set by user, otherwise
                          pixels

        :return: reference to GraphicsSet, which was added.
        """
        if not pdc:
            pdc = self.pdcTmp
        item = GraphicsSet(
            parentMapWin=self,
            graphicsType=graphicsType,
            pdc=pdc,
            setStatusFunc=setStatusFunc,
            drawFunc=drawFunc,
            mapCoords=mapCoords,
        )
        self.graphicsSetList.append(item)

        return item

    def UnregisterGraphicsToDraw(self, item):
        """Unregisters GraphicsSet instance

        :param item: (GraphicsSetItem) - item to unregister

        :return: True - if item was unregistered
        :return: False - if item was not found
        """
        if item in self.graphicsSetList:
            self.graphicsSetList.remove(item)
            return True

        return False
