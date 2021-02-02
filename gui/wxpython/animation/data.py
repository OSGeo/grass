# -*- coding: utf-8 -*-
"""
@package animation.data

@brief animation data structures

Classes:
 - data::AnimationData
 - data::AnimationLayer


(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""
import os
import copy

from grass.script.utils import parse_key_val
from grass.script import core as gcore

from core.gcmd import GException
from animation.nviztask import NvizTask
from animation.utils import validateMapNames, getRegisteredMaps, \
    checkSeriesCompatibility, validateTimeseriesName, interpolate
from core.layerlist import LayerList, Layer


class AnimationData(object):

    def __init__(self):
        self._name = None
        self._windowIndex = 0
        self._layerList = None
        # only this stds is taken into account for time computations
        # if there are any stds at all
        self._firstStdsNameType = None
        self._mapCount = None
        self._cmdMatrix = None
        self._viewModes = [('2d', _("2D view")),
                           ('3d', _("3D view"))]
        self.viewMode = '2d'

        self.nvizTask = NvizTask()
        self._nvizParameters = self.nvizTask.ListMapParameters()
        self.nvizParameter = self._nvizParameters[0]

        self.workspaceFile = None
        self.legendCmd = None

        self._startRegion = None
        self._endRegion = None
        self._zoomRegionValue = None
        self._regions = None

    def GetName(self):
        return self._name

    def SetName(self, name):
        if name == '':
            raise ValueError(_("No animation name selected."))
        self._name = name

    name = property(fget=GetName, fset=SetName)

    def GetWindowIndex(self):
        return self._windowIndex

    def SetWindowIndex(self, windowIndex):
        self._windowIndex = windowIndex

    windowIndex = property(fget=GetWindowIndex, fset=SetWindowIndex)

    def SetLayerList(self, layerList):
        """
        Throws GException if layer list's combination of stds is not valid.
        """
        mapSeriesList = []
        timeseriesList = []
        for layer in layerList:
            if layer.active and hasattr(layer, 'maps'):
                if layer.mapType in ('strds', 'stvds', 'str3ds'):
                    timeseriesList.append((layer.name, layer.mapType))
                    self._firstStdsNameType = layer.name, layer.mapType
                else:
                    mapSeriesList.append((layer.maps))
        if not timeseriesList:
            self._firstStdsNameType = None, None
        # this throws GException
        count = checkSeriesCompatibility(mapSeriesList=mapSeriesList,
                                         timeseriesList=timeseriesList)
        self._mapCount = count
        self._layerList = layerList

    def GetLayerList(self):
        return self._layerList

    layerList = property(fget=GetLayerList, fset=SetLayerList)

    def GetFirstStdsNameType(self):
        return self._firstStdsNameType

    firstStdsNameType = property(fget=GetFirstStdsNameType)

    def GetMapCount(self):
        return self._mapCount

    mapCount = property(fget=GetMapCount)

    def GetCmdMatrix(self):
        return self._cmdMatrix

    def SetCmdMatrix(self, cmdMatrix):
        self._cmdMatrix = cmdMatrix

    cmdMatrix = property(fget=GetCmdMatrix, fset=SetCmdMatrix)

    def GetWorkspaceFile(self):
        return self._workspaceFile

    def SetWorkspaceFile(self, fileName):
        if fileName is None:
            self._workspaceFile = None
            return

        if fileName == '':
            raise ValueError(_("No workspace file selected."))

        if not os.path.exists(fileName):
            raise IOError(_("File %s not found") % fileName)
        self._workspaceFile = fileName

        self.nvizTask.Load(self.workspaceFile)

    workspaceFile = property(fget=GetWorkspaceFile, fset=SetWorkspaceFile)

    def SetDefaultValues(self, windowIndex, animationIndex):
        self.windowIndex = windowIndex
        self.name = _("Animation %d") % (animationIndex + 1)
        self.layerList = LayerList()

    def GetNvizParameters(self):
        return self._nvizParameters

    nvizParameters = property(fget=GetNvizParameters)

    def GetNvizParameter(self):
        return self._nvizParameter

    def SetNvizParameter(self, param):
        self._nvizParameter = param

    nvizParameter = property(fget=GetNvizParameter, fset=SetNvizParameter)

    def GetViewMode(self):
        return self._viewMode

    def SetViewMode(self, mode):
        self._viewMode = mode

    viewMode = property(fget=GetViewMode, fset=SetViewMode)

    def GetViewModes(self):
        return self._viewModes

    viewModes = property(fget=GetViewModes)

    def SetLegendCmd(self, cmd):
        self._legendCmd = cmd

    def GetLegendCmd(self):
        return self._legendCmd

    legendCmd = property(fget=GetLegendCmd, fset=SetLegendCmd)

    def GetNvizCommands(self):
        if not self.workspaceFile or not self._layerList:
            return []

        cmds = self.nvizTask.GetCommandSeries(layerList=self._layerList,
                                              paramName=self.nvizParameter)
        region = self.nvizTask.GetRegion()

        return {'commands': cmds, 'region': region}

    def SetStartRegion(self, region):
        self._startRegion = region

    def GetStartRegion(self):
        return self._startRegion

    startRegion = property(fset=SetStartRegion, fget=GetStartRegion)

    def SetEndRegion(self, region):
        self._endRegion = region

    def GetEndRegion(self):
        return self._endRegion

    endRegion = property(fset=SetEndRegion, fget=GetEndRegion)

    def SetZoomRegionValue(self, value):
        self._zoomRegionValue = value

    def GetZoomRegionValue(self):
        return self._zoomRegionValue

    zoomRegionValue = property(
        fset=SetZoomRegionValue,
        fget=GetZoomRegionValue)

    def GetRegions(self):
        self._computeRegions(self._mapCount, self._startRegion,
                             self._endRegion, self._zoomRegionValue)
        return self._regions

    def _computeRegions(
            self, count, startRegion, endRegion=None,
            zoomValue=None):
        """Computes regions based on start region and end region or zoom value
        for each of the animation frames."""
        region = dict(gcore.region())  # cast to dict, otherwise deepcopy error
        if startRegion:
            region = dict(parse_key_val(gcore.read_command('g.region',
                                                           flags='gu',
                                                           region=startRegion),
                                        val_type=float))

        del region['cells']
        del region['cols']
        del region['rows']
        if 'projection' in region:
            del region['projection']
        if 'zone' in region:
            del region['zone']
        regions = []
        for i in range(self._mapCount):
            regions.append(copy.copy(region))
        self._regions = regions
        if not (endRegion or zoomValue):
            return

        startRegionDict = parse_key_val(
            gcore.read_command(
                'g.region',
                flags='gu',
                region=startRegion),
            val_type=float)
        if endRegion:
            endRegionDict = parse_key_val(
                gcore.read_command(
                    'g.region',
                    flags='gu',
                    region=endRegion),
                val_type=float)
            for key in ('n', 's', 'e', 'w', 'nsres', 'ewres'):
                values = interpolate(
                    startRegionDict[key],
                    endRegionDict[key],
                    self._mapCount)
                for value, region in zip(values, regions):
                    region[key] = value

        elif zoomValue:
            for i in range(self._mapCount):
                regions[i]['n'] -= zoomValue[0] * i
                regions[i]['e'] -= zoomValue[1] * i
                regions[i]['s'] += zoomValue[0] * i
                regions[i]['w'] += zoomValue[1] * i

                # handle cases when north < south and similarly EW
                if regions[i]['n'] < regions[i]['s'] or \
                   regions[i]['e'] < regions[i]['w']:
                    regions[i] = regions[i - 1]

        self._regions = regions

    def __repr__(self):
        return "%s(%r)" % (self.__class__, self.__dict__)


class AnimLayer(Layer):
    """Animation layer allows adding either space-time dataset
    or series of maps."""

    def __init__(self):
        Layer.__init__(self)
        self._mapTypes.extend(['strds', 'stvds', 'str3ds'])
        self._maps = []

    def SetName(self, name):
        if not self.hidden:
            if self._mapType is None:
                raise ValueError(
                    "To set layer name, the type of layer must be specified.")
            if self._mapType in ('strds', 'stvds', 'str3ds'):
                try:
                    name = validateTimeseriesName(name, self._mapType)
                    self._maps = getRegisteredMaps(name, self._mapType)
                except (GException, gcore.ScriptError) as e:
                    raise ValueError(str(e))
            else:
                self._maps = validateMapNames(name.split(','), self._mapType)
        self._name = name
        self.label = name

    def GetName(self):
        return self._name

    name = property(fget=GetName, fset=SetName)

    def GetMaps(self):
        return self._maps

    maps = property(fget=GetMaps)
