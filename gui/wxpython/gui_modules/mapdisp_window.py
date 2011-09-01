"""!
@package mapdisp_window.py

@brief Map display canvas - buffered window.

Classes:
 - MapWindow
 - BufferedWindow

(C) 2006-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
@author Michael Barton
@author Jachym Cepicky
"""

import os
import time
import math
import sys
import tempfile
import traceback

import wx

import grass.script as grass

import dbm
import gdialogs
import gcmd
import utils
import globalvar
import gselect
from debug import Debug
from preferences import globalSettings as UserSettings
from units import ConvertValue as UnitsConvertValue

try:
    import grass.lib.gis as gislib
    haveCtypes = True
except ImportError:
    haveCtypes = False

class MapWindow(object):
    """!Abstract map display window class
    
    Superclass for BufferedWindow class (2D display mode), and GLWindow
    (3D display mode).
    
    Subclasses have to define _bindMouseEvents() method which binds
    MouseEvent handlers.
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
        
        # stores overriden cursor
        self._overridenCursor = None

    def RegisterMouseEventHandler(self, event, handler, cursor = None):
        """!Binds event handler
        
        Call event.Skip() in handler to allow default processing in MapWindow.
        
        \code
        # your class methods
        def OnButton(self, event):
            # current map display's map window
            # expects LayerManager to be the parent
            self.mapwin = self.parent.GetLayerTree().GetMapDisplay().GetWindow()
            if self.mapwin.RegisterMouseEventHandler(wx.EVT_LEFT_DOWN, self.OnMouseAction,
                                                     wx.StockCursor(wx.CURSOR_CROSS)):
                self.parent.GetLayerTree().GetMapDisplay().Raise()
            else:
                # handle that you cannot get coordinates
        
        def OnMouseAction(self, event):
            # get real world coordinates of mouse click
            coor = self.mapwin.Pixel2Cell(event.GetPositionTuple()[:])
            self.text.SetLabel('Coor: ' + str(coor))
            self.mapwin.UnregisterMouseEventHandler(wx.EVT_LEFT_DOWN)
            event.Skip()
        \endcode
        
        @param event one of mouse events
        @param handler function to handle event
        @param cursor cursor which temporary overrides current cursor
        
        @return True if successfull
        @return False if event cannot be bind
        """
        
        # if it is a VDigitWindow it cannot be used
        # hasattr is ugly
        if hasattr(self, "digit"):
            return False
        
        self.Bind(event, handler)
        self.mouse['useBeforeGenericEvent'] = self.mouse['use']
        self.mouse['use'] = 'genericEvent'
        
        if cursor:
            self._overridenCursor = self.GetCursor()
            self.SetCursor(cursor)
        
        return True


    def UnregisterMouseEventHandler(self, event):
        """!Unbinds event handler a restores previous state
        
        You should unbind to restore normal MapWindow behaviour.
        Note that this operation will unbind any other external (non-MapWindow) handlers.
        
        @param event event to unbind
        
        @return True if successfull
        @return False if event cannot be unbind
        """
        if hasattr(self, "digit"):
            return False
        
        # it is not yet posible in wxPython to unbind exact event
        ret = self.Unbind(event)
        
        # restore bind state
        self._bindMouseEvents()
        
        # restore mouse use (previous state)
        self.mouse['use'] = self.mouse['useBeforeGenericEvent']
        
        # restore overriden cursor
        if self._overridenCursor:
            self.SetCursor(self._overridenCursor)
        
        return ret


    def OnMotion(self, event):
        """!Track mouse motion and update statusbar
        """
        if self.parent.statusbarWin['toggle'].GetSelection() == 0: # Coordinates
            precision = int(UserSettings.Get(group = 'projection', key = 'format',
                                             subkey = 'precision'))
            format = UserSettings.Get(group = 'projection', key = 'format',
                                      subkey = 'll')
            try:
                e, n = self.Pixel2Cell(event.GetPositionTuple())
            except (TypeError, ValueError):
                self.parent.statusbar.SetStatusText("", 0)
                event.Skip()
                return
            
            updated = False
            if hasattr(self, "digit"):
                updated = self._onMotion((e, n), precision)

            if not updated:
                if self.parent.statusbarWin['projection'].IsChecked():
                    if not UserSettings.Get(group = 'projection', key = 'statusbar', subkey = 'proj4'):
                        self.parent.statusbar.SetStatusText(_("Projection not defined (check the settings)"), 0)
                    else:
                        proj, coord  = utils.ReprojectCoordinates(coord = (e, n),
                                                                  projOut = UserSettings.Get(group = 'projection',
                                                                                             key = 'statusbar',
                                                                                             subkey = 'proj4'),
                                                                  flags = 'd')
                    
                        if coord:
                            e, n = coord
                            if proj in ('ll', 'latlong', 'longlat') and format == 'DMS':
                                self.parent.statusbar.SetStatusText(utils.Deg2DMS(e, n, precision = precision),
                                                                    0)
                            else:
                                self.parent.statusbar.SetStatusText("%.*f; %.*f" % \
                                                                        (precision, e, precision, n), 0)
                        else:
                            self.parent.statusbar.SetStatusText(_("Error in projection (check the settings)"), 0)
                else:
                    if self.parent.Map.projinfo['proj'] == 'll' and format == 'DMS':
                        self.parent.statusbar.SetStatusText(utils.Deg2DMS(e, n, precision = precision),
                                                            0)
                    else:
                        self.parent.statusbar.SetStatusText("%.*f; %.*f" % \
                                                                (precision, e, precision, n), 0)
        
        event.Skip()

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
    
class BufferedWindow(MapWindow, wx.Window):
    """!A Buffered window class (2D view mode)

    Superclass for VDigitWindow (vector digitizer).
    
    When the drawing needs to change, you app needs to call the
    UpdateMap() method. Since the drawing is stored in a bitmap, you
    can also save the drawing to file by calling the
    SaveToFile() method.
    """
    def __init__(self, parent, id = wx.ID_ANY,
                 Map = None, tree = None, lmgr = None,
                 style = wx.NO_FULL_REPAINT_ON_RESIZE, **kwargs):
        MapWindow.__init__(self, parent, id, Map, tree, lmgr, **kwargs)
        wx.Window.__init__(self, parent, id, style = style, **kwargs)
        
        # flags
        self.resize = False # indicates whether or not a resize event has taken place
        self.dragimg = None # initialize variable for map panning
        
        # variables for drawing on DC
        self.pen = None      # pen for drawing zoom boxes, etc.
        self.polypen = None  # pen for drawing polylines (measurements, profiles, etc)
        # List of wx.Point tuples defining a polyline (geographical coordinates)
        self.polycoords = []
        # ID of rubber band line
        self.lineid = None
        # ID of poly line resulting from cumulative rubber band lines (e.g. measurement)
        self.plineid = None
        
        # event bindings
        self.Bind(wx.EVT_PAINT,        self.OnPaint)
        self.Bind(wx.EVT_SIZE,         self.OnSize)
        self.Bind(wx.EVT_IDLE,         self.OnIdle)
        self._bindMouseEvents()
        
        self.processMouse = True
        
        # render output objects
        self.mapfile = None   # image file to be rendered
        self.img     = None   # wx.Image object (self.mapfile)
        # decoration overlays
        self.overlays = {}
        # images and their PseudoDC ID's for painting and dragging
        self.imagedict = {}   
        self.select = {}      # selecting/unselecting decorations for dragging
        self.textdict = {}    # text, font, and color indexed by id
        self.currtxtid = None # PseudoDC id for currently selected text
        
        # zoom objects
        self.zoomhistory  = [] # list of past zoom extents
        self.currzoom     = 0  # current set of extents in zoom history being used
        self.zoomtype     = 1  # 1 zoom in, 0 no zoom, -1 zoom out
        self.hitradius    = 10 # distance for selecting map decorations
        self.dialogOffset = 5  # offset for dialog (e.g. DisplayAttributesDialog)
        
        # OnSize called to make sure the buffer is initialized.
        # This might result in OnSize getting called twice on some
        # platforms at initialization, but little harm done.
        ### self.OnSize(None)
        
        self._definePseudoDC()
        # redraw all pdc's, pdcTmp layer is redrawn always (speed issue)
        self.redrawAll = True
        
        # will store an off screen empty bitmap for saving to file
        self._buffer = None
        
        self.Bind(wx.EVT_ERASE_BACKGROUND, lambda x:None)
        
        # vars for handling mouse clicks
        self.dragid   = -1
        self.lastpos  = (0, 0)
        
    def _definePseudoDC(self):
        """!Define PseudoDC objects to use
        """
        # create PseudoDC used for background map, map decorations like scales and legends
        self.pdc = wx.PseudoDC()
        # used for digitization tool
        self.pdcVector = None
        # decorations (region box, etc.)
        self.pdcDec = wx.PseudoDC()
        # pseudoDC for temporal objects (select box, measurement tool, etc.)
        self.pdcTmp = wx.PseudoDC()
        
    def _bindMouseEvents(self):
        self.Bind(wx.EVT_MOUSE_EVENTS, self.MouseActions)
        self.Bind(wx.EVT_MOTION,       self.OnMotion)
        
    def Draw(self, pdc, img = None, drawid = None, pdctype = 'image', coords = [0, 0, 0, 0]):
        """!Draws map and overlay decorations
        """
        if drawid == None:
            if pdctype == 'image' and img:
                drawid = self.imagedict[img]
            elif pdctype == 'clear':
                drawid == None
            else:
                drawid = wx.NewId()
        
        if img and pdctype == 'image':
            # self.imagedict[img]['coords'] = coords
            self.select[self.imagedict[img]['id']] = False # ?
        
        pdc.BeginDrawing()
        
        if drawid != 99:
            bg = wx.TRANSPARENT_BRUSH
        else:
            bg = wx.Brush(self.GetBackgroundColour())
        
        pdc.SetBackground(bg)
        
        Debug.msg (5, "BufferedWindow.Draw(): id=%s, pdctype = %s, coord=%s" % \
                       (drawid, pdctype, coords))
        
        # set PseudoDC id
        if drawid is not None:
            pdc.SetId(drawid)
            
        if pdctype == 'clear': # erase the display
            bg = wx.WHITE_BRUSH
            # bg = wx.Brush(self.GetBackgroundColour())
            pdc.SetBackground(bg)
            pdc.RemoveAll()
            pdc.Clear()
            pdc.EndDrawing()
            
            self.Refresh()
            return
        
        if pdctype == 'image': # draw selected image
            bitmap = wx.BitmapFromImage(img)
            w,h = bitmap.GetSize()
            pdc.DrawBitmap(bitmap, coords[0], coords[1], True) # draw the composite map
            pdc.SetIdBounds(drawid, wx.Rect(coords[0],coords[1], w, h))
        
        elif pdctype == 'box': # draw a box on top of the map
            if self.pen:
                pdc.SetBrush(wx.Brush(wx.CYAN, wx.TRANSPARENT))
                pdc.SetPen(self.pen)
                x2 = max(coords[0],coords[2])
                x1 = min(coords[0],coords[2])
                y2 = max(coords[1],coords[3])
                y1 = min(coords[1],coords[3])
                rwidth = x2-x1
                rheight = y2-y1
                rect = wx.Rect(x1, y1, rwidth, rheight)
                pdc.DrawRectangleRect(rect)
                pdc.SetIdBounds(drawid, rect)
                
        elif pdctype == 'line': # draw a line on top of the map
            if self.pen:
                pdc.SetBrush(wx.Brush(wx.CYAN, wx.TRANSPARENT))
                pdc.SetPen(self.pen)
                pdc.DrawLinePoint(wx.Point(coords[0], coords[1]),wx.Point(coords[2], coords[3]))
                pdc.SetIdBounds(drawid, wx.Rect(coords[0], coords[1], coords[2], coords[3]))
        
        elif pdctype == 'polyline': # draw a polyline on top of the map
            if self.polypen:
                pdc.SetBrush(wx.Brush(wx.CYAN, wx.TRANSPARENT))
                pdc.SetPen(self.polypen)
                if (len(coords) < 2):
                    return
                i = 1
                while i < len(coords):
                    pdc.DrawLinePoint(wx.Point(coords[i-1][0], coords[i-1][1]),
                                      wx.Point(coords[i][0], coords[i][1]))
                    i += 1
                
                # get bounding rectangle for polyline
                xlist = []
                ylist = []
                if len(coords) > 0:
                    for point in coords:
                        x,y = point
                        xlist.append(x)
                        ylist.append(y)
                    x1 = min(xlist)
                    x2 = max(xlist)
                    y1 = min(ylist)
                    y2 = max(ylist)
                    pdc.SetIdBounds(drawid, wx.Rect(x1,y1,x2,y2))
                    # self.ovlcoords[drawid] = [x1,y1,x2,y2]
        
        elif pdctype == 'point': # draw point
            if self.pen:
                pdc.SetPen(self.pen)
                pdc.DrawPoint(coords[0], coords[1])
                coordsBound = (coords[0] - 5,
                               coords[1] - 5,
                               coords[0] + 5,
                               coords[1] + 5)
                pdc.SetIdBounds(drawid, wx.Rect(coordsBound))
        
        elif pdctype == 'text': # draw text on top of map
            if not img['active']:
                return # only draw active text
            if 'rotation' in img:
                rotation = float(img['rotation'])
            else:
                rotation = 0.0
            w, h = self.GetFullTextExtent(img['text'])[0:2]
            pdc.SetFont(img['font'])
            pdc.SetTextForeground(img['color'])
            coords, bbox = self.TextBounds(img)
            if rotation == 0:
                pdc.DrawText(img['text'], coords[0], coords[1])
            else:
                pdc.DrawRotatedText(img['text'], coords[0], coords[1], rotation)
            pdc.SetIdBounds(drawid, bbox)
        
        pdc.EndDrawing()
        
        self.Refresh()
        
        return drawid
    
    def TextBounds(self, textinfo, relcoords = False):
        """!Return text boundary data
        
        @param textinfo text metadata (text, font, color, rotation)
        @param coords reference point
        
        @return coords of nonrotated text bbox (TL corner)
        @return bbox of rotated text bbox (wx.Rect)
        @return relCoords are text coord inside bbox
        """
        if 'rotation' in textinfo:
            rotation = float(textinfo['rotation'])
        else:
            rotation = 0.0
        
        coords = textinfo['coords']
        bbox = wx.Rect(coords[0], coords[1], 0, 0)
        relCoords = (0, 0)
        Debug.msg (4, "BufferedWindow.TextBounds(): text=%s, rotation=%f" % \
                   (textinfo['text'], rotation))
        
        self.Update()
        
        self.SetFont(textinfo['font'])
        
        w, h = self.GetTextExtent(textinfo['text'])
        
        if rotation == 0:
            bbox[2], bbox[3] = w, h
            if relcoords:
                return coords, bbox, relCoords
            else:
                return coords, bbox
        
        boxh = math.fabs(math.sin(math.radians(rotation)) * w) + h
        boxw = math.fabs(math.cos(math.radians(rotation)) * w) + h
        if rotation > 0 and rotation < 90:
            bbox[1] -= boxh
            relCoords = (0, boxh)
        elif rotation >= 90 and rotation < 180:
            bbox[0] -= boxw
            bbox[1] -= boxh
            relCoords = (boxw, boxh)
        elif rotation >= 180 and rotation < 270:
            bbox[0] -= boxw
            relCoords = (boxw, 0)
        bbox[2] = boxw
        bbox[3] = boxh
        bbox.Inflate(h,h)
        if relcoords:
            return coords, bbox, relCoords
        else:
            return coords, bbox

    def OnPaint(self, event):
        """!Draw PseudoDC's to buffered paint DC
        
        If self.redrawAll is False on self.pdcTmp content is re-drawn
        """
        Debug.msg(4, "BufferedWindow.OnPaint(): redrawAll=%s" % self.redrawAll)
        dc = wx.BufferedPaintDC(self, self.buffer)
        dc.Clear()
        
        # use PrepareDC to set position correctly
        self.PrepareDC(dc)
        
        # create a clipping rect from our position and size
        # and update region
        rgn = self.GetUpdateRegion().GetBox()
        dc.SetClippingRect(rgn)
        
        switchDraw = False
        if self.redrawAll is None:
            self.redrawAll = True
            switchDraw = True
        
        if self.redrawAll: # redraw pdc and pdcVector
            # draw to the dc using the calculated clipping rect
            self.pdc.DrawToDCClipped(dc, rgn)
            
            # draw vector map layer
            if hasattr(self, "digit"):
                # decorate with GDDC (transparency)
                try:
                    gcdc = wx.GCDC(dc)
                    self.pdcVector.DrawToDCClipped(gcdc, rgn)
                except NotImplementedError, e:
                    print >> sys.stderr, e
                    self.pdcVector.DrawToDCClipped(dc, rgn)
            
            self.bufferLast = None
        else: # do not redraw pdc and pdcVector
            if self.bufferLast is None:
                # draw to the dc
                self.pdc.DrawToDC(dc)
                
                if hasattr(self, "digit"):
                    # decorate with GDDC (transparency)
                    try:
                        gcdc = wx.GCDC(dc)
                        self.pdcVector.DrawToDC(gcdc)
                    except NotImplementedError, e:
                        print >> sys.stderr, e
                        self.pdcVector.DrawToDC(dc)
                
                # store buffered image
                # self.bufferLast = wx.BitmapFromImage(self.buffer.ConvertToImage())
                self.bufferLast = dc.GetAsBitmap(wx.Rect(0, 0, self.Map.width, self.Map.height))
            
            self.pdc.DrawBitmap(self.bufferLast, 0, 0, False)
            self.pdc.DrawToDC(dc)
        
        # draw decorations (e.g. region box)
        try:
            gcdc = wx.GCDC(dc)
            self.pdcDec.DrawToDC(gcdc)
        except NotImplementedError, e:
            print >> sys.stderr, e
            self.pdcDec.DrawToDC(dc)
        
        # draw temporary object on the foreground
        ### self.pdcTmp.DrawToDCClipped(dc, rgn)
        self.pdcTmp.DrawToDC(dc)
        
        if switchDraw:
            self.redrawAll = False
        
    def OnSize(self, event):
        """!Scale map image so that it is the same size as the Window
        """
        Debug.msg(3, "BufferedWindow.OnSize():")
        
        # set size of the input image
        self.Map.ChangeMapSize(self.GetClientSize())
        # align extent based on center point and display resolution
        # this causes that image is not resized when display windows is resized
        ### self.Map.AlignExtentFromDisplay()
        
        # Make new off screen bitmap: this bitmap will always have the
        # current drawing in it, so it can be used to save the image to
        # a file, or whatever.
        self.buffer = wx.EmptyBitmap(max(1, self.Map.width), max(1, self.Map.height))
        
        # get the image to be rendered
        self.img = self.GetImage()
        
        # update map display
        if self.img and self.Map.width + self.Map.height > 0: # scale image during resize
            self.img = self.img.Scale(self.Map.width, self.Map.height)
            if len(self.Map.GetListOfLayers()) > 0:
                self.UpdateMap()
        
        # re-render image on idle
        self.resize = True

        # reposition checkbox in statusbar
        self.parent.StatusbarReposition()
        
        # update statusbar
        self.parent.StatusbarUpdate()
        
    def OnIdle(self, event):
        """!Only re-render a composite map image from GRASS during
        idle time instead of multiple times during resizing.
        """
        if self.resize:
            self.UpdateMap(render = True)
        
        event.Skip()

    def SaveToFile(self, FileName, FileType, width, height):
        """!This draws the pseudo DC to a buffer that can be saved to
        a file.
        
        @param FileName file name
        @param FileType type of bitmap
        @param width image width
        @param height image height
        """
        busy = wx.BusyInfo(message = _("Please wait, exporting image..."),
                           parent = self)
        wx.Yield()
        
        self.Map.ChangeMapSize((width, height))
        ibuffer = wx.EmptyBitmap(max(1, width), max(1, height))
        self.Map.Render(force = True, windres = True)
        img = self.GetImage()
        self.Draw(self.pdc, img, drawid = 99)
        dc = wx.BufferedPaintDC(self, ibuffer)
        dc.Clear()
        self.PrepareDC(dc)
        self.pdc.DrawToDC(dc)
        if self.pdcVector:
            self.pdcVector.DrawToDC(dc)
        ibuffer.SaveFile(FileName, FileType)
        
        busy.Destroy()
        
        self.UpdateMap(render = True)
        self.Refresh()
        
    def GetOverlay(self):
        """!Converts rendered overlay files to wx.Image
        
        Updates self.imagedict
        
        @return list of images
        """
        imgs = []
        for overlay in self.Map.GetListOfLayers(l_type = "overlay", l_active = True):
            if os.path.isfile(overlay.mapfile) and os.path.getsize(overlay.mapfile):
                img = wx.Image(overlay.mapfile, wx.BITMAP_TYPE_ANY)
                self.imagedict[img] = { 'id' : overlay.id,
                                        'layer' : overlay }
                imgs.append(img)

        return imgs
    
    def GetImage(self):
        """!Converts redered map files to wx.Image
        
        Updates self.imagedict (id=99)
        
        @return wx.Image instance (map composition)
        """
        imgId = 99
        if self.mapfile and self.Map.mapfile and os.path.isfile(self.Map.mapfile) and \
                os.path.getsize(self.Map.mapfile):
            img = wx.Image(self.Map.mapfile, wx.BITMAP_TYPE_ANY)
        else:
            img = None
        
        self.imagedict[img] = { 'id': imgId }
        
        return img

    def UpdateMap(self, render = True, renderVector = True):
        """!Updates the canvas anytime there is a change to the
        underlaying images or to the geometry of the canvas.
        
        @param render re-render map composition
        @param renderVector re-render vector map layer enabled for editing (used for digitizer)
        """
        start = time.clock()
        
        self.resize = False
        
        if not self.Map.cmdfile and self.img is None:
            render = True
        
        #
        # initialize process bar (only on 'render')
        #
        if render or renderVector:
            self.parent.statusbarWin['progress'].Show()
            if self.parent.statusbarWin['progress'].GetRange() > 0:
                self.parent.statusbarWin['progress'].SetValue(1)
        
        #
        # render background image if needed
        #
        # update layer dictionary if there has been a change in layers
        if self.tree and self.tree.reorder:
            self.tree.ReorderLayers()
        
        # reset flag for auto-rendering
        if self.tree:
            self.tree.rerender = False
        
        try:
            if render:
                # update display size
                self.Map.ChangeMapSize(self.GetClientSize())
                if self.parent.statusbarWin['resolution'].IsChecked():
                    # use computation region resolution for rendering
                    windres = True
                else:
                    windres = False
                
                self.mapfile = self.Map.Render(force = True, mapWindow = self.parent,
                                               windres = windres)
            else:
                self.mapfile = self.Map.Render(force = False, mapWindow = self.parent)
            
        except gcmd.GException, e:
            gcmd.GError(message = e.value)
            self.mapfile = None
        
        self.img = self.GetImage() # id=99
        
        #
        # clear pseudoDcs
        #
        for pdc in (self.pdc,
                    self.pdcDec,
                    self.pdcTmp):
            pdc.Clear()
            pdc.RemoveAll()
        
        #
        # draw background map image to PseudoDC
        #
        if not self.img:
            self.Draw(self.pdc, pdctype = 'clear')
        else:
            try:
                id = self.imagedict[self.img]['id']
            except:
                return False
            
            self.Draw(self.pdc, self.img, drawid = id)
        
        #
        # render vector map layer
        #
        if renderVector and hasattr(self, "digit"):
            self._updateMap()
        #
        # render overlays
        #
        for img in self.GetOverlay():
            # draw any active and defined overlays
            if self.imagedict[img]['layer'].IsActive():
                id = self.imagedict[img]['id']
                self.Draw(self.pdc, img = img, drawid = id,
                          pdctype = self.overlays[id]['pdcType'], coords = self.overlays[id]['coords'])
        
        for id in self.textdict.keys():
            self.Draw(self.pdc, img = self.textdict[id], drawid = id,
                      pdctype = 'text', coords = [10, 10, 10, 10])
        
        # optionally draw computational extent box
        self.DrawCompRegionExtent()
        
        #
        # redraw pdcTmp if needed
        #
        if len(self.polycoords) > 0:
            self.DrawLines(self.pdcTmp)
        
        if not self.parent.IsStandalone() and \
                self.parent.GetLayerManager().georectifying:
            # -> georectifier (redraw GCPs)
            if self.parent.toolbars['georect']:
                coordtype = 'gcpcoord'
            else:
                coordtype = 'mapcoord'
            self.parent.GetLayerManager().georectifying.DrawGCP(coordtype)
            
        if not self.parent.IsStandalone() and \
                self.parent.GetLayerManager().gcpmanagement:
            # -> georectifier (redraw GCPs)
            if self.parent.toolbars['gcpdisp']:
                if self == self.parent.TgtMapWindow:
                    coordtype = 'target'
                else:
                    coordtype = 'source'

                self.parent.DrawGCP(coordtype)

        # 
        # clear measurement
        #
        if self.mouse["use"] == "measure":
            self.ClearLines(pdc = self.pdcTmp)
            self.polycoords = []
            self.mouse['use'] = 'pointer'
            self.mouse['box'] = 'point'
            self.mouse['end'] = [0, 0]
            self.SetCursor(self.parent.cursors["default"])
            
        stop = time.clock()
        
        #
        # hide process bar
        #
        self.parent.statusbarWin['progress'].Hide()

        #
        # update statusbar 
        #
        ### self.Map.SetRegion()
        self.parent.StatusbarUpdate()
        if grass.find_file(name = 'MASK', element = 'cell')['name']:
            # mask found
            self.parent.statusbarWin['mask'].SetLabel(_('MASK'))
        else:
            self.parent.statusbarWin['mask'].SetLabel('')
        
        
        Debug.msg (1, "BufferedWindow.UpdateMap(): render=%s, renderVector=%s -> time=%g" % \
                   (render, renderVector, (stop-start)))
        
        return True

    def DrawCompRegionExtent(self):
        """!Draw computational region extent in the display
        
        Display region is drawn as a blue box inside the computational region,
        computational region inside a display region as a red box).
        """
        if hasattr(self, "regionCoords"):
            compReg = self.Map.GetRegion()
            dispReg = self.Map.GetCurrentRegion()
            reg = None
            if self.IsInRegion(dispReg, compReg):
                self.polypen = wx.Pen(colour = wx.Colour(0, 0, 255, 128), width = 3, style = wx.SOLID)
                reg = dispReg
            else:
                self.polypen = wx.Pen(colour = wx.Colour(255, 0, 0, 128),
                                      width = 3, style = wx.SOLID)
                reg = compReg
            
            self.regionCoords = []
            self.regionCoords.append((reg['w'], reg['n']))
            self.regionCoords.append((reg['e'], reg['n']))
            self.regionCoords.append((reg['e'], reg['s']))
            self.regionCoords.append((reg['w'], reg['s']))
            self.regionCoords.append((reg['w'], reg['n']))
            # draw region extent
            self.DrawLines(pdc = self.pdcDec, polycoords = self.regionCoords)

    def IsInRegion(self, region, refRegion):
        """!
        Test if 'region' is inside of 'refRegion'

        @param region input region
        @param refRegion reference region (e.g. computational region)

        @return True if region is inside of refRegion
        @return False 
        """
        if region['s'] >= refRegion['s'] and \
                region['n'] <= refRegion['n'] and \
                region['w'] >= refRegion['w'] and \
                region['e'] <= refRegion['e']:
            return True
        
        return False

    def EraseMap(self):
        """!Erase map canvas
        """
        self.Draw(self.pdc, pdctype = 'clear')
        
        if hasattr(self, "digit"):
            self.Draw(self.pdcVector, pdctype = 'clear')
        
        self.Draw(self.pdcDec, pdctype = 'clear')
        self.Draw(self.pdcTmp, pdctype = 'clear')
        
    def DragMap(self, moveto):
        """!Drag the entire map image for panning.
        
        @param moveto dx,dy
        """
        dc = wx.BufferedDC(wx.ClientDC(self))
        dc.SetBackground(wx.Brush("White"))
        dc.Clear()
        
        self.dragimg = wx.DragImage(self.buffer)
        self.dragimg.BeginDrag((0, 0), self)
        self.dragimg.GetImageRect(moveto)
        self.dragimg.Move(moveto)
        
        self.dragimg.DoDrawImage(dc, moveto)
        self.dragimg.EndDrag()
        
    def DragItem(self, id, event):
        """!Drag an overlay decoration item
        """
        if id == 99 or id == '' or id == None: return
        Debug.msg (5, "BufferedWindow.DragItem(): id=%d" % id)
        x, y = self.lastpos
        dx = event.GetX() - x
        dy = event.GetY() - y
        self.pdc.SetBackground(wx.Brush(self.GetBackgroundColour()))
        r = self.pdc.GetIdBounds(id)
        
        if type(r) is list:
            r = wx.Rect(r[0], r[1], r[2], r[3])
        if id > 100: # text dragging
            rtop = (r[0],r[1]-r[3],r[2],r[3])
            r = r.Union(rtop)
            rleft = (r[0]-r[2],r[1],r[2],r[3])
            r = r.Union(rleft)
        self.pdc.TranslateId(id, dx, dy)
        
        r2 = self.pdc.GetIdBounds(id)
        if type(r2) is list:
            r2 = wx.Rect(r[0], r[1], r[2], r[3])
        if id > 100: # text
            self.textdict[id]['bbox'] = r2
            self.textdict[id]['coords'][0] += dx
            self.textdict[id]['coords'][1] += dy
        r = r.Union(r2)
        r.Inflate(4,4)
        self.RefreshRect(r, False)
        self.lastpos = (event.GetX(), event.GetY())
                
    def MouseDraw(self, pdc = None, begin = None, end = None):
        """!Mouse box or line from 'begin' to 'end'
        
        If not given from self.mouse['begin'] to self.mouse['end'].
        """
        if not pdc:
            return
        
        if begin is None:
            begin = self.mouse['begin']
        if end is None:
            end   = self.mouse['end']
        
        Debug.msg (5, "BufferedWindow.MouseDraw(): use=%s, box=%s, begin=%f,%f, end=%f,%f" % \
                       (self.mouse['use'], self.mouse['box'],
                        begin[0], begin[1], end[0], end[1]))
        
        if self.mouse['box'] == "box":
            boxid = wx.ID_NEW
            mousecoords = [begin[0], begin[1],
                           end[0], end[1]]
            r = pdc.GetIdBounds(boxid)
            if type(r) is list:
                r = wx.Rect(r[0], r[1], r[2], r[3])
            r.Inflate(4, 4)
            try:
                pdc.ClearId(boxid)
            except:
                pass
            self.RefreshRect(r, False)
            pdc.SetId(boxid)
            self.Draw(pdc, drawid = boxid, pdctype = 'box', coords = mousecoords)
        
        elif self.mouse['box'] == "line" or self.mouse['box'] == 'point':
            self.lineid = wx.ID_NEW
            mousecoords = [begin[0], begin[1], \
                           end[0], end[1]]
            x1 = min(begin[0],end[0])
            x2 = max(begin[0],end[0])
            y1 = min(begin[1],end[1])
            y2 = max(begin[1],end[1])
            r = wx.Rect(x1,y1,x2-x1,y2-y1)
            r.Inflate(4,4)
            try:
                pdc.ClearId(self.lineid)
            except:
                pass
            self.RefreshRect(r, False)
            pdc.SetId(self.lineid)
            self.Draw(pdc, drawid = self.lineid, pdctype = 'line', coords = mousecoords)

    def DrawLines(self, pdc = None, polycoords = None):
        """!Draw polyline in PseudoDC
        
        Set self.pline to wx.NEW_ID + 1
        
        polycoords - list of polyline vertices, geographical coordinates
        (if not given, self.polycoords is used)
        """
        if not pdc:
            pdc = self.pdcTmp
        
        if not polycoords:
            polycoords = self.polycoords
        
        if len(polycoords) > 0:
            self.plineid = wx.ID_NEW + 1
            # convert from EN to XY
            coords = []
            for p in polycoords:
                coords.append(self.Cell2Pixel(p))

            self.Draw(pdc, drawid = self.plineid, pdctype = 'polyline', coords = coords)
            
            Debug.msg (4, "BufferedWindow.DrawLines(): coords=%s, id=%s" % \
                           (coords, self.plineid))
            
            return self.plineid
        
        return -1

    def DrawCross(self, pdc, coords, size, rotation = 0,
                  text = None, textAlign = 'lr', textOffset = (5, 5)):
        """!Draw cross in PseudoDC

        @todo implement rotation

        @param pdc PseudoDC
        @param coord center coordinates
        @param rotation rotate symbol
        @param text draw also text (text, font, color, rotation)
        @param textAlign alignment (default 'lower-right')
        @textOffset offset for text (from center point)
        """
        Debug.msg(4, "BufferedWindow.DrawCross(): pdc=%s, coords=%s, size=%d" % \
                  (pdc, coords, size))
        coordsCross = ((coords[0] - size, coords[1], coords[0] + size, coords[1]),
                       (coords[0], coords[1] - size, coords[0], coords[1] + size))

        self.lineid = wx.NewId()
        for lineCoords in coordsCross:
            self.Draw(pdc, drawid = self.lineid, pdctype = 'line', coords = lineCoords)
        
        if not text:
            return self.lineid
        
        if textAlign == 'ul':
            coord = [coords[0] - textOffset[0], coords[1] - textOffset[1], 0, 0]
        elif textAlign == 'ur':
            coord = [coords[0] + textOffset[0], coords[1] - textOffset[1], 0, 0]
        elif textAlign == 'lr':
            coord = [coords[0] + textOffset[0], coords[1] + textOffset[1], 0, 0]
        else:
            coord = [coords[0] - textOffset[0], coords[1] + textOffset[1], 0, 0]
        
        self.Draw(pdc, img = text,
                  pdctype = 'text', coords = coord)
        
        return self.lineid

    def MouseActions(self, event):
        """!Mouse motion and button click notifier
        """
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
        
        # middle mouse button relesed
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
            self.OnMouseMoving(event)
                
    def OnMouseWheel(self, event):
        """!Mouse wheel moved
        """
        self.processMouse = False
        current  = event.GetPositionTuple()[:]
        wheel = event.GetWheelRotation()
        Debug.msg (5, "BufferedWindow.MouseAction(): wheel=%d" % wheel)
        # zoom 1/2 of the screen, centered to current mouse position (TODO: settings)
        begin = (current[0] - self.Map.width / 4,
                 current[1] - self.Map.height / 4)
        end   = (current[0] + self.Map.width / 4,
                 current[1] + self.Map.height / 4)
        
        if wheel > 0:
            zoomtype = 1
        else:
            zoomtype = -1
        
        # zoom
        self.Zoom(begin, end, zoomtype)
        
        # redraw map
        self.UpdateMap()
        
        # update statusbar
        self.parent.StatusbarUpdate()
        
        self.Refresh()
        self.processMouse = True
        
    def OnDragging(self, event):
        """!Mouse dragging
        """
        Debug.msg (5, "BufferedWindow.MouseAction(): Dragging")
        current  = event.GetPositionTuple()[:]
        previous = self.mouse['begin']
        move = (current[0] - previous[0],
                current[1] - previous[1])
        
        if hasattr(self, "digit"):
            digitToolbar = self.toolbar
        else:
            digitToolbar = None
        
        # dragging or drawing box with left button
        if self.mouse['use'] == 'pan' or \
                event.MiddleIsDown():
            self.DragMap(move)
        
        # dragging decoration overlay item
        elif (self.mouse['use'] == 'pointer' and 
                not digitToolbar and 
                self.dragid != None):
            self.DragItem(self.dragid, event)
        
        # dragging anything else - rubber band box or line
        else:
            if (self.mouse['use'] == 'pointer' and 
                not digitToolbar):
                return
            self.mouse['end'] = event.GetPositionTuple()[:]
            if (event.LeftIsDown() and 
                not (digitToolbar and 
                    digitToolbar.GetAction() in ("moveLine",) and 
                     self.digit.GetDisplay().GetSelected() > 0)):
                # draw box only when left mouse button is pressed
                self.MouseDraw(pdc = self.pdcTmp)
        
    def OnLeftDown(self, event):
        """!Left mouse button pressed
        """
        Debug.msg (5, "BufferedWindow.OnLeftDown(): use=%s" % \
                   self.mouse["use"])
        
        self.mouse['begin'] = event.GetPositionTuple()[:]
        
        if self.mouse["use"] in ["measure", "profile"]:
            # measure or profile
            if len(self.polycoords) == 0:
                self.mouse['end'] = self.mouse['begin']
                self.polycoords.append(self.Pixel2Cell(self.mouse['begin']))
                self.ClearLines(pdc=self.pdcTmp)
            else:
                self.mouse['begin'] = self.mouse['end']
        
        elif self.mouse['use'] in ('zoom', 'legend'):
            pass
        
        # vector digizer
        elif self.mouse["use"] == "pointer" and \
                hasattr(self, "digit"):
            if event.ControlDown():
                self.OnLeftDownUndo(event)
            else:
                self._onLeftDown(event)
        
        elif self.mouse['use'] == 'pointer':
            # get decoration or text id
            self.idlist = []
            self.dragid = ''
            self.lastpos = self.mouse['begin']
            idlist = self.pdc.FindObjects(self.lastpos[0], self.lastpos[1],
                                          self.hitradius)
            if 99 in idlist:
                idlist.remove(99)                             
            if idlist != []:
                self.dragid = idlist[0] #drag whatever is on top
        else:
            pass
        
        event.Skip()
        
    def OnLeftUp(self, event):
        """!Left mouse button released
        """
        Debug.msg (5, "BufferedWindow.OnLeftUp(): use=%s" % \
                       self.mouse["use"])
        
        self.mouse['end'] = event.GetPositionTuple()[:]
        
        if self.mouse['use'] in ["zoom", "pan"]:
            # set region in zoom or pan
            begin = self.mouse['begin']
            end = self.mouse['end']
            
            if self.mouse['use'] == 'zoom':
                # set region for click (zero-width box)
                if begin[0] - end[0] == 0 or \
                        begin[1] - end[1] == 0:
                    # zoom 1/2 of the screen (TODO: settings)
                    begin = (end[0] - self.Map.width / 4,
                             end[1] - self.Map.height / 4)
                    end   = (end[0] + self.Map.width / 4,
                             end[1] + self.Map.height / 4)
            
            self.Zoom(begin, end, self.zoomtype)

            # redraw map
            self.UpdateMap(render = True)
            
            # update statusbar
            self.parent.StatusbarUpdate()
            
        elif self.mouse["use"] == "query":
            # querying
            layers = self.GetSelectedLayer(multi = True)
            isRaster = False
            nVectors = 0
            for l in layers:
                if l.GetType() == 'raster':
                    isRaster = True
                    break
                if l.GetType() == 'vector':
                    nVectors += 1
            
            if isRaster or nVectors > 1:
                self.parent.QueryMap(self.mouse['begin'][0],self.mouse['begin'][1])
            else:
                self.parent.QueryVector(self.mouse['begin'][0], self.mouse['begin'][1])
                # clear temp canvas
                self.UpdateMap(render = False, renderVector = False)
            
        elif self.mouse["use"] == "queryVector":
            # editable mode for vector map layers
            self.parent.QueryVector(self.mouse['begin'][0], self.mouse['begin'][1])
            
            # clear temp canvas
            self.UpdateMap(render = False, renderVector = False)
        
        elif self.mouse["use"] in ["measure", "profile"]:
            # measure or profile
            if self.mouse["use"] == "measure":
                self.parent.MeasureDist(self.mouse['begin'], self.mouse['end'])
            
            self.polycoords.append(self.Pixel2Cell(self.mouse['end']))
            self.ClearLines(pdc = self.pdcTmp)
            self.DrawLines(pdc = self.pdcTmp)
        
        elif self.mouse["use"] == "pointer" and \
                not self.parent.IsStandalone() and \
                self.parent.GetLayerManager().gcpmanagement:
            # -> GCP manager
            if self.parent.toolbars['gcpdisp']:
                coord = self.Pixel2Cell(self.mouse['end'])
                if self.parent.MapWindow == self.parent.SrcMapWindow:
                    coordtype = 'source'
                else:
                    coordtype = 'target'
                
                self.parent.GetLayerManager().gcpmanagement.SetGCPData(coordtype, coord, self, confirm = True)
                self.UpdateMap(render = False, renderVector = False)
        
        elif self.mouse["use"] == "pointer" and \
                not self.parent.IsStandalone() and \
                self.parent.GetLayerManager().georectifying:
            # -> georectifying
            coord = self.Pixel2Cell(self.mouse['end'])
            if self.parent.toolbars['georect']:
                coordtype = 'gcpcoord'
            else:
                coordtype = 'mapcoord'
            
            self.parent.GetLayerManager().georectifying.SetGCPData(coordtype, coord, self)
            self.UpdateMap(render = False, renderVector = False)
            
        elif self.mouse["use"] == "pointer" and \
                hasattr(self, "digit"):
            self._onLeftUp(event)
            
        elif (self.mouse['use'] == 'pointer' and 
                self.dragid >= 0):
            # end drag of overlay decoration
            
            if self.dragid < 99 and self.dragid in self.overlays:
                self.overlays[self.dragid]['coords'] = self.pdc.GetIdBounds(self.dragid)
            elif self.dragid > 100 and self.dragid in self.textdict:
                self.textdict[self.dragid]['bbox'] = self.pdc.GetIdBounds(self.dragid)
            else:
                pass
            self.dragid = None
            self.currtxtid = None
            
        elif self.mouse['use'] == 'legend':
            self.ResizeLegend(self.mouse["begin"], self.mouse["end"])
            self.parent.dialogs['legend'].FindWindowByName("resize").SetValue(False)
            self.Map.GetOverlay(1).SetActive(True)
            self.parent.MapWindow.SetCursor(self.parent.cursors["default"])
            self.parent.MapWindow.mouse['use'] = 'pointer'
            
            self.UpdateMap()
            
    def OnButtonDClick(self, event):
        """!Mouse button double click
        """
        Debug.msg (5, "BufferedWindow.OnButtonDClick(): use=%s" % \
                   self.mouse["use"])
        
        if self.mouse["use"] == "measure":
            # measure
            self.ClearLines(pdc=self.pdcTmp)
            self.polycoords = []
            self.mouse['use'] = 'pointer'
            self.mouse['box'] = 'point'
            self.mouse['end'] = [0, 0]
            self.Refresh()
            self.SetCursor(self.parent.cursors["default"])
        
        elif self.mouse["use"] != "profile" or \
                (self.mouse['use'] != 'pointer' and \
                     hasattr(self, "digit")):
               # select overlay decoration options dialog
            clickposition = event.GetPositionTuple()[:]
            idlist  = self.pdc.FindObjects(clickposition[0], clickposition[1], self.hitradius)
            if idlist == []:
                return
            self.dragid = idlist[0]

            # self.ovlcoords[self.dragid] = self.pdc.GetIdBounds(self.dragid)
            if self.dragid > 100:
                self.currtxtid = self.dragid
                self.parent.OnAddText(None)
            elif self.dragid == 0:
                self.parent.OnAddBarscale(None)
            elif self.dragid == 1:
                self.parent.OnAddLegend(None)
        
    def OnRightDown(self, event):
        """!Right mouse button pressed
        """
        Debug.msg (5, "BufferedWindow.OnRightDown(): use=%s" % \
                   self.mouse["use"])
        
        if hasattr(self, "digit"):
            self._onRightDown(event)
        
        event.Skip()
        
    def OnRightUp(self, event):
        """!Right mouse button released
        """
        Debug.msg (5, "BufferedWindow.OnRightUp(): use=%s" % \
                   self.mouse["use"])
        
        if hasattr(self, "digit"):
            self._onRightUp(event)
        
        self.redrawAll = True
        self.Refresh()
        
        event.Skip()
        
    def OnMiddleDown(self, event):
        """!Middle mouse button pressed
        """
        if not event:
            return
        
        self.mouse['begin'] = event.GetPositionTuple()[:]
        
    def OnMiddleUp(self, event):
        """!Middle mouse button released
        """
        self.mouse['end'] = event.GetPositionTuple()[:]
        
        # set region in zoom or pan
        begin = self.mouse['begin']
        end   = self.mouse['end']
        
        self.Zoom(begin, end, 0) # no zoom
        
        # redraw map
        self.UpdateMap(render = True)
        
        # update statusbar
        self.parent.StatusbarUpdate()
        
    def OnMouseEnter(self, event):
        """!Mouse entered window and no mouse buttons were pressed
        """
        if not self.parent.IsStandalone() and \
                self.parent.GetLayerManager().gcpmanagement:
            if self.parent.toolbars['gcpdisp']:
                if not self.parent.MapWindow == self:
                    self.parent.MapWindow = self
                    self.parent.Map = self.Map
                    self.parent.UpdateActive(self)
                    # needed for wingrass
                    self.SetFocus()
        else:
            event.Skip()
        
    def OnMouseMoving(self, event):
        """!Motion event and no mouse buttons were pressed
        """
        if self.mouse["use"] == "pointer" and \
                hasattr(self, "digit"):
            self._onMouseMoving(event)
        
        event.Skip()
        
    def ClearLines(self, pdc = None):
        """!Clears temporary drawn lines from PseudoDC
        """
        if not pdc:
            pdc = self.pdcTmp
        try:
            pdc.ClearId(self.lineid)
            pdc.RemoveId(self.lineid)
        except:
            pass
        
        try:
            pdc.ClearId(self.plineid)
            pdc.RemoveId(self.plineid)
        except:
            pass
        
        Debug.msg(4, "BufferedWindow.ClearLines(): lineid=%s, plineid=%s" %
                  (self.lineid, self.plineid))
        
        return True

    def Pixel2Cell(self, (x, y)):
        """!Convert image coordinates to real word coordinates
        
        @param x, y image coordinates
        
        @return easting, northing
        @return None on error
        """
        try:
            x = int(x)
            y = int(y)
        except:
            return None
        
        if self.Map.region["ewres"] > self.Map.region["nsres"]:
            res = self.Map.region["ewres"]
        else:
            res = self.Map.region["nsres"]
        
        w = self.Map.region["center_easting"] - (self.Map.width / 2) * res
        n = self.Map.region["center_northing"] + (self.Map.height / 2) * res
        
        east  = w + x * res
        north = n - y * res
        
        return (east, north)
    
    def Cell2Pixel(self, (east, north)):
        """!Convert real word coordinates to image coordinates
        """
        try:
            east  = float(east)
            north = float(north)
        except:
            return None
        
        if self.Map.region["ewres"] > self.Map.region["nsres"]:
            res = self.Map.region["ewres"]
        else:
            res = self.Map.region["nsres"]
        
        w = self.Map.region["center_easting"] - (self.Map.width / 2) * res
        n = self.Map.region["center_northing"] + (self.Map.height / 2) * res
        
        x = (east  - w) / res
        y = (n - north) / res
        
        return (x, y)
    
    def ResizeLegend(self, begin, end):
        w = abs(begin[0] - end[0])
        h = abs(begin[1] - end[1])
        if begin[0] < end[0]:
            x = begin[0]
        else:
            x = end[0]
        if begin[1] < end[1]:
            y = begin[1]
        else:
            y = end[1]
        screenRect = wx.Rect(x, y, w, h)
        screenSize = self.GetClientSizeTuple()
        at = [(screenSize[1] - (y + h)) / float(screenSize[1]) * 100,
              (screenSize[1] - y) / float(screenSize[1]) * 100,
              x / float(screenSize[0]) * 100,
              (x + w) / float(screenSize[0]) * 100]
        for i, subcmd in enumerate(self.overlays[1]['cmd']):
            if subcmd.startswith('at='):
                self.overlays[1]['cmd'][i] = "at=%d,%d,%d,%d" % (at[0], at[1], at[2], at[3])
        self.Map.ChangeOverlay(1, True, command = self.overlays[1]['cmd'])
        self.overlays[1]['coords'] = (0,0)
        
    def Zoom(self, begin, end, zoomtype):
        """!Calculates new region while (un)zoom/pan-ing
        """
        x1, y1 = begin
        x2, y2 = end
        newreg = {}
        
        # threshold - too small squares do not make sense
        # can only zoom to windows of > 5x5 screen pixels
        if abs(x2-x1) > 5 and abs(y2-y1) > 5 and zoomtype != 0:
            if x1 > x2:
                x1, x2 = x2, x1
            if y1 > y2:
                y1, y2 = y2, y1
            
            # zoom in
            if zoomtype > 0:
                newreg['w'], newreg['n'] = self.Pixel2Cell((x1, y1))
                newreg['e'], newreg['s'] = self.Pixel2Cell((x2, y2))
            
            # zoom out
            elif zoomtype < 0:
                newreg['w'], newreg['n'] = self.Pixel2Cell((-x1 * 2, -y1 * 2))
                newreg['e'], newreg['s'] = self.Pixel2Cell((self.Map.width  + 2 * \
                                                                (self.Map.width  - x2),
                                                            self.Map.height + 2 * \
                                                                (self.Map.height - y2)))
        # pan
        elif zoomtype == 0:
            dx = x1 - x2
            dy = y1 - y2
            if dx == 0 and dy == 0:
                dx = x1 - self.Map.width / 2
                dy = y1 - self.Map.height / 2
            newreg['w'], newreg['n'] = self.Pixel2Cell((dx, dy))
            newreg['e'], newreg['s'] = self.Pixel2Cell((self.Map.width  + dx,
                                                        self.Map.height + dy))
        
        # if new region has been calculated, set the values
        if newreg != {}:
            # LL locations
            if self.parent.Map.projinfo['proj'] == 'll':
                self.Map.region['n'] = min(self.Map.region['n'], 90.0)
                self.Map.region['s'] = max(self.Map.region['s'], -90.0)
            
            ce = newreg['w'] + (newreg['e'] - newreg['w']) / 2
            cn = newreg['s'] + (newreg['n'] - newreg['s']) / 2
            
            # calculate new center point and display resolution
            self.Map.region['center_easting'] = ce
            self.Map.region['center_northing'] = cn
            self.Map.region['ewres'] = (newreg['e'] - newreg['w']) / self.Map.width
            self.Map.region['nsres'] = (newreg['n'] - newreg['s']) / self.Map.height
            if self.parent.statusbarWin['alignExtent'].IsChecked():
                self.Map.AlignExtentFromDisplay()
            else:
                for k in ('n', 's', 'e', 'w'):
                    self.Map.region[k] = newreg[k]
            
            if hasattr(self, "digit") and \
                    hasattr(self, "moveInfo"):
                self._zoom(event)
            
            self.ZoomHistory(self.Map.region['n'], self.Map.region['s'],
                             self.Map.region['e'], self.Map.region['w'])
        
        if self.redrawAll is False:
            self.redrawAll = True
        
    def ZoomBack(self):
        """!Zoom to previous extents in zoomhistory list
        """
        zoom = list()
        
        if len(self.zoomhistory) > 1:
            self.zoomhistory.pop()
            zoom = self.zoomhistory[-1]
        
        # disable tool if stack is empty
        if len(self.zoomhistory) < 2: # disable tool
            if self.parent.GetName() == 'MapWindow':
                toolbar = self.parent.toolbars['map']
            elif self.parent.GetName() == 'GRMapWindow':
                toolbar = self.parent.toolbars['georect']
            elif self.parent.GetName() == 'GCPMapWindow':
                toolbar = self.parent.toolbars['gcpdisp']
            
            toolbar.Enable('zoomback', enable = False)
        
        # zoom to selected region
        self.Map.GetRegion(n = zoom[0], s = zoom[1],
                           e = zoom[2], w = zoom[3],
                           update = True)
        # update map
        self.UpdateMap()
        
        # update statusbar
        self.parent.StatusbarUpdate()

    def ZoomHistory(self, n, s, e, w):
        """!Manages a list of last 10 zoom extents

        @param n,s,e,w north, south, east, west

        @return removed history item if exists (or None)
        """
        removed = None
        self.zoomhistory.append((n,s,e,w))
        
        if len(self.zoomhistory) > 10:
            removed = self.zoomhistory.pop(0)
        
        if removed:
            Debug.msg(4, "BufferedWindow.ZoomHistory(): hist=%s, removed=%s" %
                      (self.zoomhistory, removed))
        else:
            Debug.msg(4, "BufferedWindow.ZoomHistory(): hist=%s" %
                      (self.zoomhistory))
        
        # update toolbar
        if len(self.zoomhistory) > 1:
            enable = True
        else:
            enable = False
        
        if self.parent.GetName() == 'MapWindow':
            toolbar = self.parent.toolbars['map']
        elif self.parent.GetName() == 'GRMapWindow':
            toolbar = self.parent.toolbars['georect']
        elif self.parent.GetName() == 'GCPMapWindow':
            toolbar = self.parent.toolbars['gcpdisp']
        
        toolbar.Enable('zoomback', enable)
        
        return removed

    def ResetZoomHistory(self):
        """!Reset zoom history"""
        self.zoomhistory = list()
                
    def ZoomToMap(self, layers = None, ignoreNulls = False, render = True):
        """!Set display extents to match selected raster
        or vector map(s).

        @param layers list of layers to be zoom to
        @param ignoreNulls True to ignore null-values (valid only for rasters)
        @param render True to re-render display
        """
        zoomreg = {}
        
        if not layers:
            layers = self.GetSelectedLayer(multi = True)
        
        if not layers:
            return
        
        rast = []
        vect = []
        updated = False
        for l in layers:
            # only raster/vector layers are currently supported
            if l.type == 'raster':
                rast.append(l.GetName())
            elif l.type == 'vector':
                if hasattr(self, "digit") and \
                        self.toolbar.GetLayer() == l:
                    w, s, b, e, n, t = self.digit.GetDisplay().GetMapBoundingBox()
                    self.Map.GetRegion(n = n, s = s, w = w, e = e,
                                       update = True)
                    updated = True
                else:
                    vect.append(l.name)
            elif l.type == 'rgb':
                for rname in l.GetName().splitlines():
                    rast.append(rname)
            
        if not updated:
            self.Map.GetRegion(rast = rast,
                               vect = vect,
                               update = True)
        
        self.ZoomHistory(self.Map.region['n'], self.Map.region['s'],
                         self.Map.region['e'], self.Map.region['w'])
        
        if render:
            self.UpdateMap()
        
        self.parent.StatusbarUpdate()
        
    def ZoomToWind(self):
        """!Set display geometry to match computational region
        settings (set with g.region)
        """
        self.Map.region = self.Map.GetRegion()
        
        self.ZoomHistory(self.Map.region['n'], self.Map.region['s'],
                         self.Map.region['e'], self.Map.region['w'])
        
        self.UpdateMap()
        
        self.parent.StatusbarUpdate()

    def ZoomToDefault(self):
        """!Set display geometry to match default region settings
        """
        self.Map.region = self.Map.GetRegion(default = True)
        self.Map.AdjustRegion() # aling region extent to the display

        self.ZoomHistory(self.Map.region['n'], self.Map.region['s'],
                         self.Map.region['e'], self.Map.region['w'])
        
        self.UpdateMap()
        
        self.parent.StatusbarUpdate()
        
    def DisplayToWind(self):
        """!Set computational region (WIND file) to match display
        extents
        """
        tmpreg = os.getenv("GRASS_REGION")
        if tmpreg:
            del os.environ["GRASS_REGION"]
        
        # We ONLY want to set extents here. Don't mess with resolution. Leave that
        # for user to set explicitly with g.region
        new = self.Map.AlignResolution()
        gcmd.RunCommand('g.region',
                        parent = self,
                        overwrite = True,
                        n = new['n'],
                        s = new['s'],
                        e = new['e'],
                        w = new['w'],
                        rows = int(new['rows']),
                        cols = int(new['cols']))
        
        if tmpreg:
            os.environ["GRASS_REGION"] = tmpreg
        
    def ZoomToSaved(self):
        """!Set display geometry to match extents in
        saved region file
        """
        dlg = gdialogs.SavedRegion(parent = self,
                                   title = _("Zoom to saved region extents"),
                                   loadsave='load')
        
        if dlg.ShowModal() == wx.ID_CANCEL or not dlg.wind:
            dlg.Destroy()
            return
        
        if not grass.find_file(name = dlg.wind, element = 'windows')['name']:
            wx.MessageBox(parent = self,
                          message = _("Region <%s> not found. Operation canceled.") % dlg.wind,
                          caption = _("Error"), style = wx.ICON_ERROR | wx.OK | wx.CENTRE)
            dlg.Destroy()
            return
        
        self.Map.GetRegion(regionName = dlg.wind,
                           update = True)
        
        dlg.Destroy()
        
        self.ZoomHistory(self.Map.region['n'],
                         self.Map.region['s'],
                         self.Map.region['e'],
                         self.Map.region['w'])
        
        self.UpdateMap()
                
    def SaveDisplayRegion(self):
        """!Save display extents to named region file.
        """
        dlg = gdialogs.SavedRegion(parent = self,
                                   title = _("Save display extents to region file"),
                                   loadsave='save')
        
        if dlg.ShowModal() == wx.ID_CANCEL or not dlg.wind:
            dlg.Destroy()
            return
        
        # test to see if it already exists and ask permission to overwrite
        if grass.find_file(name = dlg.wind, element = 'windows')['name']:
            overwrite = wx.MessageBox(parent = self,
                                      message = _("Region file <%s> already exists. "
                                                  "Do you want to overwrite it?") % (dlg.wind),
                                      caption = _("Warning"), style = wx.YES_NO | wx.CENTRE)
            if (overwrite == wx.YES):
                self.SaveRegion(dlg.wind)
        else:
            self.SaveRegion(dlg.wind)
        
        dlg.Destroy()
        
    def SaveRegion(self, wind):
        """!Save region settings
        
        @param wind region name
        """
        new = self.Map.GetCurrentRegion()
        
        tmpreg = os.getenv("GRASS_REGION")
        if tmpreg:
            del os.environ["GRASS_REGION"]
        
        gcmd.RunCommand('g.region',
                        overwrite = True,
                        parent = self,
                        flags = 'u',
                        n = new['n'],
                        s = new['s'],
                        e = new['e'],
                        w = new['w'],
                        rows = int(new['rows']),
                        cols = int(new['cols']),
                        save = wind)
        
        if tmpreg:
            os.environ["GRASS_REGION"] = tmpreg
        
    def Distance(self, beginpt, endpt, screen = True):
        """!Calculete distance
        
        Ctypes required for LL-locations
        
        @param beginpt first point
        @param endpt second point
        @param screen True for screen coordinates otherwise EN
        """
        if screen:
            e1, n1 = self.Pixel2Cell(beginpt)
            e2, n2 = self.Pixel2Cell(endpt)
        else:
            e1, n1 = beginpt
            e2, n2 = endpt
            
        dEast  = (e2 - e1)
        dNorth = (n2 - n1)
        
        if self.parent.Map.projinfo['proj'] == 'll' and haveCtypes:
            dist = gislib.G_distance(e1, n1, e2, n2)
        else:
            dist = math.sqrt(math.pow((dEast), 2) + math.pow((dNorth), 2))
        
        return (dist, (dEast, dNorth))
