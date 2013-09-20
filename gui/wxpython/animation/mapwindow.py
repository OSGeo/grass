"""!
@package animation.mapwindow

@brief Animation window and bitmaps management

Classes:
 - mapwindow::BufferedWindow
 - mapwindow::AnimationWindow
 - mapwindow::BitmapProvider
 - mapwindow::BitmapPool

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""
import os
import wx
from multiprocessing import Process, Queue
import tempfile
import grass.script as grass
from core.gcmd import RunCommand, GException
from core.debug import Debug
from core.settings import UserSettings
from core.utils import _, CmdToTuple, autoCropImageFromFile

from grass.pydispatch.signal import Signal

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
        kwargs['style'] = kwargs.setdefault('style', wx.NO_FULL_REPAINT_ON_RESIZE) | wx.NO_FULL_REPAINT_ON_RESIZE
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
        size  = self.GetClientSize()

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
        del dc # need to get rid of the MemoryDC before Update() is called.
        self.Refresh()
        self.Update()


class AnimationWindow(BufferedWindow):
    def __init__(self, parent, id=wx.ID_ANY,
                 style = wx.DEFAULT_FRAME_STYLE | wx.FULL_REPAINT_ON_RESIZE | wx.BORDER_RAISED):
        Debug.msg(2, "AnimationWindow.__init__()")

        self.bitmap = wx.EmptyBitmap(1, 1)
        self.text = ''
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

        dc.Clear() # make sure you clear the bitmap!
        dc.DrawBitmap(self.bitmap, x=0, y=0)
        dc.DrawText(self.text, 0, 0)

    def OnSize(self, event):
        Debug.msg(5, "AnimationWindow.OnSize()")

        self.DrawBitmap(self.bitmap, self.text)
        
        BufferedWindow.OnSize(self, event)
        if event:
            event.Skip()

    def DrawBitmap(self, bitmap, text):
        """!Draws bitmap.
        Does not draw the bitmap if it is the same one as last time.
        """
        if self.bitmap == bitmap:
            return

        self.bitmap = bitmap
        self.text = text
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


class BitmapProvider(object):
    """!Class responsible for loading data and providing bitmaps"""
    def __init__(self, frame, bitmapPool, imageWidth=640, imageHeight=480, nprocs=4):

        self.datasource = None
        self.dataNames = None
        self.dataType = None
        self.bitmapPool = bitmapPool
        self.frame = frame
        self.imageWidth = imageWidth # width of the image to render with d.rast or d.vect
        self.imageHeight = imageHeight # height of the image to render with d.rast or d.vect
        self.nprocs = nprocs # Number of procs to be used for rendering

        self.suffix = ''
        self.nvizRegion = None
        
        self.mapsLoaded = Signal('mapsLoaded')

    def GetDataNames(self):
        return self.dataNames

    def SetData(self, datasource, dataNames = None, dataType = 'rast',
                suffix = '', nvizRegion = None):
        """!Sets data.

        @param datasource data to load (raster maps, vector maps, m.nviz.image commands)
        @param dataNames data labels (keys)
        @param dataType 'rast', 'vect', 'nviz'
        @param nvizRegion region which must be set for m.nviz.image
        """
        self.datasource = datasource
        self.dataType = dataType
        self.suffix = suffix
        self.nvizRegion = nvizRegion
        
        if dataNames:
            self.dataNames = dataNames
        else:
            self.dataNames = datasource

        self.dataNames = [name + self.suffix for name in self.dataNames]

    def GetBitmap(self, dataId):
        """!Returns bitmap with given key
        or 'no data' bitmap if no such key exists.

        @param dataId name of bitmap
        """
        if dataId:
            dataId += self.suffix
        try:
            bitmap = self.bitmapPool[dataId]
        except KeyError:
            bitmap = self.bitmapPool[None]
        return bitmap

    def WindowSizeChanged(self, width, height):
        """!Sets size when size of related window changes."""
        self.imageWidth, self.imageHeight = width, height

    def _createNoDataBitmap(self, width, height):
        """!Creates 'no data' bitmap.

        Used when requested bitmap is not available (loading data was not successful) or 
        we want to show 'no data' bitmap.
        """
        bitmap = wx.EmptyBitmap(width, height)
        dc = wx.MemoryDC()
        dc.SelectObject(bitmap)
        dc.Clear()
        text = _("No data")
        dc.SetFont(wx.Font(pointSize = 40, family = wx.FONTFAMILY_SCRIPT,
                           style = wx.FONTSTYLE_NORMAL, weight = wx.FONTWEIGHT_BOLD))
        tw, th = dc.GetTextExtent(text)
        dc.DrawText(text, (width-tw)/2,  (height-th)/2)
        dc.SelectObject(wx.NullBitmap)
        return bitmap

    def Load(self, force = False, nprocs=4):
        """!Loads data.

        Shows progress dialog.

        @param force if True reload all data, otherwise only missing data
        @param imageWidth width of the image to render with d.rast or d.vect
        @param imageHeight height of the image to render with d.rast or d.vect
        @param nprocs number of procs to be used for rendering
        """
        if nprocs <= 0:
            nprocs = 1

        count, maxLength = self._dryLoad(rasters = self.datasource,
                                         names = self.dataNames, force = force)
        progress = None
        if self.dataType in ('rast', 'vect', 'strds', 'stvds') and count > 5 or \
            self.dataType == 'nviz':
            progress = wx.ProgressDialog(title = "Loading data",
                                         message = " " * (maxLength + 20), # ?
                                         maximum = count,
                                         parent = self.frame,
                                         style = wx.PD_CAN_ABORT | wx.PD_APP_MODAL |
                                                 wx.PD_AUTO_HIDE | wx.PD_SMOOTH)
            updateFunction = progress.Update
        else:
            updateFunction = None

        if self.dataType in ('rast', 'vect', 'strds', 'stvds'):
            self._loadMaps(mapType=self.dataType, maps = self.datasource, names = self.dataNames,
                           force = force, updateFunction = updateFunction,
                           imageWidth=self.imageWidth, imageHeight=self.imageHeight, nprocs=nprocs)
        elif self.dataType == 'nviz':
            self._load3D(commands = self.datasource, region = self.nvizRegion, names = self.dataNames,
                         force = force, updateFunction = updateFunction)
        if progress:
            progress.Destroy()

        self.mapsLoaded.emit()

    def Unload(self):
        self.datasource = None
        self.dataNames = None
        self.dataType = None

    def _dryLoad(self, rasters, names, force):
        """!Tries how many bitmaps will be loaded.
        Used for progress dialog.

        @param rasters raster maps to be loaded
        @param names names used as keys for bitmaps
        @param force load everything even though it is already there
        """
        count = 0
        maxLength = 0
        for raster, name in zip(rasters, names):
            if not(name in self.bitmapPool and force is False):
                count += 1
                if len(raster) > maxLength:
                    maxLength = len(raster)

        return count, maxLength

    
    def _loadMaps(self, mapType, maps, names, force, updateFunction,
                  imageWidth, imageHeight, nprocs):
        """!Loads rasters/vectors (also from temporal dataset).

        Uses d.rast/d.vect and multiprocessing for parallel rendering

        @param mapType Must be "rast" or "vect"
        @param maps raster or vector maps to be loaded
        @param names names used as keys for bitmaps
        @param force load everything even though it is already there
        @param updateFunction function called for updating progress dialog
        @param imageWidth width of the image to render with d.rast or d.vect
        @param imageHeight height of the image to render with d.rast or d.vect
        @param nprocs number of procs to be used for rendering
        """

        count = 0

        # Variables for parallel rendering
        proc_count = 0
        proc_list = []
        queue_list = []
        name_list = []

        mapNum = len(maps)

        # create no data bitmap
        if None not in self.bitmapPool or force:
            self.bitmapPool[None] = self._createNoDataBitmap(imageWidth, imageHeight)

        for mapname, name in zip(maps, names):
            count += 1

            if name in self.bitmapPool and force is False:
                continue

            # Queue object for interprocess communication
            q = Queue()
            # The separate render process
            p = Process(target=mapRenderProcess, args=(mapType, mapname, imageWidth, imageHeight, q))
            p.start()

            queue_list.append(q)
            proc_list.append(p)
            name_list.append(name)

            proc_count += 1

            # Wait for all running processes and read/store the created images
            if proc_count == nprocs or count == mapNum:
                for i in range(len(name_list)):
                    proc_list[i].join()
                    filename = queue_list[i].get()

                    # Unfortunately the png files must be read here, 
                    # since the swig wx objects can not be serialized by the Queue object :(
                    if filename == None:
                        self.bitmapPool[name_list[i]] = wx.EmptyBitmap(imageWidth, imageHeight)
                    else:
                        self.bitmapPool[name_list[i]] = wx.BitmapFromImage(wx.Image(filename))
                        os.remove(filename)

                proc_count = 0
                proc_list = []
                queue_list = []
                name_list = []

            if updateFunction:
                keepGoing, skip = updateFunction(count, mapname)
                if not keepGoing:
                    break

    def _load3D(self, commands, region, names, force, updateFunction):
        """!Load 3D view images using m.nviz.image.

        @param commands 
        @param region 
        @param names names used as keys for bitmaps
        @param force load everything even though it is already there
        @param updateFunction function called for updating progress dialog
        """
        ncols, nrows = self.imageWidth, self.imageHeight
        count = 0
        format = 'ppm'
        tempFile = grass.tempfile(False)
        tempFileFormat = tempFile + '.' + format

        os.environ['GRASS_REGION'] = grass.region_env(**region)
        # create no data bitmap
        if None not in self.bitmapPool or force:
            self.bitmapPool[None] = self._createNoDataBitmap(ncols, nrows)
        for command, name in zip(commands, names):
            if name in self.bitmapPool and force is False:
                continue
            count += 1
            # set temporary file
            command[1]['output'] = tempFile
            # set size
            command[1]['size'] = '%d,%d' % (ncols, nrows)
            # set format
            command[1]['format'] = format

            returncode, messages = RunCommand(getErrorMsg = True, prog = command[0], **command[1])
            if returncode != 0:
                self.bitmapPool[name] = wx.EmptyBitmap(ncols, nrows)
                continue

            self.bitmapPool[name] = wx.Bitmap(tempFileFormat)

            if updateFunction:
                keepGoing, skip = updateFunction(count, name)
                if not keepGoing:
                    break
        grass.try_remove(tempFileFormat)
        os.environ.pop('GRASS_REGION')

    def LoadOverlay(self, cmd):
        """!Creates raster legend with d.legend

        @param cmd d.legend command as a list

        @return bitmap with legend
        """
        fileHandler, filename = tempfile.mkstemp(suffix=".png")
        os.close(fileHandler)
        # Set the environment variables for this process
        _setEnvironment(self.imageWidth, self.imageHeight, filename, transparent=True)

        Debug.msg(1, "Render raster legend " + str(filename))
        cmdTuple = CmdToTuple(cmd)
        returncode, stdout, messages = read2_command(cmdTuple[0], **cmdTuple[1])

        if returncode == 0:
            return wx.BitmapFromImage(autoCropImageFromFile(filename))
        else:
            os.remove(filename)
            raise GException(messages)


def mapRenderProcess(mapType, mapname, width, height, fileQueue):
    """!Render raster or vector files as png image and write the 
       resulting png filename in the provided file queue
    
    @param mapType Must be "rast" or "vect"
    @param mapname raster or vector map name to be rendered
    @param width Width of the resulting image
    @param height Height of the resulting image
    @param fileQueue The inter process communication queue storing the file name of the image
    """
    
    # temporary file, we use python here to avoid calling g.tempfile for each render process
    fileHandler, filename = tempfile.mkstemp(suffix=".png")
    os.close(fileHandler)
    
    # Set the environment variables for this process
    _setEnvironment(width, height, filename, transparent=False)

    if mapType in ('rast', 'strds'):
        Debug.msg(1, "Render raster image " + str(filename))
        returncode, stdout, messages = read2_command('d.rast', map = mapname)
    elif mapType in ('vect', 'stvds'):
        Debug.msg(1, "Render vector image " + str(filename))
        returncode, stdout, messages = read2_command('d.vect', map = mapname)
    else:
        returncode = 1
        return

    if returncode != 0:
        fileQueue.put(None)
        os.remove(filename)
        return

    fileQueue.put(filename)
    

def _setEnvironment(width, height, filename, transparent):
    os.environ['GRASS_WIDTH'] = str(width)
    os.environ['GRASS_HEIGHT'] = str(height)
    driver = UserSettings.Get(group='display', key='driver', subkey='type')
    os.environ['GRASS_RENDER_IMMEDIATE'] = driver
    os.environ['GRASS_BACKGROUNDCOLOR'] = 'ffffff'
    os.environ['GRASS_TRUECOLOR'] = "TRUE"
    if transparent:
        os.environ['GRASS_TRANSPARENT'] = "TRUE"
    else:
        os.environ['GRASS_TRANSPARENT'] = "FALSE"
    os.environ['GRASS_PNGFILE'] = str(filename)


class BitmapPool():
    """!Class storing bitmaps (emulates dictionary)"""
    def __init__(self):
        self.bitmaps = {}

    def __getitem__(self, key):
        return self.bitmaps[key]

    def __setitem__(self, key, bitmap):
        self.bitmaps[key] = bitmap

    def __contains__(self, key):
        return key in self.bitmaps

    def Clear(self, usedKeys):
        """!Removes all bitmaps which are currently not used.

        @param usedKeys keys which are currently used
        """
        for key in self.bitmaps.keys():
            if key not in usedKeys and key is not None:
                del self.bitmaps[key]


def read2_command(*args, **kwargs):
    kwargs['stdout'] = grass.PIPE
    kwargs['stderr'] = grass.PIPE
    ps = grass.start_command(*args, **kwargs)
    stdout, stderr = ps.communicate()
    return ps.returncode, stdout, stderr
