"""
@package mapdisp.py

@brief GIS map display canvas, with toolbar for various display
management functions, and second toolbar for vector
digitizing. 

Can be used either from GIS Manager or as p.mon backend.

Classes:
 - Command
 - MapWindow
 - BufferedWindow
 - MapFrame
 - MapApp

Usage:
 python mapdisp.py monitor-identifier /path/to/command/file

(C) 2006-2008 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import time
import glob
import math
import tempfile
import copy

import wx
import wx.aui

from threading import Thread

import globalvar
try:
    import subprocess
except:
    CompatPath = os.path.join(globalvar.ETCWXDIR)
    sys.path.append(CompatPath)
    from compat import subprocess

gmpath = os.path.join(globalvar.ETCWXDIR, "icons")
sys.path.append(gmpath)

import render
import toolbars
import grassenv
import track
import menuform
import gselect
import disp_print
import gcmd
import dbm
import histogram
import profile
import globalvar
import utils
import gdialogs
from vdigit import VDigitCategoryDialog as VDigitCategoryDialog
from vdigit import VDigitZBulkDialog    as VDigitZBulkDialog
from vdigit import VDigitDuplicatesDialog as VDigitDuplicatesDialog
from vdigit import GV_LINES            as VDigit_Lines_Type
from debug import Debug               as Debug
from icon  import Icons               as Icons
from preferences import globalSettings as UserSettings

import images
imagepath = images.__path__[0]
sys.path.append(imagepath)

###
### global variables
###
# for standalone app
cmdfilename = None

class Command(Thread):
    """
    Creates thread which will observe the command file and see, if
    there is new command to be executed
    """
    def __init__ (self, parent, Map):
        Thread.__init__(self)

        global cmdfilename

        self.parent = parent
        self.map = Map
        self.cmdfile = open(cmdfilename, "r")

    def run(self):
        """
        Run this in thread
        """
        dispcmd = []
        while 1:
            self.parent.redraw = False
            line = self.cmdfile.readline().strip()
            if line == "quit":
                break

            if line:
                try:
                    Debug.msg (3, "Command.run(): cmd=%s" % (line))

                    self.map.AddLayer(item=None, type="raster",
                                      name='',
                                      command=line,
                                      l_opacity=1)

                    self.parent.redraw =True

                except Exception, e:
                    print "Command Thread: ",e

            time.sleep(0.1)

        sys.exit()

class MapWindow(object):
    """Abstract map window class

    Parent for BufferedWindow class (2D display mode) and
    GLWindow (3D display mode)
    """
    def __init__(self, parent, id=wx.ID_ANY,
                 pos=wx.DefaultPosition,
                 size=wx.DefaultSize,
                 style=wx.NO_FULL_REPAINT_ON_RESIZE,
                 Map=None, tree=None, gismgr=None):
        self.parent = parent # MapFrame
        
    def EraseMap(self):
        """
        Erase the canvas (virtual method)
        """
        pass

    def UpdateMap(self):
        """
        Updates the canvas anytime there is a change to the
        underlaying images or to the geometry of the canvas.
        """
        pass

    def OnLeftDown(self, event):
        pass

    def OnLeftUp(self, event):
        pass

    def OnMouseMotion(self, event):
        pass

    def OnZoomToMap(self, event):
        pass

    def OnZoomToRaster(self, event):
        pass

    def GetSelectedLayer(self, type='layer'):
        """Get selected layer from layer tree

        @param type 'item' / 'layer' / 'nviz'

        @return layer / map layer properties / nviz properties
        @return None on failure
        """
        # get currently selected map layer
        if not self.tree or not self.tree.GetSelection():
            return None
        
        item = self.tree.GetSelection()
        if not item.IsChecked():
            return None

        if type == 'item':
            return item
        
        try:
            if type == 'nviz':
                layer = self.tree.GetPyData(item)[0]['nviz']
            else:
                layer = self.tree.GetPyData(item)[0]['maplayer']
        except:
            layer = None
            
        return layer

class BufferedWindow(MapWindow, wx.Window):
    """
    A Buffered window class.

    When the drawing needs to change, you app needs to call the
    UpdateMap() method. Since the drawing is stored in a bitmap, you
    can also save the drawing to file by calling the
    SaveToFile(self,file_name,file_type) method.
    """

    def __init__(self, parent, id,
                 pos = wx.DefaultPosition,
                 size = wx.DefaultSize,
                 style=wx.NO_FULL_REPAINT_ON_RESIZE,
                 Map=None, tree=None, gismgr=None):

        MapWindow.__init__(self, parent, id, pos, size, style,
                           Map, tree, gismgr)
        wx.Window.__init__(self, parent, id, pos, size, style)

        self.Map = Map
        self.tree = tree
        self.gismanager = gismgr

        #
        # Flags
        #
        self.resize = False # indicates whether or not a resize event has taken place
        self.dragimg = None # initialize variable for map panning

        #
        # Variable for drawing on DC
        #
        self.pen = None      # pen for drawing zoom boxes, etc.
        self.polypen = None  # pen for drawing polylines (measurements, profiles, etc)
        # List of wx.Point tuples defining a polyline (geographical coordinates)
        self.polycoords = []
        # ID of rubber band line
        self.lineid = None
        # ID of poly line resulting from cumulative rubber band lines (e.g. measurement)
        self.plineid = None
        
        #
        # Event bindings
        #
        self.Bind(wx.EVT_PAINT,        self.OnPaint)
        self.Bind(wx.EVT_SIZE,         self.OnSize)
        self.Bind(wx.EVT_IDLE,         self.OnIdle)
        self.Bind(wx.EVT_MOTION,       self.MouseActions)
        self.Bind(wx.EVT_MOUSE_EVENTS, self.MouseActions)
        self.processMouse = True
        
        #
        # Render output objects
        #
        self.mapfile = None   # image file to be rendered
        self.img = ""         # wx.Image object (self.mapfile)
        # used in digitization tool (do not redraw vector map)
        self.imgVectorMap = None
        # decoration overlays
        self.overlays = {}
        # images and their PseudoDC ID's for painting and dragging
        self.imagedict = {}   
        self.select = {}      # selecting/unselecting decorations for dragging
        self.textdict = {}    # text, font, and color indexed by id
        self.currtxtid = None # PseudoDC id for currently selected text

        #
        # Zoom objects
        #
        self.zoomhistory = [] # list of past zoom extents
        self.currzoom = 0 # current set of extents in zoom history being used

        #
        # mouse attributes like currently pressed buttons, position on
        # the screen, begin and end of dragging, and type of drawing
        #
        self.mouse = {
            'l'    : False,
            'r'    : False,
            'm'    : False,
            'begin': [0, 0], # screen coordinates
            'end'  : [0, 0],
            'use'  : "pointer",
            'box'  : "point"
            }

        self.zoomtype = 1   # 1 zoom in, 0 no zoom, -1 zoom out
        self.hitradius = 10 # distance for selecting map decorations
        self.dialogOffset = 5 # offset for dialog (e.g. DisplayAttributesDialog)

        # OnSize called to make sure the buffer is initialized.
        # This might result in OnSize getting called twice on some
        # platforms at initialization, but little harm done.
        ### self.OnSize(None)

        # create PseudoDC used for background map, map decorations like scales and legends
        self.pdc = wx.PseudoDC()
        # used for digitization tool
        self.pdcVector = None
        # decorations (region box, etc.)
        self.pdcDec = wx.PseudoDC()
        # pseudoDC for temporal objects (select box, measurement tool, etc.)
        self.pdcTmp = wx.PseudoDC()
        # redraw all pdc's, pdcTmp layer is redrawn always (speed issue)
        self.redrawAll = True

        # will store an off screen empty bitmap for saving to file
        self._buffer = ''

        self.Bind(wx.EVT_ERASE_BACKGROUND, lambda x:None)

        # vars for handling mouse clicks
        self.dragid   = -1
        self.lastpos  = (0, 0)

    def Draw(self, pdc, img=None, drawid=None, pdctype='image', coords=[0, 0, 0, 0]):
        """
        Draws map and overlay decorations
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

        Debug.msg (5, "BufferedWindow.Draw(): id=%s, pdctype=%s, coord=%s" % \
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
            pdc.SetIdBounds(drawid, (coords[0],coords[1], w, h))

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
                rect = wx.Rect(x1,y1,rwidth,rheight)
                pdc.DrawRectangleRect(rect)
                pdc.SetIdBounds(drawid,rect)
                # self.ovlcoords[drawid] = coords

        elif pdctype == 'line': # draw a line on top of the map
            if self.pen:
                pdc.SetBrush(wx.Brush(wx.CYAN, wx.TRANSPARENT))
                pdc.SetPen(self.pen)
                pdc.DrawLine(coords[0], coords[1], coords[2], coords[3])
                pdc.SetIdBounds(drawid,(coords[0], coords[1], coords[2], coords[3]))
                # self.ovlcoords[drawid] = coords

        elif pdctype == 'polyline': # draw a polyline on top of the map
            if self.polypen:
                pdc.SetBrush(wx.Brush(wx.CYAN, wx.TRANSPARENT))
                pdc.SetPen(self.polypen)
                pdc.DrawLines(coords)

                # get bounding rectangle for polyline
                xlist = []
                ylist = []
                if len(coords) > 0:
                    for point in coords:
                        x,y = point
                        xlist.append(x)
                        ylist.append(y)
                    x1=min(xlist)
                    x2=max(xlist)
                    y1=min(ylist)
                    y2=max(ylist)
                    pdc.SetIdBounds(drawid,(x1,y1,x2,y2))
                    # self.ovlcoords[drawid] = [x1,y1,x2,y2]

        elif pdctype == 'point': # draw point
            if self.pen:
                pdc.SetPen(self.pen)
                pdc.DrawPoint(coords[0], coords[1])
                coordsBound = (coords[0] - 5,
                               coords[1] - 5,
                               coords[0] + 5,
                               coords[1] + 5)
                pdc.SetIdBounds(drawid, coordsBound)
                # self.ovlcoords[drawid] = coords

        elif pdctype == 'text': # draw text on top of map
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
            pdc.SetIdBounds(drawid, (coords[0], coords[1], w, h))
            
        pdc.EndDrawing()
        
        self.Refresh()
        
        return drawid

    def TextBounds(self, textinfo):
        """
        Return text boundary data

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

    def OnPaint(self, event):
        """
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

            pdcLast = wx.PseudoDC()
            pdcLast.DrawBitmap(bmp=self.bufferLast, x=0, y=0)
            pdcLast.DrawToDC(dc)

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
        """
        Scale map image so that it is
        the same size as the Window
        """
        Debug.msg(3, "BufferedWindow.OnSize():")

        # set size of the input image
        self.Map.ChangeMapSize(self.GetClientSize())
        # align extent based on center point and display resolution
        # this causes that image is not resized when display windows is resized
        # self.Map.AlignExtentFromDisplay()

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
        """
        Only re-render a composite map image from GRASS during
        idle time instead of multiple times during resizing.
        """
        if self.resize:
            self.UpdateMap(render=True)

        event.Skip()

    def SaveToFile(self, FileName, FileType):
        """
        This draws the psuedo DC to a buffer that
        can be saved to a file.
        """
        dc = wx.BufferedPaintDC(self, self.buffer)
        self.pdc.DrawToDC(dc)
        if self.pdcVector:
            self.pdcVector.DrawToDC(dc)
        self.buffer.SaveFile(FileName, FileType)

    def GetOverlay(self):
        """
        Converts rendered overlay files to wx.Image

        Updates self.imagedict

        @return list of images
        """
        imgs = []
        for overlay in self.Map.GetListOfLayers(l_type="overlay", l_active=True):
            if os.path.isfile(overlay.mapfile) and os.path.getsize(overlay.mapfile):
                img = wx.Image(overlay.mapfile, wx.BITMAP_TYPE_ANY)
                self.imagedict[img] = { 'id' : overlay.id,
                                        'layer' : overlay }
                imgs.append(img)

        return imgs

    def GetImage(self):
        """
        Converts redered map files to wx.Image

        Updates self.imagedict (id=99)

        @return wx.Image instance (map composition)
        """
        imgId = 99
        if self.Map.mapfile and os.path.isfile(self.Map.mapfile) and \
                os.path.getsize(self.Map.mapfile):
            img = wx.Image(self.Map.mapfile, wx.BITMAP_TYPE_ANY)
        else:
            img = None

        self.imagedict[img] = { 'id': imgId }

        return img

    def UpdateMap(self, render=True, renderVector=True):
        """
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
            self.parent.onRenderGauge.Show()
            if self.parent.onRenderGauge.GetRange() > 0:
                self.parent.onRenderGauge.SetValue(1)
        
        #
        # render background image if needed
        #
        
        # update layer dictionary if there has been a change in layers
        if self.tree and self.tree.reorder == True:
            self.tree.ReorderLayers()
            
        # reset flag for auto-rendering
        if self.tree:
            self.tree.rerender = False
        
        if render:
            # update display size
            self.Map.ChangeMapSize(self.GetClientSize())
            if self.parent.compResolution.IsChecked():
                # use computation region resolution for rendering
                windres = True
            else:
                windres = False
            self.mapfile = self.Map.Render(force=True, mapWindow=self.parent,
                                           windres=windres)
        else:
            self.mapfile = self.Map.Render(force=False, mapWindow=self.parent)
        
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
            self.Draw(self.pdc, pdctype='clear')
        else:
            try:
                id = self.imagedict[self.img]['id']
            except:
                return False

            self.Draw(self.pdc, self.img, drawid=id)

        #
        # render vector map layer
        #
        digitToolbar = self.parent.toolbars['vdigit']
        if renderVector and digitToolbar and \
                digitToolbar.GetLayer():
            # set region
            self.parent.digit.driver.UpdateRegion()
            # re-calculate threshold for digitization tool
            self.parent.digit.driver.GetThreshold()
            # draw map
            self.pdcVector.Clear()
            self.pdcVector.RemoveAll()
            try:
                item = self.tree.FindItemByData('maplayer', digitToolbar.GetLayer())
            except TypeError:
                item = None

            if item and self.tree.IsItemChecked(item):
                self.parent.digit.driver.DrawMap()

            # translate tmp objects (pointer position)
            if digitToolbar.GetAction() == 'moveLine':
                if  hasattr(self, "vdigitMove") and \
                        self.vdigitMove.has_key('beginDiff'):
                    # move line
                    for id in self.vdigitMove['id']:
                        # print self.pdcTmp.GetIdBounds(id)
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
                self.Draw(self.pdc, img=img, drawid=id,
                          pdctype=self.overlays[id]['pdcType'], coords=self.overlays[id]['coords'])

        for id in self.textdict.keys():
            self.Draw(self.pdc, img=self.textdict[id], drawid=id,
                      pdctype='text', coords=[10, 10, 10, 10])

        # optionally draw computational extent box
        self.DrawCompRegionExtent()

        #
        # redraw pdcTmp if needed
        #
        if len(self.polycoords) > 0:
            self.DrawLines(self.pdcTmp)
        
        if self.parent.gismanager.georectifying:
            # -> georectifier (redraw GCPs)
            if self.parent.toolbars['georect']:
                coordtype = 'gcpcoord'
            else:
                coordtype = 'mapcoord'
            self.parent.gismanager.georectifying.DrawGCP(coordtype)
            
        stop = time.clock()

        #
        # hide process bar
        #
        self.parent.onRenderGauge.Hide()

        #
        # update statusbar
        #
        ### self.Map.SetRegion()
        self.parent.StatusbarUpdate()

        Debug.msg (2, "BufferedWindow.UpdateMap(): render=%s, renderVector=%s -> time=%g" % \
                   (render, renderVector, (stop-start)))
        
        return True

    def DrawCompRegionExtent(self):
        """Draw computational region extent in the display
        
        Display region is drawn as a blue box inside the computational region,
        computational region inside a display region as a red box).
        """
        if hasattr(self, "regionCoords"):
            compReg = self.Map.GetRegion()
            dispReg = self.Map.GetCurrentRegion()
            reg = None
            if self.IsInRegion(dispReg, compReg):
                self.polypen = wx.Pen(colour=wx.Colour(0, 0, 255, 128), width=3, style=wx.SOLID)
                reg = dispReg
            else:
                self.polypen = wx.Pen(colour=wx.Colour(255, 0, 0, 128),
                                      width=3, style=wx.SOLID)
                reg = compReg
            
            self.regionCoords = []
            self.regionCoords.append((reg['w'], reg['n']))
            self.regionCoords.append((reg['e'], reg['n']))
            self.regionCoords.append((reg['e'], reg['s']))
            self.regionCoords.append((reg['w'], reg['s']))
            self.regionCoords.append((reg['w'], reg['n']))
            # draw region extent
            self.DrawLines(pdc=self.pdcDec, polycoords=self.regionCoords)

    def IsInRegion(self, region, refRegion):
        """Test if 'region' is inside of 'refRegion'

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
        """
        Erase the canvas
        """
        self.Draw(self.pdc, pdctype='clear')
                  
        if self.pdcVector:
            self.Draw(self.pdcVector, pdctype='clear')
        
        self.Draw(self.pdcDec, pdctype='clear')
        self.Draw(self.pdcTmp, pdctype='clear')
        
    def DragMap(self, moveto):
        """
        Drag the entire map image for panning.
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

        return True

    def DragItem(self, id, event):
        """
        Drag an overlay decoration item
        """
        Debug.msg (5, "BufferedWindow.DragItem(): id=%d" % \
                       id)
        x, y = self.lastpos
        dx = event.GetX() - x
        dy = event.GetY() - y
        self.pdc.SetBackground(wx.Brush(self.GetBackgroundColour()))
        r = self.pdc.GetIdBounds(id)
        if self.dragid > 100: # text dragging
            rtop = (r[0],r[1]-r[3],r[2],r[3])
            r = r.Union(rtop)
            rleft = (r[0]-r[2],r[1],r[2],r[3])
            r = r.Union(rleft)
        self.pdc.TranslateId(id, dx, dy)
        r2 = self.pdc.GetIdBounds(id)
        r = r.Union(r2)
        r.Inflate(4,4)

        self.Update()
        self.RefreshRect(r, False)
        self.lastpos = (event.GetX(), event.GetY())
        
        if id > 100: # text
            self.textdict[id]['coords'] = r2
        
    def MouseDraw(self, pdc=None, begin=None, end=None):
        """
        Mouse box or line from 'begin' to 'end'

        If not given from self.mouse['begin'] to self.mouse['end'].

        """
        self.redrawAll = False
        
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
            r.Inflate(4,4)
            pdc.ClearId(boxid)
            self.RefreshRect(r, False)
            pdc.SetId(boxid)
            self.Draw(pdc, drawid=boxid, pdctype='box', coords=mousecoords)
        elif self.mouse['box'] == "line":
            self.lineid = wx.ID_NEW
            mousecoords = [begin[0], begin[1], \
                           end[0], end[1]]
            x1=min(begin[0],end[0])
            x2=max(begin[0],end[0])
            y1=min(begin[1],end[1])
            y2=max(begin[1],end[1])
            r = wx.Rect(x1,y1,x2-x1,y2-y1)
            r.Inflate(4,4)
            try:
                pdc.ClearId(self.lineid)
            except:
                pass
            self.RefreshRect(r, False)
            pdc.SetId(self.lineid)

            self.Draw(pdc, drawid=self.lineid, pdctype='line', coords=mousecoords)

    def DrawLines(self, pdc=None, polycoords=None):
        """Draw polyline in PseudoDC

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

            self.Draw(pdc, drawid=self.plineid, pdctype='polyline', coords=coords)

            Debug.msg (4, "BufferedWindow.DrawLines(): coords=%s, id=%s" % \
                           (coords, self.plineid))

            return self.plineid

        return -1

    def DrawCross(self, pdc, coords, size, rotation=0,
                  text=None, textAlign='lr', textOffset=(5, 5)):
        """Draw cross in PseudoDC

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
            self.Draw(pdc, drawid=self.lineid, pdctype='line', coords=lineCoords)

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
        
        self.Draw(pdc, img=text,
                  pdctype='text', coords=coord)

        return self.lineid

    def MouseActions(self, event):
        """
        Mouse motion and button click notifier
        """
        if not self.processMouse:
            return
        
        ### if self.redrawAll is False:
        ###    self.redrawAll = True
        
        wheel = event.GetWheelRotation()
        # zoom with mouse wheel
        if wheel != 0:
            self.processMouse = False
            current  = event.GetPositionTuple()[:]
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
            
        # left mouse button pressed
        elif event.LeftDown():
            self.OnLeftDown(event)

        # left mouse button released
        elif event.LeftUp():
            self.OnLeftUp(event)

        # dragging
        elif event.Dragging():
            Debug.msg (5, "BufferedWindow.MouseAction(): Dragging")
            current  = event.GetPositionTuple()[:]
            previous = self.mouse['begin']
            move = (current[0] - previous[0],
                    current[1] - previous[1])

            # dragging or drawing box with left button
            if self.mouse['use'] == 'pan':
                self.DragMap(move)

            # dragging decoration overlay item
            elif (self.mouse['use'] == 'pointer' and not self.parent.toolbars['vdigit']) and \
                    self.dragid != None and \
                    self.dragid != 99:
                self.DragItem(self.dragid, event)

            # dragging anything else - rubber band box or line
            else:
                self.mouse['end'] = event.GetPositionTuple()[:]
                if event.LeftIsDown():
                    # draw box only when left mouse button is pressed
                    self.MouseDraw(pdc=self.pdcTmp)

        # double click
        elif event.ButtonDClick():
            self.OnButtonDClick(event)

        # middle mouse button pressed
        elif event.MiddleDown():
            self.OnMiddleDown(event)

        # right mouse button pressed
        elif event.RightDown():
            self.OnRightDown(event)

        # right mouse button released
        elif event.RightUp():
            self.OnRightUp(event)

        elif event.Moving():
            self.OnMouseMoving(event)

        event.Skip()

    def OnLeftDown(self, event):
        """
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
        elif self.mouse["use"] == "pointer" and self.parent.toolbars['vdigit']:
            # digitization
            digitToolbar = self.parent.toolbars['vdigit']
            digitClass   = self.parent.digit
            east, north = self.Pixel2Cell(self.mouse['begin'])

            try:
                map = digitToolbar.GetLayer().GetName()
            except:
                map = None
                wx.MessageBox(parent=self,
                              message=_("No vector map selected for editing."),
                              caption=_("Error"), style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
                event.Skip()
                return

            # calculate position of 'update record' dialog
            position = self.mouse['begin']
            posWindow = self.ClientToScreen((position[0] + self.dialogOffset,
                                             position[1] + self.dialogOffset))

            if digitToolbar.GetAction() not in ("moveVertex", "addVertex",
                                                "removeVertex", "editLine"):
                # set pen
                self.pen = wx.Pen(colour='Red', width=2, style=wx.SHORT_DASH)
                self.polypen = wx.Pen(colour='dark green', width=2, style=wx.SOLID)

            if digitToolbar.GetAction() in ("addVertex", "removeVertex"):
                # unselect
                digitClass.driver.SetSelected([])

            if digitToolbar.GetAction() == "addLine":
                if digitToolbar.GetAction('type') in ["point", "centroid"]:
                    # add new point
                    if digitToolbar.GetAction('type') == 'point':
                        point = True
                    else:
                        point = False

                    fid = digitClass.AddPoint(map, point, east, north)
                    if fid < 0:
                        return

                    self.UpdateMap(render=False) # redraw map

                    # add new record into atribute table
                    if UserSettings.Get(group='vdigit', key="addRecord", subkey='enabled')  is True:
                        # select attributes based on layer and category
                        cats = { fid : {
                                UserSettings.Get(group='vdigit', key="layer", subkey='value') :
                                    (UserSettings.Get(group='vdigit', key="category", subkey='value'), )
                                }}
                        addRecordDlg = dbm.DisplayAttributesDialog(parent=self, map=map,
                                                                   cats=cats,
                                                                   pos=posWindow,
                                                                   action="add")
                        if addRecordDlg.mapDBInfo and \
                               addRecordDlg.ShowModal() == wx.ID_OK:
                            sqlfile = tempfile.NamedTemporaryFile(mode="w")
                            for sql in addRecordDlg.GetSQLString():
                                sqlfile.file.write(sql + ";\n")
                            sqlfile.file.flush()
                            executeCommand = gcmd.Command(cmd=["db.execute",
                                                               "--q",
                                                               "input=%s" % sqlfile.name])

                elif digitToolbar.GetAction('type') in ["line", "boundary"]:
                    # add new point to the line
                    self.polycoords.append(self.Pixel2Cell(event.GetPositionTuple()[:]))
                    self.DrawLines(pdc=self.pdcTmp)
            
            elif digitToolbar.GetAction() == "editLine" and hasattr(self, "vdigitMove"):
                self.polycoords.append(self.Pixel2Cell(self.mouse['begin']))
                self.vdigitMove['id'].append(wx.NewId())
                self.DrawLines(pdc=self.pdcTmp)

            elif digitToolbar.GetAction() == "deleteLine":
                pass

            elif digitToolbar.GetAction() in ["moveLine", "moveVertex", "editLine"] and \
                    not hasattr(self, "vdigitMove"):
                self.vdigitMove = {}
                # geographic coordinates of initial position (left-down)
                self.vdigitMove['begin'] = None
                # list of ids to modify    
                self.vdigitMove['id'] = []
                # ids geographic coordinates
                self.vdigitMove['coord'] = {}
                
                if digitToolbar.GetAction() in ["moveVertex", "editLine"]:
                    # set pen
                    pcolor = UserSettings.Get(group='vdigit', key="symbol",
                                              subkey=["highlight", "color"])
                    self.pen = self.polypen = wx.Pen(colour=pcolor,
                                                     width=2, style=wx.SHORT_DASH)
                    self.pdcTmp.SetPen(self.polypen)

            elif digitToolbar.GetAction() == "splitLine":
                # unselect
                digitClass.driver.SetSelected([])

            elif digitToolbar.GetAction() in ["displayAttrs", "displayCats"]:
                qdist = digitClass.driver.GetThreshold(type='selectThresh')
                coords = (east, north)
                if digitClass.type == 'vdigit':
                    # unselect
                    digitClass.driver.SetSelected([])

                    # select feature by point
                    cats = {}
                    if digitClass.driver.SelectLineByPoint(coords,
                                                        digitClass.GetSelectType()) is not None:
                        if UserSettings.Get(group='vdigit', key='checkForDupl',
                                            subkey='enabled'):
                            lines = digitClass.driver.GetSelected()
                        else:
                            lines = (digitClass.driver.GetSelected()[0],) # only first found
                        
                        for line in lines:
                            cats[line] = digitClass.GetLineCats(line)
                    
                if digitToolbar.GetAction() == "displayAttrs":
                    # select attributes based on coordinates (all layers)
                    if self.parent.dialogs['attributes'] is None:
                        if digitClass.type == 'vedit':
                            self.parent.dialogs['attributes'] = dbm.DisplayAttributesDialog(parent=self, map=map,
                                                                                            query=(coords, qdist),
                                                                                            pos=posWindow,
                                                                                            action="update")
                        else:
                            self.parent.dialogs['attributes'] = dbm.DisplayAttributesDialog(parent=self, map=map,
                                                                                            cats=cats,
                                                                                            action="update")
                    else:
                        # update currently open dialog
                        if digitClass.type == 'vedit':
                            self.parent.dialogs['attributes'].UpdateDialog(query=(coords, qdist))
                        else:
                            # upgrade dialog
                            self.parent.dialogs['attributes'].UpdateDialog(cats=cats)

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
                        if digitClass.type == 'vedit':
                            dlg = VDigitCategoryDialog(parent=self,
                                                        map=map,
                                                        query=(coords, qdist),
                                                        pos=posWindow,
                                                        title=_("Update categories"))
                            self.parent.dialogs['category'] = dlg
                        else:
                            dlg = VDigitCategoryDialog(parent=self,
                                                       map=map,
                                                       cats=cats,
                                                       pos=posWindow,
                                                       title=_("Update categories"))
                            self.parent.dialogs['category'] = dlg
                    else:
                        # update currently open dialog
                        if digitClass.type == 'vedit':
                            self.parent.dialogs['category'].UpdateDialog(query=(coords, qdist))
                        else:
                            # upgrade dialog
                            self.parent.dialogs['category'].UpdateDialog(cats=cats)
                            
                    if self.parent.dialogs['category']:
                        if len(cats.keys()) > 0:
                            # highlight feature & re-draw map
                            ### digitClass.driver.SetSelected(line)
                            if not self.parent.dialogs['category'].IsShown():
                                self.parent.dialogs['category'].Show()
                        else:
                            if self.parent.dialogs['category'].IsShown():
                                self.parent.dialogs['category'].Hide()
                
                self.UpdateMap(render=False)

            elif digitToolbar.GetAction() in ("copyCats", "copyAttrs"):
                if not hasattr(self, "copyCatsList"):
                    self.copyCatsList = []
                else:
                    self.copyCatsIds = []
                    self.mouse['box'] = 'box'

            elif digitToolbar.GetAction() == "copyLine":
                self.copyIds = []
                self.layerTmp = None

            elif digitToolbar.GetAction() == "zbulkLine":
                if len(self.polycoords) > 1: # start new line
                    self.polycoords = []
                    self.ClearLines(pdc=self.pdcTmp)
                self.polycoords.append(self.Pixel2Cell(event.GetPositionTuple()[:]))
                if len(self.polycoords) == 1:
                    begin = self.Pixel2Cell(self.polycoords[-1])
                    end   = self.Pixel2Cell(self.mouse['end'])
                else:
                    end   = self.Pixel2Cell(self.polycoords[-1])
                    begin = self.Pixel2Cell(self.mouse['begin'])

                    self.DrawLines(self.pdcTmp, begin, end)
        else:
            # get decoration id
            self.lastpos = self.mouse['begin']
            idlist = self.pdc.FindObjects(x=self.lastpos[0], y=self.lastpos[1],
                                          radius=self.hitradius)
            if idlist != []:
                self.dragid = idlist[0]

        event.Skip()

    def OnLeftUp(self, event):
        """
        Left mouse button released
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
            self.UpdateMap(render=True)

            # update statusbar
            self.parent.StatusbarUpdate()

        elif self.mouse["use"] == "query":
            # querying
            self.parent.QueryMap(self.mouse['begin'][0],self.mouse['begin'][1])

        elif self.mouse["use"] == "queryVector":
            # editable mode for vector map layers
            self.parent.QueryVector(self.mouse['begin'][0], self.mouse['begin'][1])

            # clear temp canvas
            self.UpdateMap(render=False, renderVector=False)
            
        elif self.mouse["use"] in ["measure", "profile"]:
            # measure or profile
            if self.mouse["use"] == "measure":
                self.parent.MeasureDist(self.mouse['begin'], self.mouse['end'])
            try:
                self.polycoords.append(self.Pixel2Cell(self.mouse['end']))
                self.pdcTmp.ClearId(self.lineid)
                self.DrawLines(pdc=self.pdcTmp)
            except:
                pass

        elif self.mouse["use"] == "pointer" and self.parent.gismanager.georectifying:
            # -> georectifying
            coord = self.Pixel2Cell(self.mouse['end'])
            if self.parent.toolbars['georect']:
                coordtype = 'gcpcoord'
            else:
                coordtype = 'mapcoord'

            self.parent.gismanager.georectifying.SetGCPData(coordtype, coord, self)
            self.UpdateMap(render=False, renderVector=False)

        elif self.mouse["use"] == "pointer" and self.parent.toolbars['vdigit']:
            # digitization tool active
            digitToolbar = self.parent.toolbars['vdigit']
            digitClass   = self.parent.digit

            pos1 = self.Pixel2Cell(self.mouse['begin'])
            pos2 = self.Pixel2Cell(self.mouse['end'])

            if hasattr(self, "vdigitMove"):
                if len(digitClass.driver.GetSelected()) == 0:
                    self.vdigitMove['begin'] = pos1 # left down
                else:
                    dx = pos2[0] - pos1[0] ### ???
                    dy = pos2[1] - pos1[1]
                    self.vdigitMove = (self.vdigitMove['begin'][0] + dx,
                                       self.vdigitMove['begin'][1] + dy)
                
                # eliminate initial mouse moving efect
                self.mouse['begin'] = self.mouse['end'] 

            if digitToolbar.GetAction() in ["deleteLine", "moveLine", "moveVertex",
                                            "copyCats", "copyAttrs", "editLine", "flipLine",
                                            "mergeLine", "snapLine",
                                            "queryLine", "breakLine", "typeConv", "connectLine"]:
                nselected = 0
                # -> delete line || move line || move vertex
                if digitToolbar.GetAction() in ["moveVertex", "editLine"]:
                    if len(digitClass.driver.GetSelected()) == 0:
                        nselected = digitClass.driver.SelectLineByPoint(pos1, type=VDigit_Lines_Type)
                        if digitToolbar.GetAction() == "editLine":
                            try:
                                selVertex = digitClass.driver.GetSelectedVertex(pos1)[0]
                            except IndexError:
                                selVertex = None

                            if selVertex:
                                # self.UpdateMap(render=False)
                                ids = digitClass.driver.GetSelected(grassId=False)
                                # move this line to tmp layer
                                self.polycoords = []
                                for id in ids:
                                    if id % 2: # register only vertices
                                        e, n = self.Pixel2Cell(self.pdcVector.GetIdBounds(id)[0:2])
                                        self.polycoords.append((e, n))
                                    # self.pdcVector.RemoveId(id)
                                digitClass.driver.DrawSelected(False) 
                                
                                if selVertex < ids[-1] / 2:
                                    # choose first or last node of line
                                    self.vdigitMove['id'].reverse()
                                    self.polycoords.reverse()
                            else:
                                # unselect
                                digitClass.driver.SetSelected([])
                                del self.vdigitMove
                                
                            self.UpdateMap(render=False)

                        
                elif digitToolbar.GetAction() in ("copyCats", "copyAttrs"):
                    if not hasattr(self, "copyCatsIds"):
                        # 'from' -> select by point
                        nselected = digitClass.driver.SelectLineByPoint(pos1, digitClass.GetSelectType())
                        if nselected:
                            self.copyCatsList = digitClass.driver.GetSelected()
                    else:
                        # -> 'to' -> select by bbox
                        digitClass.driver.SetSelected([])
                        # return number of selected features (by box/point)
                        nselected = digitClass.driver.SelectLinesByBox(pos1, pos2,
                                                                       digitClass.GetSelectType())
                        if nselected == 0:
                            if digitClass.driver.SelectLineByPoint(pos1,
                                                                   digitClass.GetSelectType()) is not None:
                                nselected = 1

                        if nselected > 0:
                            self.copyCatsIds = digitClass.driver.GetSelected()

                elif digitToolbar.GetAction() == "queryLine":
                    selected = digitClass.SelectLinesByQuery(pos1, pos2)
                    nselected = len(selected)
                    if nselected > 0:
                        digitClass.driver.SetSelected(selected)

                else:
                    # -> moveLine || deleteLine, etc. (select by point/box)
                    if digitToolbar.GetAction() == 'moveLine' and \
                           len(digitClass.driver.GetSelected()) > 0:
                        nselected = 0
                    else:
                        if digitToolbar.GetAction() == 'moveLine':
                            drawSeg = True
                        else:
                            drawSeg = False
                        nselected = digitClass.driver.SelectLinesByBox(pos1, pos2,
                                                                       digitClass.GetSelectType(),
                                                                       drawSeg)
                        
                        if nselected == 0:
                            if digitClass.driver.SelectLineByPoint(pos1,
                                                                   digitClass.GetSelectType()) is not None:
                                nselected = 1
                        
                if nselected > 0:
                    if digitToolbar.GetAction() in ["moveLine", "moveVertex"]:
                        # get pseudoDC id of objects which should be redrawn
                        if digitToolbar.GetAction() == "moveLine":
                            # -> move line
                            self.vdigitMove['id'] = digitClass.driver.GetSelected(grassId=False)
                            self.vdigitMove['coord'] = digitClass.driver.GetSelectedCoord()
                        elif digitToolbar.GetAction() == "moveVertex":
                            # -> move vertex
                            self.vdigitMove['id'] = digitClass.driver.GetSelectedVertex(pos1)
                            if len(self.vdigitMove['id']) == 0: # no vertex found
                                digitClass.driver.SetSelected([])

                                   
                    #
                    # check for duplicates
                    #
                    if UserSettings.Get(group='vdigit', key='checkForDupl', subkey='enabled') is True:
                        dupl = digitClass.driver.GetDuplicates()
                        self.UpdateMap(render=False)

                        if dupl:
                            posWindow = self.ClientToScreen((self.mouse['end'][0] + self.dialogOffset,
                                                             self.mouse['end'][1] + self.dialogOffset))
                            
                            dlg = VDigitDuplicatesDialog(parent=self, data=dupl, pos=posWindow)

                            if dlg.ShowModal() == wx.ID_OK:
                                digitClass.driver.UnSelect(dlg.GetUnSelected())
                                # update selected
                                self.UpdateMap(render=False)

                    if digitToolbar.GetAction() != "editLine":
                        # -> move line || move vertex
                        self.UpdateMap(render=False)

                else: # no vector object found
                    self.UpdateMap(render=False, renderVector=False)

            elif digitToolbar.GetAction() in ["splitLine", "addVertex", "removeVertex"]:
                pointOnLine = digitClass.driver.SelectLineByPoint(pos1,
                                                                  type=VDigit_Lines_Type)
                if pointOnLine:
                    if digitToolbar.GetAction() in ["splitLine", "addVertex"]:
                        self.UpdateMap(render=False) # highlight object
                        self.DrawCross(pdc=self.pdcTmp, coords=self.Cell2Pixel(pointOnLine),
                                       size=5)
                    elif digitToolbar.GetAction() == "removeVertex":
                        # get only id of vertex
                        try:
                            id = digitClass.driver.GetSelectedVertex(pos1)[0]
                        except IndexError:
                            id = None

                        if id:
                            x, y = self.pdcVector.GetIdBounds(id)[0:2]
                            self.pdcVector.RemoveId(id)
                            self.UpdateMap(render=False) # highlight object
                            self.DrawCross(pdc=self.pdcTmp, coords=(x, y),
                                           size=5)
                        else:
                            # unselect
                            digitClass.driver.SetSelected([])
                            self.UpdateMap(render=False)
                            
            elif digitToolbar.GetAction() == "copyLine":
                if UserSettings.Get(group='vdigit', key='bgmap',
                                    subkey='value', internal=True) == '':
                    # no background map -> copy from current vector map layer
                    nselected = digitClass.driver.SelectLinesByBox(pos1, pos2,
                                                                   digitClass.GetSelectType())

                    if nselected > 0:
                        # highlight selected features
                        self.UpdateMap(render=False)
                    else:
                        self.UpdateMap(render=False, renderVector=False)
                else:
                    # copy features from background map
                    self.copyIds = digitClass.SelectLinesFromBackgroundMap(pos1, pos2)
                    if len(self.copyIds) > 0:
                        color = UserSettings.Get(group='vdigit', key='symbol',
                                                 subkey=['highlight', 'color'])
                        colorStr = str(color[0]) + ":" + \
                            str(color[1]) + ":" + \
                            str(color[2])
                        dVectTmp = ['d.vect',
                                    'map=%s' % UserSettings.Get(group='vdigit', key='bgmap',
                                                                subkey='value', internal=True),
                                    'cats=%s' % utils.ListOfCatsToRange(self.copyIds),
                                    '-i',
                                    'color=%s' % colorStr,
                                    'fcolor=%s' % colorStr,
                                    'type=point,line,boundary,centroid',
                                    'width=2']
                        
                        self.layerTmp = self.Map.AddLayer(type='vector',
                                                          name=globalvar.QUERYLAYER,
                                                          command=dVectTmp)
                        self.UpdateMap(render=True, renderVector=False)
                    else:
                        self.UpdateMap(render=False, renderVector=False)
                    self.redrawAll = None

            elif digitToolbar.GetAction() == "zbulkLine" and len(self.polycoords) == 2:
                # select lines to be labeled
                pos1 = self.polycoords[0]
                pos2 = self.polycoords[1]
                nselected = digitClass.driver.SelectLinesByBox(pos1, pos2,
                                                               digitClass.GetSelectType())

                if nselected > 0:
                    # highlight selected features
                    self.UpdateMap(render=False)
                    self.DrawLines(pdc=self.pdcTmp) # redraw temp line
                else:
                    self.UpdateMap(render=False, renderVector=False)

            elif digitToolbar.GetAction() == "connectLine":
                if len(digitClass.driver.GetSelected()) > 0:
                    self.UpdateMap(render=False)
                    
            if len(digitClass.driver.GetSelected()) > 0:
                self.redrawAll = None
                ### self.OnPaint(None)
                
        elif self.dragid != None:
            # end drag of overlay decoration
            if self.overlays.has_key(self.dragid):
                self.overlays[self.dragid]['coords'] = self.pdc.GetIdBounds(self.dragid)
            self.dragid = None
            self.currtxtid = None
            self.Update()

        event.Skip()

    def OnButtonDClick(self, event):
        """
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
            ### self.Refresh()
            self.SetCursor(self.parent.cursors["default"])
        elif self.mouse["use"] == "profile":
            # profile
            pass
        #                self.pdc.ClearId(self.lineid)
        #                self.pdc.ClearId(self.plineid)
        #                print 'coordinates: ',self.polycoords
        #                self.polycoords = []
        #                self.mouse['begin'] = self.mouse['end'] = [0, 0]
        #                self.Refresh()
        elif self.mouse['use'] == 'pointer' and self.parent.toolbars['vdigit']:
            # digitization tool
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

        event.Skip()

    def OnRightDown(self, event):
        """
        Right mouse button pressed
        """
        Debug.msg (5, "BufferedWindow.OnRightDown(): use=%s" % \
                   self.mouse["use"])

        x,y = event.GetPositionTuple()[:]
        l = self.pdc.FindObjects(x=x, y=y, radius=self.hitradius)
        if not l:
            return

        id = l[0]

        if id != 99:
            if self.pdc.GetIdGreyedOut(id) == True:
                self.pdc.SetIdGreyedOut(id, False)
            else:
                self.pdc.SetIdGreyedOut(id, True)

                r = self.pdc.GetIdBounds(id)
                r.Inflate(4,4)
                self.RefreshRect(r, False)

        digitToolbar = self.parent.toolbars['vdigit']
        if digitToolbar:
            digitClass = self.parent.digit
            # digitization tool (confirm action)
            if digitToolbar.GetAction() in ["moveLine", "moveVertex"] and \
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
                    if digitClass.MoveSelectedVertex(pFrom, move) < 0:
                        return
                
                del self.vdigitMove
                
        event.Skip()

    def OnRightUp(self, event):
        """
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
                    wx.MessageBox(parent=self,
                                  message=_("No vector map selected for editing."),
                                  caption=_("Error"), style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
                    
                if map:
                    # mapcoords = []
                    # xy -> EN
                    # for coord in self.polycoords:
                    #    mapcoords.append(self.Pixel2Cell(coord))
                    if digitToolbar.GetAction('type') == 'line':
                        line = True
                    else:
                        line = False

                    if len(self.polycoords) < 2: # ignore 'one-point' lines
                        return
                    
                    fid = digitClass.AddLine(map, line, self.polycoords)
                    if fid < 0:
                        return
                    
                    position = self.Cell2Pixel(self.polycoords[-1])
                    self.polycoords = []
                    self.UpdateMap(render=False)
                    self.redrawAll = True
                    
                    # add new record into atribute table
                    if UserSettings.Get(group='vdigit', key="addRecord", subkey='enabled') is True:
                        posWindow = self.ClientToScreen((position[0] + self.dialogOffset,
                                                         position[1] + self.dialogOffset))

                        # select attributes based on layer and category
                        cats = { fid : {
                                UserSettings.Get(group='vdigit', key="layer", subkey='value') :
                                    (UserSettings.Get(group='vdigit', key="category", subkey='value'), )
                                }}

                        addRecordDlg = dbm.DisplayAttributesDialog(parent=self, map=map,
                                                                   cats=cats,
                                                                   pos=posWindow,
                                                                   action="add")
                        if addRecordDlg.mapDBInfo and \
                               addRecordDlg.ShowModal() == wx.ID_OK:
                            sqlfile = tempfile.NamedTemporaryFile(mode="w")
                            for sql in addRecordDlg.GetSQLString():
                                sqlfile.file.write(sql + ";\n")
                            sqlfile.file.flush()
                            executeCommand = gcmd.Command(cmd=["db.execute",
                                                              "--q",
                                                              "input=%s" % sqlfile.name])
            elif digitToolbar.GetAction() == "deleteLine":
                # -> delete selected vector features
                if digitClass.DeleteSelectedLines() < 0:
                    return
            elif digitToolbar.GetAction() == "splitLine":
                # split line
                if digitClass.SplitLine(self.Pixel2Cell(self.mouse['begin'])) < 0:
                    return
            elif digitToolbar.GetAction() == "addVertex":
                # add vertex
                if digitClass.AddVertex(self.Pixel2Cell(self.mouse['begin'])) < 0:
                    return
            elif digitToolbar.GetAction() == "removeVertex":
                # remove vertex
                if digitClass.RemoveVertex(self.Pixel2Cell(self.mouse['begin'])) < 0:
                    return
            elif digitToolbar.GetAction() in ("copyCats", "copyAttrs"):
                try:
                    if digitToolbar.GetAction() == 'copyCats':
                        if digitClass.CopyCats(self.copyCatsList,
                                               self.copyCatsIds, copyAttrb=False) < 0:
                            return
                    else:
                        if digitClass.CopyCats(self.copyCatsList,
                                               self.copyCatsIds, copyAttrb=True) < 0:
                            return
                    
                    del self.copyCatsList
                    del self.copyCatsIds
                except AttributeError:
                    pass
            elif digitToolbar.GetAction() == "editLine" and \
                    hasattr(self, "vdigitMove"):
                line = digitClass.driver.GetSelected()
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
                if len(digitClass.driver.GetSelected()) > 1:
                    if digitClass.ConnectLine() < 0:
                        return
            elif digitToolbar.GetAction() == "copyLine":
                if digitClass.CopyLine(self.copyIds) < 0:
                    return
                del self.copyIds
                if self.layerTmp:
                    self.Map.DeleteLayer(self.layerTmp)
                    self.UpdateMap(render=True, renderVector=False)
                del self.layerTmp

            elif digitToolbar.GetAction() == "zbulkLine" and len(self.polycoords) == 2:
                pos1 = self.polycoords[0]
                pos2 = self.polycoords[1]

                selected = digitClass.driver.GetSelected()
                dlg = VDigitZBulkDialog(parent=self, title=_("Z bulk-labeling dialog"),
                                        nselected=len(selected))
                if dlg.ShowModal() == wx.ID_OK:
                    if digitClass.ZBulkLine(pos1, pos2, dlg.value.GetValue(),
                                            dlg.step.GetValue()) < 0:
                        return
                self.UpdateMap(render=False, renderVector=True)
            elif digitToolbar.GetAction() == "typeConv":
                # -> feature type conversion
                # - point <-> centroid
                # - line <-> boundary
                if digitClass.TypeConvForSelectedLines() < 0:
                    return

            if digitToolbar.GetAction() != "addLine":
                # unselect and re-render
                digitClass.driver.SetSelected([])
                self.polycoords = []
                self.UpdateMap(render=False)

            self.redrawAll = True
            
        event.Skip()

    def OnMiddleDown(self, event):
        """Middle mouse button pressed"""
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

                self.UpdateMap(render=False, renderVector=False)

            elif digitToolbar.GetAction() in ["deleteLine", "moveLine", "splitLine",
                                              "addVertex", "removeVertex", "moveVertex",
                                              "copyCats", "flipLine", "mergeLine",
                                              "snapLine", "connectLine", "copyLine",
                                              "queryLine", "breakLine", "typeConv"]:
                # varios tools -> unselected selected features
                digitClass.driver.SetSelected([])
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
                        self.UpdateMap(render=True, renderVector=False)
                    del self.layerTmp

                self.polycoords = []
                self.UpdateMap(render=False) # render vector

            elif digitToolbar.GetAction() == "zbulkLine":
                # reset polyline
                self.polycoords = []
                digitClass.driver.SetSelected([])
                self.UpdateMap(render=False)
            
            self.redrawAll = True
            
    def OnMouseMoving(self, event):
        """Motion event and no mouse buttons were pressed"""
        digitToolbar = self.parent.toolbars['vdigit']
        if self.mouse["use"] == "pointer" and digitToolbar:
            digitClass = self.parent.digit
            self.mouse['end'] = event.GetPositionTuple()[:]
            Debug.msg (5, "BufferedWindow.OnMouseMoving(): coords=%f,%f" % \
                           (self.mouse['end'][0], self.mouse['end'][1]))
            if digitToolbar.GetAction() == "addLine" and digitToolbar.GetAction('type') in ["line", "boundary"]:
                if len(self.polycoords) > 0:
                    self.MouseDraw(pdc=self.pdcTmp, begin=self.Cell2Pixel(self.polycoords[-1]))
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

                            self.ClearLines(pdc=self.pdcTmp)
                            self.DrawLines(pdc=self.pdcTmp)

                        else: # edit line
                            try:
                                if self.vdigitMove['id'][-1] > 0: # previous vertex
                                    self.MouseDraw(pdc=self.pdcTmp,
                                                   begin=self.Cell2Pixel(self.polycoords[-1]))
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

    def ClearLines(self, pdc=None):
        """
        Clears temporary drawn lines from PseudoDC
        """

        if not pdc:
            return

        exit = True

        try:
            pdc.ClearId(self.lineid)
            pdc.RemoveId(self.lineid)
        except:
            exit = False

        try:
            pdc.ClearId(self.plineid)
            pdc.RemoveId(self.plineid)
        except:
            exit = False

        Debug.msg(4, "BufferedWindow.ClearLines(): lineid=%s, plineid=%s" %
                  (self.lineid, self.plineid))

        ### self.Refresh()

        return exit

    def Pixel2Cell(self, (x, y)):
        """
        Convert image coordinates to real word coordinates

        Input : int x, int y
        Output: float x, float y
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

        # extent does not correspond with whole map canvas area...
        # east  = self.Map.region['w'] + x * self.Map.region["ewres"]
        # north = self.Map.region['n'] - y * self.Map.region["nsres"]

        return (east, north)

    def Cell2Pixel(self, (east, north)):
        """
        Convert real word coordinates to image coordinates
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

        # x = int((east  - w) / res)
        # y = int((n - north) / res)

        x = (east  - w) / res
        y = (n - north) / res

        return (x, y)

    def Zoom(self, begin, end, zoomtype):
        """
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
        """
        Zoom to previous extents in zoomhistory list
        """

        zoom = []
        if len(self.zoomhistory) > 1:
            self.zoomhistory.pop()
            zoom = self.zoomhistory[len(self.zoomhistory)-1]
            # (n, s, e, w)
        if zoom:
            # zoom to selected region
            self.Map.region['center_easting'] = zoom[3] + \
                (zoom[2] - zoom[3]) / 2
            self.Map.region['center_northing'] = zoom[1] + \
                (zoom[0] - zoom[1]) / 2
            self.Map.region["ewres"] = (zoom[2] - zoom[3]) / self.Map.width
            self.Map.region["nsres"] = (zoom[0] - zoom[1]) / self.Map.height
            self.Map.AlignExtentFromDisplay()

            # update map
            self.UpdateMap()

            # update statusbar
            self.parent.StatusbarUpdate()

    def ZoomHistory(self, n, s, e, w):
        """
        Manages a list of last 10 zoom extents

        Return removed history item if exists
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

        return removed

    def OnZoomToMap(self, event):
        """
        Set display extents to match selected raster (including NULLs)
        or vector map.
        """
        self.ZoomToMap()

    def OnZoomToRaster(self, event):
        """
        Set display extents to match selected raster map (ignore NULLs)
        """
        self.ZoomToMap(zoom=True)
        
    def ZoomToMap(self, layer=None, zoom=False):
        """
        Set display extents to match selected raster
        or vector map.
        """
        zoomreg = {}

        if not layer:
            layer = self.GetSelectedLayer()
        
        if layer is None:
            return

        Debug.msg (3, "BufferedWindow.ZoomToMap(): layer=%s, type=%s" % \
                   (layer.name, layer.type))

        # selected layer must be a valid map
        if layer.type in ('raster', 'rgb', 'his', 'shaded', 'arrow'):
            if layer.type == 'raster':
                self.Map.region = self.Map.GetRegion(rast="%s" % layer.name)
            else:
                self.Map.region = self.Map.GetRegion(rast="%s" % layer.name)
        elif layer.type in ('vector', 'thememap', 'themechart'):
            if self.parent.digit and layer.name == self.parent.digit.map and \
               self.parent.digit.type == 'vdigit':
                w, s, b, e, n, t = self.parent.digit.driver.GetMapBoundingBox()
                self.Map.region = self.Map.GetRegion(n=n, s=s, w=w, e=e)
            else:
                self.Map.region = self.Map.GetRegion(vect="%s" % layer.name)
        else:
            return

        ### self.Map.SetRegion()
        ### self.Map.AlignExtentFromDisplay()

        self.ZoomHistory(self.Map.region['n'], self.Map.region['s'],
                         self.Map.region['e'], self.Map.region['w'])

        self.UpdateMap()

        self.parent.StatusbarUpdate()

    def ZoomToWind(self, event):
        """
        Set display geometry to match computational
        region settings (set with g.region)
        """
        self.Map.region = self.Map.GetRegion()
        ### self.Map.SetRegion(windres=True)

        self.ZoomHistory(self.Map.region['n'], self.Map.region['s'],
                         self.Map.region['e'], self.Map.region['w'])

        self.UpdateMap()

        self.parent.StatusbarUpdate()

    def ZoomToDefault(self, event):
        """Set display geometry to match default
        region settings"""
        self.Map.region = self.Map.GetRegion(default=True)
        self.Map.AdjustRegion() # aling region extent to the display

        self.ZoomHistory(self.Map.region['n'], self.Map.region['s'],
                         self.Map.region['e'], self.Map.region['w'])

        self.UpdateMap()

        self.parent.StatusbarUpdate()
        
    def DisplayToWind(self, event):
        """
        Set computational region (WIND file) to
        match display extents
        """
        tmpreg = os.getenv("GRASS_REGION")
        if tmpreg:
            del os.environ["GRASS_REGION"]

        # We ONLY want to set extents here. Don't mess with resolution. Leave that
        # for user to set explicitly with g.region
        new = self.Map.AlignResolution()
        
        cmdRegion = ["g.region", "--o",
                     "n=%f"    % new['n'],
                     "s=%f"    % new['s'],
                     "e=%f"    % new['e'],
                     "w=%f"    % new['w'],
                     "rows=%d" % int(new['rows']),
                     "cols=%d" % int(new['cols'])]
        
        p = gcmd.Command(cmdRegion)

        if tmpreg:
            os.environ["GRASS_REGION"] = tmpreg

    def ZoomToSaved(self, event):
        """
        Set display geometry to match extents in
        saved region file
        """

        zoomreg = {}

        dlg = gdialogs.SavedRegion(self, wx.ID_ANY, _("Zoom to saved region extents"),
                                   pos=wx.DefaultPosition, size=wx.DefaultSize,
                                   style=wx.DEFAULT_DIALOG_STYLE,
                                   loadsave='load')

        if dlg.ShowModal() == wx.ID_CANCEL:
            dlg.Destroy()
            return

        wind = dlg.wind

        p = gcmd.Command (["g.region",
	                   "-ugp", "region=%s" % wind])

        if p.returncode == 0:
            output = p.ReadStdOutput()
            for line in output:
                line = line.strip()
                if '=' in line: key,val = line.split('=')
                zoomreg[key] = float(val)
            self.Map.region['n'] = zoomreg['n']
            self.Map.region['s'] = zoomreg['s']
            self.Map.region['e'] = zoomreg['e']
            self.Map.region['w'] = zoomreg['w']
            self.ZoomHistory(self.Map.region['n'],self.Map.region['s'],self.Map.region['e'],self.Map.region['w'])
            self.UpdateMap()

        dlg.Destroy()

    def SaveDisplayRegion(self, event):
        """
        Save display extents to named region file.
        """

        dlg = gdialogs.SavedRegion(self, wx.ID_ANY, "Save display extents to region file",
                                   pos=wx.DefaultPosition, size=wx.DefaultSize,
                                   style=wx.DEFAULT_DIALOG_STYLE,
                                   loadsave='save')
        if dlg.ShowModal() == wx.ID_CANCEL:
            dlg.Destroy()
            return

        wind = dlg.wind

        # test to see if it already exists and ask permission to overwrite
        windpath = os.path.join(self.Map.env["GISDBASE"], self.Map.env["LOCATION_NAME"],
                                self.Map.env["MAPSET"],"windows",wind)

        if windpath and not os.path.exists(windpath):
            self.SaveRegion(wind)
        elif windpath and os.path.exists(windpath):
            overwrite = wx.MessageBox(_("Region file <%s> already exists. "
                                        "Do you want to overwrite it?") % (wind),
                                      _("Warning"), wx.YES_NO)
            if (overwrite == wx.YES):
                self.SaveRegion(wind)
        else:
            pass

        dlg.Destroy()

    def SaveRegion(self, wind):
        """Save region settings"""
        new = self.Map.AlignResolution()

        cmdRegion = ["g.region",
                     "-u",
                     "n=%f" % new['n'],
                     "s=%f" % new['s'],
                     "e=%f" % new['e'],
                     "w=%f" % new['w'],
                     "rows=%d" % new['rows'],
                     "cols=%d" % new['cols'],
                     "save=%s" % wind,
                     "--o"]

        tmpreg = os.getenv("GRASS_REGION")
        if tmpreg:
            del os.environ["GRASS_REGION"]
        
        p = gcmd.Command(cmdRegion)

        if tmpreg:
            os.environ["GRASS_REGION"] = tmpreg

    def Distance(self, beginpt, endpt, screen=True):
        """Calculete distance

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

class MapFrame(wx.Frame):
    """
    Main frame for map display window. Drawing takes place in child double buffered
    drawing window.
    """

    def __init__(self, parent=None, id=wx.ID_ANY, title=_("GRASS GIS - Map display"),
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_FRAME_STYLE, toolbars=["map"],
                 tree=None, notebook=None, gismgr=None, page=None,
                 Map=None, auimgr=None):
        """
        Main map display window with toolbars, statusbar and
        DrawWindow

        @param toolbars array of activated toolbars, e.g. ['map', 'digit']
        @param tree reference to layer tree
        @param notebook control book ID in Layer Manager
        @param gismgr Layer Manager panel
        @param page notebook page with layer tree
        @param Map instance of render.Map
        """
        self.gismanager = gismgr    # GIS Manager object
        self.Map        = Map       # instance of render.Map
        self.tree       = tree      # GIS Manager layer tree object
        self.page       = page      # Notebook page holding the layer tree
        self.layerbook  = notebook  # GIS Manager layer tree notebook
        self.parent     = parent

        #
        # available cursors
        #
        self.cursors = {
            # default: cross
            # "default" : wx.StockCursor(wx.CURSOR_DEFAULT),
            "default" : wx.StockCursor(wx.CURSOR_ARROW),
            "cross"   : wx.StockCursor(wx.CURSOR_CROSS),
            "hand"    : wx.StockCursor(wx.CURSOR_HAND),
            "pencil"  : wx.StockCursor(wx.CURSOR_PENCIL),
            "sizenwse": wx.StockCursor(wx.CURSOR_SIZENWSE)
            }

        wx.Frame.__init__(self, parent, id, title, pos, size, style)
        self.SetName("MapWindow")

        #
        # set the size & system icon
        #
        self.SetClientSize(size)
        self.iconsize = (16, 16)

        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass_map.ico'), wx.BITMAP_TYPE_ICO))

        #
        # Fancy gui
        #
        # self._mgr = auimgr
        self._mgr = wx.aui.AuiManager(self)

        #
        # Add toolbars
        #
        self.toolbars = { 'map' : None,
                          'vdigit' : None,
                          'georect' : None, 
                          'nviz' : None }
        for toolb in toolbars:
            self.AddToolbar(toolb)

        #
        # Add statusbar
        #
        self.statusbar = self.CreateStatusBar(number=3, style=0)
        self.statusbar.SetStatusWidths([-5, -2, -1])
        self.toggleStatus = wx.Choice(self.statusbar, wx.ID_ANY,
                                      choices = globalvar.MAP_DISPLAY_STATUSBAR_MODE)
	self.toggleStatus.SetSelection(UserSettings.Get(group='display', key='statusbarMode', subkey='selection'))
        self.statusbar.Bind(wx.EVT_CHOICE, self.OnToggleStatus, self.toggleStatus)
        # auto-rendering checkbox
        self.autoRender = wx.CheckBox(parent=self.statusbar, id=wx.ID_ANY,
                                      label=_("Render"))
        self.statusbar.Bind(wx.EVT_CHECKBOX, self.OnToggleRender, self.autoRender)
        self.autoRender.SetValue(UserSettings.Get(group='display', key='autoRendering', subkey='enabled'))
        self.autoRender.SetToolTip(wx.ToolTip (_("Enable/disable auto-rendering")))
        # show region
        self.showRegion = wx.CheckBox(parent=self.statusbar, id=wx.ID_ANY,
                                      label=_("Show computational extent"))
        self.statusbar.Bind(wx.EVT_CHECKBOX, self.OnToggleShowRegion, self.showRegion)
        
        self.showRegion.SetValue(False)
        self.showRegion.Hide()
        self.showRegion.SetToolTip(wx.ToolTip (_("Show/hide computational "
                                                 "region extent (set with g.region). "
                                                 "Display region drawn as a blue box inside the "
                                                 "computational region, "
                                                 "computational region inside a display region "
                                                 "as a red box).")))
        # set resolution
        self.compResolution = wx.CheckBox(parent=self.statusbar, id=wx.ID_ANY,
                                         label=_("Constrain display resolution to computational settings"))
        self.statusbar.Bind(wx.EVT_CHECKBOX, self.OnToggleResolution, self.compResolution)
        self.compResolution.SetValue(UserSettings.Get(group='display', key='compResolution', subkey='enabled'))
        self.compResolution.Hide()
        self.compResolution.SetToolTip(wx.ToolTip (_("Constrain display resolution "
                                                     "to computational region settings. "
                                                     "Default value for new map displays can "
                                                     "be set up in 'User GUI settings' dialog.")))
        # map scale
        self.mapScale = wx.TextCtrl(parent=self.statusbar, id=wx.ID_ANY,
                                    value="", style=wx.TE_PROCESS_ENTER,
                                    size=(150, -1))
        self.mapScale.Hide()
        self.statusbar.Bind(wx.EVT_TEXT_ENTER, self.OnChangeMapScale, self.mapScale)
        # on-render gauge
        self.onRenderGauge = wx.Gauge(parent=self.statusbar, id=wx.ID_ANY,
                                      range=0, style=wx.GA_HORIZONTAL)
        self.onRenderGauge.Hide()
        
        self.StatusbarReposition() # reposition statusbar

        #
        # Init map display (buffered DC & set default cursor)
        #
        self.MapWindow2D = BufferedWindow(self, id=wx.ID_ANY,
                                          Map=self.Map, tree=self.tree, gismgr=self.gismanager)
        # default is 2D display mode
        self.MapWindow = self.MapWindow2D
        self.MapWindow.Bind(wx.EVT_MOTION, self.OnMotion)
        self.MapWindow.SetCursor(self.cursors["default"])
        # used by Nviz (3D display mode)
        self.MapWindow3D = None 

        #
        # initialize region values
        #
        self.__InitDisplay() 

        #
        # Bind various events
        #
        self.Bind(wx.EVT_ACTIVATE, self.OnFocus)
        self.Bind(wx.EVT_CLOSE,    self.OnCloseWindow)
        self.Bind(render.EVT_UPDATE_PRGBAR, self.OnUpdateProgress)
        
        #
        # Update fancy gui style
        #
        self._mgr.AddPane(self.MapWindow, wx.aui.AuiPaneInfo().CentrePane().
                   Dockable(False).BestSize((-1,-1)).
                   CloseButton(False).DestroyOnClose(True).
                   Layer(0))
        self._mgr.Update()

        #
        # Init print module and classes
        #
        self.printopt = disp_print.PrintOptions(self, self.MapWindow)
        
        #
        # Initialization of digitization tool
        #
        self.digit = None

        #
        # Init zoom history
        #
        self.MapWindow.ZoomHistory(self.Map.region['n'],
                                   self.Map.region['s'],
                                   self.Map.region['e'],
                                   self.Map.region['w'])

        #
        # Re-use dialogs
        #
        self.dialogs = {}
        self.dialogs['attributes'] = None
        self.dialogs['category'] = None
        self.dialogs['barscale'] = None
        self.dialogs['legend'] = None

        self.decorationDialog = None # decoration/overlays

    def AddToolbar(self, name):
        """
        Add defined toolbar to the window

        Currently known toolbars are:
         - map basic map toolbar
         - digit vector digitizer
         - georect georectifier
        """
        # default toolbar
        if name == "map":
            self.toolbars['map'] = toolbars.MapToolbar(self, self.Map)

            self._mgr.AddPane(self.toolbars['map'].toolbar,
                              wx.aui.AuiPaneInfo().
                              Name("maptoolbar").Caption(_("Map Toolbar")).
                              ToolbarPane().Top().
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(False).TopDockable(True).
                              CloseButton(False).Layer(2))
        # vector digitizer
        elif name == "vdigit":
            self.toolbars['vdigit'] = toolbars.VDigitToolbar(parent=self, map=self.Map,
                                                             layerTree=self.tree,
                                                             log=self.gismanager.goutput)

            for toolRow in range(0, self.toolbars['vdigit'].numOfRows):
                self._mgr.AddPane(self.toolbars['vdigit'].toolbar[toolRow],
                                  wx.aui.AuiPaneInfo().
                                  Name("vdigittoolbar" + str(toolRow)).Caption(_("Vector digitizer toolbar")).
                                  ToolbarPane().Top().Row(toolRow + 1).
                                  LeftDockable(False).RightDockable(False).
                                  BottomDockable(False).TopDockable(True).
                                  CloseButton(False).Layer(2))

            # change mouse to draw digitized line
            self.MapWindow.mouse['box'] = "point"
            self.MapWindow.zoomtype = 0
            self.MapWindow.pen     = wx.Pen(colour='red',   width=2, style=wx.SOLID)
            self.MapWindow.polypen = wx.Pen(colour='green', width=2, style=wx.SOLID)
        # georectifier
        elif name == "georect":
            self.toolbars['georect'] = toolbars.GRToolbar(self, self.Map)

            self._mgr.AddPane(self.toolbars['georect'].toolbar,
                              wx.aui.AuiPaneInfo().
                              Name("georecttoolbar").Caption(_("Georectification toolbar")).
                              ToolbarPane().Top().
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(False).TopDockable(True).
                              CloseButton(False).Layer(2))
        # nviz
        elif name == "nviz":
            import nviz

            # check for GLCanvas and OpenGL
            msg = None
            if not nviz.haveGLCanvas:
                msg = _("Unable to start Nviz. The GLCanvas class has not been "
                        "included with this build "
                        "of wxPython! Switching back to "
                        "2D display mode.\n\nDetails: %s" % nviz.errorMsg)
            if not nviz.haveNviz:
                msg = _("Unable to start Nviz. Python extension "
                        "for Nviz was not found or loaded properly. "
                        "Switching back to 2D display mode.\n\nDetails: %s" % nviz.errorMsg)

            if msg:
                wx.MessageBox(parent=self,
                              message=msg,
                              caption=_("Error"))
                return

            #
            # add Nviz toolbar and disable 2D display mode tools
            #
            self.toolbars['nviz'] = toolbars.NvizToolbar(self, self.Map)
            self.toolbars['map'].Enable2D(False)

            #
            # update layer tree (-> enable 3d-rasters)
            #
            self.tree.EnableItemType(type='3d-raster', enable=True)
            
            #
            # update status bar
            #
            self.toggleStatus.Enable(False)

            #
            # erase map window
            #
            self.MapWindow.EraseMap()

            busy = wx.BusyInfo(message=_("Please wait, loading data..."),
                               parent=self)
            wx.Yield()
        
            #
            # create GL window & NVIZ toolbar
            #
            if not self.MapWindow3D:
                self.MapWindow3D = nviz.GLWindow(self, id=wx.ID_ANY,
                                                 Map=self.Map, tree=self.tree, gismgr=self.gismanager)
                self.nvizToolWin = nviz.NvizToolWindow(self, id=wx.ID_ANY,
                                                       mapWindow=self.MapWindow3D)
                self.MapWindow3D.OnPaint(None) # -> LoadData

            busy.Destroy()

            self.nvizToolWin.Show()

            #
            # switch from MapWindow to MapWindowGL
            # add nviz toolbar
            #
            self._mgr.DetachPane(self.MapWindow2D)
            self.MapWindow2D.Hide()
            self._mgr.AddPane(self.MapWindow3D, wx.aui.AuiPaneInfo().CentrePane().
                              Dockable(False).BestSize((-1,-1)).
                              CloseButton(False).DestroyOnClose(True).
                              Layer(0))
            self._mgr.AddPane(self.toolbars['nviz'].toolbar,
                              wx.aui.AuiPaneInfo().
                              Name("nviztoolbar").Caption(_("Nviz toolbar")).
                              ToolbarPane().Top().Row(1).
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(False).TopDockable(True).
                              CloseButton(False).Layer(2))
            self.MapWindow = self.MapWindow3D
            self.SetStatusText("", 0)
            
        self._mgr.Update()

    def RemoveToolbar (self, name):
        """
        Removes toolbar from the window

        TODO: Only hide, activate by calling AddToolbar()
        """

        # cannot hide main toolbar
        if name == "map":
            return
        elif name == "vdigit":
            # TODO: not destroy only hide
            for toolRow in range(0, self.toolbars['vdigit'].numOfRows):
                self._mgr.DetachPane (self.toolbars['vdigit'].toolbar[toolRow])
                self.toolbars['vdigit'].toolbar[toolRow].Destroy()
        else:
            self._mgr.DetachPane (self.toolbars[name].toolbar)
            self.toolbars[name].toolbar.Destroy()

        self.toolbars[name] = None

        if name == 'nviz':
            # hide nviz tools
            self.nvizToolWin.Hide()
            # unload data
            self.MapWindow3D.Reset()
            # switch from MapWindowGL to MapWindow
            self._mgr.DetachPane(self.MapWindow3D)
            self.MapWindow3D.Hide()
            self.MapWindow2D.Show()
            self._mgr.AddPane(self.MapWindow2D, wx.aui.AuiPaneInfo().CentrePane().
                              Dockable(False).BestSize((-1,-1)).
                              CloseButton(False).DestroyOnClose(True).
                              Layer(0))
            self.MapWindow = self.MapWindow2D
  
            #
            # update layer tree (-> disable 3d-rasters)
            #
            self.tree.EnableItemType(type='3d-raster', enable=False)
            
        self.toolbars['map'].combo.SetValue ("Tools")
        self.toolbars['map'].Enable2D(True)
        self.toggleStatus.Enable(True)

        self._mgr.Update()

    def __InitDisplay(self):
        """
        Initialize map display, set dimensions and map region
        """
        self.width, self.height = self.GetClientSize()

        Debug.msg(2, "MapFrame.__InitDisplay():")
        self.Map.ChangeMapSize(self.GetClientSize())
        self.Map.region = self.Map.GetRegion() # g.region -upgc
        # self.Map.SetRegion() # adjust region to match display window

    def OnUpdateProgress(self, event):
        """Update progress bar info"""
        self.onRenderGauge.SetValue(event.value)
        
        event.Skip()
        
    def OnFocus(self, event):
        """
        Change choicebook page to match display.
        Or set display for georectifying
        """
        if self.gismanager.georectifying:
            # in georectifying session; display used to get get geographic
            # coordinates for GCPs
            self.OnPointer(event)
        else:
            # change bookcontrol page to page associated with display
            if self.page:
                pgnum = self.layerbook.GetPageIndex(self.page)
                if pgnum > -1:
                    self.layerbook.SetSelection(pgnum)
        
        event.Skip()

    def OnMotion(self, event):
        """
        Mouse moved
        Track mouse motion and update status bar
        """
        # update statusbar if required
        if self.toggleStatus.GetSelection() == 0: # Coordinates
            e, n = self.MapWindow.Pixel2Cell(event.GetPositionTuple())
            if self.toolbars['vdigit'] and \
                    self.toolbars['vdigit'].GetAction() == 'addLine' and \
                    self.toolbars['vdigit'].GetAction('type') in ('line', 'boundary') and \
                    len(self.MapWindow.polycoords) > 0:
                # for linear feature show segment and total length
                distance_seg = self.MapWindow.Distance(self.MapWindow.polycoords[-1],
                                                       (e, n), screen=False)[0]
                distance_tot = distance_seg
                for idx in range(1, len(self.MapWindow.polycoords)):
                    distance_tot += self.MapWindow.Distance(self.MapWindow.polycoords[idx-1],
                                                            self.MapWindow.polycoords[idx],
                                                            screen=False )[0]
                self.statusbar.SetStatusText("%.2f, %.2f (seg: %.2f; tot: %.2f)" % \
                                                 (e, n, distance_seg, distance_tot), 0)
            else:
                if self.Map.projinfo['proj'] == 'll':
                    self.statusbar.SetStatusText("%s" % utils.Deg2DMS(e, n), 0)
                else:
                    self.statusbar.SetStatusText("%.2f, %.2f" % (e, n), 0)
        
        event.Skip()

    def OnDraw(self, event):
        """
        Re-display current map composition
        """
        self.MapWindow.UpdateMap(render=False)
        
    def OnRender(self, event):
        """
        Re-render map composition (each map layer)
        """
        # delete tmp map layers (queries)
        qlayer = self.Map.GetListOfLayers(l_name=globalvar.QUERYLAYER)
        for layer in qlayer:
            self.Map.DeleteLayer(layer)

        # deselect features in vdigit
        if self.toolbars['vdigit'] and self.digit:
            self.digit.driver.SetSelected([])
            self.MapWindow.UpdateMap(render=True, renderVector=True)
        else:
            self.MapWindow.UpdateMap(render=True)
        
        # update statusbar
        self.StatusbarUpdate()

    def OnPointer(self, event):
        """Pointer button clicked"""
        if self.toolbars['map']:
            if event:
                self.toolbars['map'].OnTool(event)
            self.toolbars['map'].action['desc'] = ''
        
        self.MapWindow.mouse['use'] = "pointer"
        self.MapWindow.mouse['box'] = "point"

        # change the cursor
        if self.toolbars['vdigit']:
            # digitization tool activated
            self.MapWindow.SetCursor(self.cursors["cross"])

            # reset mouse['box'] if needed
            if self.toolbars['vdigit'].GetAction() in ['addLine']:
                if self.toolbars['vdigit'].GetAction('type') in ['point', 'centroid']:
                    self.MapWindow.mouse['box'] = 'point'
                else: # line, boundary
                    self.MapWindow.mouse['box'] = 'line'
            elif self.toolbars['vdigit'].GetAction() in ['addVertex', 'removeVertex', 'splitLine',
                                                         'editLine', 'displayCats', 'displayAttrs',
                                                         'copyCats']:
                self.MapWindow.mouse['box'] = 'point'
            else: # moveLine, deleteLine
                self.MapWindow.mouse['box'] = 'box'
        
        elif self.gismanager.georectifying:
            self.MapWindow.SetCursor(self.cursors["cross"])
        
        else:
            self.MapWindow.SetCursor(self.cursors["default"])

    def OnZoomIn(self, event):
        """
        Zoom in the map.
        Set mouse cursor, zoombox attributes, and zoom direction
        """
        if self.toolbars['map']:
            self.toolbars['map'].OnTool(event)
            self.toolbars['map'].action['desc'] = ''
        
        self.MapWindow.mouse['use'] = "zoom"
        self.MapWindow.mouse['box'] = "box"
        self.MapWindow.zoomtype = 1
        self.MapWindow.pen = wx.Pen(colour='Red', width=2, style=wx.SHORT_DASH)
        
        # change the cursor
        self.MapWindow.SetCursor(self.cursors["cross"])

    def OnZoomOut(self, event):
        """
        Zoom out the map.
        Set mouse cursor, zoombox attributes, and zoom direction
        """
        if self.toolbars['map']:
            self.toolbars['map'].OnTool(event)
            self.toolbars['map'].action['desc'] = ''
        
        self.MapWindow.mouse['use'] = "zoom"
        self.MapWindow.mouse['box'] = "box"
        self.MapWindow.zoomtype = -1
        self.MapWindow.pen = wx.Pen(colour='Red', width=2, style=wx.SHORT_DASH)
        
        # change the cursor
        self.MapWindow.SetCursor(self.cursors["cross"])

    def OnZoomBack(self, event):
        """
        Zoom last (previously stored position)
        """
        self.MapWindow.ZoomBack()

    def OnPan(self, event):
        """
        Panning, set mouse to drag
        """
        if self.toolbars['map']:
            self.toolbars['map'].OnTool(event)
            self.toolbars['map'].action['desc'] = ''
        
        self.MapWindow.mouse['use'] = "pan"
        self.MapWindow.mouse['box'] = "pan"
        self.MapWindow.zoomtype = 0
                
        # change the cursor
        self.MapWindow.SetCursor(self.cursors["hand"])

    def OnErase(self, event):
        """
        Erase the canvas
        """
        self.MapWindow.EraseMap()

    def OnZoomRegion(self, event):
        """
        Zoom to region
        """
        self.Map.getRegion()
        self.Map.getResolution()
        self.UpdateMap()
        # event.Skip()

    def OnAlignRegion(self, event):
        """
        Align region
        """
        if not self.Map.alignRegion:
            self.Map.alignRegion = True
        else:
            self.Map.alignRegion = False
        # event.Skip()

    def OnToggleRender(self, event):
        """Enable/disable auto-rendering"""
        if self.autoRender.GetValue():
            self.OnRender(None)

    def OnToggleShowRegion(self, event):
        """Show/Hide extent in map canvas"""
        if self.showRegion.GetValue():
            # show extent
            self.MapWindow.regionCoords = []
        else:
            del self.MapWindow.regionCoords

        # redraw map if auto-rendering is enabled
        if self.autoRender.GetValue():
            self.OnRender(None)

    def OnToggleResolution(self, event):
        """Use resolution of computation region settings
        for redering image instead of display resolution"""
        # redraw map if auto-rendering is enabled
        if self.autoRender.GetValue():
            self.OnRender(None)
        
    def OnToggleStatus(self, event):
        """Toggle status text"""
        self.StatusbarUpdate()

    def OnChangeMapScale(self, event):
        """Map scale changed by user"""
        scale = event.GetString()

        try:
            if scale[:2] != '1:':
                raise ValueError
            value = int(scale[2:])
        except ValueError:
            self.mapScale.SetValue('1:%ld' % int(self.mapScaleValue))
            return

        dEW = value * (self.Map.region['cols'] / self.ppm[0])
        dNS = value * (self.Map.region['rows'] / self.ppm[1])
        self.Map.region['n'] = self.Map.region['center_northing'] + dNS / 2.
        self.Map.region['s'] = self.Map.region['center_northing'] - dNS / 2.
        self.Map.region['w'] = self.Map.region['center_easting']  - dEW / 2.
        self.Map.region['e'] = self.Map.region['center_easting']  + dEW / 2.
        
        # add to zoom history
        self.MapWindow.ZoomHistory(self.Map.region['n'], self.Map.region['s'],
                                   self.Map.region['e'], self.Map.region['w'])
        
        # redraw a map
        self.MapWindow.UpdateMap()
        self.mapScale.SetFocus()
        
    def StatusbarUpdate(self):
        """Update statusbar content"""

        self.showRegion.Hide()
        self.compResolution.Hide()
        self.mapScale.Hide()
        self.mapScaleValue = self.ppm = None

        if self.toggleStatus.GetSelection() == 0: # Coordinates
            self.statusbar.SetStatusText("", 0)
            # enable long help
            self.StatusbarEnableLongHelp()

        elif self.toggleStatus.GetSelection() == 1: # Extent
            self.statusbar.SetStatusText("%.2f - %.2f, %.2f - %.2f" %
                                         (self.Map.region["w"], self.Map.region["e"],
                                          self.Map.region["s"], self.Map.region["n"]), 0)
            # enable long help
            self.StatusbarEnableLongHelp()

        elif self.toggleStatus.GetSelection() == 2: # Comp. region
            compregion = self.Map.GetRegion()
            self.statusbar.SetStatusText("%.2f - %.2f, %.2f - %.2f (%.2f, %.2f)" %
                                         (compregion["w"], compregion["e"],
                                          compregion["s"], compregion["n"],
                                          compregion["ewres"], compregion["nsres"]), 0)
            # enable long help
            self.StatusbarEnableLongHelp()

        elif self.toggleStatus.GetSelection() == 3: # Show comp. extent
            self.statusbar.SetStatusText("", 0)
            self.showRegion.Show()
            # disable long help
            self.StatusbarEnableLongHelp(False)

        elif self.toggleStatus.GetSelection() == 4: # Display mode
            self.statusbar.SetStatusText("", 0)
            self.compResolution.Show()
            # disable long help
            self.StatusbarEnableLongHelp(False)

        elif self.toggleStatus.GetSelection() == 5: # Display geometry
            self.statusbar.SetStatusText("rows=%d; cols=%d; nsres=%.2f; ewres=%.2f" %
                                         (self.Map.region["rows"], self.Map.region["cols"],
                                          self.Map.region["nsres"], self.Map.region["ewres"]), 0)
            # enable long help
            self.StatusbarEnableLongHelp()

        elif self.toggleStatus.GetSelection() == 6: # Map scale
            # TODO: need to be fixed...
            ### screen X region problem
            ### user should specify ppm
            dc = wx.ScreenDC()
            dpSizePx = wx.DisplaySize()   # display size in pixels
            dpSizeMM = wx.DisplaySizeMM() # display size in mm (system)
            dpSizeIn = (dpSizeMM[0] / 25.4, dpSizeMM[1] / 25.4) # inches
            sysPpi  = dc.GetPPI()
            comPpi = (dpSizePx[0] / dpSizeIn[0],
                      dpSizePx[1] / dpSizeIn[1])

            ppi = comPpi                  # pixel per inch
            self.ppm = ((ppi[0] / 2.54) * 100, # pixel per meter
                        (ppi[1] / 2.54) * 100)

            Debug.msg(4, "MapFrame.StatusbarUpdate(mapscale): size: px=%d,%d mm=%f,%f "
                      "in=%f,%f ppi: sys=%d,%d com=%d,%d; ppm=%f,%f" % \
                          (dpSizePx[0], dpSizePx[1], dpSizeMM[0], dpSizeMM[1],
                           dpSizeIn[0], dpSizeIn[1],
                           sysPpi[0], sysPpi[1], comPpi[0], comPpi[1],
                           self.ppm[0], self.ppm[1]))

            region = self.Map.region

            heightCm = region['rows'] / self.ppm[1] * 100
            widthCm  = region['cols'] / self.ppm[0] * 100

            Debug.msg(4, "MapFrame.StatusbarUpdate(mapscale): width_cm=%f, height_cm=%f" %
                      (widthCm, heightCm))

            xscale = (region['e'] - region['w']) / (region['cols'] / self.ppm[0])
            yscale = (region['n'] - region['s']) / (region['rows'] / self.ppm[1])
            scale = (xscale + yscale) / 2.
            
            Debug.msg(3, "MapFrame.StatusbarUpdate(mapscale): xscale=%f, yscale=%f -> scale=%f" % \
                          (xscale, yscale, scale))

            self.statusbar.SetStatusText("")
            try:
                self.mapScale.SetValue("1:%ld" % (scale + 0.5))
            except TypeError:
                pass
            self.mapScaleValue = scale
            self.mapScale.Show()

            # disable long help
            self.StatusbarEnableLongHelp(False)

        else:
            self.statusbar.SetStatusText("", 1)

    def StatusbarEnableLongHelp(self, enable=True):
        """Enable/disable toolbars long help"""
        for toolbar in self.toolbars.itervalues():
            if toolbar:
                toolbar.EnableLongHelp(enable)
                
    def StatusbarReposition(self):
        """Reposition checkbox in statusbar"""
        # reposition checkbox
        widgets = [(0, self.showRegion),
                   (0, self.compResolution),
                   (0, self.mapScale),
                   (0, self.onRenderGauge),
                   (1, self.toggleStatus),
                   (2, self.autoRender)]
        for idx, win in widgets:
            rect = self.statusbar.GetFieldRect(idx)
            if idx == 0: # show region / mapscale / process bar
                # -> size
                wWin, hWin = win.GetBestSize()
                if win == self.onRenderGauge:
                    wWin = rect.width - 6
                # -> position
                # if win == self.showRegion:
                    # x, y = rect.x + rect.width - wWin, rect.y - 1
                    # align left
                # else:
                x, y = rect.x + 3, rect.y - 1
                w, h = wWin, rect.height + 2
            else: # choice || auto-rendering
                x, y = rect.x, rect.y - 1
                w, h = rect.width, rect.height + 2
		if idx == 2:
		    x += 5

            win.SetPosition((x, y))
            win.SetSize((w, h))

    def SaveToFile(self, event):
        """
        Save image to file
        """
        filetype =  "PNG file (*.png)|*.png|"\
                    "TIF file (*.tif)|*.tif|"\
                    "GIF file (*.gif)|*.gif"

        dlg = wx.FileDialog(self, "Choose a file name to save the image as a PNG to",
            defaultDir = "",
            defaultFile = "",
            wildcard = filetype,
            style=wx.SAVE|wx.FD_OVERWRITE_PROMPT)
        if dlg.ShowModal() == wx.ID_OK:
            base = os.path.splitext(dlg.GetPath())[0]
            ext = os.path.splitext(dlg.GetPath())[1]
            if dlg.GetFilterIndex() == 0:
                type = wx.BITMAP_TYPE_PNG
                path = dlg.GetPath()
                if ext != '.png': path = base+'.png'
            elif dlg.GetFilterIndex() == 1:
                type = wx.BITMAP_TYPE_TIF
                if ext != '.tif': path = base+'.tif'
            elif dlg.GetFilterIndex() == 2:
                type = wx.BITMAP_TYPE_TIF
                if ext != '.gif': path = base+'.gif'
            self.MapWindow.SaveToFile(path, type)

        dlg.Destroy()

    def PrintMenu(self, event):
        """
        Print map display
        """

        """
        Print options and output menu
        """
        point = wx.GetMousePosition()
        printmenu = wx.Menu()
        # Add items to the menu
        setup = wx.MenuItem(printmenu, wx.ID_ANY, _('Page setup'))
        printmenu.AppendItem(setup)
        self.Bind(wx.EVT_MENU, self.printopt.OnPageSetup, setup)

        preview = wx.MenuItem(printmenu, wx.ID_ANY, _('Print preview'))
        printmenu.AppendItem(preview)
        self.Bind(wx.EVT_MENU, self.printopt.OnPrintPreview, preview)

        doprint = wx.MenuItem(printmenu, wx.ID_ANY, _('Print display'))
        printmenu.AppendItem(doprint)
        self.Bind(wx.EVT_MENU, self.printopt.OnDoPrint, doprint)

        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(printmenu)
        printmenu.Destroy()

    def OnCloseWindow(self, event):
        """
        Window closed
        Also close associated layer tree page
        """
        pgnum = None
        self.Map.Clean()
        
        # close edited map properly
        digitToolbar = self.toolbars['vdigit']
        if digitToolbar:
            maplayer = digitToolbar.GetLayer()
            if maplayer:
                self.toolbars['vdigit'].OnExit()
                self.imgVectorMap = None
        
        if self.page:
            pgnum = self.layerbook.GetPageIndex(self.page)
            if pgnum > -1:
                self.layerbook.DeletePage(pgnum)
        
        ### self.Destroy()
        # if event:
        #    event.Skip() <- causes application crash
        
    def GetRender(self):
        """
        Returns the current instance of render.Map()
        """
        return self.Map

    def OnQueryDisplay(self, event):
        """
        Query currrent raster/vector map layers (display mode)
        """
        if self.toolbars['map'].GetAction() == 'displayAttrb': # select previous action
            self.toolbars['map'].SelectDefault(event)
            return

        self.toolbars['map'].action['desc'] = 'displayAttrb'
        
        # switch GIS Manager to output console to show query results
        self.gismanager.notebook.SetSelection(1)

        self.MapWindow.mouse['use'] = "query"
        self.MapWindow.mouse['box'] = "point"
        self.MapWindow.zoomtype = 0

        # change the cursor
        self.MapWindow.SetCursor(self.cursors["cross"])

    def OnQueryModify(self, event):
        """
        Query vector map layer (edit mode)
        """
        if self.toolbars['map'].GetAction() == 'modifyAttrb': # select previous action
            self.toolbars['map'].SelectDefault(event)
            return
        
        self.toolbars['map'].action['desc'] = 'modifyAttrb'
        
        self.MapWindow.mouse['use'] = "queryVector"
        self.MapWindow.mouse['box'] = "box"
        self.MapWindow.pen = wx.Pen(colour='Red', width=2, style=wx.SHORT_DASH)
        self.MapWindow.zoomtype = 0

        # change the cursor
        self.MapWindow.SetCursor(self.cursors["cross"])
        
    def QueryMap(self, x, y):
        """
        Query map layer features

        Currently only raster and vector map layers are supported
        """
        #set query snap distance for v.what at mapunit equivalent of 10 pixels
        qdist = 10.0 * ((self.Map.region['e'] - self.Map.region['w']) / self.Map.width)
        east, north = self.MapWindow.Pixel2Cell((x, y))

        if not self.tree.layer_selected:
            dlg = wx.MessageDialog(parent=self,
                                   message=_('No map layer selected for querying.'),
                                   caption=_('Message'),
                                   style=wx.OK | wx.ICON_INFORMATION)
            dlg.ShowModal()
            dlg.Destroy()
            return

        mapname = None
        raststr = ''
        vectstr = ''
        rcmd = []
        vcmd = []
        for layer in self.tree.GetSelections():
            type = self.tree.GetPyData(layer)[0]['maplayer'].type
            dcmd = self.tree.GetPyData(layer)[0]['cmd']
            name = utils.GetLayerNameFromCmd(dcmd)
            if name == '':
                continue
            if type in ('raster', 'rgb', 'his'):
                raststr += "%s," % name
            elif type in ('vector', 'thememap', 'themechart'):
                vectstr += "%s," % name

        # use display region settings instead of computation region settings
        tmpreg = os.getenv("GRASS_REGION")
        os.environ["GRASS_REGION"] = self.Map.SetRegion(windres=False)
        
        # build query commands for any selected rasters and vectors
        if raststr != '':
            rcmd = ['r.what', '--q',
                    '-f',
                    'input=%s' % raststr.rstrip(','),
                    'east_north=%f,%f' % (float(east), float(north))]

        if vectstr != '':
            # check for vector maps open to be edited
            digitToolbar = self.toolbars['vdigit']
            if digitToolbar:
                map = digitToolbar.GetLayer().GetName()
                vect = []
                for vector in vectstr.split(','):
                    if map == vector:
                        self.gismanager.goutput.WriteWarning("Vector map <%s> "
                                                             "opened for editing - skipped." % map)
                        continue
                    vect.append(vector)
                vectstr = ','.join(vect)
                
            if len(vectstr) <= 1:
                self.gismanager.goutput.WriteCmdLog("Nothing to query.")
                return
            
            vcmd = ['v.what', '--q',
                    '-a',
                    'map=%s' % vectstr.rstrip(','),
                    'east_north=%f,%f' % (float(east), float(north)),
                    'distance=%f' % qdist]

        # parse query command(s)
        if self.gismanager:
            if rcmd:
                self.gismanager.goutput.RunCmd(rcmd, compReg=False)
            if vcmd:
                self.gismanager.goutput.RunCmd(vcmd)
        else:
            if rcmd:
                gcmd.Command(rcmd)
            if vcmd:
                gcmd.Command(vcmd)

        # restore GRASS_REGION
        if tmpreg:
            os.environ["GRASS_REGION"] = tmpreg
        
    def QueryVector(self, x, y):
        """
        Query vector map layer features

        Attribute data of selected vector object are displayed in GUI dialog.
        Data can be modified (On Submit)
        """
        if not self.tree.layer_selected or \
                self.tree.GetPyData(self.tree.layer_selected)[0]['type'] != 'vector':
            wx.MessageBox(parent=self,
                          message=_("No vector map selected for querying."),
                          caption=_("Message"),
                          style=wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
            return 
        
        posWindow = self.ClientToScreen((x + self.MapWindow.dialogOffset,
                                         y + self.MapWindow.dialogOffset))

        qdist = 10.0 * ((self.Map.region['e'] - self.Map.region['w']) / \
                            self.Map.width)
        east, north = self.MapWindow.Pixel2Cell((x, y))

        mapName = self.tree.GetPyData(self.tree.layer_selected)[0]['maplayer'].name
        
        if self.dialogs['attributes'] is None:
            self.dialogs['attributes'] = dbm.DisplayAttributesDialog(parent=self.MapWindow,
                                                                     map=mapName,
                                                                     query=((east, north), qdist),
                                                                     pos=posWindow,
                                                                     action="update")
        else:
            # selection changed?
            if not self.dialogs['attributes'].mapDBInfo or \
                   self.dialogs['attributes'].mapDBInfo.map != mapName:
                self.dialogs['attributes'].UpdateDialog(map=mapName, query=((east, north), qdist))
            else:
                self.dialogs['attributes'].UpdateDialog(query=((east, north), qdist))

        cats = self.dialogs['attributes'].GetCats()
        try:
            qlayer = self.Map.GetListOfLayers(l_name=globalvar.QUERYLAYER)[0]
        except IndexError:
            qlayer = None
        
        if self.dialogs['attributes'].mapDBInfo and cats:
            # highlight feature & re-draw map
            if qlayer:
                qlayer.SetCmd(self.AddTmpVectorMapLayer(mapName, cats,
                                                        useId=False,
                                                        addLayer=False))
            else:
                self.AddTmpVectorMapLayer(mapName, cats, useId=False)

            self.MapWindow.UpdateMap(render=False, renderVector=False)
            if not self.dialogs['attributes'].IsShown():
                self.dialogs['attributes'].Show()
        else:
            if qlayer:
                self.Map.DeleteLayer(qlayer)
                self.MapWindow.UpdateMap(render=False, renderVector=False)
            if self.dialogs['attributes'].IsShown():
                self.dialogs['attributes'].Hide()

    def OnQuery(self, event):
        """Query tools menu"""
        if self.toolbars['map']:
            self.toolbars['map'].OnTool(event)
            action = self.toolbars['map'].GetAction()
        
        point = wx.GetMousePosition()
        toolsmenu = wx.Menu()
        # Add items to the menu
        display = wx.MenuItem(parentMenu=toolsmenu, id=wx.ID_ANY,
                              text=_("Query raster/vector map(s) (display mode)"),
                              kind=wx.ITEM_CHECK)
        toolsmenu.AppendItem(display)
        self.Bind(wx.EVT_MENU, self.OnQueryDisplay, display)
        if action == "displayAttrb":
            display.Check(True)
        
        modify = wx.MenuItem(parentMenu=toolsmenu, id=wx.ID_ANY,
                             text=_("Query vector map (edit mode)"),
                             kind=wx.ITEM_CHECK)
        toolsmenu.AppendItem(modify)
        self.Bind(wx.EVT_MENU, self.OnQueryModify, modify)
        digitToolbar = self.toolbars['vdigit']
        if self.tree.layer_selected:
            layer_selected = self.tree.GetPyData(self.tree.layer_selected)[0]['maplayer']
            if digitToolbar and \
                   digitToolbar.GetLayer() == layer_selected:
                modify.Enable(False)
            else:
                if action == "modifyAttrb":
                    modify.Check(True)
        
        self.PopupMenu(toolsmenu)
        toolsmenu.Destroy()

    def AddTmpVectorMapLayer(self, name, cats, useId=False, addLayer=True):
        """
        Add temporal vector map layer to map composition

        @param name name of map layer
        @param useId use feature id instead of category 
        """
        # color settings from ATM
        color = UserSettings.Get(group='atm', key='highlight', subkey='color')
        colorStr = str(color[0]) + ":" + \
        str(color[1]) + ":" + \
        str(color[2])

        pattern = ["d.vect",
                   "map=%s" % name,
                   "color=%s" % colorStr,
                   "fcolor=%s" % colorStr,
                   "width=%d"  % UserSettings.Get(group='atm', key='highlight', subkey='width')]
        
        if useId:
            cmd = pattern
            cmd.append('-i')
            cmd.append('cats=%s' % str(cats))
        else:
            cmd = []
            for layer in cats.keys():
                cmd.append(copy.copy(pattern))
                lcats = cats[layer]
                cmd[-1].append("layer=%d" % layer)
                cmd[-1].append("cats=%s" % utils.ListOfCatsToRange(lcats))
        
        #     if self.icon:
        #         cmd.append("icon=%s" % (self.icon))
        #     if self.pointsize:
        #         cmd.append("size=%s" % (self.pointsize))

        if addLayer:
            if useId:
                return self.Map.AddLayer(type='vector', name=globalvar.QUERYLAYER, command=cmd,
                                         l_active=True, l_hidden=True, l_opacity=1.0)
            else:
                return self.Map.AddLayer(type='command', name=globalvar.QUERYLAYER, command=cmd,
                                         l_active=True, l_hidden=True, l_opacity=1.0)
        else:
            return cmd

    def OnAnalyze(self, event):
        """
        Analysis tools menu
        """
        point = wx.GetMousePosition()
        toolsmenu = wx.Menu()
        # Add items to the menu
        measure = wx.MenuItem(toolsmenu, wx.ID_ANY, Icons["measure"].GetLabel())
        measure.SetBitmap(Icons["measure"].GetBitmap(self.iconsize))
        toolsmenu.AppendItem(measure)
        self.Bind(wx.EVT_MENU, self.OnMeasure, measure)

        profile = wx.MenuItem(toolsmenu, wx.ID_ANY, Icons["profile"].GetLabel())
        profile.SetBitmap(Icons["profile"].GetBitmap(self.iconsize))
        toolsmenu.AppendItem(profile)
        self.Bind(wx.EVT_MENU, self.Profile, profile)

        histogram = wx.MenuItem(toolsmenu, wx.ID_ANY, Icons["histogram"].GetLabel())
        histogram.SetBitmap(Icons["histogram"].GetBitmap(self.iconsize))
        toolsmenu.AppendItem(histogram)
        self.Bind(wx.EVT_MENU, self.Histogram, histogram)

        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(toolsmenu)
        toolsmenu.Destroy()

    def OnMeasure(self, event):
        """
        Init measurement routine that calculates
        map distance along transect drawn on
        map display
        """

        self.totaldist = 0.0 # total measured distance

        # switch GIS Manager to output console to show measure results
        self.gismanager.notebook.SetSelection(1)

        # change mouse to draw line for measurement
        self.MapWindow.mouse['use'] = "measure"
        self.MapWindow.mouse['box'] = "line"
        self.MapWindow.zoomtype = 0
        self.MapWindow.pen     = wx.Pen(colour='red', width=2, style=wx.SHORT_DASH)
        self.MapWindow.polypen = wx.Pen(colour='green', width=2, style=wx.SHORT_DASH)

        # change the cursor
        self.MapWindow.SetCursor(self.cursors["pencil"])

        # initiating output
        style = self.gismanager.goutput.cmd_output.StyleWarning
        self.gismanager.goutput.WriteLog(_('Click and drag with left mouse button '
                                           'to measure.%s'
                                           'Double click with left button to clear.') % \
                                             (os.linesep), style)
        if self.Map.projinfo['proj'] != 'xy':
            units = self.Map.projinfo['units']
            style = self.gismanager.goutput.cmd_output.StyleCommand
            self.gismanager.goutput.WriteLog(_('Measuring distance') + ' ('
                                             + units + '):',
                                             style)
        else:
            self.gismanager.goutput.WriteLog(_('Measuring distance:'),
                                             style)

    def MeasureDist(self, beginpt, endpt):
        """
        Calculate map distance from screen distance
        and print to output window
        """

        if self.gismanager.notebook.GetSelection() != 1:
            self.gismanager.notebook.SetSelection(1)

        dist, (north, east) = self.MapWindow.Distance(beginpt, endpt)

        dist = round(dist, 3)
        d, dunits = self.FormatDist(dist)

        self.totaldist += dist
        td, tdunits = self.FormatDist(self.totaldist)

        strdist = str(d)
        strtotdist = str(td)

        if self.Map.projinfo['proj'] == 'xy' or 'degree' not in self.Map.projinfo['unit']:
            angle = int(math.degrees(math.atan2(north,east)) + 0.5)
            angle = angle+90
            if angle < 0: angle = 360+angle

            mstring = 'segment = %s %s\ttotal distance = %s %s\tbearing = %d deg' \
                % (strdist,dunits,strtotdist,tdunits,angle)
        else:
            mstring = 'segment = %s %s\ttotal distance = %s %s' \
                % (strdist,dunits,strtotdist,tdunits)

        self.gismanager.goutput.WriteLog(mstring)

        return dist

    def Profile(self, event):
        """
        Init profile canvas and tools
        """
        raster = []
        if self.tree.layer_selected and \
               self.tree.GetPyData(self.tree.layer_selected)[0]['type'] == 'raster':
            raster.append(self.tree.GetPyData(self.tree.layer_selected)[0]['maplayer'].name)

        self.profile = profile.ProfileFrame(self,
                                            id=wx.ID_ANY, pos=wx.DefaultPosition, size=(700,300),
                                            style=wx.DEFAULT_FRAME_STYLE, rasterList=raster)
        self.profile.Show()

    def FormatDist(self, dist):
        """Format length numbers and units in a nice way,
        as a function of length. From code by Hamish Bowman
        Grass Development Team 2006"""

        mapunits = self.Map.projinfo['units']
        if mapunits == 'metres': mapunits = 'meters'
        outunits = mapunits
        dist = float(dist)
        divisor = 1.0

        # figure out which units to use
        if mapunits == 'meters':
            if dist > 2500.0:
                outunits = 'km'
                divisor = 1000.0
            else: outunits = 'm'
        elif mapunits == 'feet':
            # nano-bug: we match any "feet", but US Survey feet is really
            #  5279.9894 per statute mile, or 10.6' per 1000 miles. As >1000
            #  miles the tick markers are rounded to the nearest 10th of a
            #  mile (528'), the difference in foot flavours is ignored.
            if dist > 5280.0:
                outunits = 'miles'
                divisor = 5280.0
            else:
                outunits = 'ft'
        elif 'degree' in mapunits:
            if dist < 1:
                outunits = 'min'
                divisor = (1/60.0)
            else:
                outunits = 'deg'

        # format numbers in a nice way
        if (dist/divisor) >= 2500.0:
            outdist = round(dist/divisor)
        elif (dist/divisor) >= 1000.0:
            outdist = round(dist/divisor,1)
        elif (dist/divisor) > 0.0:
            outdist = round(dist/divisor,int(math.ceil(3-math.log10(dist/divisor))))
        else:
            outdist = float(dist/divisor)

        return (outdist, outunits)


    def Histogram(self, event):
        """
        Init histogram display canvas and tools
        """
        self.histogram = histogram.HistFrame(self,
                                             id=wx.ID_ANY, size=globalvar.HIST_WINDOW_SIZE,
                                             style=wx.DEFAULT_FRAME_STYLE)

        #show new display
        self.histogram.Show()
        self.histogram.Refresh()
        self.histogram.Update()


    def OnDecoration(self, event):
        """
        Decorations overlay menu
        """
        point = wx.GetMousePosition()
        decmenu = wx.Menu()
        # Add items to the menu
        AddScale = wx.MenuItem(decmenu, wx.ID_ANY, Icons["addbarscale"].GetLabel())
        AddScale.SetBitmap(Icons["addbarscale"].GetBitmap(self.iconsize))
        decmenu.AppendItem(AddScale)
        self.Bind(wx.EVT_MENU, self.OnAddBarscale, AddScale)

        AddLegend = wx.MenuItem(decmenu, wx.ID_ANY, Icons["addlegend"].GetLabel())
        AddLegend.SetBitmap(Icons["addlegend"].GetBitmap(self.iconsize))
        decmenu.AppendItem(AddLegend)
        self.Bind(wx.EVT_MENU, self.OnAddLegend, AddLegend)

        AddText = wx.MenuItem(decmenu, wx.ID_ANY, Icons["addtext"].GetLabel())
        AddText.SetBitmap(Icons["addtext"].GetBitmap(self.iconsize))
        decmenu.AppendItem(AddText)
        self.Bind(wx.EVT_MENU, self.OnAddText, AddText)

        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(decmenu)
        decmenu.Destroy()

    def OnAddBarscale(self, event):
        """
        Handler for scale/arrow map decoration menu selection.
        """
        if self.dialogs['barscale']:
            return

        id = 0 # unique index for overlay layer

        # If location is latlon, only display north arrow (scale won't work)
        #        proj = self.Map.projinfo['proj']
        #        if proj == 'll':
        #            barcmd = 'd.barscale -n'
        #        else:
        #            barcmd = 'd.barscale'

        # decoration overlay control dialog
        self.dialogs['barscale'] = \
            gdialogs.DecorationDialog(parent=self, title=_('Scale and North arrow'),
                                      size=(350, 200),
                                      style=wx.DEFAULT_DIALOG_STYLE | wx.CENTRE,
                                      cmd=['d.barscale'],
                                      ovlId=id,
                                      name='barscale',
                                      checktxt = _("Show/hide scale and North arrow"),
                                      ctrltxt = _("scale object"))

        self.dialogs['barscale'].Show()

    def OnAddLegend(self, event):
        """
        Handler for legend map decoration menu selection.
        """
        if self.dialogs['legend']:
            return
        
        id = 1 # index for overlay layer in render

        cmd = ['d.legend']
        if self.tree.layer_selected and \
               self.tree.GetPyData(self.tree.layer_selected)[0]['type'] == 'raster':
            cmd.append('map=%s' % self.tree.GetPyData(self.tree.layer_selected)[0]['maplayer'].name)

        # Decoration overlay control dialog
        self.dialogs['legend'] = \
            gdialogs.DecorationDialog(parent=self, title=('Legend'),
                                      size=(350, 200),
                                      style=wx.DEFAULT_DIALOG_STYLE | wx.CENTRE,
                                      cmd=cmd,
                                      ovlId=id,
                                      name='legend',
                                      checktxt = _("Show/hide legend"),
                                      ctrltxt = _("legend object")) 

        self.dialogs['legend'].Show()

    def OnAddText(self, event):
        """
        Handler for text decoration menu selection.
        """
        if self.MapWindow.dragid > -1:
            id = self.MapWindow.dragid
        else:
            # index for overlay layer in render
            if len(self.MapWindow.textdict.keys()) > 0:
                id = self.MapWindow.textdict.keys()[-1] + 1
            else:
                id = 101

        dlg = gdialogs.TextLayerDialog(parent=self, ovlId=id, title=_('Add text layer'),
                                       size=(400, 200))

        dlg.CenterOnParent()

        # If OK button pressed in decoration control dialog
        if dlg.ShowModal() == wx.ID_OK:
            text = dlg.GetValues()['text']
            coords, w, h = self.MapWindow.TextBounds(dlg.GetValues())
        
            # delete object if it has no text
            if text == '':
                try:
                    self.MapWindow.pdc.ClearId(id)
                    self.MapWindow.pdc.RemoveId(id)
                    del self.MapWindow.textdict[id]
                except:
                    pass
                return

            self.MapWindow.pdc.ClearId(id)
            self.MapWindow.pdc.SetId(id)
            self.MapWindow.textdict[id] = dlg.GetValues()
            
            self.MapWindow.Draw(self.MapWindow.pdcDec, img=self.MapWindow.textdict[id],
                                drawid=id, pdctype='text', coords=coords)
            
            self.MapWindow.UpdateMap(render=False, renderVector=False)

    def GetOptData(self, dcmd, type, params, propwin):
        """
        Callback method for decoration overlay command generated by
        dialog created in menuform.py
        """
        # Reset comand and rendering options in render.Map. Always render decoration.
        # Showing/hiding handled by PseudoDC
        self.Map.ChangeOverlay(ovltype=type, type='overlay', name='', command=dcmd,
                               l_active=True, l_render=False)
        self.params[type] = params
        self.propwin[type] = propwin

    def OnZoomMenu(self, event):
        """
        Zoom menu
        """
        point = wx.GetMousePosition()
        zoommenu = wx.Menu()
        # Add items to the menu
        zoommap = wx.MenuItem(zoommenu, wx.ID_ANY, _('Zoom to selected map'))
        zoommenu.AppendItem(zoommap)
        self.Bind(wx.EVT_MENU, self.MapWindow.OnZoomToMap, zoommap)

        zoomwind = wx.MenuItem(zoommenu, wx.ID_ANY, _('Zoom to computational region (set with g.region)'))
        zoommenu.AppendItem(zoomwind)
        self.Bind(wx.EVT_MENU, self.MapWindow.ZoomToWind, zoomwind)

        zoomdefault = wx.MenuItem(zoommenu, wx.ID_ANY, _('Zoom to default region'))
        zoommenu.AppendItem(zoomdefault)
        self.Bind(wx.EVT_MENU, self.MapWindow.ZoomToDefault, zoomdefault)

        zoomsaved = wx.MenuItem(zoommenu, wx.ID_ANY, _('Zoom to saved region'))
        zoommenu.AppendItem(zoomsaved)
        self.Bind(wx.EVT_MENU, self.MapWindow.ZoomToSaved, zoomsaved)

        savewind = wx.MenuItem(zoommenu, wx.ID_ANY, _('Set computational region from display'))
        zoommenu.AppendItem(savewind)
        self.Bind(wx.EVT_MENU, self.MapWindow.DisplayToWind, savewind)

        savezoom = wx.MenuItem(zoommenu, wx.ID_ANY, _('Save display geometry to named region'))
        zoommenu.AppendItem(savezoom)
        self.Bind(wx.EVT_MENU, self.MapWindow.SaveDisplayRegion, savezoom)

        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(zoommenu)
        zoommenu.Destroy()

    def SetProperties(self, render=False, mode=0, showCompExtent=False,
                      constrainRes=False):
        """Set properies of map display window"""
        self.autoRender.SetValue(render)
        self.toggleStatus.SetSelection(mode)
        self.StatusbarUpdate()
        self.showRegion.SetValue(showCompExtent)
        self.compResolution.SetValue(constrainRes)
        if showCompExtent:
            self.MapWindow.regionCoords = []
        
# end of class MapFrame

class MapApp(wx.App):
    """
    MapApp class
    """

    def OnInit(self):
        wx.InitAllImageHandlers()
        if __name__ == "__main__":
            Map = render.Map() # instance of Map class to render GRASS display output to PPM file
        else:
            Map = None

        self.mapFrm = MapFrame(parent=None, id=wx.ID_ANY, Map=Map, size=(640,480))
        #self.SetTopWindow(Map)
        self.mapFrm.Show()

        if __name__ == "__main__":
            # redraw map, if new command appears
            self.redraw = False
            status = Command(self, Map)
            status.start()
            self.timer = wx.PyTimer(self.watcher)
            # check each 0.1s
            self.timer.Start(100)

        return 1

    def OnExit(self):
        if __name__ == "__main__":
            # stop the timer
            self.timer.Stop()
            # terminate thread (a bit ugly)
            os.system("""echo "quit" >> %s""" % (cmdfilename))

    def watcher(self):
        """Redraw, if new layer appears"""
        if self.redraw:
            self.mapFrm.OnDraw(None)
        self.redraw = False
        return
# end of class MapApp

if __name__ == "__main__":

    ###### SET command variable
    if len(sys.argv) != 3:
        print __doc__
        sys.exit()

    title = sys.argv[1]
    cmdfilename = sys.argv[2]

    import gettext
    gettext.install("gm_map") # replace with the appropriate catalog name

    print "Starting monitor <%s>" % (title)

    gm_map = MapApp(0)
    # set title
    gm_map.mapFrm.SetTitle ("GRASS GIS - Map Display: " + title + " - Location: " + \
                                grassenv.GetGRASSVariable("LOCATION_NAME"))
    gm_map.MainLoop()

    os.remove(cmdfilename)
    os.system("""g.gisenv set="GRASS_PYCMDFILE" """)

    print "Stoping monitor <%s>" % (title)

    sys.exit()
