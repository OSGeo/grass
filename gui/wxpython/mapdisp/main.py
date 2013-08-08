"""!
@package mapdisp.main

@brief Start Map Display as standalone application

Classes:
 - mapdisp::DMonMap
 - mapdisp::Layer
 - mapdisp::LayerList
 - mapdisp::DMonGrassInterface
 - mapdisp::DMonFrame
 - mapdisp::MapApp

Usage:
python mapdisp/main.py monitor-identifier /path/to/map/file /path/to/command/file /path/to/env/file

(C) 2006-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com> (MapFrameBase)
@author Anna Kratochvilova <kratochanna gmail.com> (MapFrameBase)
"""

import os
import sys

if __name__ == "__main__":
    gui_wx_path = os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython')
    if gui_wx_path not in sys.path:
        sys.path.append(gui_wx_path)

from core          import globalvar
import wx

from core          import utils
from core.giface   import StandaloneGrassInterface
from core.gcmd     import RunCommand
from core.render   import Map, MapLayer
from core.utils import _
from mapdisp.frame import MapFrame
from grass.script  import core as grass
from core.debug    import Debug
from core.settings import UserSettings

# for standalone app
monFile = { 'cmd' : None,
            'map' : None,
            'env' : None,
            }
monName = None
monSize = list(globalvar.MAP_WINDOW_SIZE)




class DMonMap(Map):
    def __init__(self, giface, cmdfile=None, mapfile=None):
        """!Map composition (stack of map layers and overlays)

        @param cmdline full path to the cmd file (defined by d.mon)
        @param mapfile full path to the map file (defined by d.mon)
        """

        Map.__init__(self)

        self._giface = giface

        # environment settings
        self.env   = dict()

        self.cmdfile = cmdfile

        # list of layers for rendering added from cmd file
        # TODO temporary solution, layer managment by different tools in GRASS should be resovled
        self.ownedLayers = []

        if mapfile:
            self.mapfileCmd = mapfile
            self.maskfileCmd = os.path.splitext(mapfile)[0] + '.pgm'

        # generated file for g.pnmcomp output for rendering the map
        self.mapfile = monFile['map'] + '.ppm'

    def GetLayersFromCmdFile(self):
        """!Get list of map layers from cmdfile
        """
        if not self.cmdfile:
            return

        nlayers = 0

        try:
            fd = open(self.cmdfile, 'r')
            existingLayers = self.GetListOfLayers()

            # holds new rendreing order for every layer in existingLayers
            layersOrder = [-1] * len(self.GetListOfLayers())

            # next number in rendering order
            next_layer = 0;

            for line in fd.readlines():
                cmd = utils.split(line.strip())
                ltype = None

                try:
                    ltype = utils.command2ltype[cmd[0]]
                except KeyError:
                    grass.warning(_("Unsupported command %s.") % cmd[0])
                    continue

                name = utils.GetLayerNameFromCmd(cmd, fullyQualified = True,
                                                 layerType = ltype)[0]

                # creating temporary layer object to compare commands
                # neccessary to get the same format
                # supposing that there are no side effects
                tmpMapLayer = MapLayer(ltype = ltype, name = name,
                                       cmd = cmd, Map = None,
                                       active = False, hidden = True,
                                       opacity = 0)
                exists = False
                for i, layer in enumerate(existingLayers):
                    if layer.GetCmd(string=True) == tmpMapLayer.GetCmd(string=True):
                        exists = True

                        if layersOrder[i] == -1: 
                            layersOrder[i] = next_layer;
                            next_layer += 1
                        # layer must be put higher in render order (same cmd was insered more times)
                        # TODO delete rendurant cmds from cmd file?
                        else:
                            for j, l_order in enumerate(layersOrder):
                                if l_order > layersOrder[i]:
                                    layersOrder[j] -= 1;
                            layersOrder[i] = next_layer - 1;

                        break
                if exists:
                    continue

                newLayer = Map.AddLayer(self, ltype = ltype, command = cmd, active = True, name = name)
                
                existingLayers.append(newLayer)
                self.ownedLayers.append(newLayer)

                layersOrder.append(next_layer)
                next_layer += 1

                nlayers += 1

            reorderedLayers = [-1] * next_layer
            for i, layer in enumerate(existingLayers):

                # owned layer was not found in cmd file -> is deleted 
                if layersOrder[i] == -1 and layer in self.ownedLayers:
                    self.ownedLayers.remove(layer)
                    self.DeleteLayer(layer)

                # other layer e. g. added by wx.vnet are added to the top
                elif layersOrder[i] == -1 and layer not in self.ownedLayers:
                    reorderedLayers.append(layer)
                
                # owned layer found in cmd file is added into proper rendering position
                else:
                    reorderedLayers[layersOrder[i]] = layer

            self.SetLayers(reorderedLayers)

        except IOError, e:
            grass.warning(_("Unable to read cmdfile '%(cmd)s'. Details: %(det)s") % \
                              { 'cmd' : self.cmdfile, 'det' : e })
            return

        fd.close()

        self._giface.updateMap.emit()

        Debug.msg(1, "Map.GetLayersFromCmdFile(): cmdfile=%s" % self.cmdfile)
        Debug.msg(1, "                            nlayers=%d" % nlayers)
                
    def Render(self, *args, **kwargs):
        """!Render layer to image.

        For input params and returned data see overridden method in Map class.
        """
        currMon = grass.gisenv()['MONITOR']

        RunCommand('g.gisenv',
                   unset = 'MONITOR') # GRASS_RENDER_IMMEDIATE doesn't like monitors

        ret = Map.Render(self, *args, **kwargs)

        RunCommand('g.gisenv',
                    set = 'MONITOR=%s' % currMon)
        
        return ret
    
    def AddLayer(self, *args, **kwargs):
        """!Adds generic map layer to list of layers.

        For input params and returned data see overridden method in Map class.
        """
        currMon = grass.gisenv()['MONITOR']

        RunCommand('g.gisenv',
                   unset = 'MONITOR') # GRASS_RENDER_IMMEDIATE doesn't like monitors

        driver = UserSettings.Get(group = 'display', key = 'driver', subkey = 'type')
    
        if driver == 'png':
            os.environ["GRASS_RENDER_IMMEDIATE"] = "png"
        else:
            os.environ["GRASS_RENDER_IMMEDIATE"] = "cairo"

        layer = Map.AddLayer(self, *args, **kwargs)

        del os.environ["GRASS_RENDER_IMMEDIATE"]

        RunCommand('g.gisenv',
                   set='MONITOR=%s' % currMon)

        return layer


