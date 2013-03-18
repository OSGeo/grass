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

    def __repr__(self):
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

    def GetLayerInfo(self, layer):
        """!For compatibility only, will be removed."""
        return Layer(self._tree.GetPyData(layer))


class LayerManagerGrassInterface(object):
    """!@implements GrassInterface"""
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
        self.RunCmd(cmdlist, compReg = False, switchPage = False)

    def WriteLog(self, text, wrap = None,
                 switchPage = False, priority = 1):
        self.lmgr._gconsole.WriteLog(text = text, wrap = wrap, switchPage = switchPage,
                                   priority = priority)

    def WriteCmdLog(self, line, pid = None, switchPage = True):
        self.lmgr._gconsole.WriteCmdLog(line = line, pid = pid, switchPage = switchPage)

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
    """
    def __init__(self, giface, tree):
        """!
        @giface original grass interface
        @tree tree which will be used instead of the tree from giface
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
        return self.tree.GetMapDisplay()

    def __getattr__(self, name):
        return getattr(self._giface, name)
