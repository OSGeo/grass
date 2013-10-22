"""!
@package lmgr.giface

@brief Layer Manager GRASS interface

Classes:
 - giface::LayerManagerGrassInterface

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com>
"""

from grass.pydispatch.signal import Signal
from core.giface import Notification


class Layer(object):
    """!@implements core::giface::Layer

    @note Currently implemented without specifying the interface.
    It only provides all attributes of existing layer as used in lmgr.
    """
    def __init__(self, pydata):
        self._pydata = pydata

    def __getattr__(self, name):
        return self._pydata[0][name]

    def __dir__(self):
        return self._pydata[0].keys()

    def __str__(self):
        return self.maplayer.name


class LayerList(object):
    """!@implements core.giface.Layer"""
    def __init__(self, tree):
        self._tree = tree

    def __iter__(self):
        """!Iterates over the contents of the list."""
        for item in self._tree.GetSelectedLayer(multi=True):
            yield Layer(self._tree.GetPyData(item))

    def __getitem__(self, index):
        """!Select a layer from the LayerList using the index."""
        return [l for l in self][index]

    def __repr__(self):
        """!Return a representation of the object."""
        return "LayerList(%r)" % [layer for layer in self]

    def GetSelectedLayers(self, checkedOnly=True):
        items = self._tree.GetSelectedLayer(multi=True,
                                            checkedOnly=checkedOnly)
        layers = []
        for item in items:
            layer = Layer(self._tree.GetPyData(item))
            layers.append(layer)
        return layers

    # TODO: it is not clear if default of checkedOnly should be False or True
    def GetSelectedLayer(self, checkedOnly=False):
        """!Returns selected layer or None when there is no selected layer."""
        item = self._tree.GetSelectedLayer(multi=False,
                                           checkedOnly=checkedOnly)
        if item is None:
            return None
        else:
            data = self._tree.GetPyData(item)
            return Layer(data)

    def GetLayerInfo(self, layer):
        """!For compatibility only, will be removed."""
        return Layer(self._tree.GetPyData(layer))

    def AddLayer(self, ltype, name=None, checked=None,
                 opacity=1.0, cmd=None):
        """!Adds a new layer to the layer list.

        Launches property dialog if needed (raster, vector, etc.)

        @param ltype layer type (raster, vector, 3d-raster, ...)
        @param name layer name
        @param checked if True layer is checked
        @param opacity layer opacity level
        @param cmd command (given as a list)
        """
        self._tree.AddLayer(ltype=ltype, lname=name, lchecked=checked,
                            lopacity=opacity, lcmd=cmd)

    def GetLayersByName(self, name):
        items = self._tree.FindItemByData(key='name', value=name)
        if items is None:
            return []
        else:
            layers = []
            for item in items:
                layer = Layer(self._tree.GetPyData(item))
                layers.append(layer)
            return layers

    def GetLayerByData(self, key, value):
        """!Returns layer with specified.

        Returns only one layer.
        Avoid using this method, it might be removed in the future.
        """
        if key == 'name':
            print "giface.GetLayerByData(): Do not with use key='name',"
            " use GetLayersByName instead."
        item = self._tree.FindItemByData(key=key, value=value)
        if item is None:
            return None
        else:
            return Layer(self._tree.GetPyData(item))


class LayerManagerGrassInterface(object):
    """!@implements core::giface::GrassInterface"""
    def __init__(self, lmgr):
        """!Costructor is specific to the current implementation.

        Uses Layer Manager object including its private attributes.
        (It encapsulates existing Layer Manager so access to private members
        is intention.)
        """
        self.lmgr = lmgr

        # Signal when some map is created or updated by a module.
        # attributes: name: map name, ltype: map type,
        # add: if map should be added to layer tree (questionable attribute)
        self.mapCreated = Signal('LayerManagerGrassInterface.mapCreated')

        # Signal emitted to request updating of map
        self.updateMap = Signal('LayerManagerGrassInterface.updateMap')

    def RunCmd(self, *args, **kwargs):
        self.lmgr._gconsole.RunCmd(*args, **kwargs)

    def Help(self, entry):
        cmdlist = ['g.manual', 'entry=%s' % entry]
        self.RunCmd(cmdlist, compReg = False, notification=Notification.NO_NOTIFICATION)

    def WriteLog(self, text, wrap = None,
                 notification=Notification.HIGHLIGHT):
        self.lmgr._gconsole.WriteLog(text=text, wrap=wrap, 
                                     notification=notification)

    def WriteCmdLog(self, line, pid=None, notification=Notification.MAKE_VISIBLE):
        self.lmgr._gconsole.WriteCmdLog(line=line, pid=pid, notification=notification)

    def WriteWarning(self, line):
        self.lmgr._gconsole.WriteWarning(line = line)

    def WriteError(self, line):
        self.lmgr._gconsole.WriteError(line = line)

    def GetLayerTree(self):
        return self.lmgr.GetLayerTree()

    def GetLayerList(self):
        return LayerList(self.lmgr.GetLayerTree())

    def GetMapDisplay(self):
        return self.lmgr.GetMapDisplay(onlyCurrent=True)

    def GetAllMapDisplays(self):
        return self.lmgr.GetMapDisplay(onlyCurrent=False)

    def GetMapWindow(self):
        if self.lmgr.GetMapDisplay(onlyCurrent=True):
            return self.lmgr.GetMapDisplay(onlyCurrent=True).GetMapWindow()
        else:
            return None

    def GetProgress(self):
        return self.lmgr.goutput.GetProgressBar()


class LayerManagerGrassInterfaceForMapDisplay(object):
    """!Provides reference only to the given layer list (according to tree),
        not to the current.

        @implements core::giface::GrassInterface
    """
    def __init__(self, giface, tree):
        """!
        @param giface original grass interface
        @param tree tree which will be used instead of the tree from giface
        """
        self._giface = giface
        self.tree = tree

        # Signal emitted to request updating of map
        self.updateMap = Signal('LayerManagerGrassInterfaceForMapDisplay.updateMap')

    def GetLayerTree(self):
        return self.tree

    def GetLayerList(self):
        return LayerList(self.tree)

    def GetMapWindow(self):
        return self.tree.GetMapDisplay().GetMapWindow()

    def __getattr__(self, name):
        return getattr(self._giface, name)