class Layer(object):
    """!@implements core::giface::Layer"""
    def __init__(self, maplayer):
        self._maplayer = maplayer

    def __getattr__(self, name):
        if name == 'cmd':
            return utils.CmdTupleToList(self._maplayer.GetCmd())
        elif hasattr(self._maplayer, name):
            return getattr(self._maplayer, name)
        elif name == 'maplayer':
            return self._maplayer
        elif name == 'type':
            return self._maplayer.GetType()
            #elif name == 'ctrl':
        elif name == 'label':
            return self._maplayer.GetName()
            #elif name == 'propwin':


class LayerList(object):
    def __init__(self, map, giface):
        """!@implements core::giface::LayerList"""
        self._map = map
        self._giface = giface

    def GetSelectedLayers(self, checkedOnly=True):
        # hidden and selected vs checked and selected
        items = self._map.GetListOfLayers()
        layers = []
        for item in items:
            layer = Layer(item)
            layers.append(layer)
        return layers

    def GetSelectedLayer(self, checkedOnly=False):
        """!Returns selected layer or None when there is no selected layer."""
        layers = self.GetSelectedLayers()
        if len(layers) > 0:
            return layers[0]
        else:
            return None

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
        self._map.AddLayer(ltype=ltype, command=cmd,
                           name=name, active=True,
                           opacity=opacity, render=True,
                           pos=-1)
        # TODO: this should be solved by signal
        # (which should be introduced everywhere,
        # alternative is some observer list)
        self._giface.updateMap.emit(render=True, renderVector=True)

    def GetLayersByName(self, name):
        items = self._map.GetListOfLayers()
        layers = []
        for item in items:
            if item.GetName() == name:
                layer = Layer(item)
                layers.append(layer)
        return layers

    def GetLayerByData(self, key, value):
        # TODO: implementation was not tested
        items = self._map.GetListOfLayers()
        for item in items:
            layer = Layer(item)
            try:
                if getattr(layer, key) == value:
                    return layer
            except AttributeError:
                pass
        return None


