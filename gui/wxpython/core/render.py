"""
@package core.render

@brief Rendering map layers and overlays into map composition image.

Classes:
 - render::Layer
 - render::MapLayer
 - render::Overlay
 - render::Map
 - render::RenderLayerMgr
 - render::RenderMapMgr

(C) 2006-2015 by the GRASS Development Team

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
import time

import wx

from grass.script import core as grass
from grass.script.utils import try_remove, text_to_string
from grass.script.task import cmdlist_to_tuple, cmdtuple_to_list
from grass.pydispatch.signal import Signal
from grass.exceptions import CalledModuleError

from core import utils
from core.ws import RenderWMSMgr
from core.gcmd import GException, GError, RunCommand
from core.debug import Debug
from core.settings import UserSettings
from core.gthread import gThread


def get_tempfile_name(suffix, create=False):
    """Returns a name for a temporary file in system directory"""
    # this picks TMPDIR which we have set for GRASS session
    # which may mitigate problems (like not cleaning files) in case we
    # go little beyond what is in the documentation in terms of opening
    # closing and removing the tmp file
    tmp = tempfile.NamedTemporaryFile(suffix=suffix, delete=False)
    # we don't want it open, we just need the name
    name = tmp.name
    tmp.close()
    if not create:
        # remove empty file to have a clean state later
        os.remove(name)
    return name


class Layer(object):
    """Virtual class which stores information about layers (map layers and
    overlays) of the map composition.

    - For map layer use MapLayer class.
    - For overlays use Overlay class.
    """

    def __init__(self, ltype, cmd, Map, name=None, active=True,
                 hidden=False, opacity=1.0, mapfile=None, render=True):
        """Create new instance

        .. todo::

            pass cmd as tuple instead of list

        :param ltype: layer type ('raster', 'vector', 'overlay', 'command', etc.)
        :param cmd: GRASS command to render layer, given as list,
                    e.g. ['d.rast', 'map=elevation@PERMANENT']
        :param map: render.Map instance
        :param name: layer name, e.g. 'elevation@PERMANENT' (for layer tree)
        :param active: layer is active, will be rendered only if True
        :param hidden: layer is hidden, won't be listed in Layer Manager if True
        :param float opacity: layer opacity <0;1>
        :param mapfile full path to output file or None
        """
        if mapfile:
            self.mapfile = mapfile
        else:
            if ltype == 'overlay':
                tempfile_sfx = ".png"
            else:
                tempfile_sfx = ".ppm"

            self.mapfile = get_tempfile_name(suffix=tempfile_sfx)

        self.maskfile = self.mapfile.rsplit(".", 1)[0] + ".pgm"

        # stores class which manages rendering instead of simple command - e.g.
        # WMS
        self.renderMgr = None

        self.Map = Map
        self.type = None
        self.SetType(ltype)
        self.name = name

        if self.type == 'command':
            self.cmd = list()
            for c in cmd:
                self.cmd.append(cmdlist_to_tuple(c))
        else:
            self.cmd = cmdlist_to_tuple(cmd)

        self.active = active
        self.hidden = hidden
        self.opacity = opacity

        self.forceRender = render

        Debug.msg(3, "Layer.__init__(): type=%s, cmd='%s', name=%s, "
                  "active=%d, opacity=%d, hidden=%d" %
                  (self.type, self.GetCmd(string=True), self.name,
                   self.active, self.opacity, self.hidden))

    def __del__(self):
        self.Clean()
        Debug.msg(3, "Layer.__del__(): layer=%s, cmd='%s'" %
                  (self.name, self.GetCmd(string=True)))

    def __str__(self):
        return self.GetCmd(string=True)

    def __repr__(self):
        return self.__str__()

    def Render(self, env=None):
        """Render layer to image

        :return: rendered image filename
        :return: None on error or if cmdfile is defined
        """
        if not self.cmd:
            return None

        # ignore in 2D
        if self.type == 'raster_3d':
            return None

        Debug.msg(3, "Layer.Render(): type=%s, name=%s, file=%s" %
                  (self.type, self.name, self.mapfile))

        # prepare command for each layer
        layertypes = list(utils.command2ltype.values()) + ['overlay', 'command']

        if self.type not in layertypes:
            raise GException(
                _("<%(name)s>: layer type <%(type)s> is not supported") %
                {'type': self.type, 'name': self.name})

        if not env:
            env = os.environ.copy()

        # render layers
        try:
            if self.type == 'command':
                first = True
                for c in self.cmd:
                    self.renderMgr.Render(c, env)
                    if first:
                        env["GRASS_RENDER_FILE_READ"] = "TRUE"
                        first = False
            else:
                self.renderMgr.Render(self.cmd, env)
        except GException:
            sys.stderr.write(
                _("Command '%s' failed\n") %
                self.GetCmd(
                    string=True))
            sys.stderr.write(_("Details: %s\n") % e)

            # clean up after problems
            for f in [self.mapfile, self.maskfile]:
                if not f:
                    continue
                try_remove(f)
                f = None

        return self.mapfile

    def GetCmd(self, string=False):
        """Get GRASS command as list of string.

        :param string: get command as string if True otherwise as list

        :return: command list/string
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
        """Get map layer type"""
        return self.type

    def GetElement(self):
        """Get map element type"""
        if self.type == 'raster':
            return 'cell'
        return self.type

    def GetOpacity(self):
        """
        Get layer opacity level

        :return: opacity level (<0, 1>)
        """
        return self.opacity

    def GetName(self, fullyQualified=True):
        """Get map layer name

        :param bool fullyQualified: True to return fully qualified name
                                    as a string 'name@mapset' otherwise
                                    directory { 'name', 'mapset' } is
                                    returned

        :return: string / directory
        """
        if fullyQualified:
            return self.name
        else:
            if '@' in self.name:
                return {'name': self.name.split('@')[0],
                        'mapset': self.name.split('@')[1]}
            else:
                return {'name': self.name,
                        'mapset': ''}

    def IsActive(self):
        """Check if layer is activated for rendering"""
        return self.active

    def IsHidden(self):
        """Check if layer is hidden"""
        return self.hidden

    def IsRendered(self):
        """!Check if layer was rendered (if the image file exists)"""
        if os.path.exists(self.mapfile):
            return True
        return False

    def SetType(self, ltype):
        """Set layer type"""
        if ltype not in list(utils.command2ltype.values()) + ['overlay', 'command']:
            raise GException(_("Unsupported map layer type '%s'") % ltype)

        if not self.renderMgr:
            env = {}
            if ltype == 'wms':
                renderMgr = RenderWMSMgr
            else:
                renderMgr = RenderLayerMgr
                env['GRASS_RENDER_FILE'] = self.mapfile
                if ltype == 'overlay':
                    env['GRASS_RENDER_FILE_READ'] = 'FALSE'
                    env['GRASS_RENDER_TRANSPARENT'] = 'TRUE'
            self.renderMgr = renderMgr(self, env)

        self.type = ltype

    def SetName(self, name):
        """Set layer name"""
        self.name = name

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
        if self.type == 'command':
            self.cmd = []
            for c in cmd:
                self.cmd.append(cmdlist_to_tuple(c))
        else:
            self.cmd = cmdlist_to_tuple(cmd)
        Debug.msg(3, "Layer.SetCmd(): cmd='%s'" % self.GetCmd(string=True))

        # for re-rendering
        self.forceRender = True

    def IsDownloading(self):
        """Is data downloading from web server e. g. wms"""
        return self.renderMgr.IsDownloading()

    def AbortThread(self):
        """Abort running thread e. g. downloading data"""
        self.renderMgr.Abort()

    def GetRenderMgr(self):
        """Get render manager """
        return self.renderMgr

    def Clean(self):
        if self.mapfile:
            try_remove(self.mapfile)
            self.mapfile = None
        if self.maskfile:
            try_remove(self.maskfile)
            self.maskfile = None


