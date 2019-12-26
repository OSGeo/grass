"""
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

(C) 2006-2015 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com> (MapFrameBase)
@author Anna Kratochvilova <kratochanna gmail.com> (MapFrameBase)
"""

from __future__ import print_function

import os
import sys
import six
import time
import shutil
import fileinput

from grass.script.setup import set_gui_path
set_gui_path()

from core import globalvar
import wx

from core import utils
from core.giface import StandaloneGrassInterface
from core.gcmd import RunCommand
from core.render import Map, MapLayer, Overlay, RenderMapMgr
from mapdisp.frame import MapFrame
from core.debug import Debug
from core.settings import UserSettings

from grass.script.utils import try_remove
from grass.script import core as grass
from grass.script.task import cmdtuple_to_list, cmdlist_to_tuple
from grass.pydispatch.signal import Signal

# for standalone app
monFile = {'cmd': None,
           'map': None,
           'env': None,
           }
monName = None
monSize = list(globalvar.MAP_WINDOW_SIZE)
monDecor = False


class DMonMap(Map):

    def __init__(self, giface, cmdfile=None, mapfile=None):
        """Map composition (stack of map layers and overlays)

        :param cmdline: full path to the cmd file (defined by d.mon)
        :param mapfile: full path to the map file (defined by d.mon)
        """
        Map.__init__(self)

        self._giface = giface

        # environment settings
        self.env = dict()

        self.cmdfile = cmdfile

        # list of layers for rendering added from cmd file
        # TODO temporary solution, layer managment by different tools in GRASS
        # should be resovled
        self.ownedLayers = []
        self.oldOverlays = []

        if mapfile:
            self.mapfileCmd = mapfile
            self.maskfileCmd = os.path.splitext(mapfile)[0] + '.pgm'

        # generated file for g.pnmcomp output for rendering the map
        self.mapfile = monFile['map']
        if os.path.splitext(self.mapfile)[1] != '.ppm':
            self.mapfile += '.ppm'

        # signal sent when d.out.file/d.to.rast appears in cmd file, attribute
        # is cmd
        self.saveToFile = Signal('DMonMap.saveToFile')
        self.dToRast = Signal('DMonMap.dToRast')
        # signal sent when d.what.rast/vect appears in cmd file, attribute is
        # cmd
        self.query = Signal('DMonMap.query')

        self.renderMgr = RenderMapMgr(self)

        # update legend file variable with the one d.mon uses
        with open(monFile['env'], 'r') as f:
            lines = f.readlines()
            for line in lines:
                if 'GRASS_LEGEND_FILE' in line:
                    legfile = line.split('=', 1)[1].strip()
                    self.renderMgr.UpdateRenderEnv({'GRASS_LEGEND_FILE': legfile})
                    break

    def GetLayersFromCmdFile(self):
        """Get list of map layers from cmdfile
        """
        if not self.cmdfile:
            return

        nlayers = 0
        try:
            fd = open(self.cmdfile, 'r')
            lines = fd.readlines()
            fd.close()
            # detect d.out.file, delete the line from the cmd file and export
            # graphics
            if len(lines) > 0:
                if lines[-1].startswith('d.out.file') or \
                   lines[-1].startswith('d.to.rast'):
                    dCmd = lines[-1].strip()
                    fd = open(self.cmdfile, 'w')
                    fd.writelines(lines[:-1])
                    fd.close()
                    if lines[-1].startswith('d.out.file'):
                        self.saveToFile.emit(cmd=utils.split(dCmd))
                    else:
                        self.dToRast.emit(cmd=utils.split(dCmd))
                    return
                if lines[-1].startswith('d.what'):
                    dWhatCmd = lines[-1].strip()
                    fd = open(self.cmdfile, 'w')
                    fd.writelines(lines[:-1])
                    fd.close()
                    if '=' in utils.split(dWhatCmd)[1]:
                        maps = utils.split(dWhatCmd)[1].split('=')[
                            1].split(',')
                    else:
                        maps = utils.split(dWhatCmd)[1].split(',')
                    self.query.emit(ltype=utils.split(dWhatCmd)[
                                    0].split('.')[-1], maps=maps)
                    return
            else:
                # clean overlays after erase
                self.oldOverlays = []
                overlays = list(self._giface.GetMapDisplay().decorations.keys())
                for each in overlays:
                    self._giface.GetMapDisplay().RemoveOverlay(each)

            existingLayers = self.GetListOfLayers()

            # holds new rendreing order for every layer in existingLayers
            layersOrder = [-1] * len(existingLayers)

            # next number in rendering order
            next_layer = 0
            mapFile = None
            render_env = dict()
            for line in lines:
                if line.startswith('#'):
                    if 'GRASS_RENDER_FILE' in line:
                        mapFile = line.split('=', 1)[1].strip()
                    try:
                        k, v = line[2:].strip().split('=', 1)
                    except:
                        pass
                    render_env[k] = v
                    continue

                cmd = utils.split(line.strip())

                ltype = None
                try:
                    ltype = utils.command2ltype[cmd[0]]
                except KeyError:
                    grass.warning(_("Unsupported command %s.") % cmd[0])
                    continue

                name = utils.GetLayerNameFromCmd(cmd, fullyQualified=True,
                                                 layerType=ltype)[0]

                args = {}

                if ltype in ('barscale', 'rastleg', 'northarrow', 'text', 'vectleg'):
                    # TODO: this is still not optimal
                    # it is there to prevent adding the same overlay multiple times
                    if cmd in self.oldOverlays:
                        continue
                    if ltype == 'rastleg':
                        self._giface.GetMapDisplay().AddLegendRast(cmd=cmd)
                    elif ltype == 'barscale':
                        self._giface.GetMapDisplay().AddBarscale(cmd=cmd)
                    elif ltype == 'northarrow':
                        self._giface.GetMapDisplay().AddArrow(cmd=cmd)
                    elif ltype == 'text':
                        self._giface.GetMapDisplay().AddDtext(cmd=cmd)
                    elif ltype == 'vectleg':
                        self._giface.GetMapDisplay().AddLegendVect(cmd=cmd)
                    self.oldOverlays.append(cmd)
                    continue

                classLayer = MapLayer
                args['ltype'] = ltype

                exists = False
                for i, layer in enumerate(existingLayers):
                    if layer.GetCmd(
                            string=True) == utils.GetCmdString(cmdlist_to_tuple(cmd)):
                        exists = True

                        if layersOrder[i] == -1:
                            layersOrder[i] = next_layer
                            next_layer += 1
                        # layer must be put higher in render order (same cmd was insered more times)
                        # TODO delete rendurant cmds from cmd file?
                        else:
                            for j, l_order in enumerate(layersOrder):
                                if l_order > layersOrder[i]:
                                    layersOrder[j] -= 1
                            layersOrder[i] = next_layer - 1

                        break
                if exists:
                    continue

                mapLayer = classLayer(name=name,
                                      cmd=cmd,
                                      Map=None,
                                      hidden=True,
                                      render=False,
                                      mapfile=mapFile,
                                      **args)
                mapLayer.GetRenderMgr().updateProgress.connect(self.GetRenderMgr().ReportProgress)
                if render_env:
                    mapLayer.GetRenderMgr().UpdateRenderEnv(render_env)
                    render_env = dict()

                newLayer = self._addLayer(mapLayer)

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

                # owned layer found in cmd file is added into proper rendering
                # position
                else:
                    reorderedLayers[layersOrder[i]] = layer

            self.SetLayers(reorderedLayers)

        except IOError as e:
            grass.warning(
                _("Unable to read cmdfile '%(cmd)s'. Details: %(det)s") %
                {'cmd': self.cmdfile, 'det': e})
            return

        Debug.msg(1, "Map.GetLayersFromCmdFile(): cmdfile=%s, nlayers=%d" %
                  (self.cmdfile, nlayers))

        self._giface.updateMap.emit(render=False)

    def Render(self, *args, **kwargs):
        """Render layer to image.

        For input params and returned data see overridden method in Map class.
        """
        return Map.Render(self, *args, **kwargs)

    def AddLayer(self, *args, **kwargs):
        """Adds generic map layer to list of layers.

        For input params and returned data see overridden method in Map class.
        """
        driver = UserSettings.Get(group='display', key='driver', subkey='type')

        if driver == 'png':
            os.environ["GRASS_RENDER_IMMEDIATE"] = "png"
        else:
            os.environ["GRASS_RENDER_IMMEDIATE"] = "cairo"

        layer = Map.AddLayer(self, *args, **kwargs)

        del os.environ["GRASS_RENDER_IMMEDIATE"]

        return layer


