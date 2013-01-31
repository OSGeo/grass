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

import grass.script as grass
from core.gcmd import RunCommand
from core.debug import Debug
from utils import ComputeScaledRect

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
        size  = self.ClientSize

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
    def __init__(self, parent, id = wx.ID_ANY, 
                 style = wx.DEFAULT_FRAME_STYLE | wx.FULL_REPAINT_ON_RESIZE | wx.BORDER_RAISED):
        Debug.msg(2, "AnimationWindow.__init__()")

        self.bitmap = wx.EmptyBitmap(1, 1)
        self.x = self.y = 0
        self.text = ''
        self.size = wx.Size()
        self.rescaleNeeded = False
        self.region = None
        self.parent = parent

        BufferedWindow.__init__(self, parent = parent, id = id, style = style)
        self.SetBackgroundColour(wx.BLACK)
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.Bind(wx.EVT_SIZE, self.OnSize)


    def Draw(self, dc):
        """!Draws bitmap."""
        Debug.msg(5, "AnimationWindow.Draw()")

        dc.Clear() # make sure you clear the bitmap!
        dc.DrawBitmap(self.bitmap, x = self.x, y = self.y)
        dc.DrawText(self.text, 0, 0)

    def OnSize(self, event):
        Debug.msg(5, "AnimationWindow.OnSize()")
        self._computeBitmapCoordinates()

        self.DrawBitmap(self.bitmap, self.text)
        
        BufferedWindow.OnSize(self, event)
        if event:
            event.Skip()
        
    def IsRescaled(self):
        return self.rescaleNeeded

    def _rescaleIfNeeded(self, bitmap):
        """!If the bitmap has different size than the window, rescale it."""
        bW, bH = bitmap.GetSize()
        wW, wH = self.size
        if abs(bW - wW) > 5 and abs(bH - wH) > 5:
            self.rescaleNeeded = True
            im = wx.ImageFromBitmap(bitmap)
            im.Rescale(*self.size)
            bitmap = wx.BitmapFromImage(im)
        else:
            self.rescaleNeeded = False
        return bitmap
        
    def DrawBitmap(self, bitmap, text):
        """!Draws bitmap.
        Does not draw the bitmap if it is the same one as last time.
        """
        bmp = self._rescaleIfNeeded(bitmap)
        if self.bitmap == bmp:
            return

        self.bitmap = bmp
        self.text = text
        self.UpdateDrawing()

    def _computeBitmapCoordinates(self):
        """!Computes where to place the bitmap
        to be in the center of the window."""
        if not self.region:
            return

        cols = self.region['cols']
        rows = self.region['rows']
        params = ComputeScaledRect((cols, rows), self.GetClientSize())
        self.x = params['x']
        self.y = params['y']
        self.size = (params['width'], params['height'])

    def SetRegion(self, region):
        """!Sets region for size computations.
        Region is set from outside to avoid calling g.region multiple times.
        """
        self.region = region
        self._computeBitmapCoordinates()

    def GetAdjustedSize(self):
        return self.size

    def GetAdjustedPosition(self):
        return self.x, self.y

