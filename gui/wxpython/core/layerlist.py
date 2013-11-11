# -*- coding: utf-8 -*-
"""!
@package core.layerlist

@brief Non GUI classes for layer management (so far used for class simplelmgr only)

Classes:
 - layerlist::LayerList
 - layerlist::Layer
 - layerlist::LayerListToRendererConverter

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova (kratochanna gmail.com)
"""

from grass.script import core as gcore


class LayerList(object):
    """!Non GUI class managing list of layers.

    It provides API for handling layers. In the future,
    a non GUI class (e.g. named LayerTree) which includes this API,
    should be used for Layer Manager.
    """
    def __init__(self):
        self._list = []

    def GetSelectedLayers(self, activeOnly=True):
        """!Returns list of selected layers.

        @param activeOnly return only active layers
        """
        layers = []
        for layer in self._list:
            if layer.IsSelected():
                if activeOnly and layer.IsActive():
                    layers.append(layer)
                else:
                    layers.append(layer)
        return layers

    def GetSelectedLayer(self, activeOnly=False):
        """!Returns selected layer or None when there is no selected layer.

        @param activeOnly return only active layers
        """
        layers = self.GetSelectedLayers(activeOnly)
        if layers:
            return layers[0]
        return None

    def GetActiveLayers(self):
        """!Returns list of active layers."""
        return [layer for layer in self._list if layer.IsActive()]

    def GetLayersByTypes(self, mapTypes):
        """!Returns layers by types.

        @param mapTypes list of types
        """
        layers = []
        for layer in self._list:
            if layer.mapType in mapTypes:
                layers.append(layer)
        return layers

    def AddNewLayer(self, name, mapType, cmd, active=True, hidden=False,
                    opacity=100, label=None, pos=0):
        """!Creates new layer and adds it to the list (insert to the first position).

        @param ltype layer type (raster, vector, 3d-raster, ...)
        @param cmd command (given as a list)
        @param active if True layer is active
        @param hidden if True layer is hidden
        @param opacity layer opacity level (0 - 100)
        @param name layer name (set automatically from cmd)
        @param label layer label (set automatically from name)
        @param pos add layer to position
        """
        layer = Layer()
        layer.hidden = hidden
        layer.mapType = mapType
        layer.cmd = cmd
        layer.active = active
        layer.opacity = opacity
        layer.name = name
        if label:
            layer.label = label

        self._list.insert(pos, layer)
        return layer

    def AddLayer(self, layer):
        """!Adds a layer to the layer list.
        """
        self._list.insert(0, layer)

    def InsertLayer(self, index, layer):
        """!Adds a layer to the layer list.
        """
        self._list.insert(index, layer)

    def RemoveLayer(self, layer):
        """!Removes layer."""
        self._list.remove(layer)

    def GetLayerByData(self, key, value):
        """!Returns layer with specified.

        @note Returns only one layer. This might change.

        @warning Avoid using this method, it might be removed in the future.
        """
        raise NotImplementedError()

    def GetLayerIndex(self, layer):
        """!Get index of layer."""
        return self._list.index(layer)

    def MoveLayerUp(self, layer):
        """!Moves layer up (1 step)."""
        idx = self._list.index(layer)
        if idx > 0:
            lr = self._list.pop(idx)
            self._list.insert(idx - 1, lr)

    def MoveLayerDown(self, layer):
        """!Moves layer down (1 step)."""
        idx = self._list.index(layer)
        if idx < len(self._list) - 1:
            lr = self._list.pop(idx)
            self._list.insert(idx + 1, lr)

    def __iter__(self):
        for layer in self._list:
            yield layer

    def __getitem__(self, index):
        return self._list[index]

    def __len__(self):
        return len(self._list)

    def __str__(self):
        text = ''
        for layer in self._list:
            text += str(layer.name) + '\n'
        return text