class MapLayer(Layer):

    def __init__(self, *args, **kwargs):
        """Represents map layer in the map canvas
        """
        Layer.__init__(self, *args, **kwargs)
        if self.type in ('vector', 'thememap'):
            self._legrow = get_tempfile_name(suffix=".legrow", create=True)
        else:
            self._legrow = ''

    def GetMapset(self):
        """Get mapset of map layer

        :return: mapset name
        :return: '' on error (no name given)
        """
        if not self.name:
            return ''

        try:
            return self.name.split('@')[1]
        except IndexError:
            return self.name

    def Clean(self):
        Layer.Clean(self)
        if self._legrow:
            try_remove(self._legrow)
            self._legrow = None


class Overlay(Layer):

    def __init__(self, id, *args, **kwargs):
        """Represents overlay displayed in map canvas

        :param id: overlay id (for PseudoDC)
        """
        Layer.__init__(self, ltype='overlay', *args, **kwargs)
        self.id = id


class RenderLayerMgr(wx.EvtHandler):

    def __init__(self, layer, env):
        """Render layer into image

        :param layer: Layer to be rendered
        """
        self.layer = layer

        wx.EvtHandler.__init__(self)
        self.thread = gThread()

        self.updateProgress = Signal('RenderLayerMgr.updateProgress')
        self.renderingFailed = Signal('RenderLayerMgr.renderingFailed')

        self._startTime = None
        self._render_env = env

    def UpdateRenderEnv(self, env):
        self._render_env.update(env)

    def Render(self, cmd, env):
        """Render layer

        :param cmd: display command given as tuple
        :param env: environmental variables used for rendering
        """
        Debug.msg(1, "RenderLayerMgr.Render(%s): force=%d img=%s" %
                  (self.layer, self.layer.forceRender, self.layer.mapfile))

        env_cmd = env.copy()
        env_cmd.update(self._render_env)
        env_cmd['GRASS_RENDER_FILE'] = self.layer.mapfile
        if self.layer.GetType() in ('vector', 'thememap'):
            if not self.layer._legrow:
                self.layer._legrow = grass.tempfile(create=True)
            if os.path.isfile(self.layer._legrow):
                os.remove(self.layer._legrow)
            env_cmd['GRASS_LEGEND_FILE'] = text_to_string(self.layer._legrow)

        cmd_render = copy.deepcopy(cmd)
        cmd_render[1]['quiet'] = True  # be quiet

        self._startTime = time.time()
        self.thread.Run(callable=self._render, cmd=cmd_render, env=env_cmd,
                        ondone=self.OnRenderDone, userdata={'cmd': cmd})
        self.layer.forceRender = False

    def _render(self, cmd, env):
        p = grass.start_command(cmd[0], env=env, stderr=grass.PIPE, **cmd[1])
        stdout, stderr = p.communicate()
        if p.returncode:
            return grass.decode(stderr)
        else:
            return None

    def Abort(self):
        """Abort rendering process"""
        Debug.msg(1, "RenderLayerMgr({0}).Abort()".format(self.layer))
        self.thread.Terminate()

        # force rendering layer next time
        self.layer.forceRender = True
        self.thread.Terminate(False)

    def IsDownloading(self):
        """Is downloading

        :return: always False
        """
        return False

    def OnRenderDone(self, event):
        """Rendering done

        Emits updateProcess
        """
        Debug.msg(1, "RenderLayerMgr.OnRenderDone(%s): err=%s time=%f" %
                  (self.layer, event.ret, time.time() - self._startTime))
        if event.ret is not None:
            cmd = cmdtuple_to_list(event.userdata['cmd'])
            self.renderingFailed.emit(cmd=cmd, error=event.ret)
            # don't remove layer if overlay, we need to keep the old one
            if self.layer.type != 'overlay':
                try_remove(self.layer.mapfile)

        self.updateProgress.emit(layer=self.layer)


