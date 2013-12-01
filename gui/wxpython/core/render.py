"""!
@package core.render

@brief Rendering map layers and overlays into map composition image.

@todo Implement RenderManager also for other layers (see WMS
implementation for details)

@todo Render classes should not care about updating statusbar (change emiting events).

Classes:
 - render::Layer
 - render::MapLayer
 - render::Overlay
 - render::Map

(C) 2006-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import glob
import math
import copy
import tempfile
import types

import wx

from grass.script import core as grass
from grass.pydispatch.signal import Signal

from core          import utils
from core.utils import _
from core.ws       import RenderWMSMgr
from core.gcmd     import GException, GError, RunCommand
from core.debug    import Debug
from core.settings import UserSettings


USE_GPNMCOMP = True

class Layer(object):
    """!Virtual class which stores information about layers (map layers and
    overlays) of the map composition.
    
    - For map layer use MapLayer class.
    - For overlays use Overlay class.
    """
    def __init__(self, ltype, cmd, Map, name = None,
                 active = True, hidden = False, opacity = 1.0):
        """!Create new instance
        
        @todo pass cmd as tuple instead of list
        
        @param ltype layer type ('raster', 'vector', 'overlay', 'command', etc.)
        @param cmd GRASS command to render layer,
        given as list, e.g. ['d.rast', 'map=elevation@PERMANENT']
        @param Map render.Map instance
        @param name layer name, e.g. 'elevation@PERMANENT' (for layer tree)
        @param active layer is active, will be rendered only if True
        @param hidden layer is hidden, won't be listed in Layer Manager if True
        @param opacity layer opacity <0;1>
        """
        
        # generated file for each layer
        if USE_GPNMCOMP or ltype == 'overlay':
            if ltype == 'overlay':
                tempfile_sfx = ".png"
            else:
                tempfile_sfx = ".ppm"

            self.mapfile = tempfile.mkstemp(suffix = tempfile_sfx)[1]
            self.maskfile = self.mapfile.rsplit(".",1)[0] + ".pgm"
        else:
            self.mapfile = self.maskfile = None

        # stores class which manages rendering instead of simple command - e.g. WMS
        self.renderMgr = None

        self.Map = Map
        self.type      = None
        self.SetType(ltype)
        self.name  = name
        
        if self.type == 'command':
            self.cmd = list()
            for c in cmd:
                self.cmd.append(utils.CmdToTuple(c))
        else:
            self.cmd = utils.CmdToTuple(cmd)
        
        self.active  = active
        self.hidden  = hidden
        self.opacity = opacity
        
        self.forceRender = True
        
        Debug.msg (3, "Layer.__init__(): type=%s, cmd='%s', name=%s, " \
                       "active=%d, opacity=%d, hidden=%d" % \
                       (self.type, self.GetCmd(string = True), self.name, self.active,
                        self.opacity, self.hidden))
                
    def __del__(self):
        Debug.msg (3, "Layer.__del__(): layer=%s, cmd='%s'" %
                   (self.name, self.GetCmd(string = True)))
        
    def Render(self):
        """!Render layer to image
        
        @return rendered image filename
        @return None on error or if cmdfile is defined
        """
        if not self.cmd:
            return None
        
        # ignore in 2D
        if self.type == '3d-raster':
            return None
        
        Debug.msg (3, "Layer.Render(): type=%s, name=%s" % \
                       (self.type, self.name))
        
        # prepare command for each layer
        layertypes = utils.command2ltype.values() + ['overlay', 'command']
        
        if self.type not in layertypes:
            raise GException(_("<%(name)s>: layer type <%(type)s> is not supported") % \
                                 {'type' : self.type, 'name' : self.name})
        
        if self.mapfile:
            os.environ["GRASS_PNGFILE"] = self.mapfile
        
        # execute command
        try:
            if self.type == 'command':
                read = False
                for c in self.cmd:
                    ret, msg = self._runCommand(c)
                    if ret != 0:
                        break
                    if not read:
                        os.environ["GRASS_PNG_READ"] = "TRUE"
                
                os.environ["GRASS_PNG_READ"] = "FALSE"
            else:
                ret, msg = self._runCommand(self.cmd)
            if ret != 0:
                sys.stderr.write(_("Command '%s' failed\n") % self.GetCmd(string = True))
                if msg:
                    sys.stderr.write(_("Details: %s\n") % msg)
                raise GException()
        
        except GException:
            # clean up after problems
            for f in [self.mapfile, self.maskfile]:
                if not f:
                    continue
                grass.try_remove(f)
                f = None
        
        # stop monitor
        if self.mapfile and "GRASS_PNGFILE" in os.environ:
            del os.environ["GRASS_PNGFILE"]
        
        self.forceRender = False
        
        return self.mapfile
    
    def _runCommand(self, cmd):
        """!Run command to render data
        """ 
        if self.type == 'wms':
            ret = 0
            msg = ''
            self.renderMgr.Render(cmd)
        else:
            ret, msg = RunCommand(cmd[0],
                                  getErrorMsg = True,
                                  quiet = True,
                                  **cmd[1])
        
        return ret, msg
    
    def GetCmd(self, string = False):
        """!Get GRASS command as list of string.
        
        @param string get command as string if True otherwise as list
        
        @return command list/string
        """
        if string:
            if self.type == 'command':
                scmd = []
                for c in self.cmd:
                    scmd.append(utils.GetCmdString(c))
                
                return ';'.join(scmd)
            else:
                return utils.GetCmdString(self.cmd)
        else:
            return self.cmd

    def GetType(self):
        """!Get map layer type"""
        return self.type
    
    def GetElement(self):
        """!Get map element type"""
        if self.type == 'raster':
            return 'cell'
        return self.type
    
    def GetOpacity(self, float = False):
        """
        Get layer opacity level
        
        @param float get opacity level in <0,1> otherwise <0,100>
        
        @return opacity level
        """
        if float:
            return self.opacity
        
        return int (self.opacity * 100)

    def GetName(self, fullyQualified = True):
        """!Get map layer name

        @param fullyQualified True to return fully qualified name as a
        string 'name@mapset' otherwise directory { 'name', 'mapset' }
        is returned

        @return string / directory
        """
        if fullyQualified:
            return self.name
        else:
            if '@' in self.name:
                return { 'name' : self.name.split('@')[0],
                         'mapset' : self.name.split('@')[1] }
            else:
                return { 'name' : self.name,
                         'mapset' : '' }
        
    def IsActive(self):
        """!Check if layer is activated for rendering"""
        return self.active

    def IsHidden(self):
        """!Check if layer is hidden"""
        return self.hidden
    
    def SetType(self, ltype):
        """!Set layer type"""
        if ltype not in utils.command2ltype.values() + ['overlay', 'command']:
            raise GException(_("Unsupported map layer type '%s'") % ltype)
        
        if ltype == 'wms' and not isinstance(self.renderMgr, RenderWMSMgr):
            self.renderMgr = RenderWMSMgr(layer=self, 
                                          mapfile=self.mapfile, 
                                          maskfile=self.maskfile)
        elif self.type == 'wms' and ltype != 'wms':
            self.renderMgr = None
        
        self.type = ltype

    def SetName(self, name):
        """!Set layer name"""
        self.name = name
        
    def SetActive(self, enable = True):
        """!Active or deactive layer"""
        self.active = bool(enable)

    def SetHidden(self, enable = False):
        """!Hide or show map layer in Layer Manager"""
        self.hidden = bool(enable)

    def SetOpacity(self, value):
        """!Set opacity value"""
        if value < 0:
            value = 0.
        elif value > 1:
            value = 1.
        
        self.opacity = float(value)
        
    def SetCmd(self, cmd):
        """!Set new command for layer"""
        if self.type == 'command':
            self.cmd = []
            for c in cmd:
                self.cmd.append(utils.CmdToTuple(c))
        else:
            self.cmd = utils.CmdToTuple(cmd)
        Debug.msg(3, "Layer.SetCmd(): cmd='%s'" % self.GetCmd(string = True))
        
        # for re-rendering
        self.forceRender = True

    def IsDownloading(self):
        """!Is data downloading from web server e. g. wms"""
        if self.renderMgr is None:
            return False
        else:
            return self.renderMgr.IsDownloading()

    def AbortThread(self):
        """!Abort running thread e. g. downloading data"""
        if self.renderMgr is None:
            return
        else:
            self.renderMgr.Abort()

    def GetRenderMgr(self):
        """!Get render manager """
        return self.renderMgr

class MapLayer(Layer):
    def __init__(self, ltype, cmd, Map, name = None,
                 active = True, hidden = False, opacity = 1.0): 
        """!Represents map layer in the map canvas
        
        @param ltype layer type ('raster', 'vector', 'command', etc.)
        @param cmd GRASS command to render layer,
        given as list, e.g. ['d.rast', 'map=elevation@PERMANENT']
        @param Map render.Map instance
        @param name layer name, e.g. 'elevation@PERMANENT' (for layer tree) or None
        @param active layer is active, will be rendered only if True
        @param hidden layer is hidden, won't be listed in Layer Manager if True
        @param opacity layer opacity <0;1>
        """
        Layer.__init__(self, ltype, cmd, Map, name,
                       active, hidden, opacity)
        
    def GetMapset(self):
        """!Get mapset of map layer
        
        @return mapset name
        @return '' on error (no name given)
        """
        if not self.name:
            return ''
        
        try:
            return self.name.split('@')[1]
        except IndexError:
            return self.name
        
class Overlay(Layer):
    def __init__(self, id, ltype, cmd, Map,
                 active = True, hidden = True, opacity = 1.0):
        """!Represents overlay displayed in map canvas
        
        @param id overlay id (for PseudoDC)
        @param type overlay type ('barscale', 'legend', etc.)
        @param cmd GRASS command to render overlay,
        given as list, e.g. ['d.legend', 'map=elevation@PERMANENT']
        @param Map render.Map instance
        @param active layer is active, will be rendered only if True
        @param hidden layer is hidden, won't be listed in Layer Manager if True
        @param opacity layer opacity <0;1>
        """
        Layer.__init__(self, 'overlay', cmd, Map, ltype,
                       active, hidden, opacity)
        self.id = id
        
class Map(object):
    def __init__(self, gisrc = None):
        """!Map composition (stack of map layers and overlays)
        
        @param gisrc alternative gisrc (used eg. by georectifier)
        """
        # region/extent settigns
        self.wind      = dict() # WIND settings (wind file)
        self.region    = dict() # region settings (g.region)
        self.width     = 640    # map width
        self.height    = 480    # map height
        
        # list of layers
        self.layers    = list()  # stack of available GRASS layer
        
        self.overlays  = list()  # stack of available overlays
        self.ovlookup  = dict()  # lookup dictionary for overlay items and overlays
        
        # environment settings
        self.env   = dict()
        
        # path to external gisrc
        self.gisrc = gisrc
        
        # generated file for g.pnmcomp output for rendering the map
        self.mapfile = grass.tempfile(create = False) + '.ppm'
        
        # setting some initial env. variables
        self._initGisEnv() # g.gisenv
        self.GetWindow()
        
        # info to report progress
        self.progressInfo = None

        # GRASS environment variable (for rendering)
        self.env = {"GRASS_BACKGROUNDCOLOR" : "000000",
               "GRASS_PNG_COMPRESSION" : "0",
               "GRASS_TRUECOLOR"       : "TRUE",
               "GRASS_TRANSPARENT"     : "TRUE",
               "GRASS_PNG_READ"        : "FALSE",
               }

        for k, v in self.env.iteritems():
            os.environ[k] = v

        # projection info
        self.projinfo = self._projInfo()

        # is some layer being downloaded?
        self.downloading = False
        
        self.layerChanged = Signal('Map.layerChanged')
        self.updateProgress = Signal('Map.updateProgress')

    def _runCommand(self, cmd, **kwargs):
        """!Run command in environment defined by self.gisrc if
        defined"""
        # use external gisrc if defined
        gisrc_orig = os.getenv("GISRC")
        if self.gisrc:
            os.environ["GISRC"] = self.gisrc
        
        ret = cmd(**kwargs)
        
        # back to original gisrc
        if self.gisrc:
            os.environ["GISRC"] = gisrc_orig
        
        return ret
    
    def _initGisEnv(self):
        """!Stores GRASS variables (g.gisenv) to self.env variable
        """
        if not os.getenv("GISBASE"):
            sys.exit(_("GISBASE not set. You must be in GRASS GIS to run this program."))
        
        self.env = self._runCommand(grass.gisenv)
            
    def GetProjInfo(self):
        """!Get projection info"""
        return self.projinfo
    
    def _projInfo(self):
        """!Return region projection and map units information
        """
        projinfo = dict()
        if not grass.find_program('g.proj', '--help'):
            sys.exit(_("GRASS module '%s' not found. Unable to start map "
                       "display window.") % 'g.proj')
        
        ret = self._runCommand(RunCommand, prog = 'g.proj',
                               read = True, flags = 'p')
        
        if not ret:
            return projinfo
        
        for line in ret.splitlines():
            if ':' in line:
                key, val = map(lambda x: x.strip(), line.split(':'))
                if key in ['units']:
                    val = val.lower()
                projinfo[key] = val
            elif "XY location (unprojected)" in line:
                projinfo['proj'] = 'xy'
                projinfo['units'] = ''
                break
        
        return projinfo
    
    def GetWindow(self):
        """!Read WIND file and set up self.wind dictionary"""
        # FIXME: duplicated region WIND == g.region (at least some values)
        filename = os.path.join (self.env['GISDBASE'],
                                 self.env['LOCATION_NAME'],
                                 self.env['MAPSET'],
                                 "WIND")
        try:
            windfile = open (filename, "r")
        except IOError, e:
            sys.exit(_("Error: Unable to open '%(file)s'. Reason: %(ret)s. wxGUI exited.\n") % \
                         { 'file' : filename, 'ret' : e})
        
        for line in windfile.readlines():
            line = line.strip()
            key, value = line.split(":", 1)
            self.wind[key.strip()] = value.strip()
        
        windfile.close()
        
        return self.wind
        
    def AdjustRegion(self):
        """!Adjusts display resolution to match monitor size in
        pixels. Maintains constant display resolution, not related to
        computational region. Do NOT use the display resolution to set
        computational resolution. Set computational resolution through
        g.region.
        """
        mapwidth    = abs(self.region["e"] - self.region["w"])
        mapheight   = abs(self.region['n'] - self.region['s'])
        
        self.region["nsres"] =  mapheight / self.height
        self.region["ewres"] =  mapwidth  / self.width
        self.region['rows']  = round(mapheight / self.region["nsres"])
        self.region['cols']  = round(mapwidth / self.region["ewres"])
        self.region['cells'] = self.region['rows'] * self.region['cols']
        
        Debug.msg (3, "Map.AdjustRegion(): %s" % self.region)
        
        return self.region

    def AlignResolution(self):
        """!Sets display extents to even multiple of current
        resolution defined in WIND file from SW corner. This must be
        done manually as using the -a flag can produce incorrect
        extents.
        """
        # new values to use for saving to region file
        new = {}
        n = s = e = w = 0.0
        nsres = ewres = 0.0
        
        # Get current values for region and display
        reg = self.GetRegion()
        nsres = reg['nsres']
        ewres = reg['ewres']
        
        n = float(self.region['n'])
        s = float(self.region['s'])
        e = float(self.region['e'])
        w = float(self.region['w'])
        
        # Calculate rows, columns, and extents
        new['rows'] = math.fabs(round((n-s)/nsres))
        new['cols'] = math.fabs(round((e-w)/ewres))
        
        # Calculate new extents
        new['s'] = nsres * round(s / nsres)
        new['w'] = ewres * round(w / ewres)
        new['n'] = new['s'] + (new['rows'] * nsres)
        new['e'] = new['w'] + (new['cols'] * ewres)
        
        return new

    def AlignExtentFromDisplay(self):
        """!Align region extent based on display size from center
        point"""
        # calculate new bounding box based on center of display
        if self.region["ewres"] > self.region["nsres"]:
            res = self.region["ewres"]
        else:
            res = self.region["nsres"]
        
        Debug.msg(3, "Map.AlignExtentFromDisplay(): width=%d, height=%d, res=%f, center=%f,%f" % \
                      (self.width, self.height, res, self.region['center_easting'],
                       self.region['center_northing']))
            
        ew = (self.width / 2) * res
        ns = (self.height / 2) * res
        
        self.region['n'] = self.region['center_northing'] + ns
        self.region['s'] = self.region['center_northing'] - ns
        self.region['e'] = self.region['center_easting'] + ew
        self.region['w'] = self.region['center_easting'] - ew
        
        # LL locations
        if self.projinfo['proj'] == 'll':
            self.region['n'] = min(self.region['n'], 90.0)
            self.region['s'] = max(self.region['s'], -90.0)
        
    def ChangeMapSize(self, size):
        """!Change size of rendered map.
        
        @param width,height map size given as tuple
        """
        try:
            self.width  = int(size[0])
            self.height = int(size[1])
            if self.width < 1 or self.height < 1:
                sys.stderr.write(_("Invalid map size %d,%d\n") % (self.width, self.height))
                raise ValueError
        except ValueError:
            self.width  = 640
            self.height = 480
        
        Debug.msg(2, "Map.ChangeMapSize(): width=%d, height=%d" % \
                      (self.width, self.height))
        
    def GetRegion(self, rast=None, zoom=False, vect=None, rast3d=None, regionName=None,
                  n=None, s=None, e=None, w=None, default=False,
                  update=False, add3d=False):
        """!Get region settings (g.region -upgc)
        
        Optionally extent, raster or vector map layer can be given.
        
        @param rast list of raster maps
        @param zoom zoom to raster map (ignore NULLs)
        @param vect list of vector maps
        @param rast3d 3d raster map (not list, no support of multiple 3d rasters in g.region)
        @param regionName  named region or None
        @param n,s,e,w force extent
        @param default force default region settings
        @param update if True update current display region settings
        @param add3d add 3d region settings
        
        @return region settings as dictionary, e.g. {
        'n':'4928010', 's':'4913700', 'w':'589980',...}
        
        @see GetCurrentRegion()
        """
        region = {}
        
        tmpreg = os.getenv("GRASS_REGION")
        if tmpreg:
            del os.environ["GRASS_REGION"]
        
        # use external gisrc if defined
        gisrc_orig = os.getenv("GISRC")
        if self.gisrc:
            os.environ["GISRC"] = self.gisrc
        
        # do not update & shell style output
        cmd = {}
        cmd['flags'] = 'ugpc'
        
        if default:
            cmd['flags'] += 'd'
        
        if add3d:
            cmd['flags'] += '3'
            
        if regionName:
            cmd['region'] = regionName
        
        if n:
            cmd['n'] = n
        if s:
            cmd['s'] = s
        if e:
            cmd['e'] = e
        if w:
            cmd['w'] = w
        
        if rast:
            if zoom:
                cmd['zoom'] = rast[0]
            else:
                cmd['rast'] = ','.join(rast)
        
        if vect:
            cmd['vect'] = ','.join(vect)

        if rast3d:
            cmd['rast3d'] = rast3d
        
        ret, reg, msg = RunCommand('g.region',
                                   read = True,
                                   getErrorMsg = True,
                                   **cmd)
        
        if ret != 0:
            if rast:
                message = _("Unable to zoom to raster map <%s>.") % rast[0] + \
                    "\n\n" + _("Details:") + " %s" % msg
            elif vect:
                message = _("Unable to zoom to vector map <%s>.") % vect[0] + \
                    "\n\n" + _("Details:") + " %s" % msg
            elif rast3d:
                message = _("Unable to zoom to 3d raster map <%s>.") % rast3d + \
                    "\n\n" + _("Details:") + " %s" % msg
            else:
                message = _("Unable to get current geographic extent. "
                            "Force quiting wxGUI. Please manually run g.region to "
                            "fix the problem.")
            GError(message)
            return self.region
        
        for r in reg.splitlines():
            key, val = r.split("=", 1)
            try:
                region[key] = float(val)
            except ValueError:
                region[key] = val
        
        # back to original gisrc
        if self.gisrc:
            os.environ["GISRC"] = gisrc_orig
        
        # restore region
        if tmpreg:
            os.environ["GRASS_REGION"] = tmpreg
        
        Debug.msg (3, "Map.GetRegion(): %s" % region)
        
        if update:
            self.region = region
        
        return region

    def GetCurrentRegion(self):
        """!Get current display region settings
        
        @see GetRegion()
        """
        return self.region

    def SetRegion(self, windres = False, windres3 = False):
        """!Render string for GRASS_REGION env. variable, so that the
        images will be rendered from desired zoom level.
        
        @param windres uses resolution from WIND file rather than
        display (for modules that require set resolution like
        d.rast.num)

        @return String usable for GRASS_REGION variable or None
        """
        grass_region = ""
        
        if windres:
            compRegion = self.GetRegion(add3d = windres3)
            region = copy.copy(self.region)
            for key in ('nsres', 'ewres', 'cells'):
                region[key] = compRegion[key]
            if windres3:
                for key in ('nsres3', 'ewres3', 'tbres', 'cells3',
                            'cols3', 'rows3', 'depths'):
                    if key in compRegion:
                        region[key] = compRegion[key]
                    
        else:
            # adjust region settings to match monitor
            region = self.AdjustRegion()
        
        # read values from wind file
        try:
            for key in self.wind.keys():
                
                if key == 'north':
                    grass_region += "north: %s; " % \
                        (region['n'])
                    continue
                elif key == "south":
                    grass_region += "south: %s; " % \
                        (region['s'])
                    continue
                elif key == "east":
                    grass_region += "east: %s; " % \
                        (region['e'])
                    continue
                elif key == "west":
                    grass_region += "west: %s; " % \
                        (region['w'])
                    continue
                elif key == "e-w resol":
                    grass_region += "e-w resol: %f; " % \
                        (region['ewres'])
                    continue
                elif key == "n-s resol":
                    grass_region += "n-s resol: %f; " % \
                        (region['nsres'])
                    continue
                elif key == "cols":
                    if windres:
                        continue
                    grass_region += 'cols: %d; ' % \
                        region['cols']
                    continue
                elif key == "rows":
                    if windres:
                        continue
                    grass_region += 'rows: %d; ' % \
                        region['rows']
                    continue
                elif key == "n-s resol3" and windres3:
                    grass_region += "n-s resol3: %f; " % \
                        (region['nsres3'])
                elif key == "e-w resol3" and windres3:
                    grass_region += "e-w resol3: %f; " % \
                        (region['ewres3'])
                elif key == "t-b resol" and windres3:
                    grass_region += "t-b resol: %f; " % \
                        (region['tbres'])
                elif key == "cols3" and windres3:
                    grass_region += "cols3: %d; " % \
                        (region['cols3'])
                elif key == "rows3" and windres3:
                    grass_region += "rows3: %d; " % \
                        (region['rows3'])
                elif key == "depths" and windres3:
                    grass_region += "depths: %d; " % \
                        (region['depths'])
                else:
                    grass_region += key + ": "  + self.wind[key] + "; "
            
            Debug.msg (3, "Map.SetRegion(): %s" % grass_region)
            
            return grass_region
        
        except:
            return None
    
    def GetListOfLayers(self, ltype = None, mapset = None, name = None,
                        active = None, hidden = None):
        """!Returns list of layers of selected properties or list of
        all layers.

        @param ltype layer type, e.g. raster/vector/wms/overlay (value or tuple of values)
        @param mapset all layers from given mapset (only for maplayers)
        @param name all layers with given name
        @param active only layers with 'active' attribute set to True or False
        @param hidden only layers with 'hidden' attribute set to True or False
        
        @return list of selected layers
        """
        selected = []
        
        if type(ltype) == types.StringType:
            one_type = True
        else:
            one_type = False
        
        if one_type and ltype == 'overlay':
            llist = self.overlays
        else:
            llist = self.layers
        
        # ["raster", "vector", "wms", ... ]
        for layer in llist:
            # specified type only
            if ltype != None:
                if one_type and layer.type != ltype:
                    continue
                elif not one_type and layer.type not in ltype:
                    continue
            
            # mapset
            if (mapset != None and ltype != 'overlay') and \
                    layer.GetMapset() != mapset:
                continue
            
            # name
            if name != None and layer.name != name:
                continue
            
            # hidden and active layers
            if active != None and \
                   hidden != None:
                if layer.active == active and \
                       layer.hidden == hidden:
                    selected.append(layer)
            
            # active layers
            elif active != None:
                if layer.active == active:
                    selected.append(layer)
            
            # hidden layers
            elif hidden != None:
                if layer.hidden == hidden:
                    selected.append(layer)
            
            # all layers
            else:
                selected.append(layer)
        
        Debug.msg (3, "Map.GetListOfLayers(): numberof=%d" % len(selected))
        
        return selected

    def _renderLayers(self, force = False, overlaysOnly = False):
        """!Render all map layers into files

        @param force True to force rendering
        @param overlaysOnly True to render only overlays

        @return list of maps, masks and opacities
        """
        maps = list()
        masks = list()
        opacities = list()
        # render map layers
        if overlaysOnly:
            layers = self.overlays
        else:
            layers = self.layers + self.overlays
        
        self.downloading = False

        self.ReportProgress(layer=None)


        for layer in layers:
            # skip non-active map layers
            if not layer or not layer.active:
                continue
            
            # render
            if force or layer.forceRender:
                if not layer.Render():
                    continue

            if layer.IsDownloading():
                self.downloading = True 

            self.ReportProgress(layer=layer)

            # skip map layers when rendering fails
            if not os.path.exists(layer.mapfile):
                continue
            
            # add image to compositing list
            if layer.type != "overlay":
                maps.append(layer.mapfile)
                masks.append(layer.maskfile)
                opacities.append(str(layer.opacity))
            
            Debug.msg(3, "Map.Render() type=%s, layer=%s " % (layer.type, layer.name))

        return maps, masks, opacities
        
    def GetMapsMasksAndOpacities(self, force, windres):
        """!
        Used by Render function.
        
        @return maps, masks, opacities
        """
        return self._renderLayers(force)
    
    def Render(self, force = False, windres = False):
        """!Creates final image composite
        
        This function can conditionaly use high-level tools, which
        should be avaliable in wxPython library
        
        @param force force rendering
        @param windres use region resolution (True) otherwise display resolution
        
        @return name of file with rendered image or None
        """
        wx.BeginBusyCursor()
        # use external gisrc if defined
        gisrc_orig = os.getenv("GISRC")
        if self.gisrc:
            os.environ["GISRC"] = self.gisrc
        
        tmp_region = os.getenv("GRASS_REGION")
        os.environ["GRASS_REGION"] = self.SetRegion(windres)
        os.environ["GRASS_WIDTH"]  = str(self.width)
        os.environ["GRASS_HEIGHT"] = str(self.height)
        driver = UserSettings.Get(group = 'display', key = 'driver', subkey = 'type')
        if driver == 'png':
            os.environ["GRASS_RENDER_IMMEDIATE"] = "png"
        else:
            os.environ["GRASS_RENDER_IMMEDIATE"] = "cairo"
        
        maps, masks, opacities = self.GetMapsMasksAndOpacities(force, windres)
        
        # ugly hack for MSYS
        if sys.platform != 'win32':
            mapstr = ",".join(maps)
            maskstr = ",".join(masks)
        else:
            mapstr = ""
            for item in maps:
                mapstr += item.replace('\\', '/')
            mapstr = mapstr.rstrip(',')
            maskstr = ""
            for item in masks:
                maskstr += item.replace('\\', '/')
            maskstr = maskstr.rstrip(',')
            
        # run g.pngcomp to get composite image
        bgcolor = ':'.join(map(str, UserSettings.Get(group = 'display', key = 'bgcolor',
                                                     subkey = 'color')))
        
        if maps:
            ret, msg = RunCommand('g.pnmcomp',
                                  getErrorMsg = True,
                                  overwrite = True,
                                  input = '%s' % ",".join(maps),
                                  mask = '%s' % ",".join(masks),
                                  opacity = '%s' % ",".join(opacities),
                                  bgcolor = bgcolor,
                                  width = self.width,
                                  height = self.height,
                                  output = self.mapfile)
            
            if ret != 0:
                print >> sys.stderr, _("ERROR: Rendering failed. Details: %s") % msg
                wx.EndBusyCursor()
                return None
        
        Debug.msg (3, "Map.Render() force=%s file=%s" % (force, self.mapfile))
        
        # back to original region
        if tmp_region:
            os.environ["GRASS_REGION"] = tmp_region
        else:
            del os.environ["GRASS_REGION"]
        
        # back to original gisrc
        if self.gisrc:
            os.environ["GISRC"] = gisrc_orig
        
        wx.EndBusyCursor()
        if not maps:
            return None
        
        return self.mapfile

    def AddLayer(self, ltype, command, name = None,
                 active = True, hidden = False, opacity = 1.0, render = False,
                 pos = -1):
        """!Adds generic map layer to list of layers
        
        @param ltype layer type ('raster', 'vector', etc.)
        @param command  GRASS command given as list
        @param name layer name
        @param active layer render only if True
        @param hidden layer not displayed in layer tree if True
        @param opacity opacity level range from 0(transparent) - 1(not transparent)
        @param render render an image if True
        @param pos position in layer list (-1 for append)
        
        @return new layer on success
        @return None on failure
        """
        wx.BeginBusyCursor()
        # opacity must be <0;1>
        if opacity < 0:
            opacity = 0
        elif opacity > 1:
            opacity = 1
        layer = MapLayer(ltype = ltype, name = name, cmd = command, Map = self,
                         active = active, hidden = hidden, opacity = opacity)
        
        # add maplayer to the list of layers
        if pos > -1:
            self.layers.insert(pos, layer)
        else:
            self.layers.append(layer)
        
        Debug.msg (3, "Map.AddLayer(): layer=%s" % layer.name)
        if render:
            if not layer.Render():
                raise GException(_("Unable to render map layer <%s>.") % name)
        
        renderMgr = layer.GetRenderMgr()
        if renderMgr:
            renderMgr.dataFetched.connect(self.layerChanged)
            renderMgr.updateProgress.connect(self.ReportProgress)

        wx.EndBusyCursor()
        
        return layer

    def DeleteAllLayers(self, overlay = False):
        """!Delete all layers 

        @param overlay True to delete also overlayes
        """
        self.layers = []
        if overlay:
            self.overlays = []
        
    def DeleteLayer(self, layer, overlay = False):
        """!Removes layer from list of layers
        
        @param layer layer instance in layer tree
        @param overlay delete overlay (use self.DeleteOverlay() instead)

        @return removed layer on success or None
        """
        Debug.msg (3, "Map.DeleteLayer(): name=%s" % layer.name)
        
        if overlay:
            list = self.overlays
        else:
            list = self.layers
        
        if layer in list:
            if layer.mapfile:
                base = os.path.split(layer.mapfile)[0]
                mapfile = os.path.split(layer.mapfile)[1]
                tempbase = mapfile.split('.')[0]
                if base == '' or tempbase == '':
                    return None
                basefile = os.path.join(base, tempbase) + r'.*'
                for f in glob.glob(basefile):
                    os.remove(f)
            list.remove(layer)
            
            return layer
        
        return None

    def SetLayers(self, layers):
        self.layers = layers

        # only for debug
        # might be removed including message, it seems more than clear
        layerNameList = ""
        for layer in self.layers:
            if layer.GetName():
                layerNameList += layer.GetName() + ','
        Debug.msg(5, "Map.SetLayers(): layers=%s" % (layerNameList))

    def ChangeLayer(self, layer, render = False, **kargs):
        """!Change map layer properties

        @param layer map layer instance
        @param ltype layer type ('raster', 'vector', etc.)
        @param command  GRASS command given as list
        @param name layer name
        @param active layer render only if True
        @param hidden layer not displayed in layer tree if True
        @param opacity opacity level range from 0(transparent) - 1(not transparent)
        @param render render an image if True
        """
        Debug.msg (3, "Map.ChangeLayer(): layer=%s" % layer.name)
        
        if 'ltype' in kargs:
            layer.SetType(kargs['ltype']) # check type
        
        if 'command' in kargs:
            layer.SetCmd(kargs['command'])
        
        if 'name' in kargs:
            layer.SetName(kargs['name'])
        
        if 'active' in kargs:
            layer.SetActive(kargs['active'])
        
        if 'hidden' in kargs:
            layer.SetHidden(kargs['hidden'])
        
        if 'opacity' in kargs:
            layer.SetOpacity(kargs['opacity'])
        
        if render and not layer.Render():
            raise GException(_("Unable to render map layer <%s>.") % 
                             layer.GetName())
        
        return layer

    def ChangeOpacity(self, layer, opacity):
        """!Changes opacity value of map layer

        @param layer layer instance in layer tree
        @param opacity opacity level <0;1>
        """
        # opacity must be <0;1>
        if opacity < 0: opacity = 0
        elif opacity > 1: opacity = 1
        
        layer.opacity = opacity
        Debug.msg (3, "Map.ChangeOpacity(): layer=%s, opacity=%f" % \
                   (layer.name, layer.opacity))

    def ChangeLayerActive(self, layer, active):
        """!Enable or disable map layer
        
        @param layer layer instance in layer tree
        @param active to be rendered (True)
        """
        layer.active = active
        
        Debug.msg (3, "Map.ChangeLayerActive(): name='%s' -> active=%d" % \
                   (layer.name, layer.active))

    def ChangeLayerName (self, layer, name):
        """!Change name of the layer
        
        @param layer layer instance in layer tree
        @param name  layer name to set up
        """
        Debug.msg (3, "Map.ChangeLayerName(): from=%s to=%s" % \
                   (layer.name, name))
        layer.name =  name

    def RemoveLayer(self, name = None, id = None):
        """!Removes layer from layer list
        
        Layer is defined by name@mapset or id.
        
        @param name layer name (must be unique)
        @param id layer index in layer list    def __init__(self, targetFile, region, bandsNum, gdalDriver, fillValue = None):

        @return removed layer on success
        @return None on failure
        """
        # delete by name
        if name:
            retlayer = None
            for layer in self.layers:
                if layer.name == name:
                    retlayer = layer
                    os.remove(layer.mapfile)
                    os.remove(layer.maskfile)
                    self.layers.remove(layer)
                    return retlayer
        # del by id
        elif id != None:
            return self.layers.pop(id)
        
        return None

    def GetLayerIndex(self, layer, overlay = False):
        """!Get index of layer in layer list.
        
        @param layer layer instace in layer tree
        @param overlay use list of overlays instead
        
        @return layer index
        @return -1 if layer not found
        """
        if overlay:
            list = self.overlays
        else:
            list = self.layers
            
        if layer in list:
            return list.index(layer)
        
        return -1

    def AddOverlay(self, id, ltype, command,
                   active = True, hidden = True, opacity = 1.0, render = False):
        """!Adds overlay (grid, barscale, legend, etc.) to list of
        overlays
        
        @param id overlay id (PseudoDC)
        @param ltype overlay type (barscale, legend)
        @param command GRASS command to render overlay
        @param active overlay activated (True) or disabled (False)
        @param hidden overlay is not shown in layer tree (if True)
        @param render render an image (if True)
        
        @return new layer on success
        @return None on failure
        """
        Debug.msg (2, "Map.AddOverlay(): cmd=%s, render=%d" % (command, render))
        overlay = Overlay(id = id, ltype = ltype, cmd = command, Map = self,
                          active = active, hidden = hidden, opacity = opacity)
        
        # add maplayer to the list of layers
        self.overlays.append(overlay)
        
        if render and command != '' and not overlay.Render():
            raise GException(_("Unable to render overlay <%s>.") % 
                             ltype)
        
        return self.overlays[-1]

    def ChangeOverlay(self, id, render = False, **kargs):
        """!Change overlay properities
        
        Add new overlay if overlay with 'id' doesn't exist.
        
        @param id overlay id (PseudoDC)
        @param ltype overlay ltype (barscale, legend)
        @param command GRASS command to render overlay
        @param active overlay activated (True) or disabled (False)
        @param hidden overlay is not shown in layer tree (if True)
        @param render render an image (if True)
        
        @return new layer on success
        """
        overlay = self.GetOverlay(id, list = False)
        if  overlay is None:
            overlay = Overlay(id, ltype = None, cmd = None)
        
        if 'ltype' in kargs:
            overlay.SetName(kargs['ltype']) # ltype -> overlay
        
        if 'command' in kargs:
            overlay.SetCmd(kargs['command'])
        
        if 'active' in kargs:
            overlay.SetActive(kargs['active'])
        
        if 'hidden' in kargs:
            overlay.SetHidden(kargs['hidden'])
        
        if 'opacity' in kargs:
            overlay.SetOpacity(kargs['opacity'])
        
        if render and overlay.GetCmd() != [] and not overlay.Render():
            raise GException(_("Unable to render overlay <%s>.") % 
                             overlay.GetType())
        
        return overlay

    def GetOverlay(self, id, list = False):
        """!Return overlay(s) with 'id'
        
        @param id overlay id
        @param list return list of overlays of True
        otherwise suppose 'id' to be unique
        
        @return list of overlays (list=True)
        @return overlay (list=False)
        @return None (list=False) if no overlay or more overlays found
        """
        ovl = []
        for overlay in self.overlays:
            if overlay.id == id:
                ovl.append(overlay)
                
        if not list:
            if len(ovl) != 1:
                return None
            else:
                return ovl[0]
        
        return ovl

    def DeleteOverlay(self, overlay):
        """!Delete overlay
        
        @param overlay overlay layer
        
        @return removed overlay on success or None
        """
        return self.DeleteLayer(overlay, overlay = True)

    def _clean(self, llist):
        for layer in llist:
            if layer.maskfile:
                grass.try_remove(layer.maskfile)
            if layer.mapfile:
                grass.try_remove(layer.mapfile)
            llist.remove(layer)
        
    def Clean(self):
        """!Clean layer stack - go trough all layers and remove them
        from layer list.

        Removes also mapfile and maskfile.
        """
        self._clean(self.layers)
        self._clean(self.overlays)
        
    def ReverseListOfLayers(self):
        """!Reverse list of layers"""
        return self.layers.reverse()

    def RenderOverlays(self, force):
        """!Render overlays only (for nviz)"""
        for layer in self.overlays:
            if force or layer.forceRender:
                layer.Render()

    def AbortAllThreads(self):
        """!Abort all layers threads e. g. donwloading data"""
        for l in self.layers + self.overlays:
            l.AbortThread()

    def ReportProgress(self, layer):
        """!Calculates progress in rendering/downloading
        and emits signal to inform progress bar about progress.
        """
        if self.progressInfo is None or layer is None:
            self.progressInfo = {'progresVal' : 0, # current progress value
                                 'downloading' : [], # layers, which are downloading data
                                 'rendered' : [], # already rendered layers
                                 'range' : len(self.GetListOfLayers(active = True)) + 
                                           len(self.GetListOfLayers(active = True, ltype = 'overlay')) -
                                           len(self.GetListOfLayers(active = True, ltype = '3d-raster'))}
        else:
            if layer not in self.progressInfo['rendered']:
                self.progressInfo['rendered'].append(layer)
            if layer.IsDownloading() and \
                    layer not in self.progressInfo['downloading']:
                self.progressInfo['downloading'].append(layer)
            else:
                self.progressInfo['progresVal'] += 1
                if layer in self.progressInfo['downloading']:
                    self.progressInfo['downloading'].remove(layer)
            
        # for updating statusbar text
        stText = ''
        first = True
        for layer in self.progressInfo['downloading']:
            if first:
                stText += _("Downloading data ")
                first = False
            else:
                stText += ', '
            stText += '<%s>' % layer.GetName()
        if stText:
            stText += '...'
        
        if  self.progressInfo['range'] != len(self.progressInfo['rendered']):
            if stText:
                stText = _('Rendering & ') + stText
            else:
                stText = _('Rendering...')

        self.updateProgress.emit(range=self.progressInfo['range'],
                                 value=self.progressInfo['progresVal'],
                                 text=stText)