class Layer(object):
    """@implements core::giface::Layer"""

    def __init__(self, maplayer):
        self._maplayer = maplayer

    def __getattr__(self, name):
        if name == 'cmd':
            return cmdtuple_to_list(self._maplayer.GetCmd())
        elif hasattr(self._maplayer, name):
            return getattr(self._maplayer, name)
        elif name == 'maplayer':
            return self._maplayer
        elif name == 'type':
            return self._maplayer.GetType()
            # elif name == 'ctrl':
        elif name == 'label':
            return self._maplayer.GetName()
            # elif name == 'propwin':


class LayerList(object):
    """@implements core::giface::LayerList"""

    def __init__(self, map, giface):
        self._map = map
        self._giface = giface
        self._index = 0

    def __len__(self):
        return len(self._map.GetListOfLayers())

    def __iter__(self):
        return self

    def __next__(self):
        items = self._map.GetListOfLayers()
        try:
            result = items[self._index]
        except IndexError:
            raise StopIteration
        self._index += 1
        return result

    def next(self):
        return self.__next__()

    def GetSelectedLayers(self, checkedOnly=True):
        # hidden and selected vs checked and selected
        items = self._map.GetListOfLayers()
        layers = []
        for item in items:
            layer = Layer(item)
            layers.append(layer)
        return layers

    def GetSelectedLayer(self, checkedOnly=False):
        """Returns selected layer or None when there is no selected layer."""
        layers = self.GetSelectedLayers()
        if len(layers) > 0:
            return layers[0]
        else:
            return None

    def AddLayer(self, ltype, name=None, checked=None,
                 opacity=1.0, cmd=None):
        """Adds a new layer to the layer list.

        Launches property dialog if needed (raster, vector, etc.)

        :param ltype: layer type (raster, vector, raster_3d, ...)
        :param name: layer name
        :param checked: if True layer is checked
        :param opacity: layer opacity level
        :param cmd: command (given as a list)
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
    """@implements GrassInterface"""

    def __init__(self, mapframe):
        StandaloneGrassInterface.__init__(self)
        self._mapframe = mapframe

    def GetLayerList(self):
        return LayerList(self._mapframe.GetMap(), giface=self)

    def GetMapWindow(self):
        return self._mapframe.GetMapWindow()

    def GetMapDisplay(self):
        return self._mapframe

    def GetProgress(self):
        return self._mapframe.GetProgressBar()

    def ShowStatusbar(self, show=True):
        if not self._mapframe.statusbarManager:
            self._mapframe.CreateStatusbar()

        self._mapframe.statusbarManager.Show(show)

    def IsStatusbarShown(self):
        if not self._mapframe.statusbarManager:
            return False

        return self._mapframe.statusbarManager.IsShown()

    def ShowAllToolbars(self, show=True):
        if not show:  # hide
            action = self._mapframe.RemoveToolbar
        else:
            action = self._mapframe.AddToolbar
        toolbars = self._mapframe.GetToolbarNames()
        if not toolbars:
            toolbars.append('map')
        for toolbar in toolbars:
            action(toolbar)

    def AreAllToolbarsShown(self):
        toolbar = self._mapframe.GetMapToolbar()
        if toolbar is None:
            return False

        return toolbar.IsShown()


class DMonFrame(MapFrame):

    def OnZoomToMap(self, event):
        layers = self.MapWindow.GetMap().GetListOfLayers()
        self.MapWindow.ZoomToMap(layers=layers)

    def OnSize(self, event):
        super(DMonFrame, self).OnSize(event)

        # update env file
        width, height = self.MapWindow.GetClientSize()
        for line in fileinput.input(monFile['env'], inplace=True):
            if 'GRASS_RENDER_WIDTH' in line:
                print('GRASS_RENDER_WIDTH={0}'.format(width))
            elif 'GRASS_RENDER_HEIGHT' in line:
                print('GRASS_RENDER_HEIGHT={0}'.format(height))
            else:
                print(line.rstrip('\n'))


class MapApp(wx.App):

    def OnInit(self):
        grass.set_raise_on_error(True)
        # actual use of StandaloneGrassInterface not yet tested
        # needed for adding functionality in future
        self._giface = DMonGrassInterface(None)

        return True

    def CreateMapFrame(self, name, decorations=True):
        toolbars = []
        if decorations:
            toolbars.append('map')

        if __name__ == "__main__":
            self.cmdTimeStamp = os.path.getmtime(monFile['cmd'])
            self.Map = DMonMap(giface=self._giface, cmdfile=monFile['cmd'],
                               mapfile=monFile['map'])

            self.timer = wx.PyTimer(self.watcher)
            # check each 0.5s
            global mtime
            mtime = 500
            self.timer.Start(mtime)
        else:
            self.Map = None

        self.mapFrm = DMonFrame(
            parent=None,
            id=wx.ID_ANY,
            title=name,
            Map=self.Map,
            giface=self._giface,
            size=monSize,
            toolbars=toolbars,
            statusbar=decorations)

        # FIXME: hack to solve dependency
        self._giface._mapframe = self.mapFrm

        self.mapFrm.GetMapWindow().SetAlwaysRenderEnabled(False)

        # set default properties
        self.mapFrm.SetProperties(render=UserSettings.Get(
            group='display', key='autoRendering', subkey='enabled'),
                                 mode=UserSettings.Get(
            group='display', key='statusbarMode', subkey='selection'),
                                 alignExtent=UserSettings.Get(
            group='display', key='alignExtent', subkey='enabled'),
                                 constrainRes=UserSettings.Get(
            group='display', key='compResolution', subkey='enabled'),
                                 showCompExtent=UserSettings.Get(
            group='display', key='showCompExtent', subkey='enabled'))

        self.Map.saveToFile.connect(lambda cmd: self.mapFrm.DOutFile(cmd))
        self.Map.dToRast.connect(lambda cmd: self.mapFrm.DToRast(cmd))
        self.Map.query.connect(
            lambda ltype,
            maps: self.mapFrm.SetQueryLayersAndActivate(
                ltype=ltype,
                maps=maps))

        return self.mapFrm

    def OnExit(self):
        if __name__ == "__main__":
            # stop the timer
            if self.timer.IsRunning:
                self.timer.Stop()
            # terminate thread
            for f in six.itervalues(monFile):
                try_remove(f)
        return True

    def watcher(self):
        """Redraw, if new layer appears (check's timestamp of
        cmdfile)
        """
        ###
        # TODO: find a better solution
        ###
        # the check below disabled, it's too much invasive to call
        # g.gisenv in the watcher...
        # try:
        # GISBASE and other system enviromental variables can not be used
        # since the process inherited them from GRASS
        # raises exception when vaiable does not exists
        # grass.gisenv()['GISDBASE']
        # except KeyError:
        #    self.timer.Stop()
        #    return

        # todo: events
        try:
            currentCmdFileTime = os.path.getmtime(monFile['cmd'])
            if currentCmdFileTime > self.cmdTimeStamp:
                self.timer.Stop()
                self.cmdTimeStamp = currentCmdFileTime
                self.mapFrm.GetMap().GetLayersFromCmdFile()
                self.timer.Start(mtime)
        except OSError as e:
            grass.warning("%s" % e)
            self.timer.Stop()

    def GetMapFrame(self):
        """Get Map Frame instance"""
        return self.mapFrm

if __name__ == "__main__":
    if len(sys.argv) != 6:
        print(__doc__)
        sys.exit(0)

    # set command variable
    monName = sys.argv[1]
    monPath = sys.argv[2]
    monFile = {'map': os.path.join(monPath, 'map.ppm'),
               'cmd': os.path.join(monPath, 'cmd'),
               'env': os.path.join(monPath, 'env')}

    # monitor size
    monSize = (int(sys.argv[3]), int(sys.argv[4]))

    monDecor = not bool(int(sys.argv[5]))
    grass.verbose(_("Starting map display <%s>...") % (monName))

    # create pid file
    pidFile = os.path.join(monPath, "pid")
    fd = open(pidFile, 'w')
    if not fd:
        grass.fatal(_("Unable to create file <%s>") % pidFile)
    fd.write("%s\n" % os.getpid())
    fd.close()

    RunCommand('g.gisenv',
               set='MONITOR_%s_PID=%d' % (monName.upper(), os.getpid()))

    start = time.time()
    gmMap = MapApp(0)
    mapFrame = gmMap.CreateMapFrame(monName, monDecor)
    mapFrame.Show()
    Debug.msg(1, "WxMonitor started in %.6f sec" %
              (time.time() - start))

    gmMap.MainLoop()

    grass.verbose(_("Stopping map display <%s>...") % (monName))

    # clean up GRASS env variables
    try:
        shutil.rmtree(monPath)
    except OSError:
        pass

    RunCommand('g.gisenv',
               unset='MONITOR')

    sys.exit(0)
