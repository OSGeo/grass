"""!
@package mapdisp_window.py

@brief GIS map display canvas, buffered window.

Classes:
 - MapWindow
 - BufferedWindow

(C) 2006-2010 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
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
import dbm_dialogs
import gdialogs
import gcmd
import utils
import globalvar
import gselect
from debug import Debug
from preferences import globalSettings as UserSettings
from units import ConvertValue as UnitsConvertValue
from vdigit import GV_LINES as VDigit_Lines_Type
from vdigit import VDigitCategoryDialog
from vdigit import VDigitZBulkDialog
from vdigit import VDigitDuplicatesDialog

class MapWindow(object):
    """!Abstract map window class

    Parent for BufferedWindow class (2D display mode) and
    GLWindow (3D display mode)
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
        
    def EraseMap(self):
        """!Erase the canvas (virtual method)
        """
        pass

    def UpdateMap(self):
        """!Updates the canvas anytime there is a change to the
        underlaying images or to the geometry of the canvas.
        """
        pass

    def OnLeftDown(self, event):
        pass

    def OnLeftUp(self, event):
        pass

    def OnKeyDown(self, event):
        pass
    
    def OnMotion(self, event):
        """!Mouse moved
        Track mouse motion and update status bar
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
                return
            
            if self.parent.toolbars['vdigit'] and \
                    self.parent.toolbars['vdigit'].GetAction() == 'addLine' and \
                    self.parent.toolbars['vdigit'].GetAction('type') in ('line', 'boundary') and \
                    len(self.polycoords) > 0:
                # for linear feature show segment and total length
                distance_seg = self.Distance(self.polycoords[-1],
                                             (e, n), screen = False)[0]
                distance_tot = distance_seg
                for idx in range(1, len(self.polycoords)):
                    distance_tot += self.Distance(self.polycoords[idx-1],
                                                  self.polycoords[idx],
                                                  screen = False)[0]
                self.parent.statusbar.SetStatusText("%.*f, %.*f (seg: %.*f; tot: %.*f)" % \
                                                 (precision, e, precision, n,
                                                  precision, distance_seg,
                                                  precision, distance_tot), 0)
            else:
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

    def OnZoomToMap(self, event):
        pass

    def OnZoomToRaster(self, event):
        pass
    
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
    """!A Buffered window class.

    When the drawing needs to change, you app needs to call the
    UpdateMap() method. Since the drawing is stored in a bitmap, you
    can also save the drawing to file by calling the
    SaveToFile(self,file_name,file_type) method.
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
        ### self.Bind(wx.EVT_MOTION,       self.MouseActions)
        self.Bind(wx.EVT_MOUSE_EVENTS, self.MouseActions)
        self.Bind(wx.EVT_MOTION,       self.OnMotion)
        
        self.processMouse = True
        
        # render output objects
        self.mapfile = None   # image file to be rendered
        self.img     = None   # wx.Image object (self.mapfile)
        # used in digitization tool (do not redraw vector map)
        self.imgVectorMap = None
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

        self.DefinePseudoDC()
        # redraw all pdc's, pdcTmp layer is redrawn always (speed issue)
        self.redrawAll = True

        # will store an off screen empty bitmap for saving to file
        self._buffer = ''

        self.Bind(wx.EVT_ERASE_BACKGROUND, lambda x:None)
        self.Bind(wx.EVT_KEY_DOWN , self.OnKeyDown)
        
        # vars for handling mouse clicks
        self.dragid   = -1
        self.lastpos  = (0, 0)

    def DefinePseudoDC(self):
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
        
        ### pdc.Clear()

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
                # self.ovlcoords[drawid] = coords

        elif pdctype == 'polyline': # draw a polyline on top of the map
            if self.polypen:
                pdc.SetBrush(wx.Brush(wx.CYAN, wx.TRANSPARENT))
                pdc.SetPen(self.polypen)
                ### pdc.DrawLines(coords)
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
                # self.ovlcoords[drawid] = coords

        elif pdctype == 'text': # draw text on top of map
            if not img['active']:
                return #only draw active text
            if img.has_key('rotation'):
                rotation = float(img['rotation'])
            else:
                rotation = 0.0
            w, h = self.GetFullTextExtent(img['text'])[0:2]
            pdc.SetFont(img['font'])
            pdc.SetTextForeground(img['color'])
            coords, w, h = self.TextBounds(img)
            if rotation == 0:
                pdc.DrawText(img['text'], coords[0], coords[1])
            else:
                pdc.DrawRotatedText(img['text'], coords[0], coords[1], rotation)
            pdc.SetIdBounds(drawid, wx.Rect(coords[0], coords[1], w, h))
            
        pdc.EndDrawing()
        
        self.Refresh()
        
        return drawid

    def TextBounds(self, textinfo):
        """!Return text boundary data

        @param textinfo text metadata (text, font, color, rotation)
        @param coords reference point
        """
        if textinfo.has_key('rotation'):
            rotation = float(textinfo['rotation'])
        else:
            rotation = 0.0
        
        coords = textinfo['coords']
        
        Debug.msg (4, "BufferedWindow.TextBounds(): text=%s, rotation=%f" % \
                   (textinfo['text'], rotation))

        self.Update()
        ### self.Refresh()

        self.SetFont(textinfo['font'])

        w, h = self.GetTextExtent(textinfo['text'])

        if rotation == 0:
            coords[2], coords[3] = coords[0] + w, coords[1] + h
            return coords, w, h

        boxh = math.fabs(math.sin(math.radians(rotation)) * w) + h
        boxw = math.fabs(math.cos(math.radians(rotation)) * w) + h
        coords[2] = coords[0] + boxw
        coords[3] = coords[1] + boxh
        
        return coords, boxw, boxh

    def OnKeyDown(self, event):
        """!Key pressed"""
        shift = event.ShiftDown()
        kc = event.GetKeyCode()
        
        vdigitToolbar = self.parent.toolbars['vdigit']
        ### vdigit
        if vdigitToolbar:
            event = None
            if not shift:
                if kc == ord('P'):
                    event = wx.CommandEvent(winid = vdigitToolbar.addPoint)
                    tool = vdigitToolbar.OnAddPoint
                elif kc == ord('L'):
                    event = wx.CommandEvent(winid = vdigitToolbar.addLine)
                    tool = vdigitToolbar.OnAddLine
            if event:
                vdigitToolbar.OnTool(event)
                tool(event)
        
    def OnPaint(self, event):
        """!
        Draw PseudoDC's to buffered paint DC

        self.pdc for background and decorations
        self.pdcVector for vector map which is edited
        self.pdcTmp for temporaly drawn objects (self.polycoords)

        If self.redrawAll is False on self.pdcTmp content is re-drawn
        """
        Debug.msg(4, "BufferedWindow.OnPaint(): redrawAll=%s" % self.redrawAll)
        
        dc = wx.BufferedPaintDC(self, self.buffer)
        
        ### dc.SetBackground(wx.Brush("White"))
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
            if self.pdcVector:
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
                
                if self.pdcVector:
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
        """!
        Converts rendered overlay files to wx.Image

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
        """!
        Updates the canvas anytime there is a change to the
        underlaying images or to the geometry of the canvas.
        
        @param render re-render map composition
        @param renderVector re-render vector map layer enabled for editing (used for digitizer)
        """
        start = time.clock()
        
        self.resize = False
        
        # if len(self.Map.GetListOfLayers()) == 0:
        #    return False
        
        if self.img is None:
            render = True
        
        #
        # initialize process bar (only on 'render')
        #
        if render is True or renderVector is True:
            self.parent.statusbarWin['progress'].Show()
            if self.parent.statusbarWin['progress'].GetRange() > 0:
                self.parent.statusbarWin['progress'].SetValue(1)
        
        #
        # render background image if needed
        #
        
        # update layer dictionary if there has been a change in layers
        if self.tree and self.tree.reorder == True:
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
            gcmd.GError(message = e)
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
        digitToolbar = self.parent.toolbars['vdigit']
        if renderVector and digitToolbar and \
                digitToolbar.GetLayer():
            # set region
            self.parent.digit.GetDisplay().UpdateRegion()
            # re-calculate threshold for digitization tool
            # self.parent.digit.GetDisplay().GetThreshold()
            # draw map
            if self.pdcVector:
                self.pdcVector.Clear()
                self.pdcVector.RemoveAll()
            try:
                item = self.tree.FindItemByData('maplayer', digitToolbar.GetLayer())
            except TypeError:
                item = None
            
            if item and self.tree.IsItemChecked(item):
                self.parent.digit.GetDisplay().DrawMap()

            # translate tmp objects (pointer position)
            if digitToolbar.GetAction() == 'moveLine':
                if  hasattr(self, "vdigitMove") and \
                        self.vdigitMove.has_key('beginDiff'):
                    # move line
                    for id in self.vdigitMove['id']:
                        self.pdcTmp.TranslateId(id,
                                                self.vdigitMove['beginDiff'][0],
                                                self.vdigitMove['beginDiff'][1])
                    del self.vdigitMove['beginDiff']
        
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
        """!
        Erase the canvas
        """
        self.Draw(self.pdc, pdctype = 'clear')
                  
        if self.pdcVector:
            self.Draw(self.pdcVector, pdctype = 'clear')
        
        self.Draw(self.pdcDec, pdctype = 'clear')
        self.Draw(self.pdcTmp, pdctype = 'clear')
        
    def DragMap(self, moveto):
        """!
        Drag the entire map image for panning.

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
        """!
        Drag an overlay decoration item
        """
        if id == 99 or id == '' or id == None: return
        Debug.msg (5, "BufferedWindow.DragItem(): id=%d" % id)
        x, y = self.lastpos
        dx = event.GetX() - x
        dy = event.GetY() - y
        self.pdc.SetBackground(wx.Brush(self.GetBackgroundColour()))
        r = self.pdc.GetIdBounds(id)
        ### FIXME in vdigit/pseudodc.i
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
            self.textdict[id]['coords'] = r2
        r = r.Union(r2)
        r.Inflate(4,4)
        self.RefreshRect(r, False)
        self.lastpos = (event.GetX(), event.GetY())
                
    def MouseDraw(self, pdc = None, begin = None, end = None):
        """!
        Mouse box or line from 'begin' to 'end'

        If not given from self.mouse['begin'] to self.mouse['end'].

        """
