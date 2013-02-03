"""!
@package mapdisp.main

@brief Start Map Display as standalone application

Classes:
 - mapdisp::MapApp

Usage:
python mapdisp/main.py monitor-identifier /path/to/map/file /path/to/command/file /path/to/env/file

(C) 2006-2011 by the GRASS Development Team

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
    sys.path.append(os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython'))
from core          import globalvar
import wx

from core          import utils
from core.giface   import StandaloneGrassInterface
from core.gcmd     import RunCommand
from core.render   import Map
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
    def __init__(self, gisrc = None, cmdfile = None, mapfile = None, envfile = None, monitor = None):
        """!Map composition (stack of map layers and overlays)

        @param cmdline full path to the cmd file (defined by d.mon)
        @param mapfile full path to the map file (defined by d.mon)
        @param envfile full path to the env file (defined by d.mon)
        @param monitor name of monitor (defined by d.mon)
        """

        Map.__init__(self)

        # environment settings
        self.env   = dict()

        self.cmdfile = cmdfile
        self.envfile = envfile
        self.monitor = monitor

        if mapfile:
            self.mapfileCmd = mapfile
            self.maskfileCmd = os.path.splitext(mapfile)[0] + '.pgm'

        # generated file for g.pnmcomp output for rendering the map
        self.mapfile = grass.tempfile(create = False) + '.ppm'

        self._writeEnvFile(self.env) # self.env is expected to be defined in parent class
        self._writeEnvFile({"GRASS_PNG_READ" : "TRUE"})

    def GetLayersFromCmdFile(self):
        """!Get list of map layers from cmdfile
        """
        if not self.cmdfile:
            return
        
        nlayers = 0
        try:
            fd = open(self.cmdfile, 'r')
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

                self.AddLayer(ltype = ltype, command = cmd, active = False, name = name)
                nlayers += 1
        except IOError, e:
            grass.warning(_("Unable to read cmdfile '%(cmd)s'. Details: %(det)s") % \
                              { 'cmd' : self.cmdfile, 'det' : e })
            return
        
        fd.close()

        Debug.msg(1, "Map.GetLayersFromCmdFile(): cmdfile=%s" % self.cmdfile)
        Debug.msg(1, "                            nlayers=%d" % nlayers)
                
    def _parseCmdFile(self):
        """!Parse cmd file for standalone application
        """
        nlayers = 0
        try:
            fd = open(self.cmdfile, 'r')
            grass.try_remove(self.mapfile)
            cmdLines = fd.readlines()
            RunCommand('g.gisenv',
                       set = 'MONITOR_%s_CMDFILE=' % self.monitor)

            for cmd in cmdLines:
                cmdStr = utils.split(cmd.strip())
                cmd = utils.CmdToTuple(cmdStr)
                RunCommand(cmd[0], **cmd[1])
                nlayers += 1
            
            RunCommand('g.gisenv',
                       set = 'MONITOR_%s_CMDFILE=%s' % (self.monitor, self.cmdfile))
        except IOError, e:
            grass.warning(_("Unable to read cmdfile '%(cmd)s'. Details: %(det)s") % \
                              { 'cmd' : self.cmdfile, 'det' : e })
            return
        
        fd.close()

        Debug.msg(1, "Map.__parseCmdFile(): cmdfile=%s" % self.cmdfile)
        Debug.msg(1, "                      nlayers=%d" % nlayers)
        
        return nlayers

    def _renderCmdFile(self, force, windres):
        if not force:
            return ([self.mapfileCmd],
                    [self.maskfileCmd],
                    ['1.0'])
        
        region = os.environ["GRASS_REGION"] = self.SetRegion(windres)
        self._writeEnvFile({'GRASS_REGION' : region})
        currMon = grass.gisenv()['MONITOR']
        if currMon != self.monitor:
            RunCommand('g.gisenv',
                       set = 'MONITOR=%s' % self.monitor)
        
        grass.try_remove(self.mapfileCmd) # GRASS_PNG_READ is TRUE
        
        nlayers = self._parseCmdFile()
        if self.overlays:
            RunCommand('g.gisenv',
                       unset = 'MONITOR') # GRASS_RENDER_IMMEDIATE doesn't like monitors
            driver = UserSettings.Get(group = 'display', key = 'driver', subkey = 'type')
            if driver == 'png':
                os.environ["GRASS_RENDER_IMMEDIATE"] = "png"
            else:
                os.environ["GRASS_RENDER_IMMEDIATE"] = "cairo"
            self._renderLayers(overlaysOnly = True)
            del os.environ["GRASS_RENDER_IMMEDIATE"]
            RunCommand('g.gisenv',
                       set = 'MONITOR=%s' % currMon)
        
        if currMon != self.monitor:
            RunCommand('g.gisenv',
                       set = 'MONITOR=%s' % currMon)
            
        if nlayers > 0:
            return ([self.mapfileCmd],
                    [self.maskfileCmd],
                    ['1.0'])
        else:
            return ([], [], [])
    
    def _writeEnvFile(self, data):
        """!Write display-related variable to the file (used for
        standalone app)
        """
        if not self.envfile:
            return
        
        try:
            fd = open(self.envfile, "r")
            for line in fd.readlines():
                key, value = line.split('=')
                if key not in data.keys():
                    data[key] = value
            fd.close()
            
            fd = open(self.envfile, "w")
            for k, v in data.iteritems():
                fd.write('%s=%s\n' % (k.strip(), str(v).strip()))
        except IOError, e:
            grass.warning(_("Unable to open file '%(file)s' for writting. Details: %(det)s") % \
                              { 'cmd' : self.envfile, 'det' : e })
            return
        
        fd.close()
    
    def ChangeMapSize(self, (width, height)):
        """!Change size of rendered map.
        
        @param width,height map size
        """
        Map.ChangeMapSize(self, (width, height))
        
        self._writeEnvFile({'GRASS_WIDTH' : self.width,
                            'GRASS_HEIGHT' : self.height})
    
    def GetMapsMasksAndOpacities(self, force, windres):
        """!
        Used by Render function.
        
        @return maps, masks, opacities
        """
        return self._renderCmdFile(force, windres)


class Layer(object):
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
            #elif name == 'maplayer' : None,
            #elif name == 'propwin':


class LayerList(object):
    def __init__(self, map):
        self._map = map

#    def __iter__(self):
#        for in :
#            yield

    def GetSelectedLayers(self, checkedOnly=True):
        # hidden and selected vs checked and selected
        items = self._map.GetListOfLayers()
        layers = []
        for item in items:
            layer = Layer(item)
            layers.append(layer)
        return layers


class DMonGrassInterface(StandaloneGrassInterface):
    def __init__(self, mapframe):
        StandaloneGrassInterface.__init__(self)
        self._mapframe = mapframe

    def GetLayerList(self):
        return LayerList(self._mapframe.GetMap())


class DMonFrame(MapFrame):
    def OnZoomToMap(self, event):
        layers = self.MapWindow.GetMap().GetListOfLayers()
        self.MapWindow.ZoomToMap(layers = layers)
        

class MapApp(wx.App):
    def OnInit(self):
        wx.InitAllImageHandlers()
        if __name__ == "__main__":
            self.cmdTimeStamp = os.path.getmtime(monFile['cmd'])
            self.Map = DMonMap(cmdfile = monFile['cmd'], mapfile = monFile['map'],
                           envfile = monFile['env'], monitor = monName)
        else:
            self.Map = None

        # actual use of StandaloneGrassInterface not yet tested
        # needed for adding functionality in future
        giface = DMonGrassInterface(None)
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
                self.mapFrm.OnDraw(None)
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

    import gettext
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)
    
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
