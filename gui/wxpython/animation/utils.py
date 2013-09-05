"""!
@package animation.utils

@brief Miscellaneous functions and enum classes

Classes:
 - utils::TemporalMode
 - utils::TemporalType
 - utils::Orientation
 - utils::ReplayMode


(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""
import wx
try:
    from PIL import Image
    hasPIL = True
except ImportError:
    hasPIL = False

import grass.temporal as tgis
import grass.script as grass

from core.gcmd import GException
from core.utils import _

class TemporalMode:
    TEMPORAL = 1
    NONTEMPORAL = 2

class TemporalType:
    ABSOLUTE = 1
    RELATIVE = 2

class Orientation:
    FORWARD = 1
    BACKWARD = 2

class ReplayMode:
    ONESHOT = 1
    REVERSE = 2
    REPEAT = 3

def validateTimeseriesName(timeseries, etype = 'strds'):
    """!Checks if space time dataset exists and completes missing mapset.

    Raises GException if dataset doesn't exist.
    """
    trastDict = tgis.tlist_grouped(etype)
    if timeseries.find("@") >= 0:
        nameShort, mapset = timeseries.split('@', 1)
        if nameShort in trastDict[mapset]:
            return timeseries
        else:
            raise GException(_("Space time dataset <%s> not found.") % timeseries)


    for mapset, names in trastDict.iteritems():
        if timeseries in names:
            return timeseries + "@" + mapset

    raise GException(_("Space time dataset <%s> not found.") % timeseries)

def validateMapNames(names, etype):
    """!Checks if maps exist and completes missing mapset.

    Input is list of map names.
    Raises GException if map doesn't exist.
    """
    mapDict = grass.list_grouped(etype)

    newNames = []
    for name in names:
        if name.find("@") >= 0:
            nameShort, mapset = name.split('@', 1)
            if nameShort in mapDict[mapset]:
                newNames.append(name)
            else:
                raise GException(_("Map <%s> not found.") % name)
        else:
            found = False
            for mapset, mapNames in mapDict.iteritems():
                if name in mapNames:
                    found = True
                    newNames.append(name + "@" + mapset)
            if not found:
                raise GException(_("Map <%s> not found.") % name)
    return newNames


def getRegisteredMaps(timeseries, etype):
    """!Returns list of maps registered in dataset"""
    timeseriesMaps = []
    if etype == 'strds':
        sp = tgis.SpaceTimeRasterDataset(ident=timeseries)
    elif etype == 'stvds':
        sp = tgis.SpaceTimeVectorDataset(ident=timeseries)

    if sp.is_in_db() == False:
        raise GException(_("Space time dataset <%s> not found.") % timeseries)
        
    sp.select()
    rows = sp.get_registered_maps(columns="id", where=None, order="start_time", dbif=None)
    timeseriesMaps = []
    if rows:
        for row in rows:
            timeseriesMaps.append(row["id"])
    return timeseriesMaps


def ComputeScaledRect(sourceSize, destSize):
    """!Fits source rectangle into destination rectangle
    by scaling and centering.

    @code
   
    >>> ComputeScaledRect(sourceSize = (10, 40), destSize = (100, 50))
    {'height': 50, 'scale': 1.25, 'width': 13, 'x': 44, 'y': 0}
    
    @endcode

    @param sourceSize size of source rectangle
    @param destSize size of destination rectangle
    """
    ratio1 = destSize[0] / float(sourceSize[0])
    ratio2 = destSize[1] / float(sourceSize[1])
    if ratio1 < ratio2:
        scale = ratio1
        width = int(sourceSize[0] * scale + 0.5)
        height = int(sourceSize[1] * scale + 0.5)
        x = 0
        y = int((destSize[1] - height) / 2. + 0.5)
    else:
        scale = ratio2
        width = int(sourceSize[0] * scale + 0.5)
        height = int(sourceSize[1] * scale + 0.5)
        y = 0
        x = int((destSize[0] - width) / 2. + 0.5)

    return {'width': width, 'height': height, 'x': x, 'y': y, 'scale': scale}

def RenderText(text, font):
    """!Renderes text with given font to bitmap."""
    dc = wx.MemoryDC()
    dc.SetFont(font)
    w, h = dc.GetTextExtent(text)
    bmp = wx.EmptyBitmap(w + 2, h + 2)
    dc.SelectObject(bmp)
    dc.SetBrush(wx.TRANSPARENT_BRUSH)
    dc.SetBackgroundMode(wx.TRANSPARENT)
    dc.Clear()
    dc.DrawText(text, 1, 1)
    dc.SelectObject(wx.NullBitmap)

    return bmp

def WxImageToPil(image):
    """!Converts wx.Image to PIL image"""
    pilImage = Image.new( 'RGB', (image.GetWidth(), image.GetHeight()) )
    pilImage.fromstring( image.GetData() )
    return pilImage