#        self.redrawAll = False
        
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
        """!
        Draw polyline in PseudoDC

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
        """!
        Mouse motion and button click notifier
        """
        if not self.processMouse:
            return
        
        ### if self.redrawAll is False:
        ###    self.redrawAll = True
        
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
        
        # event.Skip()
        
    def OnMouseWheel(self, event):
        """!
        Mouse wheel moved
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

        ### self.OnPaint(None)

        # update statusbar
        self.parent.StatusbarUpdate()

        self.Refresh()
        self.processMouse = True
#        event.Skip()

    def OnDragging(self, event):
        """!
        Mouse dragging with left button down
        """
        Debug.msg (5, "BufferedWindow.MouseAction(): Dragging")
        current  = event.GetPositionTuple()[:]
        previous = self.mouse['begin']
        move = (current[0] - previous[0],
                current[1] - previous[1])
        
        digitToolbar = self.parent.toolbars['vdigit']
        
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
                not digitToolbar): return
            self.mouse['end'] = event.GetPositionTuple()[:]
            digitClass = self.parent.digit
            if (event.LeftIsDown() and 
                not (digitToolbar and 
                    digitToolbar.GetAction() in ("moveLine",) and 
                    digitClass.GetDisplay().GetSelected() > 0)):
                # draw box only when left mouse button is pressed
                self.MouseDraw(pdc = self.pdcTmp)
        
        # event.Skip()

    def OnLeftDownVDigitAddLine(self, event):
        """!Left mouse button down - vector digitizer add new line
        action
        """
        digitToolbar = self.parent.toolbars['vdigit']
        digitClass   = self.parent.digit
        
        try:
            mapLayer = digitToolbar.GetLayer().GetName()
        except:
            return
        
        if digitToolbar.GetAction('type') in ['point', 'centroid']:
            # add new point / centroiud
            east, north = self.Pixel2Cell(self.mouse['begin'])
            fid = digitClass.AddFeature(digitToolbar.GetAction('type'), [(east, north)])
            if fid < 0:
                return
            
            self.UpdateMap(render = False) # redraw map
            
            # add new record into atribute table
            if UserSettings.Get(group = 'vdigit', key = "addRecord", subkey = 'enabled'):
                # select attributes based on layer and category
                cats = { fid : {
                        UserSettings.Get(group = 'vdigit', key = "layer", subkey = 'value') :
                            (UserSettings.Get(group = 'vdigit', key = "category", subkey = 'value'), )
                        }}
                
                posWindow = self.ClientToScreen((self.mouse['end'][0] + self.dialogOffset,
                                                 self.mouse['end'][1] + self.dialogOffset))
                
                addRecordDlg = dbm_dialogs.DisplayAttributesDialog(parent = self, map = mapLayer,
                                                                   cats = cats,
                                                                   pos = posWindow,
                                                                   action = "add")

                if not point:
                    self.__geomAttrb(fid, addRecordDlg, 'area', digitClass,
                                     digitToolbar.GetLayer())
                    self.__geomAttrb(fid, addRecordDlg, 'perimeter', digitClass,
                                     digitToolbar.GetLayer())

                if addRecordDlg.mapDBInfo and \
                        addRecordDlg.ShowModal() == wx.ID_OK:
                    sqlfile = tempfile.NamedTemporaryFile(mode = "w")
                    for sql in addRecordDlg.GetSQLString():
                        sqlfile.file.write(sql + ";\n")
                    sqlfile.file.flush()
                    
                    gcmd.RunCommand('db.execute',
                                    parent = self,
                                    quiet = True, 
                                    input = sqlfile.name)
                
                if addRecordDlg.mapDBInfo:
                    self.__updateATM()
        
        elif digitToolbar.GetAction('type') in ["line", "boundary"]:
            # add new point to the line
            self.polycoords.append(self.Pixel2Cell(event.GetPositionTuple()[:]))
            self.DrawLines(pdc = self.pdcTmp)
    
    def __geomAttrb(self, fid, dialog, attrb, digit, mapLayer):
        """!Trac geometry attributes?"""
        item = self.tree.FindItemByData('maplayer', mapLayer)
        vdigit = self.tree.GetPyData(item)[0]['vdigit']
        if vdigit and \
                vdigit.has_key('geomAttr') and \
                vdigit['geomAttr'].has_key(attrb):
            val = -1
            if attrb == 'length':
                val = digit.GetLineLength(fid)
                type = attrb
            elif attrb == 'area':
                val = digit.GetAreaSize(fid)
                type = attrb
            elif attrb == 'perimeter':
                val = digit.GetAreaPerimeter(fid)
                type = 'length'
            
            if val > 0:
                layer = int(UserSettings.Get(group = 'vdigit', key = "layer", subkey = 'value'))
                column = vdigit['geomAttr'][attrb]['column']
                val = UnitsConvertValue(val, type, vdigit['geomAttr'][attrb]['units'])
                dialog.SetColumnValue(layer, column, val)
                dialog.OnReset()
        
    def __geomAttrbUpdate(self, fids):
        """!Update geometry atrributes of currently selected features

        @param fid list feature id
        """
        mapLayer = self.parent.toolbars['vdigit'].GetLayer()
        vectorName =  mapLayer.GetName()
        digit = self.parent.digit
        item = self.tree.FindItemByData('maplayer', mapLayer)
        vdigit = self.tree.GetPyData(item)[0]['vdigit']
        
        if vdigit is None or not vdigit.has_key('geomAttr'):
            return
        
        dbInfo = gselect.VectorDBInfo(vectorName)
        sqlfile = tempfile.NamedTemporaryFile(mode = "w")
        for fid in fids:
            for layer, cats in digit.GetLineCats(fid).iteritems():
                table = dbInfo.GetTable(layer)
                for attrb, item in vdigit['geomAttr'].iteritems():
                    val = -1
                    if attrb == 'length':
                        val = digit.GetLineLength(fid)
                        type = attrb
                    elif attrb == 'area':
                        val = digit.GetAreaSize(fid)
                        type = attrb
                    elif attrb == 'perimeter':
                        val = digit.GetAreaPerimeter(fid)
                        type = 'length'

                    if val < 0:
                        continue
                    val = UnitsConvertValue(val, type, item['units'])
                    
                    for cat in cats:
                        sqlfile.write('UPDATE %s SET %s = %f WHERE %s = %d;\n' % \
                                          (table, item['column'], val,
                                           dbInfo.GetKeyColumn(layer), cat))
            sqlfile.file.flush()
            gcmd.RunCommand('db.execute',
                            parent = True,
                            quiet = True,
                            input = sqlfile.name)
            
    def __updateATM(self):
        """!Update open Attribute Table Manager

        @todo: use AddDataRow() instead
        """
        # update ATM
        digitToolbar = self.parent.toolbars['vdigit']
        digitVector = digitToolbar.GetLayer().GetName()
                            
        for atm in self.lmgr.dialogs['atm']:
            atmVector = atm.GetVectorName()
            if atmVector == digitVector:
                layer = UserSettings.Get(group = 'vdigit', key = "layer", subkey = 'value')
                # TODO: use AddDataRow instead
                atm.LoadData(layer)
        
    def OnLeftDownVDigitEditLine(self, event):
        """!
        Left mouse button down - vector digitizer edit linear feature
        - add new vertex.
        """
        digitToolbar = self.parent.toolbars['vdigit']
        digitClass   = self.parent.digit
        
        self.polycoords.append(self.Pixel2Cell(self.mouse['begin']))
        self.vdigitMove['id'].append(wx.NewId())
        self.DrawLines(pdc = self.pdcTmp)

    def OnLeftDownVDigitMoveLine(self, event):
        """!
        Left mouse button down - vector digitizer move feature/vertex,
        edit linear feature
        """
        digitToolbar = self.parent.toolbars['vdigit']
        digitClass   = self.parent.digit
        
        self.vdigitMove = {}
        # geographic coordinates of initial position (left-down)
        self.vdigitMove['begin'] = None
        # list of ids to modify    
        self.vdigitMove['id'] = []
        # ids geographic coordinates
        self.vdigitMove['coord'] = {}
                
        if digitToolbar.GetAction() in ["moveVertex", "editLine"]:
            # set pen
            pcolor = UserSettings.Get(group = 'vdigit', key = "symbol",
                                      subkey = ["highlight", "color"])
            self.pen = self.polypen = wx.Pen(colour = pcolor,
                                             width = 2, style = wx.SHORT_DASH)
            self.pdcTmp.SetPen(self.polypen)

    def OnLeftDownVDigitDisplayCA(self, event):
        """!
        Left mouse button down - vector digitizer display categories
        or attributes action
        """
        digitToolbar = self.parent.toolbars['vdigit']
        digitClass   = self.parent.digit
        
        try:
            mapLayer = digitToolbar.GetLayer().GetName()
        except:
            return
        
        coords = self.Pixel2Cell(self.mouse['begin'])
        
        # unselect
        digitClass.GetDisplay().SetSelected([])
        
        # select feature by point
        cats = {}
        if digitClass.GetDisplay().SelectLineByPoint(coords,
                                               digitClass.GetSelectType()) is None:
            return

        if UserSettings.Get(group = 'vdigit', key = 'checkForDupl',
                            subkey = 'enabled'):
            lines = digitClass.GetDisplay().GetSelected()
        else:
            lines = (digitClass.GetDisplay().GetSelected()[0],) # only first found
                        
        for line in lines:
            cats[line] = digitClass.GetLineCats(line)
                   
        posWindow = self.ClientToScreen((self.mouse['end'][0] + self.dialogOffset,
                                         self.mouse['end'][1] + self.dialogOffset))
    
        if digitToolbar.GetAction() == "displayAttrs":
            # select attributes based on coordinates (all layers)
            if self.parent.dialogs['attributes'] is None:
                self.parent.dialogs['attributes'] = \
                    dbm_dialogs.DisplayAttributesDialog(parent = self, map = mapLayer,
                                                        cats = cats,
                                                        action = "update")
            else:
                # upgrade dialog
                self.parent.dialogs['attributes'].UpdateDialog(cats = cats)

            if self.parent.dialogs['attributes']:
                if len(cats.keys()) > 0:
                    # highlight feature & re-draw map
                    if not self.parent.dialogs['attributes'].IsShown():
                        self.parent.dialogs['attributes'].Show()
                else:
                    if self.parent.dialogs['attributes'] and \
                            self.parent.dialogs['attributes'].IsShown():
                        self.parent.dialogs['attributes'].Hide()

        else: # displayCats
            if self.parent.dialogs['category'] is None:
                # open new dialog
                dlg = VDigitCategoryDialog(parent = self,
                                           map = mapLayer,
                                           cats = cats,
                                           pos = posWindow,
                                           title = _("Update categories"))
                self.parent.dialogs['category'] = dlg
            else:
                # update currently open dialog
                self.parent.dialogs['category'].UpdateDialog(cats = cats)
                            
            if self.parent.dialogs['category']:
                if len(cats.keys()) > 0:
                    # highlight feature & re-draw map
                    if not self.parent.dialogs['category'].IsShown():
                        self.parent.dialogs['category'].Show()
                else:
                    if self.parent.dialogs['category'].IsShown():
                        self.parent.dialogs['category'].Hide()
                
        self.UpdateMap(render = False)
 
    def OnLeftDownVDigitCopyCA(self, event):
        """!
        Left mouse button down - vector digitizer copy categories
        or attributes action
        """
        digitToolbar = self.parent.toolbars['vdigit']
        digitClass   = self.parent.digit
        
        if not hasattr(self, "copyCatsList"):
            self.copyCatsList = []
        else:
            self.copyCatsIds = []
            self.mouse['box'] = 'box'
        
    def OnLeftDownVDigitCopyLine(self, event):
        """!
        Left mouse button down - vector digitizer copy lines action
        """
        digitToolbar = self.parent.toolbars['vdigit']
        digitClass   = self.parent.digit
        
        if not hasattr(self, "copyIds"):
            self.copyIds = []
            self.layerTmp = None
        
    def OnLeftDownVDigitBulkLine(self, event):
        """!Left mouse button down - vector digitizer label 3d vector
        lines
        """
        digitToolbar = self.parent.toolbars['vdigit']
        digitClass   = self.parent.digit
        
        if len(self.polycoords) > 1: # start new line
            self.polycoords = []
            self.ClearLines(pdc = self.pdcTmp)
        self.polycoords.append(self.Pixel2Cell(event.GetPositionTuple()[:]))
        if len(self.polycoords) == 1:
            begin = self.Pixel2Cell(self.polycoords[-1])
            end   = self.Pixel2Cell(self.mouse['end'])
        else:
            end   = self.Pixel2Cell(self.polycoords[-1])
            begin = self.Pixel2Cell(self.mouse['begin'])
            
            self.DrawLines(self.pdcTmp, polycoords = (begin, end))
        
    def OnLeftDown(self, event):
        """!
        Left mouse button pressed
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
        elif self.mouse['use'] == 'zoom':
            pass

        #
        # vector digizer
        #
        elif self.mouse["use"] == "pointer" and \
                self.parent.toolbars['vdigit']:
            digitToolbar = self.parent.toolbars['vdigit']
            digitClass   = self.parent.digit
            
            try:
                mapLayer = digitToolbar.GetLayer().GetName()
            except:
                wx.MessageBox(parent = self,
                              message = _("No vector map selected for editing."),
                              caption = _("Vector digitizer"),
                              style = wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
                event.Skip()
                return
            
            if digitToolbar.GetAction() not in ("moveVertex",
                                                "addVertex",
                                                "removeVertex",
                                                "editLine"):
                # set pen
                self.pen = wx.Pen(colour = 'Red', width = 2, style = wx.SHORT_DASH)
                self.polypen = wx.Pen(colour = 'dark green', width = 2, style = wx.SOLID)

            if digitToolbar.GetAction() in ("addVertex",
                                            "removeVertex",
                                            "splitLines"):
                # unselect
                digitClass.GetDisplay().SetSelected([])

            if digitToolbar.GetAction() == "addLine":
                self.OnLeftDownVDigitAddLine(event)
            
            elif digitToolbar.GetAction() == "editLine" and \
                    hasattr(self, "vdigitMove"):
                self.OnLeftDownVDigitEditLine(event)

            elif digitToolbar.GetAction() in ("moveLine", 
                                              "moveVertex",
                                              "editLine") and \
                    not hasattr(self, "vdigitMove"):
                self.OnLeftDownVDigitMoveLine(event)

            elif digitToolbar.GetAction() in ("displayAttrs"
                                              "displayCats"):
                self.OnLeftDownVDigitDisplayCA(event)
            
            elif digitToolbar.GetAction() in ("copyCats",
                                              "copyAttrs"):
                self.OnLeftDownVDigitCopyCA(event)
            
            elif digitToolbar.GetAction() == "copyLine":
                self.OnLeftDownVDigitCopyLine(event)
            
            elif digitToolbar.GetAction() == "zbulkLine":
                self.OnLeftDownVDigitBulkLine(event)
            
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

    def OnLeftUpVDigitVarious(self, event):
        """!
        Left mouse button up - vector digitizer various actions
        """
        digitToolbar = self.parent.toolbars['vdigit']
        digitClass   = self.parent.digit
        
        pos1 = self.Pixel2Cell(self.mouse['begin'])
        pos2 = self.Pixel2Cell(self.mouse['end'])
        
        nselected = 0
        # -> delete line || move line || move vertex
        if digitToolbar.GetAction() in ("moveVertex",
                                        "editLine"):
            if len(digitClass.GetDisplay().GetSelected()) == 0:
                nselected = digitClass.GetDisplay().SelectLineByPoint(pos1, type = VDigit_Lines_Type)
                
                if digitToolbar.GetAction() == "editLine":
                    try:
                        selVertex = digitClass.GetDisplay().GetSelectedVertex(pos1)[0]
                    except IndexError:
                        selVertex = None
                        
                    if selVertex:
                        # self.UpdateMap(render=False)
                        ids = digitClass.GetDisplay().GetSelected(grassId = False)
                        # move this line to tmp layer
                        self.polycoords = []
                        for id in ids:
                            if id % 2: # register only vertices
                                e, n = self.Pixel2Cell(self.pdcVector.GetIdBounds(id)[0:2])
                                self.polycoords.append((e, n))
                        digitClass.GetDisplay().DrawSelected(False) 
                                
                        if selVertex < ids[-1] / 2:
                            # choose first or last node of line
                            self.vdigitMove['id'].reverse()
                            self.polycoords.reverse()
                    else:
                        # unselect
                        digitClass.GetDisplay().SetSelected([])
                        del self.vdigitMove
                
                    self.UpdateMap(render = False)
            
        elif digitToolbar.GetAction() in ("copyCats",
                                          "copyAttrs"):
            if not hasattr(self, "copyCatsIds"):
                # 'from' -> select by point
                nselected = digitClass.GetDisplay().SelectLineByPoint(pos1, digitClass.GetSelectType())
                if nselected:
                    self.copyCatsList = digitClass.GetDisplay().GetSelected()
            else:
                # -> 'to' -> select by bbox
                digitClass.GetDisplay().SetSelected([])
                # return number of selected features (by box/point)
                nselected = digitClass.GetDisplay().SelectLinesByBox(pos1, pos2,
                                                               digitClass.GetSelectType())
                if nselected == 0:
                    if digitClass.GetDisplay().SelectLineByPoint(pos1,
                                                           digitClass.GetSelectType()) is not None:
                        nselected = 1
                        
                if nselected > 0:
                    self.copyCatsIds = digitClass.GetDisplay().GetSelected()

        elif digitToolbar.GetAction() == "queryLine":
            selected = digitClass.SelectLinesByQuery(pos1, pos2)
            nselected = len(selected)
            if nselected > 0:
                digitClass.GetDisplay().SetSelected(selected)

        else:
            # -> moveLine || deleteLine, etc. (select by point/box)
            if digitToolbar.GetAction() == 'moveLine' and \
                    len(digitClass.GetDisplay().GetSelected()) > 0:
                nselected = 0
            else:
                if digitToolbar.GetAction() == 'moveLine':
                    drawSeg = True
                else:
                    drawSeg = False

                nselected = digitClass.GetDisplay().SelectLinesByBox(pos1, pos2,
                                                               digitClass.GetSelectType(),
                                                               drawSeg)
                    
                if nselected == 0:
                    if digitClass.GetDisplay().SelectLineByPoint(pos1,
                                                           digitClass.GetSelectType()) is not None:
                        nselected = 1
        
        if nselected > 0:
            if digitToolbar.GetAction() in ("moveLine",
                                            "moveVertex"):
                # get pseudoDC id of objects which should be redrawn
                if digitToolbar.GetAction() == "moveLine":
                    # -> move line
                    self.vdigitMove['id'] = digitClass.GetDisplay().GetSelected(grassId = False)
                    self.vdigitMove['coord'] = digitClass.GetDisplay().GetSelectedCoord()
                else: # moveVertex
                    self.vdigitMove['id'] = digitClass.GetDisplay().GetSelectedVertex(pos1)
                    if len(self.vdigitMove['id']) == 0: # no vertex found
                        digitClass.GetDisplay().SetSelected([])
                
            #
            # check for duplicates
            #
            if UserSettings.Get(group = 'vdigit', key = 'checkForDupl', subkey = 'enabled') is True:
                dupl = digitClass.GetDisplay().GetDuplicates()
                self.UpdateMap(render = False)
                    
                if dupl:
                    posWindow = self.ClientToScreen((self.mouse['end'][0] + self.dialogOffset,
                                                     self.mouse['end'][1] + self.dialogOffset))
                    
                    dlg = VDigitDuplicatesDialog(parent = self, data = dupl, pos = posWindow)
                    
                    if dlg.ShowModal() == wx.ID_OK:
                        digitClass.GetDisplay().UnSelect(dlg.GetUnSelected())
                        # update selected
                        self.UpdateMap(render = False)
                
            if digitToolbar.GetAction() != "editLine":
                # -> move line || move vertex
                self.UpdateMap(render = False)
        
        else: # no vector object found
            if not (digitToolbar.GetAction() in ("moveLine",
                                                 "moveVertex") and \
                        len(self.vdigitMove['id']) > 0):
                # avoid left-click when features are already selected
                self.UpdateMap(render = False, renderVector = False)
        
    def OnLeftUpVDigitModifyLine(self, event):
        """!
        Left mouse button up - vector digitizer split line, add/remove
        vertex action
        """
        digitToolbar = self.parent.toolbars['vdigit']
        digitClass   = self.parent.digit
        
        pos1 = self.Pixel2Cell(self.mouse['begin'])
        
        pointOnLine = digitClass.GetDisplay().SelectLineByPoint(pos1,
                                                          type = VDigit_Lines_Type)

        if not pointOnLine:
            return

        if digitToolbar.GetAction() in ["splitLine", "addVertex"]:
            self.UpdateMap(render = False) # highlight object
            self.DrawCross(pdc = self.pdcTmp, coords = self.Cell2Pixel(pointOnLine),
                           size = 5)
        else: # removeVertex
            # get only id of vertex
            try:
                id = digitClass.GetDisplay().GetSelectedVertex(pos1)[0]
            except IndexError:
                id = None

            if id:
                x, y = self.pdcVector.GetIdBounds(id)[0:2]
                self.pdcVector.RemoveId(id)
                self.UpdateMap(render = False) # highlight object
                self.DrawCross(pdc = self.pdcTmp, coords = (x, y),
                               size = 5)
            else:
                # unselect
                digitClass.GetDisplay().SetSelected([])
                self.UpdateMap(render = False)

    def OnLeftUpVDigitCopyLine(self, event):
        """!
        Left mouse button up - vector digitizer copy feature action
        """
        digitToolbar = self.parent.toolbars['vdigit']
        digitClass   = self.parent.digit
        
        pos1 = self.Pixel2Cell(self.mouse['begin'])
        pos2 = self.Pixel2Cell(self.mouse['end'])
        
        if UserSettings.Get(group = 'vdigit', key = 'bgmap',
                            subkey = 'value', internal = True) == '':
            # no background map -> copy from current vector map layer
            nselected = digitClass.GetDisplay().SelectLinesByBox(pos1, pos2,
                                                           digitClass.GetSelectType())

            if nselected > 0:
                # highlight selected features
                self.UpdateMap(render = False)
            else:
                self.UpdateMap(render = False, renderVector = False)
        else:
            # copy features from background map
            self.copyIds += digitClass.SelectLinesFromBackgroundMap(pos1, pos2)
            if len(self.copyIds) > 0:
                color = UserSettings.Get(group = 'vdigit', key = 'symbol',
                                         subkey = ['highlight', 'color'])
                colorStr = str(color[0]) + ":" + \
                    str(color[1]) + ":" + \
                    str(color[2])
                dVectTmp = ['d.vect',
                            'map=%s' % UserSettings.Get(group = 'vdigit', key = 'bgmap',
                                                        subkey = 'value', internal = True),
                            'cats=%s' % utils.ListOfCatsToRange(self.copyIds),
                            '-i',
                            'color=%s' % colorStr,
                            'fcolor=%s' % colorStr,
                            'type=point,line,boundary,centroid',
                            'width=2']
                        
                if not self.layerTmp:
                    self.layerTmp = self.Map.AddLayer(type = 'vector',
                                                      name = globalvar.QUERYLAYER,
                                                      command = dVectTmp)
                else:
                    self.layerTmp.SetCmd(dVectTmp)
                
                self.UpdateMap(render = True, renderVector = False)
            else:
                self.UpdateMap(render = False, renderVector = False)
            
            self.redrawAll = None
            
    def OnLeftUpVDigitBulkLine(self, event):
        """!
        Left mouse button up - vector digitizer z-bulk line action
        """
        digitToolbar = self.parent.toolbars['vdigit']
        digitClass   = self.parent.digit
        
        # select lines to be labeled
        pos1 = self.polycoords[0]
        pos2 = self.polycoords[1]
        nselected = digitClass.GetDisplay().SelectLinesByBox(pos1, pos2,
                                                       digitClass.GetSelectType())

        if nselected > 0:
            # highlight selected features
            self.UpdateMap(render = False)
            self.DrawLines(pdc = self.pdcTmp) # redraw temp line
        else:
            self.UpdateMap(render = False, renderVector = False)

    def OnLeftUpVDigitConnectLine(self, event):
        """!
        Left mouse button up - vector digitizer connect line action
        """
        digitToolbar = self.parent.toolbars['vdigit']
        digitClass   = self.parent.digit
        
        if len(digitClass.GetDisplay().GetSelected()) > 0:
            self.UpdateMap(render = False)
        
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
            self.parent.QueryMap(self.mouse['begin'][0],self.mouse['begin'][1])

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
                self.parent.GetLayerManager().georectifying:
            # -> georectifying
            coord = self.Pixel2Cell(self.mouse['end'])
            if self.parent.toolbars['georect']:
                coordtype = 'gcpcoord'
            else:
                coordtype = 'mapcoord'

            self.parent.GetLayerManager().georectifying.SetGCPData(coordtype, coord, self)
            self.UpdateMap(render = False, renderVector = False)

        elif self.mouse["use"] == "pointer" and self.parent.toolbars['vdigit']:
            # digitization tool active
            digitToolbar = self.parent.toolbars['vdigit']
            digitClass   = self.parent.digit
            
            if hasattr(self, "vdigitMove"):
                if len(digitClass.GetDisplay().GetSelected()) == 0:
                    self.vdigitMove['begin'] = self.Pixel2Cell(self.mouse['begin']) # left down
                
                # eliminate initial mouse moving efect
                self.mouse['begin'] = self.mouse['end'] 

            if digitToolbar.GetAction() in ("deleteLine",
                                            "moveLine",
                                            "moveVertex",
                                            "copyCats",
                                            "copyAttrs",
                                            "editLine",
                                            "flipLine",
                                            "mergeLine",
                                            "snapLine",
                                            "queryLine",
                                            "breakLine",
                                            "typeConv",
                                            "connectLine"):
                self.OnLeftUpVDigitVarious(event)

            elif digitToolbar.GetAction() in ("splitLine",
                                              "addVertex",
                                              "removeVertex"):
                self.OnLeftUpVDigitModifyLine(event)

            elif digitToolbar.GetAction() == "copyLine":
                self.OnLeftUpVDigitCopyLine(event)
            
            elif digitToolbar.GetAction() == "zbulkLine" and \
                    len(self.polycoords) == 2:
                self.OnLeftUpVDigitBulkLine(event)
            
            elif digitToolbar.GetAction() == "connectLine":
                self.OnLeftUpConnectLine(event)
            
            if len(digitClass.GetDisplay().GetSelected()) > 0:
                self.redrawAll = None
            
        elif (self.mouse['use'] == 'pointer' and 
                self.dragid >= 0):
            # end drag of overlay decoration
            
            if self.dragid < 99 and self.overlays.has_key(self.dragid):
                self.overlays[self.dragid]['coords'] = self.pdc.GetIdBounds(self.dragid)
            elif self.dragid > 100 and self.textdict.has_key(self.dragid):
                self.textdict[self.dragid]['coords'] = self.pdc.GetIdBounds(self.dragid)
            else:
                pass
            self.dragid = None
            self.currtxtid = None
        
    def OnButtonDClick(self, event):
        """!
        Mouse button double click
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

        elif self.mouse["use"] == "profile":
            pass

        elif self.mouse['use'] == 'pointer' and \
                self.parent.toolbars['vdigit']:
            # vector digitizer
            pass

        else:
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
        """!
        Right mouse button pressed
        """
        Debug.msg (5, "BufferedWindow.OnRightDown(): use=%s" % \
                   self.mouse["use"])
                   
        digitToolbar = self.parent.toolbars['vdigit']
        if digitToolbar:
            digitClass = self.parent.digit
            # digitization tool (confirm action)
            if digitToolbar.GetAction() in ("moveLine",
                                            "moveVertex") and \
                    hasattr(self, "vdigitMove"):

                pFrom = self.vdigitMove['begin']
                pTo = self.Pixel2Cell(event.GetPositionTuple())
                
                move = (pTo[0] - pFrom[0],
                        pTo[1] - pFrom[1])
                
                if digitToolbar.GetAction() == "moveLine":
                    # move line
                    if digitClass.MoveSelectedLines(move) < 0:
                        return
                elif digitToolbar.GetAction() == "moveVertex":
                    # move vertex
                    fid = digitClass.MoveSelectedVertex(pFrom, move)
                    if fid < 0:
                        return

                    self.__geomAttrbUpdate([fid,])
                
                del self.vdigitMove
                
        event.Skip()

    def OnRightUp(self, event):
        """!
        Right mouse button released
        """
        Debug.msg (5, "BufferedWindow.OnRightUp(): use=%s" % \
                   self.mouse["use"])

        digitToolbar = self.parent.toolbars['vdigit']
        if digitToolbar:
            digitClass = self.parent.digit
            # digitization tool (confirm action)
            if digitToolbar.GetAction() == "addLine" and \
                    digitToolbar.GetAction('type') in ["line", "boundary"]:
                # -> add new line / boundary
                try:
                    map = digitToolbar.GetLayer().GetName()
                except:
                    map = None
                    wx.MessageBox(parent = self,
                                  message = _("No vector map selected for editing."),
                                  caption = _("Error"), style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
                    
                if map:
                    if len(self.polycoords) < 2: # ignore 'one-point' lines
                        return
                    
                    fid = digitClass.AddFeature(digitToolbar.GetAction('type'), self.polycoords)
                    if fid < 0:
                        return
                    
                    position = self.Cell2Pixel(self.polycoords[-1])
                    self.polycoords = []
                    self.UpdateMap(render = False)
                    self.redrawAll = True
                    self.Refresh()
                    
                    # add new record into atribute table
                    if UserSettings.Get(group = 'vdigit', key = "addRecord", subkey = 'enabled') and \
                            (line is True or \
                                 (not line and fid > 0)):
                        posWindow = self.ClientToScreen((position[0] + self.dialogOffset,
                                                         position[1] + self.dialogOffset))

                        # select attributes based on layer and category
                        cats = { fid : {
                                UserSettings.Get(group = 'vdigit', key = "layer", subkey = 'value') :
                                    (UserSettings.Get(group = 'vdigit', key = "category", subkey = 'value'), )
                                }}
                        
                        addRecordDlg = dbm_dialogs.DisplayAttributesDialog(parent = self, map = map,
                                                                           cats = cats,
                                                                           pos = posWindow,
                                                                           action = "add")

                        self.__geomAttrb(fid, addRecordDlg, 'length', digitClass,
                                         digitToolbar.GetLayer())
                        # auto-placing centroid
                        self.__geomAttrb(fid, addRecordDlg, 'area', digitClass,
                                         digitToolbar.GetLayer())
                        self.__geomAttrb(fid, addRecordDlg, 'perimeter', digitClass,
                                         digitToolbar.GetLayer())

                        if addRecordDlg.mapDBInfo and \
                               addRecordDlg.ShowModal() == wx.ID_OK:
                            sqlfile = tempfile.NamedTemporaryFile(mode = "w")
                            for sql in addRecordDlg.GetSQLString():
                                sqlfile.file.write(sql + ";\n")
                            sqlfile.file.flush()
                            gcmd.RunCommand('db.execute',
                                            parent = True,
                                            quiet = True,
                                            input = sqlfile.name)
                        
                        if addRecordDlg.mapDBInfo:
                            self.__updateATM()
            
            elif digitToolbar.GetAction() == "deleteLine":
                # -> delete selected vector features
                if digitClass.DeleteSelectedLines() < 0:
                    return
                self.__updateATM()
            elif digitToolbar.GetAction() == "splitLine":
                # split line
                if digitClass.SplitLine(self.Pixel2Cell(self.mouse['begin'])) < 0:
                    return
            elif digitToolbar.GetAction() == "addVertex":
                # add vertex
                fid = digitClass.AddVertex(self.Pixel2Cell(self.mouse['begin']))
                if fid < 0:
                    return
            elif digitToolbar.GetAction() == "removeVertex":
                # remove vertex
                fid = digitClass.RemoveVertex(self.Pixel2Cell(self.mouse['begin']))
                if fid < 0:
                    return
                self.__geomAttrbUpdate([fid,])
            elif digitToolbar.GetAction() in ("copyCats", "copyAttrs"):
                try:
                    if digitToolbar.GetAction() == 'copyCats':
                        if digitClass.CopyCats(self.copyCatsList,
                                               self.copyCatsIds, copyAttrb = False) < 0:
                            return
                    else:
                        if digitClass.CopyCats(self.copyCatsList,
                                               self.copyCatsIds, copyAttrb = True) < 0:
                            return
                    
                    del self.copyCatsList
                    del self.copyCatsIds
                except AttributeError:
                    pass
                
                self.__updateATM()
                
            elif digitToolbar.GetAction() == "editLine" and \
                    hasattr(self, "vdigitMove"):
                line = digitClass.GetDisplay().GetSelected()
                if digitClass.EditLine(line, self.polycoords) < 0:
                    return
                
                del self.vdigitMove
                
            elif digitToolbar.GetAction() == "flipLine":
                if digitClass.FlipLine() < 0:
                    return
            elif digitToolbar.GetAction() == "mergeLine":
                if digitClass.MergeLine() < 0:
                    return
            elif digitToolbar.GetAction() == "breakLine":
                if digitClass.BreakLine() < 0:
                    return
            elif digitToolbar.GetAction() == "snapLine":
                if digitClass.SnapLine() < 0:
                    return
            elif digitToolbar.GetAction() == "connectLine":
                if len(digitClass.GetDisplay().GetSelected()) > 1:
                    if digitClass.ConnectLine() < 0:
                        return
            elif digitToolbar.GetAction() == "copyLine":
                if digitClass.CopyLine(self.copyIds) < 0:
                    return
                del self.copyIds
                if self.layerTmp:
                    self.Map.DeleteLayer(self.layerTmp)
                    self.UpdateMap(render = True, renderVector = False)
                del self.layerTmp

            elif digitToolbar.GetAction() == "zbulkLine" and len(self.polycoords) == 2:
                pos1 = self.polycoords[0]
                pos2 = self.polycoords[1]

                selected = digitClass.GetDisplay().GetSelected()
                dlg = VDigitZBulkDialog(parent = self, title = _("Z bulk-labeling dialog"),
                                        nselected = len(selected))
                if dlg.ShowModal() == wx.ID_OK:
                    if digitClass.ZBulkLines(pos1, pos2, dlg.value.GetValue(),
                                             dlg.step.GetValue()) < 0:
                        return
                self.UpdateMap(render = False, renderVector = True)
            elif digitToolbar.GetAction() == "typeConv":
                # -> feature type conversion
                # - point <-> centroid
                # - line <-> boundary
                if digitClass.TypeConvForSelectedLines() < 0:
                    return

            if digitToolbar.GetAction() != "addLine":
                # unselect and re-render
                digitClass.GetDisplay().SetSelected([])
                self.polycoords = []
                self.UpdateMap(render = False)

            self.redrawAll = True
            self.Refresh()
            
        event.Skip()

    def OnMiddleDown(self, event):
        """!Middle mouse button pressed
        """
        if event:
            self.mouse['begin'] = event.GetPositionTuple()[:]
        
        digitToolbar = self.parent.toolbars['vdigit']
        # digitization tool
        if self.mouse["use"] == "pointer" and digitToolbar:
            digitClass = self.parent.digit
            if (digitToolbar.GetAction() == "addLine" and \
                    digitToolbar.GetAction('type') in ["line", "boundary"]) or \
                    digitToolbar.GetAction() == "editLine":
                # add line or boundary -> remove last point from the line
                try:
                    removed = self.polycoords.pop()
                    Debug.msg(4, "BufferedWindow.OnMiddleDown(): polycoords_poped=%s" % \
                                  [removed,])

                    self.mouse['begin'] = self.Cell2Pixel(self.polycoords[-1])
                except:
                    pass

                if digitToolbar.GetAction() == "editLine":
                    # remove last vertex & line
                    if len(self.vdigitMove['id']) > 1:
                        self.vdigitMove['id'].pop()

                self.UpdateMap(render = False, renderVector = False)

            elif digitToolbar.GetAction() in ["deleteLine", "moveLine", "splitLine",
                                              "addVertex", "removeVertex", "moveVertex",
                                              "copyCats", "flipLine", "mergeLine",
                                              "snapLine", "connectLine", "copyLine",
                                              "queryLine", "breakLine", "typeConv"]:
                # varios tools -> unselected selected features
                digitClass.GetDisplay().SetSelected([])
                if digitToolbar.GetAction() in ["moveLine", "moveVertex", "editLine"] and \
                        hasattr(self, "vdigitMove"):

                    del self.vdigitMove
                    
                elif digitToolbar.GetAction() == "copyCats":
                    try:
                        del self.copyCatsList
                        del self.copyCatsIds
                    except AttributeError:
                        pass
                
                elif digitToolbar.GetAction() == "copyLine":
                    del self.copyIds
                    if self.layerTmp:
                        self.Map.DeleteLayer(self.layerTmp)
                        self.UpdateMap(render = True, renderVector = False)
                    del self.layerTmp

                self.polycoords = []
                self.UpdateMap(render = False) # render vector

            elif digitToolbar.GetAction() == "zbulkLine":
                # reset polyline
                self.polycoords = []
                digitClass.GetDisplay().SetSelected([])
                self.UpdateMap(render = False)
            
            self.redrawAll = True

    def OnMiddleUp(self, event):
        """!
        Middle mouse button released
        """
        self.mouse['end'] = event.GetPositionTuple()[:]
        
        # set region in zoom or pan
        begin = self.mouse['begin']
        end = self.mouse['end']
        
        self.Zoom(begin, end, 0) # no zoom
        
        # redraw map
        self.UpdateMap(render = True)
        
        # update statusbar
        self.parent.StatusbarUpdate()
        
    def OnMouseEnter(self, event):
        """!
        Mouse entered window and no mouse buttons were pressed
        """
        if self.parent.GetLayerManager().gcpmanagement:
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
        """!
        Motion event and no mouse buttons were pressed
        """
        digitToolbar = self.parent.toolbars['vdigit']
        if self.mouse["use"] == "pointer" and digitToolbar:
            digitClass = self.parent.digit
            self.mouse['end'] = event.GetPositionTuple()[:]
            Debug.msg (5, "BufferedWindow.OnMouseMoving(): coords=%f,%f" % \
                           (self.mouse['end'][0], self.mouse['end'][1]))
            if digitToolbar.GetAction() == "addLine" and digitToolbar.GetAction('type') in ["line", "boundary"]:
                if len(self.polycoords) > 0:
                    self.MouseDraw(pdc = self.pdcTmp, begin = self.Cell2Pixel(self.polycoords[-1]))
            elif digitToolbar.GetAction() in ["moveLine", "moveVertex", "editLine"] \
                    and hasattr(self, "vdigitMove"):
                dx = self.mouse['end'][0] - self.mouse['begin'][0]
                dy = self.mouse['end'][1] - self.mouse['begin'][1]
                
                if len(self.vdigitMove['id']) > 0:
                    # draw lines on new position
                    if digitToolbar.GetAction() == "moveLine":
                        # move line
                        for id in self.vdigitMove['id']:
                            self.pdcTmp.TranslateId(id, dx, dy)
                    elif digitToolbar.GetAction() in ["moveVertex", "editLine"]:
                        # move vertex ->
                        # (vertex, left vertex, left line,
                        # right vertex, right line)

                        # do not draw static lines
                        if digitToolbar.GetAction() == "moveVertex":
                            self.polycoords = []
                            ### self.pdcTmp.TranslateId(self.vdigitMove['id'][0], dx, dy)
                            self.pdcTmp.RemoveId(self.vdigitMove['id'][0])
                            if self.vdigitMove['id'][1] > 0: # previous vertex
                                x, y = self.Pixel2Cell(self.pdcTmp.GetIdBounds(self.vdigitMove['id'][1])[0:2])
                                self.pdcTmp.RemoveId(self.vdigitMove['id'][1]+1)
                                self.polycoords.append((x, y))
                            ### x, y = self.Pixel2Cell(self.pdcTmp.GetIdBounds(self.vdigitMove['id'][0])[0:2])
                            self.polycoords.append(self.Pixel2Cell(self.mouse['end']))
                            if self.vdigitMove['id'][2] > 0: # next vertex
                                x, y = self.Pixel2Cell(self.pdcTmp.GetIdBounds(self.vdigitMove['id'][2])[0:2])
                                self.pdcTmp.RemoveId(self.vdigitMove['id'][2]-1)
                                self.polycoords.append((x, y))
                            
                            self.ClearLines(pdc = self.pdcTmp)
                            self.DrawLines(pdc = self.pdcTmp)

                        else: # edit line
                            try:
                                if self.vdigitMove['id'][-1] > 0: # previous vertex
                                    self.MouseDraw(pdc = self.pdcTmp,
                                                   begin = self.Cell2Pixel(self.polycoords[-1]))
                            except: # no line
                                self.vdigitMove['id'] = []
                                self.polycoords = []

                self.Refresh() # TODO: use RefreshRect()
                self.mouse['begin'] = self.mouse['end']

            elif digitToolbar.GetAction() == "zbulkLine":
                if len(self.polycoords) == 1:
                    # draw mouse moving
                    self.MouseDraw(self.pdcTmp)

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

        ### self.Refresh()

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

    def Zoom(self, begin, end, zoomtype):
        """!
        Calculates new region while (un)zoom/pan-ing
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
                if newreg['n'] > 90.0:
                    newreg['n'] = 90.0
                if newreg['s'] < -90.0:
                    newreg['s'] = -90.0
            
            ce = newreg['w'] + (newreg['e'] - newreg['w']) / 2
            cn = newreg['s'] + (newreg['n'] - newreg['s']) / 2
            
            if hasattr(self, "vdigitMove"):
                # xo = self.Cell2Pixel((self.Map.region['center_easting'], self.Map.region['center_northing']))
                # xn = self.Cell2Pixel(ce, cn))
                tmp = self.Pixel2Cell(self.mouse['end'])
            
            # calculate new center point and display resolution
            self.Map.region['center_easting'] = ce
            self.Map.region['center_northing'] = cn
            self.Map.region["ewres"] = (newreg['e'] - newreg['w']) / self.Map.width
            self.Map.region["nsres"] = (newreg['n'] - newreg['s']) / self.Map.height
            self.Map.AlignExtentFromDisplay()

            if hasattr(self, "vdigitMove"):
                tmp1 = self.mouse['end']
                tmp2 = self.Cell2Pixel(self.vdigitMove['begin'])
                dx = tmp1[0] - tmp2[0]
                dy = tmp1[1] - tmp2[1]
                self.vdigitMove['beginDiff'] = (dx, dy)
                for id in self.vdigitMove['id']:
                    self.pdcTmp.RemoveId(id)
            
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
        """!
        Manages a list of last 10 zoom extents

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
                digitToolbar = self.parent.toolbars['vdigit']
                if digitToolbar and digitToolbar.GetLayer() == l:
                    w, s, b, e, n, t = self.parent.digit.GetDisplay().GetMapBoundingBox()
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
        ### self.Map.SetRegion(windres=True)

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

    def Distance(self, beginpt, endpt, screen=True):
        """!Calculete distance

        LL-locations not supported

        @todo Use m.distance

        @param beginpt first point
        @param endpt second point
        @param screen True for screen coordinates otherwise EN
        """
        x1, y1 = beginpt
        x2, y2 = endpt
        if screen:
            dEast  = (x2 - x1) * self.Map.region["ewres"]
            dNorth = (y2 - y1) * self.Map.region["nsres"]
        else:
            dEast  = (x2 - x1)
            dNorth = (y2 - y1)
        
        return (math.sqrt(math.pow((dEast),2) + math.pow((dNorth),2)), (dEast, dNorth))
