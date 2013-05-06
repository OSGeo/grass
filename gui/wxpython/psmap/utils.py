"""!
@package psmap.utils

@brief utilities for wxpsmap (classes, functions)

Classes:
 - utils::Rect2D
 - utils::Rect2DPP
 - utils::Rect2DPS
 - utils::UnitConversion

(C) 2012 by Anna Kratochvilova, and the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""
import os
import wx
import string
from math import ceil, floor, sin, cos, pi

try:
    import Image as PILImage
    havePILImage = True
except ImportError:
    havePILImage = False

import grass.script as grass
from core.gcmd          import RunCommand

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

class UnitConversion:
    """! Class for converting units"""
    def __init__(self, parent = None):
        self.parent = parent
        if self.parent:
            ppi = wx.ClientDC(self.parent).GetPPI()
        else: 
            ppi = (72, 72)
        self._unitsPage = { 'inch'          : {'val': 1.0, 'tr' : _("inch")},
                            'point'         : {'val': 72.0, 'tr' : _("point")},
                            'centimeter'    : {'val': 2.54, 'tr' : _("centimeter")},
                            'millimeter'    : {'val': 25.4, 'tr' : _("millimeter")}}
        self._unitsMap = {  'meters'        : {'val': 0.0254, 'tr' : _("meters")},
                            'kilometers'    : {'val': 2.54e-5, 'tr' : _("kilometers")},
                            'feet'          : {'val': 1./12, 'tr' : _("feet")},
                            'miles'         : {'val': 1./63360, 'tr' : _("miles")},
                            'nautical miles': {'val': 1/72913.386, 'tr' : _("nautical miles")}}

        self._units = { 'pixel'     : {'val': ppi[0], 'tr' : _("pixel")},
                        'meter'     : {'val': 0.0254, 'tr' : _("meter")},
                        'nautmiles' : {'val': 1/72913.386, 'tr' :_("nautical miles")},
                        'degrees'   : {'val': 0.0254 , 'tr' : _("degree")} #like 1 meter, incorrect
                        }
        self._units.update(self._unitsPage)
        self._units.update(self._unitsMap)

    def getPageUnitsNames(self):
        return sorted(self._unitsPage[unit]['tr'] for unit in self._unitsPage.keys())
    
    def getMapUnitsNames(self):
        return sorted(self._unitsMap[unit]['tr'] for unit in self._unitsMap.keys())
    
    def getAllUnits(self):
        return sorted(self._units.keys())
    
    def findUnit(self, name):
        """!Returns unit by its tr. string"""
        for unit in self._units.keys():
            if self._units[unit]['tr'] == name:
                return unit
        return None
    
    def findName(self, unit):
        """!Returns tr. string of a unit"""
        try:
            return self._units[unit]['tr']
        except KeyError:
            return None
    
    def convert(self, value, fromUnit = None, toUnit = None):
        return float(value)/self._units[fromUnit]['val']*self._units[toUnit]['val']

def convertRGB(rgb):
    """!Converts wx.Colour(r,g,b,a) to string 'r:g:b' or named color,
            or named color/r:g:b string to wx.Colour, depending on input""" 
    # transform a wx.Colour tuple into an r:g:b string    
    if type(rgb) == wx.Colour:
        for name, color in grass.named_colors.items(): 
            if  rgb.Red() == int(color[0] * 255) and\
                rgb.Green() == int(color[1] * 255) and\
                rgb.Blue() == int(color[2] * 255):
                return name
        return str(rgb.Red()) + ':' + str(rgb.Green()) + ':' + str(rgb.Blue())
    # transform a GRASS named color or an r:g:b string into a wx.Colour tuple
    else:
        color = (grass.parse_color(rgb)[0]*255,
                 grass.parse_color(rgb)[1]*255,
                 grass.parse_color(rgb)[2]*255)
        color = wx.Colour(*color)
        if color.IsOk():
            return color
        else:  
            return None
        
        
def PaperMapCoordinates(mapInstr, x, y, paperToMap = True):
    """!Converts paper (inch) coordinates <-> map coordinates.

    @param mapInstr map frame instruction
    @param x,y paper coords in inches or mapcoords in map units
    @param paperToMap specify conversion direction
    """
    region = grass.region()
    mapWidthPaper = mapInstr['rect'].GetWidth()
    mapHeightPaper = mapInstr['rect'].GetHeight()
    mapWidthEN = region['e'] - region['w']
    mapHeightEN = region['n'] - region['s']

    if paperToMap:
        diffX = x - mapInstr['rect'].GetX()
        diffY = y - mapInstr['rect'].GetY()
        diffEW = diffX * mapWidthEN / mapWidthPaper
        diffNS = diffY * mapHeightEN / mapHeightPaper
        e = region['w'] + diffEW
        n = region['n'] - diffNS

        if projInfo()['proj'] == 'll':
            return e, n
        else:
            return int(e), int(n)

    else:
        diffEW = x - region['w']
        diffNS = region['n'] - y
        diffX = mapWidthPaper * diffEW / mapWidthEN
        diffY = mapHeightPaper * diffNS / mapHeightEN
        xPaper = mapInstr['rect'].GetX() + diffX
        yPaper = mapInstr['rect'].GetY() + diffY

        return xPaper, yPaper


def AutoAdjust(self, scaleType,  rect, map = None, mapType = None, region = None):
    """!Computes map scale, center and map frame rectangle to fit region (scale is not fixed)"""
    currRegionDict = {}
    if scaleType == 0 and map:# automatic, region from raster or vector
        res = ''
        if mapType == 'raster': 
            try:
                res = grass.read_command("g.region", flags = 'gu', rast = map)
            except grass.ScriptError:
                pass
        elif mapType == 'vector':
            res = grass.read_command("g.region", flags = 'gu', vect = map)
        currRegionDict = grass.parse_key_val(res, val_type = float)
    elif scaleType == 1 and region: # saved region
        res = grass.read_command("g.region", flags = 'gu', region = region)
        currRegionDict = grass.parse_key_val(res, val_type = float)
    elif scaleType == 2: # current region
        env = grass.gisenv()
        windFilePath = os.path.join(env['GISDBASE'], env['LOCATION_NAME'], env['MAPSET'], 'WIND')
        try:
            windFile = open(windFilePath, 'r').read()
        except IOError:
            currRegionDict = grass.region()
        regionDict = grass.parse_key_val(windFile, sep = ':', val_type = float)
        region = grass.read_command("g.region", flags = 'gu', n = regionDict['north'], s = regionDict['south'],
                                    e = regionDict['east'], w = regionDict['west'])
        currRegionDict = grass.parse_key_val(region, val_type = float)
                                                                
    else:
        return None, None, None
    
    if not currRegionDict:
        return None, None, None
    rX = rect.x
    rY = rect.y
    rW = rect.width
    rH = rect.height
    if not hasattr(self, 'unitConv'):
        self.unitConv = UnitConversion(self)
    toM = 1
    if projInfo()['proj'] != 'xy':
        toM = float(projInfo()['meters'])

    mW = self.unitConv.convert(value = (currRegionDict['e'] - currRegionDict['w']) * toM, fromUnit = 'meter', toUnit = 'inch')
    mH = self.unitConv.convert(value = (currRegionDict['n'] - currRegionDict['s']) * toM, fromUnit = 'meter', toUnit = 'inch')
    scale = min(rW/mW, rH/mH)
    
    if rW/rH > mW/mH:
        x = rX - (rH*(mW/mH) - rW)/2
        y = rY
        rWNew = rH*(mW/mH)
        rHNew = rH
    else:
        x = rX
        y = rY - (rW*(mH/mW) - rH)/2
        rHNew = rW*(mH/mW)
        rWNew = rW

    # center
    cE = (currRegionDict['w'] + currRegionDict['e'])/2
    cN = (currRegionDict['n'] + currRegionDict['s'])/2
    return scale, (cE, cN), Rect2D(x, y, rWNew, rHNew) #inch

def SetResolution(dpi, width, height):
    """!If resolution is too high, lower it
    
    @param dpi max DPI
    @param width map frame width
    @param height map frame height
    """
    region = grass.region()
    if region['cols'] > width * dpi or region['rows'] > height * dpi:
        rows = height * dpi
        cols = width * dpi
        RunCommand('g.region', rows = rows, cols = cols)
               
def ComputeSetRegion(self, mapDict):
    """!Computes and sets region from current scale, map center coordinates and map rectangle"""

    if mapDict['scaleType'] == 3: # fixed scale
        scale = mapDict['scale']
            
        if not hasattr(self, 'unitConv'):
            self.unitConv = UnitConversion(self)
        
        fromM = 1
        if projInfo()['proj'] != 'xy':
            fromM = float(projInfo()['meters'])
        rectHalfInch = (mapDict['rect'].width/2, mapDict['rect'].height/2)
        rectHalfMeter = (self.unitConv.convert(value = rectHalfInch[0], fromUnit = 'inch', toUnit = 'meter')/ fromM /scale,
                         self.unitConv.convert(value = rectHalfInch[1], fromUnit = 'inch', toUnit = 'meter')/ fromM /scale) 
        
        centerE = mapDict['center'][0]
        centerN = mapDict['center'][1]
        
        raster = self.instruction.FindInstructionByType('raster')
        if raster:
            rasterId = raster.id 
        else:
            rasterId = None

        if rasterId:
            RunCommand('g.region', n = ceil(centerN + rectHalfMeter[1]),
                       s = floor(centerN - rectHalfMeter[1]),
                       e = ceil(centerE + rectHalfMeter[0]),
                       w = floor(centerE - rectHalfMeter[0]),
                       rast = self.instruction[rasterId]['raster'])
        else:
            RunCommand('g.region', n = ceil(centerN + rectHalfMeter[1]),
                       s = floor(centerN - rectHalfMeter[1]),
                       e = ceil(centerE + rectHalfMeter[0]),
                       w = floor(centerE - rectHalfMeter[0]))
                    
def projInfo():
    """!Return region projection and map units information,
    taken from render.py"""
    
    projinfo = dict()
    
    ret = RunCommand('g.proj', read = True, flags = 'p')
    
    if not ret:
        return projinfo
    
    for line in ret.splitlines():
        if ':' in line:
            key, val = line.split(':')
            projinfo[key.strip()] = val.strip()
        elif "XY location (unprojected)" in line:
            projinfo['proj'] = 'xy'
            projinfo['units'] = ''
            break
    
    return projinfo

def GetMapBounds(filename, portrait = True):
    """!Run ps.map -b to get information about map bounding box
    
        @param filename psmap input file
        @param portrait page orientation"""
    orient = ''
    if not portrait:
        orient = 'r'
    try:
        bb = map(float, grass.read_command('ps.map',
                                           flags = 'b' + orient,
                                           quiet = True,
                                           input = filename).strip().split('=')[1].split(','))
    except (grass.ScriptError, IndexError):
        GError(message = _("Unable to run `ps.map -b`"))
        return None
    return Rect2D(bb[0], bb[3], bb[2] - bb[0], bb[1] - bb[3])

def getRasterType(map):
    """!Returns type of raster map (CELL, FCELL, DCELL)"""
    if map is None:
        map = ''
    file = grass.find_file(name = map, element = 'cell')
    if file['file']:
        rasterType = grass.raster_info(map)['datatype']
        return rasterType
    else:
        return None
   
def PilImageToWxImage(pilImage, copyAlpha = True):
    """!Convert PIL image to wx.Image
    
    Based on http://wiki.wxpython.org/WorkingWithImages
    """
    hasAlpha = pilImage.mode[-1] == 'A'
    if copyAlpha and hasAlpha :  # Make sure there is an alpha layer copy.
        wxImage = wx.EmptyImage( *pilImage.size )
        pilImageCopyRGBA = pilImage.copy()
        pilImageCopyRGB = pilImageCopyRGBA.convert('RGB')    # RGBA --> RGB
        pilImageRgbData = pilImageCopyRGB.tostring()
        wxImage.SetData(pilImageRgbData)
        wxImage.SetAlphaData(pilImageCopyRGBA.tostring()[3::4])  # Create layer and insert alpha values.

    else :    # The resulting image will not have alpha.
        wxImage = wx.EmptyImage(*pilImage.size)
        pilImageCopy = pilImage.copy()
        pilImageCopyRGB = pilImageCopy.convert('RGB')    # Discard any alpha from the PIL image.
        pilImageRgbData = pilImageCopyRGB.tostring()
        wxImage.SetData(pilImageRgbData)

    return wxImage

def BBoxAfterRotation(w, h, angle):
    """!Compute bounding box or rotated rectangle
    
    @param w rectangle width
    @param h rectangle height
    @param angle angle (0, 360) in degrees
    """
    angleRad = angle / 180. * pi
    ct = cos(angleRad)
    st = sin(angleRad)
    
    hct = h * ct
    wct = w * ct
    hst = h * st
    wst = w * st
    y = x = 0
    
    if 0 < angle <= 90:
        y_min = y
        y_max = y + hct + wst
        x_min = x - hst
        x_max = x + wct
    elif 90 < angle <= 180:
        y_min = y + hct
        y_max = y + wst
        x_min = x - hst + wct
        x_max = x
    elif 180 < angle <= 270:
        y_min = y + wst + hct
        y_max = y
        x_min = x + wct
        x_max = x - hst
    elif 270 < angle <= 360:
        y_min = y + wst
        y_max = y + hct
        x_min = x
        x_max = x + wct - hst
        
    width = int(ceil(abs(x_max) + abs(x_min)))
    height = int(ceil(abs(y_max) + abs(y_min)))
    return width, height

# hack for Windows, loading EPS works only on Unix
# these functions are taken from EpsImagePlugin.py
def loadPSForWindows(self):
    # Load EPS via Ghostscript
    if not self.tile:
        return
    self.im = GhostscriptForWindows(self.tile, self.size, self.fp)
    self.mode = self.im.mode
    self.size = self.im.size
    self.tile = []

def GhostscriptForWindows(tile, size, fp):
    """Render an image using Ghostscript (Windows only)"""
    # Unpack decoder tile
    decoder, tile, offset, data = tile[0]
    length, bbox = data

    import tempfile, os

    file = tempfile.mkstemp()[1]

    # Build ghostscript command - for Windows
    command = ["gswin32c",
               "-q",                    # quite mode
               "-g%dx%d" % size,        # set output geometry (pixels)
               "-dNOPAUSE -dSAFER",     # don't pause between pages, safe mode
               "-sDEVICE=ppmraw",       # ppm driver
               "-sOutputFile=%s" % file # output file
              ]

    command = string.join(command)

    # push data through ghostscript
    try:
        gs = os.popen(command, "w")
        # adjust for image origin
        if bbox[0] != 0 or bbox[1] != 0:
            gs.write("%d %d translate\n" % (-bbox[0], -bbox[1]))
        fp.seek(offset)
        while length > 0:
            s = fp.read(8192)
            if not s:
                break
            length = length - len(s)
            gs.write(s)
        status = gs.close()
        if status:
            raise IOError("gs failed (status %d)" % status)
        im = PILImage.core.open_ppm(file)

    finally:
        try: os.unlink(file)
        except: pass

    return im
