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


class Layer(object):
    def __init__(self, pydata):
        self._pydata = pydata

    def __getattr__(self, name):
        return self._pydata[0][name]


class LayerList(object):
    def __init__(self, tree):
        self._tree = tree

#    def __iter__(self):
#        for in :
#            yield 

    def GetSelectedLayers(self, checkedOnly = True):
        items = self._tree.GetSelectedLayer(multi = True, checkedOnly = True)
        layers = []
        for item in items:
            layer = Layer(self._tree.GetPyData(item))
            layers.append(layer)
        return layers

    def GetLayerInfo(self, layer):
        return Layer(self._tree.GetPyData(layer))


class LayerManagerGrassInterface:
    def __init__(self, lmgr):
        self.lmgr = lmgr

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
        return LayerList(self.lmgr.GetLayerTree())

    def GetLayerList(self):
        return LayerList(self.lmgr.GetLayerTree())

    def GetMapDisplay(self):
        """!Get current map display.

        @return MapFrame instance
        @return None no mapdisplay open
        """
        return self.lmgr.GetMapDisplay(onlyCurrent=True)

    def GetAllMapDisplays(self):
        """!Get list of all map displays.

        @return list of MapFrame instances
        """
        return self.lmgr.GetMapDisplay(onlyCurrent=False)

    def GetMapWindow(self):
        if self.lmgr.GetMapDisplay(onlyCurrent=True):
            return self.lmgr.GetMapDisplay(onlyCurrent=True).GetMapWindow()
        else:
            return None

    def GetProgress(self):
        return self.lmgr.goutput.GetProgressBar()
