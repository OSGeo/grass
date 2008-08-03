"""
@package render

Rendering map layers and overlays into map composition image

Classes:
 - Layer
 - MapLayer
 - Overlay
 - Map

C) 2006-2008 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Michael Barton, Jachym Cepicky,
Martin Landa <landa.martin gmail.com>

@date 2006-2008
"""

import os
import sys
import glob
import math
try:
    import subprocess
except:
    compatPath = os.path.join(globalvar.ETCWXDIR, "compat")
    sys.path.append(compatPath)
    import subprocess

import wx

import globalvar
import utils
import gcmd
from debug import Debug as Debug
from preferences import globalSettings as UserSettings

#
# use g.pnmcomp for creating image composition or
# wxPython functionality
#
USE_GPNMCOMP = True

class Layer(object):
    """Virtual class which stores information about layers (map layers and
    overlays) of the map composition.

    For map layer use MapLayer class.
    For overlays use Overlay class.
    """

    def __init__(self, type, cmd, name=None,
                 active=True, hidden=False, opacity=1.0):
        """
        @param type layer type ('raster', 'vector', 'overlay', 'command', etc.)
        @param cmd GRASS command to render layer,
        given as list, e.g. ['d.rast', 'map=elevation@PERMANENT']
        @param name layer name, e.g. 'elevation@PERMANENT' (for layer tree)
        @param active layer is active, will be rendered only if True
        @param hidden layer is hidden, won't be listed in Layer Manager if True
        @param opacity layer opacity <0;1>
        """
        self.type    = type
        self.name    = name
        self.cmdlist = cmd

        self.active  = active
        self.hidden  = hidden
        self.opacity = opacity

        self.force_render = True
        
        Debug.msg (3, "Layer.__init__(): type=%s, cmd='%s', name=%s, " \
                       "active=%d, opacity=%d, hidden=%d" % \
                       (self.type, self.GetCmd(string=True), self.name, self.active,
                        self.opacity, self.hidden))

        # generated file for layer
        self.gtemp = utils.GetTempfile()
        self.maskfile = self.gtemp + ".pgm"

    def __del__(self):
        Debug.msg (3, "Layer.__del__(): layer=%s, cmd='%s'" %
                   (self.name, self.GetCmd(string=True)))

    def Render(self):
        """Render layer to image

        @return rendered image filename
        @return None on error
        """
        if len(self.cmdlist) == 0:
            return None

        Debug.msg (3, "Layer.Render(): type=%s, name=%s" % \
                       (self.type, self.name))

        #
        # to be sure, set temporary file with layer and mask
        #
        if not self.gtemp:
            gtemp = utils.GetTempfile()
            self.maskfile = gtemp + ".pgm"
            if self.type == 'overlay':
                self.mapfile  = gtemp + ".png"
            else:
                self.mapfile  = gtemp + ".ppm"

        #
        # prepare command for each layer
        #
        layertypes = ['raster', 'rgb', 'his', 'shaded', 'rastarrow', 'rastnum',
                      'vector','thememap','themechart',
                      'grid', 'geodesic', 'rhumb', 'labels',
                      'command',
                      'overlay']

        if self.type not in layertypes:
            raise gcmd.GStdError(_("<%(name)s>: layer type <%(type)s> is not supported yet.") % \
                                     {'type' : self.type, 'name' : self.name})
        
        #
        # start monitor
        #
        if UserSettings.Get(group='display', key='driver', subkey='type') == 'cairo':
            os.environ["GRASS_CAIROFILE"] = self.mapfile
            if 'cairo' not in gcmd.Command(['d.mon', '-p']).ReadStdOutput()[0]:
                gcmd.Command(['d.mon',
                              'start=cairo'], stderr=None)
        else:
            os.environ["GRASS_PNGFILE"] = self.mapfile

        #
        # execute command
        #
        try:
            runcmd = gcmd.Command(cmd=self.cmdlist + ['--q'],
                                  stderr=None)
            if runcmd.returncode != 0:
                self.mapfile = None
                self.maskfile = None

        except gcmd.CmdError, e:
            print >> sys.stderr, e
            self.mapfile = None
            self.maskfile = None

        #
        # stop monitor
        #
        if UserSettings.Get(group='display', key='driver', subkey='type') == 'cairo':
            gcmd.Command(['d.mon',
                          'stop=cairo'], stderr=None)
            os.unsetenv("GRASS_CAIROFILE")
        else:
            os.unsetenv("GRASS_PNGFILE")

        self.force_render = False
        
        return self.mapfile

    def GetCmd(self, string=False):
        """
        Get GRASS command as list of string.

        @param string get command as string if True otherwise as list

        @return command list/string
        """
        if string:
            return ' '.join(self.cmdlist)
        else:
            return self.cmdlist

    def GetOpacity(self, float=False):
        """
        Get layer opacity level

        @param float get opacity level in <0,1> otherwise <0,100>

        @return opacity level
        """
        if float:
            return self.opacity
        
        return int (self.opacity * 100)

    def GetName(self):
        """Get map layer name"""
        return self.name
    
    def IsActive(self):
        """Check if layer is activated for rendering"""
        return self.active

    def SetType(self, type):
        """Set layer type"""
        if type not in ('raster', 'vector', 'overlay', 'command'):
            raise gcmd.GStdError(_("Unsupported map layer type '%s'") % str(type))
        
        self.type = type

    def SetName(self, name):
        """Set layer name"""
        self.name = name

    def SetCmd(self, cmd):
        """Set layer name"""
        self.cmdlist = cmd

    def SetActive(self, enable=True):
        """Active or deactive layer"""
        self.active = bool(enable)

    def SetHidden(self, enable=False):
        """Hide or show map layer in Layer Manager"""
        self.hidden = bool(enable)

    def SetOpacity(self, value):
        """Set opacity value"""
        if value < 0:
            value = 0.
        elif value > 1:
            value = 1.
        
        self.opacity = float(value)
        
    def SetCmd(self, cmd):
        """Set new command for layer"""
        self.cmdlist = cmd
        Debug.msg(3, "Layer.SetCmd(): cmd='%s'" % self.GetCmd(string=True))

        # for re-rendering
        self.force_render = True
        