class RenderMapMgr(wx.EvtHandler):

    def __init__(self, Map):
        """Render map layers as image composition

        :param Map: Map object to be rendered
        """
        wx.EvtHandler.__init__(self)

        self.Map = Map

        self.updateMap = Signal('RenderMapMgr.updateMap')
        self.updateProgress = Signal('RenderMapMgr.updateProgress')
        self.renderingFailed = Signal('RenderMapMgr.renderingFailed')
        self.renderDone = Signal('RenderMapMgr.renderDone')
        self.renderDone.connect(self.OnRenderDone)

        # GRASS environment variable (for rendering)
        self._render_env = {"GRASS_RENDER_BACKGROUNDCOLOR": "000000",
                            "GRASS_RENDER_FILE_COMPRESSION": "0",
                            "GRASS_RENDER_TRUECOLOR": "TRUE",
                            "GRASS_RENDER_TRANSPARENT": "TRUE",
                            "GRASS_LEGEND_FILE": text_to_string(self.Map.legfile)
                            }

        self._init()
        self._rendering = False
        self._old_legend = []

    def _init(self, env=None):
        """Init render manager

        :param env: environmental variables or None
        """
        self._startTime = time.time()
        self.progressInfo = None
        self._env = env
        self.layers = []

        # re-render from scratch
        if os.path.exists(self.Map.mapfile):
            os.remove(self.Map.mapfile)

    def UpdateRenderEnv(self, env):
        self._render_env.update(env)

    def _renderLayers(self, env, force=False, overlaysOnly=False):
        """Render all map layers into files

        :param dict env: environmental variables to be used for rendering process
        :param bool force: True to force rendering
        :param bool overlaysOnly: True to render only overlays

        :return: number of layers to be rendered
        """
        self.layers = self.Map.GetListOfLayers(ltype='overlay', active=True)
        if not overlaysOnly:
            self.layers += self.Map.GetListOfLayers(active=True,
                                                    ltype='raster_3d',
                                                    except_ltype=True)

        # reset progress
        self.ReportProgress()

        # render map layers if forced
        nlayers = 0
        for layer in self.layers:
            if force or layer.forceRender:
                nlayers += 1
                layer.Render(env)
            else:
                layer.GetRenderMgr().updateProgress.emit(layer=layer)

        Debug.msg(1, "RenderMapMgr.Render(): %d layers to be rendered "
                  "(force=%d, all active layers -> %d)" % (nlayers, force,
                                                           len(self.layers)))

        return nlayers

    def GetRenderEnv(self, windres=False):
        env = os.environ.copy()
        env.update(self._render_env)
        # use external gisrc if defined
        if self.Map.gisrc:
            env['GISRC'] = self.Map.gisrc
        env['GRASS_REGION'] = self.Map.SetRegion(windres)
        env['GRASS_RENDER_WIDTH'] = str(self.Map.width)
        env['GRASS_RENDER_HEIGHT'] = str(self.Map.height)

        if UserSettings.Get(group='display', key='driver',
                            subkey='type') == 'png':
            env['GRASS_RENDER_IMMEDIATE'] = 'png'
        else:
            env['GRASS_RENDER_IMMEDIATE'] = 'cairo'

        return env

    def RenderOverlays(self, force=False):
        """Render only overlays

        :param bool force: force rendering all map layers
        """
        if self._rendering:
            Debug.msg(
                1, "RenderMapMgr().RenderOverlays(): cancelled (already rendering)")
            return

        wx.BeginBusyCursor()
        self._rendering = True

        env = self.GetRenderEnv()
        self._init(env)
        # no layer composition afterwards
        if self._renderLayers(env, force, overlaysOnly=True) == 0:
            self.renderDone.emit()

    def Render(self, force=False, windres=False):
        """Render map composition

        :param bool force: force rendering all map layers in the composition
        :param windres: True for region resolution instead for map resolution
        """
        if self._rendering:
            Debug.msg(
                1, "RenderMapMgr().Render(): cancelled (already rendering)")
            return

        wx.BeginBusyCursor()
        self._rendering = True

        env = self.GetRenderEnv(windres)
        self._init(env)
        if self._renderLayers(env, force) == 0:
            self.renderDone.emit()


    def OnRenderDone(self):
        """Rendering process done

        Make image composiotion, emits updateMap event.
        """
        stopTime = time.time()

        maps = list()
        masks = list()
        opacities = list()

        # TODO: g.pnmcomp is now called every time
        # even when only overlays are rendered
        for layer in self.layers:
            if layer.GetType() == 'overlay':
                continue

            if os.path.isfile(layer.mapfile):
                maps.append(layer.mapfile)
                masks.append(layer.maskfile)
                opacities.append(str(layer.opacity))

        # run g.pngcomp to get composite image
        bgcolor = ':'.join(map(str, UserSettings.Get(
            group='display', key='bgcolor', subkey='color')))
        startCompTime = time.time()
        if maps:
            ret, msg = RunCommand('g.pnmcomp',
                                  getErrorMsg=True,
                                  overwrite=True,
                                  input='%s' % ",".join(maps),
                                  mask='%s' % ",".join(masks),
                                  opacity='%s' % ",".join(opacities),
                                  bgcolor=bgcolor,
                                  width=self._env['GRASS_RENDER_WIDTH'],
                                  height=self._env['GRASS_RENDER_HEIGHT'],
                                  output=self.Map.mapfile,
                                  env=self._env)
            if ret != 0:
                self._rendering = False
                if wx.IsBusy():
                    wx.EndBusyCursor()
                raise GException(_("Rendering failed: %s" % msg))

        stop = time.time()
        Debug.msg(1, "RenderMapMgr.OnRenderDone() time=%f sec (comp: %f)" %
                  (stop - self._startTime, stop - startCompTime))

        # Update legfile
        new_legend = []
        with open(self.Map.legfile, "w") as outfile:
            for layer in reversed(self.layers):
                if layer.GetType() not in ('vector', 'thememap'):
                    continue

                if os.path.isfile(layer._legrow) and not layer.hidden:
                    with open(layer._legrow) as infile:
                        line = infile.read()
                        outfile.write(line)
                        new_legend.append(line)

        self._rendering = False
        if wx.IsBusy():
            wx.EndBusyCursor()

        # if legend file changed, rerender vector legend
        if new_legend != self._old_legend:
            self._old_legend = new_legend
            for layer in self.layers:
                if layer.GetType() == 'overlay' and layer.GetName() == 'vectleg':
                    layer.forceRender = True
            self.Render()
        else:
            self.updateMap.emit()

    def Abort(self):
        """Abort all rendering processes"""
        Debug.msg(1, "RenderMapMgr.Abort()")
        for layer in self.layers:
            layer.GetRenderMgr().Abort()

        self._init()
        if wx.IsBusy():
            wx.EndBusyCursor()
        self.updateProgress.emit(range=0, value=0, text=_("Rendering aborted"))

    def ReportProgress(self, layer=None):
        """Calculates progress in rendering/downloading
        and emits signal to inform progress bar about progress.

        Emits renderDone event when progressVal is equal to range.

        :param layer: Layer to be processed or None to reset
        """
        if self.progressInfo is None or layer is None:
            self.progressInfo = {'progresVal': 0,   # current progress value
                                 'downloading': [],  # layers, which are downloading data
                                 'rendered': [],    # already rendered layers
                                 'range': len(self.layers)}
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

        if self.progressInfo['range'] != len(self.progressInfo['rendered']):
            if stText:
                stText = _('Rendering & ') + stText
            else:
                stText = _('Rendering...')

        self.updateProgress.emit(range=self.progressInfo['range'],
                                 value=self.progressInfo['progresVal'],
                                 text=stText)

        if layer and self.progressInfo[
                'progresVal'] == self.progressInfo['range']:
            self.renderDone.emit()

    def RenderingFailed(self, cmd, error):
        self.renderingFailed.emit(cmd=cmd, error=error)