class Layer(object):
    """!Object representing layer.

    Properties of the object are checked during setting.
    Map types can be extended if needed.

    >>> layer = Layer()
    >>> layer.selected = True
    >>> layer.IsSelected()
    True
    >>> layer.opacity = 0.1
    Traceback (most recent call last):
    ...
    ValueError: Opacity must be an integer between 0 and 100, not 0.1.
    >>> layer.name = 'blablabla'
    Traceback (most recent call last):
    ...
    ValueError: To set layer name, the type of layer must be specified.
    >>> layer.mapType = 'rast'
    >>> layer.name = 'blablabla'
    Traceback (most recent call last):
    ...
    ValueError: Map <blablabla> not found.
    """
    def __init__(self):
        self._mapType = None
        self._name = None
        self._label = None
        self._cmd = None
        self._opacity = 1

        self._selected = False
        self._active = True
        self._hidden = False
        self._initialized = False

        self._mapTypes = ['rast', 'vect', 'rast3d']
        self._internalTypes = {'rast': 'cell',
                               'vect': 'vect',
                               'rast3d': 'grid3'}

    def GetName(self):
        return self._name

    def SetName(self, name):
        """!Sets name of the layer.

        It checks the name of the layer by g.findfile
        (raises ValueError if map does not exist).
        Therefore map type has to be set first.
        """
        if not self.hidden:
            fullName = name.split('@')
            if len(fullName) == 1:
                if self._mapType is None:
                    raise ValueError("To set layer name, the type of layer must be specified.")

                res = gcore.find_file(name=fullName,
                                      element=self._internalTypes[self._mapType])
                if not res['mapset']:
                    raise ValueError("Map <{name}> not found.".format(name=name))
                self._name = name + '@' + res['mapset']
            else:
                self._name = name
        self.label = name

    name = property(fget=GetName, fset=SetName)

    def GetLabel(self):
        return self._label

    def SetLabel(self, label):
        self._label = label

    label = property(fget=GetLabel, fset=SetLabel)

    def GetCmd(self):
        return self._cmd

    def SetCmd(self, cmd):
        self._cmd = cmd

    cmd = property(fget=GetCmd, fset=SetCmd)

    def GetMapType(self):
        return self._mapType

    def SetMapType(self, mapType):
        """!Sets map type of the layer.

        @param mapType can be 'rast', 'vect', 'rast3'
        """
        if mapType not in self._mapTypes:
            raise ValueError("Wrong map type used: {mtype}".format(mtype=mapType))

        self._mapType = mapType

    mapType = property(fget=GetMapType, fset=SetMapType)

    def GetOpacity(self):
        """!Returns opacity value.

        @return opacity as integer between 0 and 100
        """
        return int(self._opacity * 100)

    def SetOpacity(self, opacity):
        """!Sets opacity of the layer.

        @param opacity integer between 0 and 100
        """
        if not (0 <= opacity <= 100) or opacity != int(opacity):
            raise ValueError("Opacity must be an integer between 0 and 100, not {op}.".format(op=opacity))
        self._opacity = opacity / 100.

    opacity = property(fget=GetOpacity, fset=SetOpacity)

    def Select(self, select=True):
        self._selected = select

    def IsSelected(self):
        return self._selected

    selected = property(fget=IsSelected, fset=Select)

    def IsActive(self):
        return self._active

    def Activate(self, active=True):
        """!Sets if layer is active (checked)."""
        self._active = active

    active = property(fget=IsActive, fset=Activate)

    def IsHidden(self):
        return self._hidden

    def Hide(self, hide=True):
        """!Sets if layer is hidden."""
        self._hidden = hide

    hidden = property(fget=IsHidden, fset=Hide)


class LayerListToRendererConverter:
    """!Help class for converting LayerList layers into renderer list (Map)"""
    def __init__(self, renderer):
        """!

        @param layerList instance of LayerList
        @param renderer instance of Map
        """
        self._renderer = renderer

    def _getRendererLayer(self, index):
        """!Returns corresponding layer of renderer."""
        rLayers = self._renderer.GetListOfLayers()
        index = len(rLayers) - index - 1
        return rLayers[index]

    def ConvertAll(self, layerList):
        """!Removes all layers in Map and adds new layers form layerList.
        It's not meant for continuous update because everything is rerendered.
        """
        self._renderer.DeleteAllLayers()
        for layer in reversed(layerList):
            self.AddLayer(index=-1, layer=layer)

    def ChangeLayerOpacity(self, index, layer):
        """!Changes layer opacity in renderer."""
        rLayer = self._getRendererLayer(index)
        self._renderer.ChangeLayer(rLayer, opacity=layer.opacity / 100.)

    def ChangeLayerCmd(self, index, layer):
        """!Changes layer cmd in renderer."""
        rLayer = self._getRendererLayer(index)
        self._renderer.ChangeLayer(rLayer, command=layer.cmd)

    def ChangeLayerActive(self, index, layer):
        """!Changes layer active state in renderer."""
        rLayer = self._getRendererLayer(index)
        self._renderer.ChangeLayer(rLayer, active=layer.active)

    def MoveLayerUp(self, index):
        """!Moves layer up in renderer."""
        rLayers = self._renderer.GetListOfLayers()
        index = len(rLayers) - index - 1
        rLayer = rLayers.pop(index)
        rLayers.insert(index + 1, rLayer)
        self._renderer.SetLayers(rLayers)

    def MoveLayerDown(self, index):
        """!Moves layer down in renderer."""
        rLayers = self._renderer.GetListOfLayers()
        index = len(rLayers) - index - 1
        rLayer = rLayers.pop(index)
        rLayers.insert(index - 1, rLayer)
        self._renderer.SetLayers(rLayers)

    def AddLayer(self, index, layer):
        """!Adds layer to renderer (prepends)."""
        mapType = None
        if layer.mapType == 'rast':
            mapType = 'raster'
        elif layer.mapType == 'vect':
            mapType = 'vector'
        elif layer.mapType == 'rast3d':
            mapType = '3d-raster'
        self._renderer.AddLayer(ltype=mapType, command=layer.cmd,
                                name=layer.name, active=layer.active,
                                hidden=False, opacity=layer.opacity / 100.,
                                render=True, pos=-1)

    def RemoveLayer(self, index):
        """!Removes layer from renderer."""
        self._renderer.DeleteLayer(self._getRendererLayer(index))
