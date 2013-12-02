"""!
@package animation.utils

@brief Miscellaneous functions and enum classes

Classes:
 - utils::TemporalMode
 - utils::TemporalType
 - utils::Orientation
 - utils::ReplayMode


(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Perasova <kratochanna gmail.com>
"""
import os
import wx
import hashlib
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


def validateTimeseriesName(timeseries, etype='strds'):
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
    """!Returns list of maps registered in dataset.
    Can throw ScriptError if the dataset doesn't exist.
    """
    timeseriesMaps = []
    sp = tgis.open_old_space_time_dataset(timeseries, etype)

    rows = sp.get_registered_maps(columns="id", where=None, order="start_time")
    timeseriesMaps = []
    if rows:
        for row in rows:
            timeseriesMaps.append(row["id"])
    return timeseriesMaps


def checkSeriesCompatibility(mapSeriesList=None, timeseriesList=None):
    """!Checks whether time series (map series and stds) are compatible,
        which means they have equal number of maps ad times (in case of stds).
        This is needed within layer list, not within the entire animation tool.
        Throws GException if these are incompatible.

        @return number of maps for animation
    """
    timeseriesInfo = {'count': set(), 'temporalType': set(), 'mapType': set(),
                      'mapTimes': set()}

    if timeseriesList:
        for stds, etype in timeseriesList:
            sp = tgis.open_old_space_time_dataset(stds, etype)
            mapType = sp.get_map_time()  # interval, ...
            tempType = sp.get_initial_values()[0]  # absolute
            timeseriesInfo['mapType'].add(mapType)
            timeseriesInfo['temporalType'].add(tempType)
            rows = sp.get_registered_maps_as_objects(where=None,
                                                     order="start_time")

            if rows:
                times = []
                timeseriesInfo['count'].add(len(rows))
                for row in rows:
                    if tempType == 'absolute':
                        time = row.get_absolute_time()
                    else:
                        time = row.get_relative_time()
                    times.append(time)
                timeseriesInfo['mapTimes'].add(tuple(times))
            else:
                timeseriesInfo['mapTimes'].add(None)
                timeseriesInfo['count'].add(None)

    if len(timeseriesInfo['count']) > 1:
        raise GException(_("The number of maps in space-time datasets "
                           "has to be the same."))

    if len(timeseriesInfo['temporalType']) > 1:
        raise GException(_("The temporal type (absolute/relative) of space-time datasets "
                           "has to be the same."))

    if len(timeseriesInfo['mapType']) > 1:
        raise GException(_("The map type (point/interval) of space-time datasets "
                           "has to be the same."))

    if len(timeseriesInfo['mapTimes']) > 1:
        raise GException(_("The temporal extents of maps in space-time datasets "
                           "have to be the same."))

    if mapSeriesList:
        count = set()
        for mapSeries in mapSeriesList:
            count.add(len(mapSeries))
        if len(count) > 1:
            raise GException(_("The number of maps to animate has to be "
                               "the same for each map series."))

        if timeseriesList and list(count)[0] != list(timeseriesInfo['count'])[0]:
            raise GException(_("The number of maps to animate has to be "
                               "the same as the number of maps in temporal dataset."))

    if mapSeriesList:
        return list(count)[0]
    if timeseriesList:
        return list(timeseriesInfo['count'])[0]


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
    pilImage = Image.new('RGB', (image.GetWidth(), image.GetHeight()))
    pilImage.fromstring(image.GetData())
    return pilImage


def HashCmd(cmd):
    """!Returns a hash from command given as a list."""
    name = '_'.join(cmd)
    return hashlib.sha1(name).hexdigest()


def HashCmds(cmds):
    """!Returns a hash from list of commands."""
    name = ';'.join([item for sublist in cmds for item in sublist])
    return hashlib.sha1(name).hexdigest()


def GetFileFromCmd(dirname, cmd, extension='ppm'):
    """!Returns file path created as a hash from command."""
    return os.path.join(dirname, HashCmd(cmd) + '.' + extension)


def GetFileFromCmds(dirname, cmds, extension='ppm'):
    """!Returns file path created as a hash from list of commands."""
    return os.path.join(dirname, HashCmds(cmds) + '.' + extension)


def layerListToCmdsMatrix(layerList):
    """!Goes thru layerList and create matrix of commands
    for the composition of map series.:

    @returns matrix of cmds for composition
    """
    count = 0
    for layer in layerList:
        if layer.active and hasattr(layer, 'maps'):
            # assuming the consistency of map number is checked already
            count = len(layer.maps)
            break
    cmdsForComposition = []
    for layer in layerList:
        if not layer.active:
            continue
        if hasattr(layer, 'maps'):
            for i, part in enumerate(layer.cmd):
                if part.startswith('map='):
                    cmd = layer.cmd[:]
                    cmds = []
                    for map_ in layer.maps:
                        cmd[i] = 'map={}'.format(map_)
                        cmds.append(cmd[:])
                    cmdsForComposition.append(cmds)
        else:
            cmdsForComposition.append([layer.cmd] * count)

    return zip(*cmdsForComposition)


def sampleCmdMatrixAndCreateNames(cmdMatrix, sampledSeries):
    """!Applies information from temporal sampling
    to the command matrix."""
    namesList = []
    j = -1
    lastName = ''
    for name in sampledSeries:
        if name is not None:
            if lastName != name:
                lastName = name
                j += 1
            namesList.append(HashCmds(cmdMatrix[j]))
        else:
            namesList.append(None)
    assert(j == len(cmdMatrix) - 1)
    return namesList