class MapLayer(Layer):
    """Represents map layer in the map canvas"""
    def __init__(self, type, cmd, name=None,
                 active=True, hidden=False, opacity=1.0):
        """
        @param type layer type ('raster', 'vector', 'command', etc.)
        @param cmd GRASS command to render layer,
        given as list, e.g. ['d.rast', 'map=elevation@PERMANENT']
        @param name layer name, e.g. 'elevation@PERMANENT' (for layer tree) or None
        @param active layer is active, will be rendered only if True
        @param hidden layer is hidden, won't be listed in Layer Manager if True
        @param opacity layer opacity <0;1>
        """
        Layer.__init__(self, type, cmd, name,
                       active, hidden, opacity)

        self.mapfile = self.gtemp + ".ppm"

    def GetMapset(self):
        """
        Get mapset of map layer

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
    """Represents overlay displayed in map canvas"""
    def __init__(self, id, type, cmd,
                 active=True, hidden=True, opacity=1.0):
        """
        @param id overlay id (for PseudoDC)
        @param type overlay type ('barscale', 'legend', etc.)
        @param cmd GRASS command to render overlay,
        given as list, e.g. ['d.legend', 'map=elevation@PERMANENT']
        @param active layer is active, will be rendered only if True
        @param hidden layer is hidden, won't be listed in Layer Manager if True
        @param opacity layer opacity <0;1>
        """
        Layer.__init__(self, 'overlay', cmd, type,
                       active, hidden, opacity)

        self.id = id
        self.mapfile = self.gtemp + ".png"

class Map(object):
    """
    Map composition (stack of map layers and overlays)
    """
    def __init__(self, gisrc=None):
        # 
        # region/extent settigns
        #
        self.wind      = {}    # WIND settings (wind file)
        self.region    = {}    # region settings (g.region)
        self.width     = 640   # map width
        self.height    = 480   # map height

        #
        # list of layers
        #
        self.layers    = []  # stack of available GRASS layer

        self.overlays  = []  # stack of available overlays
        self.ovlookup  = {}  # lookup dictionary for overlay items and overlays

        #
        # environment settings
        #
        # enviroment variables, like MAPSET, LOCATION_NAME, etc.
        self.env         = {}
        # path to external gisrc
        self.gisrc = gisrc
        
        # 
        # generated file for rendering the map
        #
        self.mapfile   = utils.GetTempfile()

        # setting some initial env. variables
        self.InitGisEnv() # g.gisenv
        self.InitRegion()

        #
        # GRASS environment variable (for rendering)
        #
        os.environ["GRASS_TRANSPARENT"] = "TRUE"
        os.environ["GRASS_BACKGROUNDCOLOR"] = "ffffff"

    def InitRegion(self):
        """
        Initialize current region settings.

        Set up 'self.region' using g.region command and
        self.wind according to the wind file.

        Adjust self.region based on map window size.
        """

        #
        # setting region ('g.region -upg')
        #
        ### not needed here (MapFrame.OnSize())
        # self.region = self.GetRegion()

        #
        # read WIND file
        #
        self.GetWindow()

        #
        # setting resolution
        #
        # not needed here (MapFrame.OnSize())
        # self.SetRegion()

    def InitGisEnv(self):
        """
        Stores GRASS variables (g.gisenv) to self.env variable
        """

        if not os.getenv("GISBASE"):
            print >> sys.stderr, _("GISBASE not set. You must be in GRASS GIS to run this program.")
            sys.exit(1)
            
        # use external gisrc if defined
        gisrc_orig = os.getenv("GISRC")
        if self.gisrc:
            os.environ["GISRC"] = self.gisrc

        gisenvCmd = gcmd.Command(["g.gisenv"])

        for line in gisenvCmd.ReadStdOutput():
            line = line.strip()
            key, val = line.split("=")
            val = val.replace(";","")
            val = val.replace("'","")
            self.env[key] = val

        # back to original gisrc
        if self.gisrc:
            os.environ["GISRC"] = gisrc_orig

    def GetWindow(self):
        """Read WIND file and set up self.wind dictionary"""
        # FIXME: duplicated region WIND == g.region (at least some values)
        windfile = os.path.join (self.env['GISDBASE'],
                                 self.env['LOCATION_NAME'],
                                 self.env['MAPSET'],
                                 "WIND")
        try:
            windfile = open (windfile, "r")
        except StandardError, e:
            sys.stderr.write("%s %<s>: %s" % (_("Unable to open file"), windfile, e))
            sys.exit(1)

        for line in windfile.readlines():
            line = line.strip()
            key, value = line.split(":",1)
            key = key.strip()
            value = value.strip()
            self.wind[key] = value
            
        windfile.close()

        return self.wind
        
    def AdjustRegion(self):
        """
        Adjusts display resolution to match monitor size in pixels.
        Maintains constant display resolution, not related to computational
        region. Do NOT use the display resolution to set computational
        resolution. Set computational resolution through g.region.
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
        """
        Sets display extents to even multiple of
        current resolution defined in WIND file from SW corner.
        This must be done manually as using the -a flag
        can produce incorrect extents.
        """

        # new values to use for saving to region file
        new = {}
        n = s = e = w = 0.0
        nwres = ewres = 0.0

        # Get current values for region and display
        nsres = self.GetRegion()['nsres']
        ewres = self.GetRegion()['ewres']

        n = float(self.region['n'])
        s = float(self.region['s'])
        e = float(self.region['e'])
        w = float(self.region['w'])

        # Calculate rows, columns, and extents
        new['rows'] = math.fabs(round((n-s)/nsres))
        new['cols'] = math.fabs(round((e-w)/ewres))

        # Calculate new extents
        new['s'] = nsres * round(s/nsres)
        new['w'] = ewres * round(w/ewres)
        new['n'] = new['s'] + (new['rows'] * nsres)
        new['e'] = new['w'] + (new['cols'] * ewres)

        return new

    def AlignExtentFromDisplay(self):
        """Align region extent based on display size from center point"""

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

    def ChangeMapSize(self, (width, height)):
        """Change size of rendered map.
        
        @param width,height map size

        @return True on success
        @return False on failure
        """
        try:
            self.width  = int(width)
            self.height = int(height)
            Debug.msg(2, "Map.ChangeMapSize(): width=%d, height=%d" % \
                          (self.width, self.height))
            return True
        except:
            self.width  = 640
            self.height = 480
            return False

    def GetRegion(self, rast=None, zoom=False, vect=None,
                  n=None, s=None, e=None, w=None, default=False):
        """
        Get region settings (g.region -upgc)

        Optionaly extent, raster or vector map layer can be given.

        @param rast raster name or None
        @param vect vector name or None
        @param zoom zoom to raster (ignore NULLs)
        @param n,s,e,w force extent
        @param default force default region settings
        
        @return region settings as directory, e.g. {
        'n':'4928010', 's':'4913700', 'w':'589980',...}
        """

        region = {}

        tmpreg = os.getenv("GRASS_REGION")
        os.unsetenv("GRASS_REGION")

        # use external gisrc if defined
        gisrc_orig = os.getenv("GISRC")
        if self.gisrc:
            os.environ["GISRC"] = self.gisrc

        # do not update & shell style output
        cmdList = ["g.region", "-u", "-g", "-p", "-c"]

        if default:
            cmdList.append('-d')
            
        if n:
            cmdList.append('n=%s' % n)
        if s:
            cmdList.append('s=%s' % s)
        if e:
            cmdList.append('e=%s' % e)
        if w:
            cmdList.append('w=%s' % w)

        if rast:
            if zoom:
                cmdList.append('zoom=%s' % rast)
            else:
                cmdList.append('rast=%s' % rast)

        if vect:
            cmdList.append('vect=%s' % vect)
        
        try:
            cmdRegion = gcmd.Command(cmdList)
        except gcmd.CmdError, e:
            if rast:
                e.message = _("Unable to zoom to raster map <%s>.") % rast + \
                '%s%s' % (os.linesep, os.linesep) + e.message
            elif vect:
                e.message = _("Unable to zoom to vector map <%s>.") % vect + \
                '%s%s' % (os.linesep, os.linesep) + e.message

            print >> sys.stderr, e
            return self.region

        for reg in cmdRegion.ReadStdOutput():
            key, val = reg.split("=", 1)
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
        
        return region

    def GetCurrentRegion(self):
        """Get current display region settings"""
        return self.region

    def SetRegion(self, windres=False):
        """
        Render string for GRASS_REGION env. variable, so that the images will be rendered
        from desired zoom level.

        @param windres uses resolution from WIND file rather than display (for modules that require set
        resolution like d.rast.num)

        @return String usable for GRASS_REGION variable or None
        """
        grass_region = ""

        # adjust region settings to match monitor
        if not windres:
            self.region = self.AdjustRegion()

        #        newextents = self.AlignResolution()
        #        self.region['n'] = newextents['n']
        #        self.region['s'] = newextents['s']
        #        self.region['e'] = newextents['e']
        #        self.region['w'] = newextents['w']

        # read values from wind file
        try:
            for key in self.wind.keys():
                if key == 'north':
                    grass_region += "north: %s; " % \
                        (self.region['n'])
                    continue
                elif key == "south":
                    grass_region += "south: %s; " % \
                        (self.region['s'])
                    continue
                elif key == "east":
                    grass_region += "east: %s; " % \
                        (self.region['e'])
                    continue
                elif key == "west":
                    grass_region += "west: %s; " % \
                        (self.region['w'])
                    continue
                elif key == "e-w resol":
                    grass_region += "e-w resol: %f; " % \
                        (self.region['ewres'])
                    continue
                elif key == "n-s resol":
                    grass_region += "n-s resol: %f; " % \
                        (self.region['nsres'])
                    continue
                elif key == "cols":
                    grass_region += 'cols: %d; ' % \
                        self.region['cols']
                    continue
                elif key == "rows":
                    grass_region += 'rows: %d; ' % \
                        self.region['rows']
                    continue
                else:
                    grass_region += key + ": "  + self.wind[key] + "; "

            Debug.msg (3, "Map.SetRegion(): %s" % grass_region)

            return grass_region

        except:
            return None

    def ProjInfo(self):
        """
        Return region projection and map units information
        """

        projinfo = {}

        p = gcmd.Command(['g.proj', '-p'])

        if p.returncode == 0:
            for line in p.ReadStdOutput():
                if ':' in line:
                    key,val = line.split(':')
                    key = key.strip()
                    val = val.strip()
                    projinfo[key] = val
                elif "XY location (unprojected)" in line:
                    projinfo['proj'] = "xy"
            return projinfo
        else:
            return None

    def GetListOfLayers(self, l_type=None, l_mapset=None, l_name=None,
                        l_active=None, l_hidden=None):
        """
        Returns list of layers of selected properties or list of all
        layers. 

        @param l_type layer type, e.g. raster/vector/wms/overlay (value or tuple of values)
        @param l_mapset all layers from given mapset (only for maplayers)
        @param l_name all layers with given name
        @param l_active only layers with 'active' attribute set to True or False
        @param l_hidden only layers with 'hidden' attribute set to True or False

        @return list of selected layers
        """

        selected = []

        if type(l_type) == type(''):
            one_type = True
        else:
            one_type = False

        if one_type and l_type == 'overlay':
            list = self.overlays
        else:
            list = self.layers

        # ["raster", "vector", "wms", ... ]
        for layer in list:
            # specified type only
            if l_type != None:
                if one_type and layer.type != l_type:
                    continue
                elif not one_type and layer.type not in l_type:
                    continue

            # mapset
            if (l_mapset != None and type != 'overlay') and \
                    layer.GetMapset() != l_mapset:
                continue

            # name
            if l_name != None and layer.name != l_name:
                continue

            # hidden and active layers
            if l_active != None and \
                   l_hidden != None:
                if layer.active == l_active and \
                       layer.hidden == l_hidden:
                    selected.append(layer)

            # active layers
            elif l_active != None:
                if layer.active == l_active:
                    selected.append(layer)

            # hidden layers
            elif l_hidden != None:
                if layer.hidden == l_hidden:
                    selected.append(layer)

            # all layers
            else:
                selected.append(layer)

        Debug.msg (3, "Map.GetListOfLayers(): numberof=%d" % len(selected))

        return selected

    def Render(self, force=False, mapWindow=None, windres=False):
        """
        Creates final image composite

        This function can conditionaly use high-level tools, which
        should be avaliable in wxPython library
        
        @param force force rendering
        @param reference for MapFrame instance (for progress bar)
        @param windres use region resolution (True) otherwise display resolution

        @return name of file with rendered image or None
        """

        maps = []
        masks =[]
        opacities = []

        # use external gisrc if defined
        gisrc_orig = os.getenv("GISRC")
        if self.gisrc:
            os.environ["GISRC"] = self.gisrc

        tmp_region = os.getenv("GRASS_REGION")
        os.environ["GRASS_REGION"] = self.SetRegion(windres)
        os.environ["GRASS_WIDTH"]  = str(self.width)
        os.environ["GRASS_HEIGHT"] = str(self.height)
        if UserSettings.Get(group='display', key='driver', subkey='type') == 'cairo':
            os.environ["GRASS_AUTO_WRITE"] = "TRUE"
            os.unsetenv("GRASS_RENDER_IMMEDIATE")
        else:
            os.environ["GRASS_PNG_AUTO_WRITE"] = "TRUE"
            os.environ["GRASS_COMPRESSION"] = "0"
            os.environ["GRASS_TRUECOLOR"] = "TRUE"
            os.environ["GRASS_RENDER_IMMEDIATE"] = "TRUE"

        # render map layers
        for layer in self.layers + self.overlays:
            # skip dead or disabled map layers
            if layer == None or layer.active == False:
                continue
            
            # render if there is no mapfile
            if force or \
               layer.force_render or \
               layer.mapfile == None or \
               (not os.path.isfile(layer.mapfile) or not os.path.getsize(layer.mapfile)):
                if not layer.Render():
                    continue
            
            # update process bar
            if mapWindow is not None:
                mapWindow.onRenderCounter += 1

            wx.Yield()
            
            # add image to compositing list
            if layer.type != "overlay":
                maps.append(layer.mapfile)
                masks.append(layer.maskfile)
                opacities.append(str(layer.opacity))
                
            Debug.msg (3, "Map.Render() type=%s, layer=%s " % (layer.type, layer.name))

	# ugly hack for MSYS
	if not subprocess.mswindows:
	    mapstr = ",".join(maps)
	    maskstr = ",".join(masks)
            mapoutstr = self.mapfile
	else:
	    mapstr = ""
	    for item in maps:
                mapstr += item.replace('\\', '/')		
	    mapstr = mapstr.rstrip(',')
	    maskstr = ""
            for item in masks:
		maskstr += item.replace('\\', '/')
	    maskstr = maskstr.rstrip(',')
	    mapoutstr = self.mapfile.replace('\\', '/')

        # compose command
        complist = ["g.pnmcomp",
                   "in=%s" % ",".join(maps),
	           "mask=%s" % ",".join(masks),
                   "opacity=%s" % ",".join(opacities),
                   "background=255:255:255",
                   "width=%s" % str(self.width),
                   "height=%s" % str(self.height),
                   "output=%s" % self.mapfile]


        # render overlays

        os.unsetenv("GRASS_REGION")

        if tmp_region:
            os.environ["GRASS_REGION"] = tmp_region

        # run g.pngcomp to get composite image
        try:
            gcmd.Command(complist)
        except gcmd.CmdError, e:
            print >> sys.stderr, e
            return None

        # back to original gisrc
        if self.gisrc:
            os.environ["GISRC"] = gisrc_orig

        Debug.msg (2, "Map.Render() force=%s file=%s" % (force, self.mapfile))

        return self.mapfile

    def AddLayer(self, type, command, name=None,
                 l_active=True, l_hidden=False, l_opacity=1.0, l_render=False,
                 pos=-1):
        """
        Adds generic map layer to list of layers

        @param type layer type ('raster', 'vector', etc.)
        @param command  GRASS command given as list
        @param name layer name
        @param l_active layer render only if True
        @param l_hidden layer not displayed in layer tree if True
        @param l_opacity opacity level range from 0(transparent) - 1(not transparent)
        @param l_render render an image if True
        @param pos position in layer list (-1 for append)

        @return new layer on success
        @return None on failure

        """
        # l_opacity must be <0;1>
        if l_opacity < 0: l_opacity = 0
        elif l_opacity > 1: l_opacity = 1

        layer = MapLayer(type=type, name=name, cmd=command,
                         active=l_active, hidden=l_hidden, opacity=l_opacity)

        # add maplayer to the list of layers
        if pos > -1:
            self.layers.insert(pos, layer)
        else:
            self.layers.append(layer)
        
        Debug.msg (3, "Map.AddLayer(): layer=%s" % layer.name)
        if l_render:
            if not layer.Render():
                raise gcmd.GStdError(_("Unable to render map layer <%s>.") % (name))

        return layer

    def DeleteLayer(self, layer, overlay=False):
        """
        Removes layer from list of layers

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

    def ReorderLayers(self, layerList):
        """
        Reorder list to match layer tree

        @param layerList list of layers
        """
        self.layers = layerList

        layerNameList = ""
        for layer in self.layers:
            if layer.name:
                layerNameList += layer.name + ','
        Debug.msg (4, "Map.ReoderLayers(): layers=%s" % \
                   (layerNameList))

    def ChangeLayer(self, layer, render=False, **kargs):
        """
        Change map layer properties

        @param layer map layer instance
        @param type layer type ('raster', 'vector', etc.)
        @param command  GRASS command given as list
        @param name layer name
        @param active layer render only if True
        @param hidden layer not displayed in layer tree if True
        @param opacity opacity level range from 0(transparent) - 1(not transparent)
        @param render render an image if True
        """

        Debug.msg (3, "Map.ChangeLayer(): layer=%s" % layer.name)

        if kargs.has_key('type'):
            layer.SetType(kargs['type']) # check type

        if kargs.has_key('command'):
            layer.SetCmd(kargs['command'])
            
        if kargs.has_key('name'):
            layer.SetName(kargs['name'])

        if kargs.has_key('active'):
            layer.SetActive(kargs['active'])

        if kargs.has_key('hidden'):
            layer.SetHidden(kargs['hidden'])

        if kargs.has_key('opacity'):
            layer.SetOpacity(kargs['opacity'])
            
        if render and not layer.Render():
            raise gcmd.GException(_("Unable to render map layer <%s>.") % 
                                  (name))

        return layer

    def ChangeOpacity(self, layer, l_opacity):
        """
        Changes opacity value of map layer

        @param layer layer instance in layer tree
        @param l_opacity opacity level <0;1>
        """
        # l_opacity must be <0;1>
        if l_opacity < 0: l_opacity = 0
        elif l_opacity > 1: l_opacity = 1

        layer.opacity = l_opacity
        Debug.msg (3, "Map.ChangeOpacity(): layer=%s, opacity=%f" % \
                   (layer.name, layer.opacity))

    def ChangeLayerActive(self, layer, active):
        """
        Enable or disable map layer

        @param layer layer instance in layer tree
        @param active to be rendered (True)
        """
        layer.active = active

        Debug.msg (3, "Map.ChangeLayerActive(): name='%s' -> active=%d" % \
                   (layer.name, layer.active))

    def ChangeLayerName (self, layer, name):
        """
        Change name of the layer

        @param layer layer instance in layer tree
        @param name  layer name to set up
        """
        Debug.msg (3, "Map.ChangeLayerName(): from=%s to=%s" % \
                   (layer.name, name))
        layer.name =  name

    def RemoveLayer(self, name=None, id=None):
        """
        Removes layer from layer list

        Layer is defined by name@mapset or id.

        @param name layer name (must be unique)
        @param id layer index in layer list

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
                    return layer
        # del by id
        elif id != None:
            return self.layers.pop(id)

        return None

    def GetLayerIndex(self, layer, overlay=False):
        """
        Get index of layer in layer list.

        @param layer layer instace in layer tree
        @param overlay use list of overlays instead
        
        @return layer index
        @return -1 if layer not found
        """
        if overlay:
            list = self.overlay
        else:
            list = self.layers
            
        if layer in list:
            return list.index(layer)

        return -1

    def AddOverlay(self, id, type, command,
                   l_active=True, l_hidden=True, l_opacity=1.0, l_render=False):
        """
        Adds overlay (grid, barscale, legend, etc.) to list of overlays
        
        @param id overlay id (PseudoDC)
        @param type overlay type (barscale, legend)
        @param command GRASS command to render overlay
        @param l_active overlay activated (True) or disabled (False)
        @param l_hidden overlay is not shown in layer tree (if True)
        @param l_render render an image (if True)

        @return new layer on success
        @retutn None on failure
        """

        Debug.msg (2, "Map.AddOverlay(): cmd=%s, render=%d" % (command, l_render))
        overlay = Overlay(id=id, type=type, cmd=command,
                          active=l_active, hidden=l_hidden, opacity=l_opacity)

        # add maplayer to the list of layers
        self.overlays.append(overlay)

        if l_render and command != '' and not overlay.Render():
            raise gcmd.GException(_("Unable render overlay <%s>.") % 
                                  (name))

        return self.overlays[-1]

    def ChangeOverlay(self, id, type, command,
                      l_active=True, l_hidden=False, l_opacity=1, l_render=False):
        """
        Change overlay properities

        Add new overlay if overlay with 'id' doesn't exist.

        @param id overlay id (PseudoDC)
        @param type overlay type (barscale, legend)
        @param command GRASS command to render overlay
        @param l_active overlay activated (True) or disabled (False)
        @param l_hidden overlay is not shown in layer tree (if True)
        @param l_render render an image (if True)

        @return new layer on success
        """
        overlay = self.GetOverlay(id, list=False)
        if  overlay is None:
            overlay = Overlay(id, type, command,
                              l_active, l_hidden, l_opacity)
        else:
            overlay.id = id
            overlay.name = type
            overlay.cmdlist = command
            overlay.active = l_active
            overlay.hidden = l_hidden
            overlay.opacity = l_opacity

        if l_render and command != [] and not overlay.Render():
            raise gcmd.GException(_("Unable render overlay <%s>") % 
                                  (name))

        return overlay

    def GetOverlay(self, id, list=False):
        """Return overlay(s) with 'id'

        @param id overlay id
        @param list return list of overlays of True
        otherwise suppose 'id' to be unique
        
        @return list of overlays (list=True)
        @return overlay (list=False)
        @retur None (list=False) if no overlay or more overlays found
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
        """Delete overlay

        @param id overlay id

        @return removed overlay on success or None
        """
        return self.DeleteLayer(overlay, overlay=True)

    def Clean(self):
        """
        Clean layer stack - go trough all layers and remove them from layer list
        Removes also l_mapfile and l_maskfile

        @return 1 on faulure
        @return None on success
        """
        try:
            for layer in self.layers:
                if layer.mapfile:
                    base = os.path.split(layer.mapfile)[0]
                    mapfile = os.path.split(layer.mapfile)[1]
                    tempbase = mapfile.split('.')[0]
                    basefile = os.path.join(base,tempbase)+r'.*'
                    for f in glob.glob(basefile):
                        os.remove(f)
                self.layers.remove(layer)
            for overlay in self.overlays:
                if overlay.ovlfile:
                    base = os.path.split(overlay.ovlfile)[0]
                    mapfile = os.path.split(overlay.ovlfile)[1]
                    tempbase = mapfile.split('.')[0]
                    basefile = os.path.join(base,tempbase)+r'.*'
                    for f in glob.glob(basefile):
                        os.remove(f)
                self.overlays.remove(overlay)
            return None
        except:
            return 1
        self.layers = []

    def ReverseListOfLayers(self):
        """Reverse list of layers"""
        return self.layers.reverse()

if __name__ == "__main__":
    """
    Test of Display class.
    Usage: display=Render()
    """

    print "Initializing..."
    os.system("g.region -d")

    map = Map()
    map.width = 640
    map.height = 480

    map.AddLayer(item=None,
                 type="raster",
                 name="elevation.dem",
                 command = ["d.rast", "elevation.dem@PERMANENT", "catlist=1000-1500", "-i"],
                 l_opacity=.7)

    map.AddLayer(item=None,
                 type="vector",
                 name="streams",
                 command = ["d.vect", "streams@PERMANENT", "color=red", "width=3", "type=line"])

    image = map.Render(force=True)

    if image:
        os.system("display %s" % image)