class DMonGrassInterface(StandaloneGrassInterface):
    """!@implements GrassInterface"""
    def __init__(self, mapframe):
        StandaloneGrassInterface.__init__(self)
        self._mapframe = mapframe

    def GetLayerList(self):
        return LayerList(self._mapframe.GetMap(), giface=self)

    def GetMapWindow(self):
        return self._mapframe.GetMapWindow()


class DMonFrame(MapFrame):
    def OnZoomToMap(self, event):
        layers = self.MapWindow.GetMap().GetListOfLayers()
        self.MapWindow.ZoomToMap(layers = layers)
        

class MapApp(wx.App):
    def OnInit(self):
        if not globalvar.CheckWxVersion([2, 9]):
            wx.InitAllImageHandlers()

        # actual use of StandaloneGrassInterface not yet tested
        # needed for adding functionality in future
        giface = DMonGrassInterface(None)

        if __name__ == "__main__":
            self.cmdTimeStamp = os.path.getmtime(monFile['cmd'])
            self.Map = DMonMap(giface=giface, cmdfile=monFile['cmd'],
                               mapfile = monFile['map'])
        else:
            self.Map = None

        self.mapFrm = DMonFrame(parent = None, id = wx.ID_ANY, Map = self.Map,
                                giface = giface, size = monSize)
        # FIXME: hack to solve dependency
        giface._mapframe = self.mapFrm
        # self.SetTopWindow(Map)
        self.mapFrm.GetMapWindow().SetAlwaysRenderEnabled(True)
        self.mapFrm.Show()
        
        if __name__ == "__main__":
            self.timer = wx.PyTimer(self.watcher)
            #check each 0.5s
            global mtime
            mtime = 500
            self.timer.Start(mtime)
            
        return True
    
    def OnExit(self):
        if __name__ == "__main__":
            # stop the timer
            # self.timer.Stop()
            # terminate thread
            for f in monFile.itervalues():
                grass.try_remove(f)
            
    def watcher(self):
        """!Redraw, if new layer appears (check's timestamp of
        cmdfile)
        """
        try:
            # GISBASE and other sytem enviromental variables can not be used
            # since the process inherited them from GRASS
            # raises exception when vaiable does not exists
            grass.gisenv()['GISDBASE']
        except KeyError:
            self.timer.Stop()
            return
        
        # todo: events
        try:
            currentCmdFileTime = os.path.getmtime(monFile['cmd'])
            if currentCmdFileTime > self.cmdTimeStamp:
                self.timer.Stop()
                self.cmdTimeStamp = currentCmdFileTime
                self.mapFrm.GetMap().GetLayersFromCmdFile()
                self.timer.Start(mtime)
        except OSError, e:
            grass.warning("%s" % e)
            self.timer.Stop()

    def GetMapFrame(self):
        """!Get Map Frame instance"""
        return self.mapFrm

if __name__ == "__main__":
    # set command variable
    if len(sys.argv) < 5:
        print __doc__
        sys.exit(1)
    
    monName = sys.argv[1]
    monFile = { 'map' : sys.argv[2],
                'cmd' : sys.argv[3],
                'env' : sys.argv[4],
                }
    if len(sys.argv) >= 6:
        try:
            monSize[0] = int(sys.argv[5])
        except ValueError:
            pass
    
    if len(sys.argv) == 7:
        try:
            monSize[1] = int(sys.argv[6])
        except ValueError:
            pass

    
    grass.verbose(_("Starting map display <%s>...") % (monName))

    RunCommand('g.gisenv',
               set = 'MONITOR_%s_PID=%d' % (monName, os.getpid()))
    
    gmMap = MapApp(0)
    # set title
    gmMap.mapFrm.SetTitle(_("GRASS GIS Map Display: " +
                            monName + 
                            " - Location: " + grass.gisenv()["LOCATION_NAME"]))
    
    gmMap.MainLoop()
    
    grass.verbose(_("Stopping map display <%s>...") % (monName))

    # clean up GRASS env variables
    env = grass.gisenv()
    env_name = 'MONITOR_%s' % monName
    for key in env.keys():
        if key.find(env_name) == 0:
            RunCommand('g.gisenv',
                       unset = '%s' % key)
        if key == 'MONITOR' and env[key] == monName:
            RunCommand('g.gisenv',
                       unset = '%s' % key)
    
    sys.exit(0)
