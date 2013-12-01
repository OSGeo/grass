# -*- coding: utf-8 -*-
"""!
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

from grass.script import core as gcore

from core.utils import _
from core.gcmd import GException
from animation.nviztask import NvizTask
from animation.utils import validateMapNames, getRegisteredMaps, \
    checkSeriesCompatibility, validateTimeseriesName
from core.layerlist import LayerList, Layer
import grass.temporal as tgis


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
        """!
        Throws GException if layer list's combination of stds is not valid.
        """
        mapSeriesList = []
        timeseriesList = []
        for layer in layerList:
            if layer.active and hasattr(layer, 'maps'):
                if layer.mapType in ('strds', 'stvds'):
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

    def __repr__(self):
        return "%s(%r)" % (self.__class__, self.__dict__)


class AnimLayer(Layer):
    """!Animation layer allows to add either space-time dataset
    or series of maps."""
    def __init__(self):
        Layer.__init__(self)
        self._mapTypes.extend(['strds', 'stvds'])
        self._maps = []
        tgis.init()

    def SetName(self, name):
        if not self.hidden:
            if self._mapType is None:
                raise ValueError("To set layer name, the type of layer must be specified.")
            if self._mapType in ('strds', 'stvds'):
                try:
                    name = validateTimeseriesName(name, self._mapType)
                    self._maps = getRegisteredMaps(name, self._mapType)
                except (GException, gcore.ScriptError), e:
                    raise ValueError(str(e))
            else:
                self._maps = validateMapNames(name.split(','), self._internalTypes[self._mapType])
        self._name = name
        self.label = name

    def GetName(self):
        return self._name

    name = property(fget=GetName, fset=SetName)

    def GetMaps(self):
        return self._maps

    maps = property(fget=GetMaps)