class BitmapProvider(object):
    """!Class responsible for loading data and providing bitmaps"""
    def __init__(self, frame, bitmapPool):

        self.datasource = None
        self.dataNames = None
        self.dataType = None
        self.region = None
        self.bitmapPool = bitmapPool
        self.frame = frame
        self.size = wx.Size()
        self.loadSize = wx.Size()

        self.suffix = ''
        self.nvizRegion = None

    def GetDataNames(self):
        return self.dataNames

    def SetData(self, datasource, dataNames = None, dataType = 'rast',
                suffix = '', nvizRegion = None):
        """!Sets data.

        @param datasource data to load (raster maps, m.nviz.image commands)
        @param dataNames data labels (keys)
        @param dataType 'rast', 'nviz'
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

    def GetLoadSize(self):
        return self.loadSize

    def WindowSizeChanged(self, event, sizeMethod):
        """!Sets size when size of related window changes."""
        # sizeMethod is GetClientSize, must be used instead of GetSize
        self.size = sizeMethod()
        event.Skip()

    def _createNoDataBitmap(self, ncols, nrows):
        """!Creates 'no data' bitmap.

        Used when requested bitmap is not available (loading data was not successful) or 
        we want to show 'no data' bitmap.
        """
        bitmap = wx.EmptyBitmap(ncols, nrows)
        dc = wx.MemoryDC()
        dc.SelectObject(bitmap)
        dc.Clear()
        text = _("No data")
        dc.SetFont(wx.Font(pointSize = 40, family = wx.FONTFAMILY_SCRIPT,
                           style = wx.FONTSTYLE_NORMAL, weight = wx.FONTWEIGHT_BOLD))
        tw, th = dc.GetTextExtent(text)
        dc.DrawText(text, (ncols-tw)/2,  (nrows-th)/2)
        dc.SelectObject(wx.NullBitmap)
        return bitmap

    def Load(self, force = False):
        """!Loads data.

        Shows progress dialog.

        @param force if True reload all data, otherwise only missing data
        """
        count, maxLength = self._dryLoad(rasters = self.datasource,
                                         names = self.dataNames, force = force)
        progress = None
        if self.dataType == 'rast' and count > 5 or \
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

        if self.dataType == 'rast':
            size, scale = self._computeScale()
            # loading ...
            self._loadRasters(rasters = self.datasource, names = self.dataNames,
                             size = size, scale = scale, force = force, updateFunction = updateFunction)
        elif self.dataType == 'nviz':
            self._load3D(commands = self.datasource, region = self.nvizRegion, names = self.dataNames,
                         force = force, updateFunction = updateFunction)
        if progress:
            progress.Destroy()

    def Unload(self):
        self.datasource = None
        self.dataNames = None
        self.dataType = None

    def _computeScale(self):
        """!Computes parameters for creating bitmaps."""
        region = grass.region()
        ncols, nrows = region['cols'], region['rows']
        params = ComputeScaledRect((ncols, nrows), self.size)

        return ((params['width'], params['height']), params['scale'])

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

    def _loadRasters(self, rasters, names, size, scale, force, updateFunction):
        """!Loads rasters (also rasters from temporal dataset).

        Uses r.out.ppm.

        @param rasters raster maps to be loaded
        @param names names used as keys for bitmaps
        @param size size of new bitmaps
        @param scale used for adjustment of region resolution for r.out.ppm
        @param force load everything even though it is already there
        @param updateFunction function called for updating progress dialog
        """
        region = grass.region()
        for key in ('rows', 'cols', 'cells'):
            region.pop(key)
        # sometimes it renderes nonsense - depends on resolution
        # should we set the resolution of the raster?
        region['nsres'] /= scale
        region['ewres'] /= scale
        os.environ['GRASS_REGION'] = grass.region_env(**region)
        ncols, nrows = size
        self.loadSize = size
        count = 0

        # create no data bitmap
        if None not in self.bitmapPool or force:
            self.bitmapPool[None] = self._createNoDataBitmap(ncols, nrows)
        for raster, name in zip(rasters, names):
            if name in self.bitmapPool and force is False:
                continue
            count += 1
            # RunCommand has problem with DecodeString
            returncode, stdout, messages = read2_command('r.out.ppm', input = raster,
                                                         flags = 'h', output = '-', quiet = True)
            if returncode != 0:
                self.bitmapPool[name] = wx.EmptyBitmap(ncols, nrows)
                continue
                
            bitmap = wx.BitmapFromBuffer(ncols, nrows, stdout)
            self.bitmapPool[name] = bitmap

            if updateFunction:
                keepGoing, skip = updateFunction(count, raster)
                if not keepGoing:
                    break

        os.environ.pop('GRASS_REGION')

    def _load3D(self, commands, region, names, force, updateFunction):
        """!Load 3D view images using m.nviz.image.

        @param commands 
        @param region 
        @param names names used as keys for bitmaps
        @param force load everything even though it is already there
        @param updateFunction function called for updating progress dialog
        """
        ncols, nrows = self.size
        self.loadSize = ncols, nrows
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
        """!Removes all bitmaps which are currentlu not used.

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