class Map(object):

    def __init__(self, gisrc=None):
        """Map composition (stack of map layers and overlays)

        :param gisrc: alternative gisrc (used eg. by georectifier)
        """
        Debug.msg(1, "Map.__init__(): gisrc=%s" % gisrc)
        # region/extent settigns
        self.wind = dict()  # WIND settings (wind file)
        self.region = dict()  # region settings (g.region)
        self.width = 640    # map width
        self.height = 480    # map height

        # list of layers
        self.layers = list()  # stack of available GRASS layer

        self.overlays = list()  # stack of available overlays
        self.ovlookup = dict()  # lookup dictionary for overlay items and overlays

        # path to external gisrc
        self.gisrc = gisrc

        # generated file for g.pnmcomp output for rendering the map
        self.legfile = get_tempfile_name(suffix='.leg')
        self.mapfile = get_tempfile_name(suffix='.ppm')

        # setting some initial env. variables
        if not self.GetWindow():
            sys.stderr.write(_("Trying to recover from default region..."))
            RunCommand('g.region', flags='d')

        # projection info
        self.projinfo = self._projInfo()

        self.layerChanged = Signal('Map.layerChanged')
        self.layerRemoved = Signal('Map:layerRemoved')
        self.layerAdded = Signal('Map:layerAdded')

        self.renderMgr = RenderMapMgr(self)

    def GetRenderMgr(self):
        """Get render manager """
        return self.renderMgr

    def GetProjInfo(self):
        """Get projection info"""
        return self.projinfo

    def _projInfo(self):
        """Return region projection and map units information
        """
        projinfo = dict()
        if not grass.find_program('g.proj', '--help'):
            sys.exit(_("GRASS module '%s' not found. Unable to start map "
                       "display window.") % 'g.proj')
        env = os.environ.copy()
        if self.gisrc:
            env['GISRC'] = self.gisrc
        ret = RunCommand(prog='g.proj', read=True, flags='p', env=env)

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
        """Read WIND file and set up self.wind dictionary"""
        # FIXME: duplicated region WIND == g.region (at least some values)
        env = grass.gisenv()
        filename = os.path.join(env['GISDBASE'],
                                env['LOCATION_NAME'],
                                env['MAPSET'],
                                "WIND")
        try:
            windfile = open(filename, "r")
        except IOError as e:
            sys.exit(
                _("Error: Unable to open '%(file)s'. Reason: %(ret)s. wxGUI exited.\n") % {
                    'file': filename,
                    'ret': e})

        for line in windfile.readlines():
            line = line.strip()
            try:
                key, value = line.split(":", 1)
            except ValueError as e:
                sys.stderr.write(
                    _("\nERROR: Unable to read WIND file: %s\n") %
                    e)
                return None

            self.wind[key.strip()] = value.strip()

        windfile.close()

        return self.wind

    def AdjustRegion(self):
        """Adjusts display resolution to match monitor size in
        pixels. Maintains constant display resolution, not related to
        computational region. Do NOT use the display resolution to set
        computational resolution. Set computational resolution through
        g.region.
        """
        mapwidth = abs(self.region["e"] - self.region["w"])
        mapheight = abs(self.region['n'] - self.region['s'])

        self.region["nsres"] = mapheight / self.height
        self.region["ewres"] = mapwidth / self.width
        self.region['rows'] = round(mapheight / self.region["nsres"])
        self.region['cols'] = round(mapwidth / self.region["ewres"])
        self.region['cells'] = self.region['rows'] * self.region['cols']

        Debug.msg(3, "Map.AdjustRegion(): %s" % self.region)

        return self.region

    def AlignResolution(self):
        """Sets display extents to even multiple of current
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
        new['rows'] = math.fabs(round((n - s) / nsres))
        new['cols'] = math.fabs(round((e - w) / ewres))

        # Calculate new extents
        new['s'] = nsres * round(s / nsres)
        new['w'] = ewres * round(w / ewres)
        new['n'] = new['s'] + (new['rows'] * nsres)
        new['e'] = new['w'] + (new['cols'] * ewres)

        return new

    def AlignExtentFromDisplay(self):
        """Align region extent based on display size from center
        point"""
        # calculate new bounding box based on center of display
        if self.region["ewres"] > self.region["nsres"]:
            res = self.region["ewres"]
        else:
            res = self.region["nsres"]

        Debug.msg(
            3,
            "Map.AlignExtentFromDisplay(): width=%d, height=%d, res=%f, center=%f,%f" %
            (self.width,
             self.height,
             res,
             self.region['center_easting'],
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
        """Change size of rendered map.

        :param size: map size given as tuple
        """
        try:
            self.width = int(size[0])
            self.height = int(size[1])
            if self.width < 1 or self.height < 1:
                sys.stderr.write(
                    _("Invalid map size %d,%d\n") %
                    (self.width, self.height))
                raise ValueError
        except ValueError:
            self.width = 640
            self.height = 480

        Debug.msg(2, "Map.ChangeMapSize(): width=%d, height=%d" %
                  (self.width, self.height))

    def GetRegion(
            self, rast=None, zoom=False, vect=None, rast3d=None,
            regionName=None, n=None, s=None, e=None, w=None, default=False,
            update=False, add3d=False):
        """Get region settings (g.region -upgc)

        Optionally extent, raster or vector map layer can be given.

        :param rast: list of raster maps
        :param zoom: zoom to raster map (ignore NULLs)
        :param vect: list of vector maps
        :param rast3d: 3d raster map (not list, no support of multiple 3d rasters in g.region)
        :param regionName:  named region or None
        :param n,s,e,w: force extent
        :param default: force default region settings
        :param update: if True update current display region settings
        :param add3d: add 3d region settings

        :return: region settings as dictionary, e.g. {
                 'n':'4928010', 's':'4913700', 'w':'589980',...}

        :func:`GetCurrentRegion()`
        """
        region = {}

        env = os.environ.copy()
        if self.gisrc:
            env['GISRC'] = self.gisrc

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
                cmd['raster'] = ','.join(rast)

        if vect:
            cmd['vector'] = ','.join(vect)

        if rast3d:
            cmd['raster_3d'] = rast3d

        ret, reg, msg = RunCommand('g.region',
                                   read=True,
                                   getErrorMsg=True,
                                   env=env,
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
                message = _(
                    "Unable to get current geographic extent. "
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

        Debug.msg(3, "Map.GetRegion(): %s" % region)

        if update:
            self.region = region

        return region

    def GetCurrentRegion(self):
        """Get current display region settings

        :func:`GetRegion()`
        """
        return self.region

    def SetRegion(self, windres=False, windres3=False):
        """Render string for GRASS_REGION env. variable, so that the
        images will be rendered from desired zoom level.

        :param windres: uses resolution from WIND file rather than
                        display (for modules that require set resolution
                        like d.rast.num)

        :return: String usable for GRASS_REGION variable or None
        """
        grass_region = ""

        if windres:
            compRegion = self.GetRegion(add3d=windres3)
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
                    grass_region += "e-w resol: %.10f; " % \
                        (region['ewres'])
                    continue
                elif key == "n-s resol":
                    grass_region += "n-s resol: %.10f; " % \
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
                    grass_region += key + ": " + self.wind[key] + "; "

            Debug.msg(3, "Map.SetRegion(): %s" % grass_region)

            return grass_region

        except:
            return None

    def GetListOfLayers(self, ltype=None, mapset=None, name=None,
                        active=None, hidden=None, except_ltype=False):
        """Returns list of layers of selected properties or list of
        all layers.

        :param ltype: layer type, e.g. raster/vector/wms/overlay (value or tuple of values)
        :param mapset: all layers from given mapset (only for maplayers)
        :param name: all layers with given name
        :param active: only layers with 'active' attribute set to True or False
        :param hidden: only layers with 'hidden' attribute set to True or False
        :param except_ltype: True to return all layers with type not in ltype

        :return: list of selected layers
        """
        selected = []

        if isinstance(ltype, str):
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
            if ltype is not None:
                if one_type:
                    if (not except_ltype and layer.type != ltype) or \
                            (except_ltype and layer.type == ltype):
                        continue
                elif not one_type:
                    if (not except_ltype and layer.type not in ltype) or \
                       (except_ltype and layer.type in ltype):
                        continue

            # mapset
            if (mapset is not None and ltype != 'overlay') and \
                    layer.GetMapset() != mapset:
                continue

            # name
            if name is not None and layer.name != name:
                continue

            # hidden and active layers
            if active is not None and \
                    hidden is not None:
                if layer.active == active and \
                        layer.hidden == hidden:
                    selected.append(layer)

            # active layers
            elif active is not None:
                if layer.active == active:
                    selected.append(layer)

            # hidden layers
            elif hidden is not None:
                if layer.hidden == hidden:
                    selected.append(layer)

            # all layers
            else:
                selected.append(layer)

        Debug.msg(
            3, "Map.GetListOfLayers(ltype=%s): -> %d" %
            (ltype, len(selected)))

        return selected

    def Render(self, force=False, windres=False):
        """Creates final image composite

        This function can conditionaly use high-level tools, which
        should be avaliable in wxPython library

        :param force: force rendering
        :param windres: use region resolution (True) otherwise display
                        resolution
        """
        self.renderMgr.Render(force, windres)

    def _addLayer(self, layer, pos=-1):
        if layer.type == 'overlay':
            llist = self.overlays
        else:
            llist = self.layers

        # add maplayer to the list of layers
        if pos > -1:
            llist.insert(pos, layer)
        else:
            llist.append(layer)

        Debug.msg(
            3, "Map._addLayer(): layer=%s type=%s" %
            (layer.name, layer.type))

        return layer

    def AddLayer(self, ltype, command, name=None,
                 active=True, hidden=False, opacity=1.0, render=False,
                 pos=-1):
        """Adds generic map layer to list of layers

        :param ltype: layer type ('raster', 'vector', etc.)
        :param command:  GRASS command given as list
        :param name: layer name
        :param active: layer render only if True
        :param hidden: layer not displayed in layer tree if True
        :param opacity: opacity level range from 0(transparent) - 1(not transparent)
        :param render: render an image if True
        :param pos: position in layer list (-1 for append)

        :return: new layer on success
        :return: None on failure
        """
        # opacity must be <0;1>
        if opacity < 0:
            opacity = 0
        elif opacity > 1:
            opacity = 1
        layer = MapLayer(ltype=ltype, name=name, cmd=command, Map=self,
                         active=active, hidden=hidden, opacity=opacity)

        self._addLayer(layer, pos)

        renderMgr = layer.GetRenderMgr()
        Debug.msg(
            1, "Map.AddLayer(): ltype={0}, command={1}".format(
                ltype, layer.GetCmd(string=True)))
        if renderMgr:
            if layer.type == 'wms':
                renderMgr.dataFetched.connect(self.renderMgr.ReportProgress)
            renderMgr.updateProgress.connect(self.renderMgr.ReportProgress)
            renderMgr.renderingFailed.connect(self.renderMgr.RenderingFailed)
        layer.forceRender = render

        self.layerAdded.emit(layer=layer)

        return layer

    def DeleteAllLayers(self, overlay=False):
        """Delete all layers

        :param overlay: True to delete also overlayes
        """
        self.layers = []
        if overlay:
            self.overlays = []

    def DeleteLayer(self, layer, overlay=False):
        """Removes layer from list of layers

        :param layer: layer instance in layer tree
        :param overlay: delete overlay (use self.DeleteOverlay() instead)

        :return: removed layer on success or None
        """
        Debug.msg(3, "Map.DeleteLayer(): name=%s" % layer.name)

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
                # this comes all the way from r28605, so leaving
                # it as it is, although it does not really fit with the
                # new system (but probably works well enough)
                for f in glob.glob(basefile):
                    os.remove(f)

            if layer.GetType() in ('vector', 'thememap'):
                os.remove(layer._legrow)

            list.remove(layer)

            self.layerRemoved.emit(layer=layer)
            return layer

        return None

    def SetLayers(self, layers):
        self.layers = layers
        Debug.msg(5, "Map.SetLayers(): layers={0}".format([layer.GetCmd(string=True) for layer in layers]))

    def ChangeLayer(self, layer, render=False, **kargs):
        """Change map layer properties

        :param layer: map layer instance
        :param ltype: layer type ('raster', 'vector', etc.)
        :param command:  GRASS command given as list
        :param name: layer name
        :param active: layer render only if True
        :param hidden: layer not displayed in layer tree if True
        :param opacity: opacity level range from 0(transparent) - 1(not transparent)
        :param render: render an image if True
        """
        Debug.msg(3, "Map.ChangeLayer(): layer=%s" % layer.name)

        if 'ltype' in kargs:
            layer.SetType(kargs['ltype'])  # check type

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

        self.forceRender = render

        # not needed since there is self.forceRender
        # self.layerChanged(layer=layer)

        return layer

    def ChangeOpacity(self, layer, opacity):
        """Changes opacity value of map layer

        :param layer: layer instance in layer tree
        :param opacity: opacity level <0;1>
        """
        # opacity must be <0;1>
        if opacity < 0:
            opacity = 0
        elif opacity > 1:
            opacity = 1

        layer.opacity = opacity
        Debug.msg(3, "Map.ChangeOpacity(): layer=%s, opacity=%f" %
                  (layer.name, layer.opacity))

    def ChangeLayerActive(self, layer, active):
        """Enable or disable map layer

        :param layer: layer instance in layer tree
        :param active: to be rendered (True)
        """
        layer.active = active
        if active:
            layer.forceRender = True

        Debug.msg(3, "Map.ChangeLayerActive(): name='%s' -> active=%d" %
                  (layer.name, layer.active))

    def ChangeLayerName(self, layer, name):
        """Change name of the layer

        :param layer: layer instance in layer tree
        :param name:  layer name to set up
        """
        Debug.msg(3, "Map.ChangeLayerName(): from=%s to=%s" %
                  (layer.name, name))
        layer.name = name

    def RemoveLayer(self, name=None, id=None):
        """Removes layer from layer list

        Layer is defined by name@mapset or id.

        :param name: layer name (must be unique)
        :param id: layer index in layer list def __init__(self,
                   targetFile, region, bandsNum, gdalDriver,
                   fillValue = None):

        :return: removed layer on success
        :return: None on failure
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
        elif id is not None:
            return self.layers.pop(id)

        return None

    def GetLayerIndex(self, layer, overlay=False):
        """Get index of layer in layer list.

        :param layer: layer instace in layer tree
        :param overlay: use list of overlays instead

        :return: layer index
        :return: -1 if layer not found
        """
        if overlay:
            list = self.overlays
        else:
            list = self.layers

        if layer in list:
            return list.index(layer)

        return -1

    def AddOverlay(self, id, ltype, command,
                   active=True, hidden=True, opacity=1.0, render=False):
        """Adds overlay (grid, barscale, legend, etc.) to list of
        overlays

        :param id: overlay id (PseudoDC)
        :param ltype: overlay type (barscale, legend)
        :param command: GRASS command to render overlay
        :param active: overlay activated (True) or disabled (False)
        :param hidden: overlay is not shown in layer tree (if True)
        :param render: render an image (if True)

        :return: new layer on success
        :return: None on failure
        """
        overlay = Overlay(id=id, name=ltype, cmd=command, Map=self,
                          active=active, hidden=hidden, opacity=opacity)

        self._addLayer(overlay)

        renderMgr = overlay.GetRenderMgr()
        Debug.msg(
            1, "Map.AddOverlay(): cmd={0}".format(overlay.GetCmd(string=True)))
        if renderMgr:
            renderMgr.updateProgress.connect(self.renderMgr.ReportProgress)
            renderMgr.renderingFailed.connect(self.renderMgr.RenderingFailed)
        overlay.forceRender = render

        return overlay

    def ChangeOverlay(self, id, **kargs):
        """Change overlay properities

        Add new overlay if overlay with 'id' doesn't exist.

        :param id: overlay id (PseudoDC)
        :param ltype: overlay ltype (barscale, legend)
        :param command: GRASS command to render overlay
        :param active: overlay activated (True) or disabled (False)
        :param hidden: overlay is not shown in layer tree (if True)
        :param render: render an image (if True)

        :return: new layer on success
        """
        overlay = self.GetOverlay(id, list=False)
        if overlay is None:
            overlay = Overlay(id, ltype=None, cmd=None)

        if 'ltype' in kargs:
            overlay.SetName(kargs['ltype'])  # ltype -> overlay

        if 'command' in kargs:
            overlay.SetCmd(kargs['command'])

        if 'active' in kargs:
            overlay.SetActive(kargs['active'])

        if 'hidden' in kargs:
            overlay.SetHidden(kargs['hidden'])

        if 'opacity' in kargs:
            overlay.SetOpacity(kargs['opacity'])

        if 'render' in kargs:
            overlay.forceRender = kargs['render']

        return overlay

    def GetOverlay(self, id, list=False):
        """Return overlay(s) with 'id'

        :param id: overlay id
        :param list: return list of overlays of True
                     otherwise suppose 'id' to be unique

        :return: list of overlays (list=True)
        :return: overlay (list=False)
        :return: None (list=False) if no overlay or more overlays found
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

        :param overlay: overlay layer

        :return: removed overlay on success or None
        """
        return self.DeleteLayer(overlay, overlay=True)

    def _clean(self, llist):
        for layer in llist:
            layer.Clean()
        del llist[:]

    def Clean(self):
        """Clean layer stack - go trough all layers and remove them
        from layer list.

        Removes also mapfile and maskfile.
        """
        self._clean(self.layers)
        self._clean(self.overlays)
        try_remove(self.mapfile)
        try_remove(self.legfile)

    def ReverseListOfLayers(self):
        """Reverse list of layers"""
        return self.layers.reverse()

    def RenderOverlays(self, force):
        """Render overlays only (for nviz)"""
        self.renderMgr.RenderOverlays(force)

    def AbortAllThreads(self):
        """Abort all layers threads e. g. donwloading data"""
        self.renderMgr.Abort()
