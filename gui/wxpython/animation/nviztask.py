"""!
@package animation.nviztask

@brief Conversion from workspace file to m.nviz.image command

Classes:
 - nviztask::NvizTask

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""

import os
import sys
try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree  # Python <= 2.4

if __name__ == '__main__':
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))

from core.workspace import ProcessWorkspaceFile
from core.gcmd import RunCommand, GException
from core.utils import GetLayerNameFromCmd, CmdToTuple, _
from grass.script import task as gtask


class NvizTask:
    def __init__(self):
        self.task = None
        self.filename = None
        self.region = {}

    def Load(self, filename):
        self.task = gtask.grassTask("m.nviz.image")
        self.filename = filename
        try:
            gxwXml = ProcessWorkspaceFile(etree.parse(self.filename))
        except Exception:
            raise GException(_("Reading workspace file <%s> failed.\n"
                               "Invalid file, unable to parse XML document.") % filename)
        # for display in gxwXml.displays:
            # pprint(display)
        # for layer in gxwXml.layers:
            # pprint(layer)
        # pprint(gxwXml.nviz_state)

        if not gxwXml.nviz_state:
            raise GException(_("No 3d view information in workspace file <%s>.") % self.filename)

        self._getExtent(gxwXml)
        self._processState(gxwXml.nviz_state)
        self._processLayers(gxwXml.layers)

    def _getExtent(self, root):
        for display in root.displays:
            if display['viewMode'] == '3d':
                self.region['w'], self.region['s'],\
                self.region['e'], self.region['n'] = display['extent']

    def _processLayers(self, layers):
        for layer in layers:
            if not layer['checked']:
                continue

            if not layer['nviz']:
                continue
            layerName, found = GetLayerNameFromCmd(layer['cmd'], fullyQualified=False,
                                                   param='map')
            if not found:
                continue

            if 'surface' in layer['nviz']:
                self._processSurface(layer['nviz']['surface'], mapName=layerName)

    def _processSurface(self, surface, mapName):
        self._setMultiTaskParam('elevation_map', mapName)

        # attributes like color, shine, transparency
        attributes = ('color', 'shine', 'transp')  # mask missing
        parameters = (('color_map', 'color'),
                     ('shininess_map', 'shininess_value'),
                     ('transparency_map', 'transparency_value'))
        for attr, params in zip(attributes, parameters):
            mapname = None
            const = None
            if attr in surface['attribute']:
                if surface['attribute'][attr]['map']:
                    mapname = surface['attribute'][attr]['value']
                else:
                    const = surface['attribute'][attr]['value']
            else:
                if attr == 'transp':
                    const = 0
                elif attr == 'color':
                    mapname = mapName

            if mapname:
                self._setMultiTaskParam(params[0], mapname)
            else:
                self._setMultiTaskParam(params[1], const)

        # draw mode
        for mode in ('mode', 'shading', 'style'):
            value = surface['draw']['mode']['desc'][mode]
            self._setMultiTaskParam(mode, value)
        # wire color
        value = surface['draw']['wire-color']['value']
        self._setMultiTaskParam('wire_color', value)
        # resolution
        for mode1, mode2 in zip(('coarse', 'fine'), ('resolution_coarse', 'resolution_fine')):
            value = surface['draw']['resolution'][mode1]
            self._setMultiTaskParam(mode2, value)

        # position
        pos = []
        for coor in ('x', 'y', 'z'):
            pos.append(str(surface['position'][coor]))
        value = ','.join(pos)
        self._setMultiTaskParam('surface_position', value)

    def _processState(self, state):
        color = state['view']['background']['color']
        self.task.set_param('bgcolor', self._join(color, delim=':'))
        self.task.set_param('position', self._join((state['view']['position']['x'],
                                                    state['view']['position']['y'])))
        self.task.set_param('height', state['iview']['height']['value'])
        self.task.set_param('perspective', state['view']['persp']['value'])
        self.task.set_param('twist', state['view']['twist']['value'])
        # TODO: fix zexag
        self.task.set_param('zexag', state['view']['z-exag']['value'])
        self.task.set_param('focus', self._join((state['iview']['focus']['x'],
                                                 state['iview']['focus']['y'],
                                                 state['iview']['focus']['z'])))
        self.task.set_param('light_position', self._join((state['light']['position']['x'],
                                                          state['light']['position']['y'],
                                                          state['light']['position']['z'] / 100.)))
        color = state['light']['color'][:3]
        self.task.set_param('light_color', self._join(color, delim=':'))
        self.task.set_param('light_brightness', int(state['light']['bright']))
        self.task.set_param('light_ambient', state['light']['ambient'])

    def _setMultiTaskParam(self, param, value):
        last = self.task.get_param(param)['value']
        self.task.set_param(param, self._join((last, value)))

    def _join(self, toJoin, delim=','):
        toJoin = filter(self._ignore, toJoin)
        return delim.join(map(str, toJoin))

    def _ignore(self, value):
        if value == '' or value is None:
            return False
        else:
            return True

    def ListMapParameters(self):
        # params = self.task.get_list_params()
        # parameter with 'map' name
        # params = filter(lambda x: 'map' in x, params)
        return ('elevation_map', 'color_map', 'vline', 'vpoint')

    def GetCommandSeries(self, layerList, paramName):
        commands = []
        if not self.task:
            return commands

        if len(layerList) > 1:
            raise GException(_("Please add only one layer in the list."))
            return
        layer = layerList[0]
        if hasattr(layer, 'maps'):
            series = layer.maps
        else:
            raise GException(_("No map series nor space-time dataset is added."))

        for value in series:
            self.task.set_param(paramName, value)
            # FIXME: we assume we want always default color map
            if paramName == 'elevation_map':
                self.task.set_param('color_map', '')
            self.task.set_flag('overwrite', True)
            self.task.set_param('output', 'tobechanged')
            cmd = self.task.get_cmd(ignoreErrors=False, ignoreRequired=False, ignoreDefault=True)
            commands.append(cmd)

        return commands

    def GetCommand(self):
        if not self.task:
            return None
        self.task.set_flag('overwrite', True)
        self.task.set_param('output', 'tobechanged')
        cmd = self.task.get_cmd(ignoreErrors=False, ignoreRequired=False, ignoreDefault=True)
        return CmdToTuple(cmd)

    def GetRegion(self):
        return self.region


def test():
    nviz = NvizTask('/home/anna/testy/nviz/t12.gxw')
    # nviz = NvizState('/home/anna/testy/nviz/t3.gxw')

    # cmd = nviz.GetCommand()
    cmds = nviz.GetCommandSeries(['aspect', 'elevation'], 'color_map')
    for cmd in cmds:
        print cmd
        returncode, message = RunCommand(getErrorMsg=True, prog=cmd[0], **cmd[1])
        print returncode, message


if __name__ == '__main__':

    test()
