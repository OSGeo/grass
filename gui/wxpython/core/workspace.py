"""
@package core.workspace

@brief Open/save workspace definition file

Classes:
 - workspace::ProcessWorkspaceFile
 - workspace::WriteWorkspaceFile
 - workspace::ProcessGrcFile

(C) 2007-2016 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os

import wx
import six
try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

from core.utils import normalize_whitespace
from core.settings import UserSettings
from core.gcmd import EncodeString, GetDefaultEncoding
from nviz.main import NvizSettings

from grass.script import core as gcore


def get_database_location_mapset():
    """Returns GRASS database, location, and mapset as a tuple"""
    gisenv = gcore.gisenv()
    return (gisenv['GISDBASE'],
            gisenv['LOCATION_NAME'],
            gisenv['MAPSET'])


class ProcessWorkspaceFile:

    def __init__(self, tree):
        """A ElementTree handler for the GXW XML file, as defined in
        grass-gxw.dtd.
        """
        self.tree = tree
        self.root = self.tree.getroot()

        #
        # layer manager properties
        #
        self.layerManager = {'pos': None,  # window position
                             'size': None,  # window size
                             'cwd': None}  # current working directory

        #
        # list of mapdisplays
        #
        self.displays = []
        #
        # list of map layers
        #
        self.layers = []
        #
        # list of overlays
        self.overlays = []
        #
        # nviz state
        #
        self.nviz_state = {}

        self.displayIndex = -1  # first display has index '0'

        self.__processFile()

        if NvizSettings:
            self.nvizDefault = NvizSettings()
        else:
            self.nvizDefault = None

    def __filterValue(self, value):
        """Filter value

        :param value:
        """
        value = value.replace('&lt;', '<')
        value = value.replace('&gt;', '>')

        return value

    def __getNodeText(self, node, tag, default=''):
        """Get node text"""
        p = node.find(tag)
        if p is not None:
            # if empty text is inside tag,
            # etree returns None
            if p.text is None:
                return ''

            return normalize_whitespace(p.text)

        return default

    def __processFile(self):
        """Process workspace file"""

        self.__processSession()
        
        #
        # layer manager
        #
        node_lm = self.root.find('layer_manager')
        if node_lm is not None:
            posAttr = node_lm.get('dim', '')
            if posAttr:
                posVal = list(map(int, posAttr.split(',')))
                try:
                    self.layerManager['pos'] = (posVal[0], posVal[1])
                    self.layerManager['size'] = (posVal[2], posVal[3])
                except:
                    pass
            # current working directory
            cwdPath = self.__getNodeText(node_lm, 'cwd')
            if cwdPath:
                self.layerManager['cwd'] = cwdPath

        #
        # displays
        #
        for display in self.root.findall('display'):
            self.displayIndex += 1

            # window position and size
            posAttr = display.get('dim', '')
            if posAttr:
                posVal = list(map(int, posAttr.split(',')))
                try:
                    pos = (posVal[0], posVal[1])
                    size = (posVal[2], posVal[3])
                except:
                    pos = None
                    size = None
                # this happens on Windows when mapwindow is minimized when
                # saving workspace
                else:
                    if posVal[0] == -32000:
                        pos = None
                        size = None
            else:
                pos = None
                size = None

            extentAttr = display.get('extent', '')
            if extentAttr:
                # w, s, e, n
                extent = map(float, extentAttr.split(','))
            else:
                extent = None

            # projection
            node_projection = display.find('projection')
            if node_projection is not None:
                projection = {
                    'enabled': True, 'epsg': node_projection.get(
                        'epsg', ''), 'proj': self.__getNodeText(
                        node_projection, 'value')}
            else:
                projection = {'enabled': False}

            self.displays.append({
                "name": display.get('name'),
                "render": bool(int(display.get('render', "0"))),
                "mode": int(display.get('mode', 0)),
                "showCompExtent": bool(int(display.get('showCompExtent', "0"))),
                "showStatusbar": bool(int(display.get('showStatusbar', "0"))),
                "showToolbars": bool(int(display.get('showToolbars', "0"))),
                "pos": pos,
                "size": size,
                "extent": extent,
                "tbres": display.get('tbres', '0'),
                "alignExtent": bool(int(display.get('alignExtent', "0"))),
                "constrainRes": bool(int(display.get('constrainRes', "0"))),
                "projection": projection,
                "viewMode": display.get('viewMode', '2d')})

            # process all layers/groups in the display
            self.__processLayers(display)
            # process nviz_state
            self.__processNvizState(display)

    def __processSession(self):
        session = self.root.find('session')
        if session is None:
            self.database = None
            self.location = None
            self.mapset = None
            return
        self.database = self.__filterValue(self.__getNodeText(session, 'database'))
        self.location = self.__filterValue(self.__getNodeText(session, 'location'))
        self.mapset = self.__filterValue(self.__getNodeText(session, 'mapset'))

    def __processLayers(self, node, inGroup=-1):
        """Process layers/groups of selected display

        :param node: display tree node
        :param inGroup: in group -> index of group item otherwise -1
        """
        for item in node:
            if item.tag == 'group':
                # -> group
                self.layers.append({
                    "type": 'group',
                    "name": item.get('name', ''),
                    "checked": bool(int(item.get('checked', "0"))),
                    "opacity": None,
                    "cmd": None,
                    "group": inGroup,
                    "display": self.displayIndex,
                    "vdigit": None,
                    "nviz": None})

                self.__processLayers(
                    item, inGroup=len(
                        self.layers) - 1)  # process items in group

            elif item.tag == 'layer':
                cmd, selected, vdigit, nviz = self.__processLayer(item)
                lname = item.get('name', None)
                if lname and '\\n' in lname:
                    lname = lname.replace('\\n', os.linesep)

                self.layers.append({
                    "type": item.get('type', None),
                    "name": lname,
                    "checked": bool(int(item.get('checked', "0"))),
                    "opacity": float(item.get('opacity', '1.0')),
                    "cmd": cmd,
                    "group": inGroup,
                    "display": self.displayIndex,
                    "selected": selected,
                    "vdigit": vdigit,
                    "nviz": nviz})

            elif item.tag == 'overlay':
                cmd = self.__processOverlay(item)

                self.overlays.append({
                    "display": self.displayIndex,
                    "cmd": cmd})

    def __processLayer(self, layer):
        """Process layer item

        :param layer: tree node
        """
        cmd = list()

        #
        # layer attributes (task) - 2D settings
        #
        node_task = layer.find('task')
        if node_task is None and layer.get('type') == 'command':
            # TODO: perhaps the XML format should be changed and command
            # should be changed to contain task
            # TODO: where the command layer gets actually processed?
            pass
        else:
            cmd.append(node_task.get('name', "unknown"))

            # flags
            for p in node_task.findall('flag'):
                flag = p.get('name', '')
                if len(flag) > 1:
                    cmd.append('--' + flag)
                else:
                    cmd.append('-' + flag)

            # parameters
            for p in node_task.findall('parameter'):
                cmd.append('%s=%s' % (p.get('name', ''),
                                      self.__filterValue(
                                          self.__getNodeText(p, 'value'))))

        if layer.find('selected') is not None:
            selected = True
        else:
            selected = False

        #
        # Vector digitizer settings
        #
        node_vdigit = layer.find('vdigit')
        if node_vdigit is not None:
            vdigit = self.__processLayerVdigit(node_vdigit)
        else:
            vdigit = None

        #
        # Nviz (3D settings)
        #
        node_nviz = layer.find('nviz')
        if node_nviz is not None:
            nviz = self.__processLayerNviz(node_nviz)
        else:
            nviz = None

        return (cmd, selected, vdigit, nviz)

    def __processOverlay(self, node_overlay):
        """
        Process overlay item
        :param overlay: tree node
        """
        cmd = list()

        cmd.append(node_overlay.get('name', "unknown"))

        # flags
        for f in node_overlay.findall('flag'):
            flag = f.get('name', '')
            if len(flag) > 1:
                cmd.append('--' + flag)
            else:
                cmd.append('-' + flag)

        # parameters
        for p in node_overlay.findall('parameter'):
            cmd.append('%s=%s' % (p.get('name', ''),
                                  self.__filterValue(
                                      self.__getNodeText(p, 'value'))))

        return cmd

    def __processLayerVdigit(self, node_vdigit):
        """Process vector digitizer layer settings

        :param node_vdigit: vdigit node
        """
        # init nviz layer properties
        vdigit = dict()
        for node in node_vdigit.findall('geometryAttribute'):
            if 'geomAttr' not in vdigit:
                vdigit['geomAttr'] = dict()
            type = node.get('type')
            vdigit['geomAttr'][type] = dict()
            vdigit['geomAttr'][type]['column'] = node.get('column')  # required
            # default map units
            vdigit['geomAttr'][type]['units'] = node.get('units', 'mu')

        return vdigit

    def __processLayerNviz(self, node_nviz):
        """Process 3D layer settings

        :param node_nviz: nviz node
        """
        # init nviz layer properties
        nviz = {}
        if node_nviz.find('surface') is not None:  # -> raster
            nviz['surface'] = {}
            for sec in ('attribute', 'draw', 'position'):
                nviz['surface'][sec] = {}
            self.__processLayerNvizSurface(nviz, node_nviz.find('surface'))
        elif node_nviz.find('volume') is not None:  # -> raster
            nviz['volume'] = {}
            for sec in ('attribute', 'draw', 'position'):
                nviz['volume'][sec] = {}
            self.__processLayerNvizVolume(nviz, node_nviz.find('volume'))
        elif node_nviz.find('vlines') is not None or \
                node_nviz.find('vpoints') is not None:  # -> vector
            nviz['vector'] = {}
            for sec in ('lines', 'points'):
                nviz['vector'][sec] = {}
            if node_nviz.find('vlines'):
                self.__processLayerNvizVectorLines(
                    nviz, node_nviz.find('vlines'))
            if node_nviz.find('vpoints'):
                self.__processLayerNvizVectorPoints(
                    nviz, node_nviz.find('vpoints'))

        return nviz

    def __processLayerNvizSurface(self, nvizData, nodeSurface):
        """Process 3D layer settings - surface

        :param nodeData: nviz data dict
        :param nodeSurface: nviz surface node
        """
        # attributes
        for attrb in nodeSurface.findall('attribute'):
            tagName = str(attrb.tag)
            attrbName = attrb.get('name', '')
            dc = nvizData['surface'][tagName][attrbName] = {}
            if attrb.get('map', '0') == '0':
                dc['map'] = False
            else:
                dc['map'] = True
            value = self.__getNodeText(attrb, 'value')
            try:
                dc['value'] = int(value)
            except ValueError:
                try:
                    dc['value'] = float(value)
                except ValueError:
                    dc['value'] = str(value)

        # draw
        node_draw = nodeSurface.find('draw')
        if node_draw is not None:
            tagName = str(node_draw.tag)
            nvizData['surface'][tagName]['all'] = False
            nvizData['surface'][tagName]['mode'] = {}
            nvizData['surface'][tagName]['mode'][
                'value'] = -1  # to be calculated
            nvizData['surface'][tagName]['mode']['desc'] = {}
            nvizData['surface'][tagName]['mode']['desc']['shading'] = \
                str(node_draw.get('shading', ''))
            nvizData['surface'][tagName]['mode']['desc']['style'] = \
                str(node_draw.get('style', ''))
            nvizData['surface'][tagName]['mode']['desc']['mode'] = \
                str(node_draw.get('mode', ''))

            # resolution
            for node_res in node_draw.findall('resolution'):
                resType = str(node_res.get('type', ''))
                if 'resolution' not in nvizData['surface']['draw']:
                    nvizData['surface']['draw']['resolution'] = {}
                value = int(self.__getNodeText(node_res, 'value'))
                nvizData['surface']['draw']['resolution'][resType] = value

            # wire-color
            node_wire_color = node_draw.find('wire_color')
            if node_wire_color is not None:
                nvizData['surface']['draw']['wire-color'] = {}
                value = str(self.__getNodeText(node_wire_color, 'value'))
                nvizData['surface']['draw']['wire-color']['value'] = value
        # position
        node_pos = nodeSurface.find('position')
        if node_pos is not None:
            dc = nvizData['surface']['position']
            for coor in ['x', 'y', 'z']:
                node = node_pos.find(coor)
                if node is None:
                    continue
                value = int(self.__getNodeText(node_pos, coor))
                dc[coor] = value

    def __processLayerNvizVolume(self, nvizData, nodeVolume):
        """Process 3D layer settings - volume

        :param nodeData: nviz data dict
        :param nodeVolume: nviz volume node
        """
        # attributes
        for attrb in nodeVolume.findall('attribute'):
            tagName = str(attrb.tag)
            attrbName = attrb.get('name', '')
            dc = nvizData['volume'][tagName][attrbName] = {}
            if attrb.get('map') == '0':
                dc['map'] = False
            elif attrb.get('map') == '1':
                dc['map'] = True
            else:
                dc['map'] = None
            value = self.__getNodeText(attrb, 'value')
            try:
                dc['value'] = int(value)
            except ValueError:
                try:
                    dc['value'] = float(value)
                except ValueError:
                    dc['value'] = str(value)

        # draw
        node_draw = nodeVolume.find('draw')
        if node_draw is not None:
            node_box = node_draw.find('box')
            if node_box is not None:
                nvizData['volume']['draw']['box'] = {}
                enabled = bool(int(self.__getNodeText(node_box, 'enabled')))
                nvizData['volume']['draw']['box']['enabled'] = enabled
            node_mode = node_draw.find('mode')
            if node_mode is not None:
                nvizData['volume']['draw']['mode'] = {}
                desc = self.__getNodeText(node_mode, 'desc')
                value = int(self.__getNodeText(node_mode, 'value'))
                nvizData['volume']['draw']['mode']['desc'] = desc
                nvizData['volume']['draw']['mode']['value'] = value
            node_res = node_draw.find('resolution')
            if node_res is not None:
                nvizData['volume']['draw']['resolution'] = {}
                for vol_type in ('isosurface', 'slice'):
                    nd = node_res.find(vol_type)
                    value = int(self.__getNodeText(nd, 'value'))
                    nvizData['volume']['draw']['resolution'][
                        vol_type] = {'value': value}
            node_shading = node_draw.find('shading')
            if node_shading is not None:
                nvizData['volume']['draw']['shading'] = {}
                for vol_type in ('isosurface', 'slice'):
                    nd = node_shading.find(vol_type)
                    value = int(self.__getNodeText(nd, 'value'))
                    desc = self.__getNodeText(nd, 'desc')
                    nvizData['volume']['draw']['shading'][
                        vol_type] = {'value': value, 'desc': desc}

        nvizData['volume']['isosurface'] = []
        for isosurfaceNode in nodeVolume.findall('isosurface'):
            isoDict = {}
            for att in ('topo', 'transp', 'shine', 'color'):
                attNode = isosurfaceNode.find(att)
                if attNode is not None:
                    isMap = attNode.find('map').text
                    value = attNode.find('value').text
                    if not isMap:
                        isoDict[att] = {'map': None}
                        isoDict[att]['value'] = float(value)
                    elif isMap == '1':
                        isoDict[att] = {'map': True}
                        isoDict[att]['value'] = value
                    else:
                        isoDict[att] = {'map': False}
                        isoDict[att]['value'] = float(value)
            inout = isosurfaceNode.find('inout')
            if inout is not None:
                isoDict['inout'] = {'value': int(
                    float(inout.find('value').text))}
            nvizData['volume']['isosurface'].append(isoDict)

        nvizData['volume']['slice'] = []
        for sliceNode in nodeVolume.findall('slice'):
            sliceDict = {}
            sliceDict['transp'] = {
                'value': int(
                    sliceNode.find('transp').find('value').text)}
            sliceDict['position'] = {}
            for child in sliceNode.find('position'):
                if child.tag == 'axis':
                    sliceDict['position'][child.tag] = int(child.text)
                else:
                    sliceDict['position'][child.tag] = float(child.text)
            nvizData['volume']['slice'].append(sliceDict)

        # position
        node_pos = nodeVolume.find('position')
        if node_pos is not None:
            dc = nvizData['volume']['position']
            for coor in ['x', 'y', 'z']:
                node = node_pos.find(coor)
                if node is None:
                    continue
                value = int(self.__getNodeText(node_pos, coor))
                dc[coor] = value

    def __processLayerNvizVectorPoints(self, nvizData, nodePoints):
        """Process 3D layer settings - vector points

        :param nodeData: nviz data dict
        :param nodeVector: nviz vector points node
        """
        marker = str(nodePoints.get('marker', ''))
        markerId = list(
            UserSettings.Get(
                group='nviz',
                key='vector',
                subkey=[
                    'points',
                    'marker'],
                settings_type='internal')).index(marker)
        nvizData['vector']['points']['marker'] = {'value': markerId}

        node_mode = nodePoints.find('mode')
        if node_mode is not None:
            nvizData['vector']['points']['mode'] = {}
            nvizData['vector']['points']['mode'][
                'type'] = str(node_mode.get('type', 'surface'))
            nvizData['vector']['points']['mode']['surface'] = {}
            nvizData['vector']['points']['mode']['surface']['value'] = []
            nvizData['vector']['points']['mode']['surface']['show'] = []

            # map
            for node_map in node_mode.findall('map'):
                nvizData['vector']['points']['mode']['surface']['value'].append(
                    self.__processLayerNvizNode(node_map, 'name', str))
                nvizData['vector']['points']['mode']['surface']['show'].append(
                    bool(self.__processLayerNvizNode(node_map, 'checked', int)))

        # color
        self.__processLayerNvizNode(nodePoints, 'color', str,
                                    nvizData['vector']['points'])

        # width
        self.__processLayerNvizNode(nodePoints, 'width', int,
                                    nvizData['vector']['points'])

        # height
        self.__processLayerNvizNode(nodePoints, 'height', float,
                                    nvizData['vector']['points'])

        # height
        self.__processLayerNvizNode(nodePoints, 'size', float,
                                    nvizData['vector']['points'])

        # thematic
        node_thematic = nodePoints.find('thematic')
        thematic = nvizData['vector']['points']['thematic'] = {}
        thematic['rgbcolumn'] = self.__processLayerNvizNode(
            node_thematic, 'rgbcolumn', str)
        thematic['sizecolumn'] = self.__processLayerNvizNode(
            node_thematic, 'sizecolumn', str)
        for col in ('rgbcolumn', 'sizecolumn'):
            if thematic[col] == 'None':
                thematic[col] = None
        thematic['layer'] = self.__processLayerNvizNode(
            node_thematic, 'layer', int)
        for use in ('usecolor', 'usesize', 'usewidth'):
            if node_thematic.get(use, ''):
                thematic[use] = int(node_thematic.get(use, '0'))

    def __processLayerNvizVectorLines(self, nvizData, nodeLines):
        """Process 3D layer settings - vector lines

        :param nodeData: nviz data dict
        :param nodeVector: nviz vector lines node
        """
        node_mode = nodeLines.find('mode')
        if node_mode is not None:
            nvizData['vector']['lines']['mode'] = {}
            nvizData['vector']['lines']['mode'][
                'type'] = str(node_mode.get('type', ''))
            nvizData['vector']['lines']['mode']['surface'] = {}
            nvizData['vector']['lines']['mode']['surface']['value'] = []
            nvizData['vector']['lines']['mode']['surface']['show'] = []

            # map
            for node_map in node_mode.findall('map'):
                nvizData['vector']['lines']['mode']['surface']['value'].append(
                    self.__processLayerNvizNode(node_map, 'name', str))
                nvizData['vector']['lines']['mode']['surface']['show'].append(
                    bool(self.__processLayerNvizNode(node_map, 'checked', int)))

        # color
        self.__processLayerNvizNode(nodeLines, 'color', str,
                                    nvizData['vector']['lines'])

        # width
        self.__processLayerNvizNode(nodeLines, 'width', int,
                                    nvizData['vector']['lines'])

        # height
        self.__processLayerNvizNode(nodeLines, 'height', int,
                                    nvizData['vector']['lines'])

        # thematic
        node_thematic = nodeLines.find('thematic')
        thematic = nvizData['vector']['lines']['thematic'] = {}
        thematic['rgbcolumn'] = self.__processLayerNvizNode(
            node_thematic, 'rgbcolumn', str)
        thematic['sizecolumn'] = self.__processLayerNvizNode(
            node_thematic, 'sizecolumn', str)
        for col in ('rgbcolumn', 'sizecolumn'):
            if thematic[col] == 'None':
                thematic[col] = None
        thematic['layer'] = self.__processLayerNvizNode(
            node_thematic, 'layer', int)
        for use in ('usecolor', 'usesize', 'usewidth'):
            if node_thematic.get(use, ''):
                thematic[use] = int(node_thematic.get(use, '0'))

    def __processLayerNvizNode(self, node, tag, cast, dc=None):
        """Process given tag nviz/vector"""
        node_tag = node.find(tag)
        if node_tag is not None:
            if node_tag.find('value') is not None:
                value = cast(self.__getNodeText(node_tag, 'value'))
            else:
                try:
                    value = cast(node_tag.text)
                except ValueError:
                    if cast == str:
                        value = ''
                    else:
                        value = None
            if dc:
                dc[tag] = dict()
                dc[tag]['value'] = value
            else:
                return value

    def __processNvizState(self, node):
        """Process tag nviz_state"""
        node_state = node.find('nviz_state')
        if node_state is None:
            return
        self.nviz_state['display'] = self.displayIndex
        #
        # view
        #
        node_view = node_state.find('view')
        view = {}
        iview = {}

        node_position = node_view.find('v_position')
        view['position'] = {}
        view['position']['x'] = self.__processLayerNvizNode(
            node_position, 'x', float)
        view['position']['y'] = self.__processLayerNvizNode(
            node_position, 'y', float)
        node_persp = node_view.find('persp')
        view['persp'] = {}
        iview['persp'] = {}
        view['persp']['value'] = self.__processLayerNvizNode(
            node_persp, 'value', int)
        view['persp']['step'] = self.__processLayerNvizNode(
            node_persp, 'step', int)
        iview['persp']['min'] = self.__processLayerNvizNode(
            node_persp, 'min', int)
        iview['persp']['max'] = self.__processLayerNvizNode(
            node_persp, 'max', int)
        node_height = node_view.find('v_height')
        iview['height'] = {}
        iview['height']['value'] = self.__processLayerNvizNode(
            node_height, 'value', int)
        iview['height']['min'] = self.__processLayerNvizNode(
            node_height, 'min', int)
        iview['height']['max'] = self.__processLayerNvizNode(
            node_height, 'max', int)
        node_twist = node_view.find('twist')
        view['twist'] = {}
        iview['twist'] = {}
        view['twist']['value'] = self.__processLayerNvizNode(
            node_twist, 'value', int)
        iview['twist']['min'] = self.__processLayerNvizNode(
            node_twist, 'min', int)
        iview['twist']['max'] = self.__processLayerNvizNode(
            node_twist, 'max', int)
        node_zexag = node_view.find('z-exag')
        view['z-exag'] = {}
        iview['z-exag'] = {}
        view[
            'z-exag']['value'] = self.__processLayerNvizNode(node_zexag, 'value', float)
        view[
            'z-exag']['min'] = self.__processLayerNvizNode(node_zexag, 'min', int)
        view[
            'z-exag']['max'] = self.__processLayerNvizNode(node_zexag, 'max', int)
        iview[
            'z-exag']['llRatio'] = self.__processLayerNvizNode(node_zexag, 'llRatio', float)
        node_focus = node_view.find('focus')
        iview['focus'] = {}
        iview['focus']['x'] = self.__processLayerNvizNode(node_focus, 'x', int)
        iview['focus']['y'] = self.__processLayerNvizNode(node_focus, 'y', int)
        iview['focus']['z'] = self.__processLayerNvizNode(node_focus, 'z', int)
        node_dir = node_view.find('dir')
        if node_dir:
            iview['dir'] = {}
            iview['dir']['x'] = self.__processLayerNvizNode(node_dir, 'x', int)
            iview['dir']['y'] = self.__processLayerNvizNode(node_dir, 'y', int)
            iview['dir']['z'] = self.__processLayerNvizNode(node_dir, 'z', int)
            iview['dir']['use'] = True
        else:
            iview['dir'] = {}
            iview['dir']['x'] = -1
            iview['dir']['y'] = -1
            iview['dir']['z'] = -1
            iview['dir']['use'] = False
        node_rot = node_view.find('rotation')
        if node_rot is not None:
            rotation = node_rot.text
            if rotation:
                iview['rotation'] = [float(item)
                                     for item in rotation.split(',')]

        view['background'] = {}
        color = self.__processLayerNvizNode(node_view, 'background_color', str)
        view['background']['color'] = tuple(map(int, color.split(':')))

        self.nviz_state['view'] = view
        self.nviz_state['iview'] = iview
        #
        # light
        #
        node_light = node_state.find('light')
        light = {}

        node_position = node_light.find('l_position')
        light['position'] = {}
        light['position']['x'] = self.__processLayerNvizNode(
            node_position, 'x', float)
        light['position']['y'] = self.__processLayerNvizNode(
            node_position, 'y', float)
        light['position']['z'] = self.__processLayerNvizNode(
            node_position, 'z', int)

        light['bright'] = self.__processLayerNvizNode(
            node_light, 'bright', int)
        light['ambient'] = self.__processLayerNvizNode(
            node_light, 'ambient', int)
        color = self.__processLayerNvizNode(node_light, 'color', str)
        light['color'] = tuple(map(int, color.split(':')))

        self.nviz_state['light'] = light

        node_constants = node_state.find('constant_planes')
        constants = []
        if node_constants:
            for node_plane in node_constants.findall('plane'):
                plane = {}
                plane['color'] = self.__processLayerNvizNode(
                    node_plane, 'color', str)
                plane['resolution'] = self.__processLayerNvizNode(
                    node_plane, 'fine_resolution', int)
                plane['value'] = self.__processLayerNvizNode(
                    node_plane, 'height', int)
                plane['object'] = {}
                constants.append({'constant': plane})
        self.nviz_state['constants'] = constants


class WriteWorkspaceFile(object):
    """Generic class for writing workspace file"""

    def __init__(self, lmgr, file):
        self.outfile = file
        file = StringIO()
        self.file = file
        self.lmgr = lmgr
        self.indent = 0

        # write header

        self.file.write(
            '<?xml version="1.0" encoding="%s"?>\n' %
            GetDefaultEncoding(
                forceUTF8=True))
        self.file.write('<!DOCTYPE gxw SYSTEM "grass-gxw.dtd">\n')
        self.file.write('%s<gxw>\n' % (' ' * self.indent))

        self.indent = + 4

        database, location, mapset = get_database_location_mapset()

        file.write('{indent}<session>\n'.format(indent=' ' * self.indent))
        self.indent += 4
        file.write('{indent}<database>{database}</database>\n'.format(
            indent=' ' * self.indent, database=database))
        file.write('{indent}<location>{location}</location>\n'.format(
            indent=' ' * self.indent, location=location))
        file.write('{indent}<mapset>{mapset}</mapset>\n'.format(
            indent=' ' * self.indent, mapset=mapset))
        self.indent -= 4
        file.write('{indent}</session>\n'.format(indent=' ' * self.indent))

        # layer manager
        windowPos = self.lmgr.GetPosition()
        windowSize = self.lmgr.GetSize()
        file.write(
            '%s<layer_manager dim="%d,%d,%d,%d">\n' %
            (' ' *
             self.indent,
             windowPos[0],
             windowPos[1],
             windowSize[0],
             windowSize[1]))
        self.indent += 4
        cwdPath = self.lmgr.GetCwdPath()
        if cwdPath:
            file.write('%s<cwd>%s</cwd>\n' % (' ' * self.indent, cwdPath))
        self.indent -= 4
        file.write('%s</layer_manager>\n' % (' ' * self.indent))

        # list of displays
        for page in range(0, self.lmgr.GetLayerNotebook().GetPageCount()):
            dispName = self.lmgr.GetLayerNotebook().GetPageText(page)
            mapTree = self.lmgr.GetLayerNotebook().GetPage(page).maptree
            region = mapTree.GetMap().GetCurrentRegion()
            compRegion = gcore.region(region3d=True)
            mapdisp = mapTree.GetMapDisplay()

            displayPos = mapdisp.GetPosition()
            displaySize = mapdisp.GetSize()
            if mapdisp.toolbars['map'].combo.GetSelection() == 1:
                viewmode = '3d'
            else:
                viewmode = '2d'

            file.write('%s<display '
                       'name="%s" render="%d" '
                       'mode="%d" showCompExtent="%d" '
                       'alignExtent="%d" '
                       'constrainRes="%d" '
                       'showStatusbar="%d" '
                       'showToolbars="%d" '
                       'dim="%d,%d,%d,%d" '
                       'extent="%f,%f,%f,%f,%f,%f" '
                       'tbres="%f" '  # needed only for animation tool
                       'viewMode="%s" >\n' % (' ' * self.indent,
                                              dispName,
                                              int(mapdisp.mapWindowProperties.autoRender),
                                              mapdisp.statusbarManager.GetMode(),
                                              int(mapdisp.mapWindowProperties.showRegion),
                                              int(mapdisp.mapWindowProperties.alignExtent),
                                              int(mapdisp.mapWindowProperties.resolution),
                                              int(mapdisp.statusbarManager.IsShown()),
                                              int(mapdisp.GetMapToolbar().IsShown()),
                                              displayPos[0],
                                              displayPos[1],
                                              displaySize[0],
                                              displaySize[1],
                                              region['w'],
                                              region['s'],
                                              region['e'],
                                              region['n'],
                                              compRegion['b'],
                                              compRegion['t'],
                                              compRegion['tbres'],
                                              viewmode
                                              ))
            # projection statusbar info
            if mapdisp.GetProperty('projection') and UserSettings.Get(
                    group='display', key='projection', subkey='proj4'):
                self.indent += 4
                file.write('%s<projection' % (' ' * self.indent))
                epsg = UserSettings.Get(
                    group='display', key='projection', subkey='epsg')
                if epsg:
                    file.write(' epsg="%s"' % epsg)
                file.write('>\n')
                proj = UserSettings.Get(
                    group='display', key='projection', subkey='proj4')
                self.indent += 4
                file.write('%s<value>%s</value>\n' % (' ' * self.indent, proj))
                self.indent -= 4
                file.write('%s</projection>\n' % (' ' * self.indent))
                self.indent -= 4

            # list of layers
            item = mapTree.GetFirstChild(mapTree.root)[0]
            self.__writeLayer(mapTree, item)

            if mapdisp.MapWindow3D is not None:
                nvizDisp = mapdisp.MapWindow3D
                self.__writeNvizState(
                    view=nvizDisp.view,
                    iview=nvizDisp.iview,
                    light=nvizDisp.light,
                    constants=nvizDisp.constants)

            # list of map elements
            item = mapTree.GetFirstChild(mapTree.root)[0]
            self.__writeOverlay(mapdisp)

            file.write('%s</display>\n' % (' ' * self.indent))

        self.indent = - 4
        file.write('%s</gxw>\n' % (' ' * self.indent))

        self.outfile.write(EncodeString(file.getvalue()))
        file.close()

    def __filterValue(self, value):
        """Make value XML-valid"""
        value = value.replace('<', '&lt;')
        value = value.replace('>', '&gt;')
        value = value.replace('&', '&amp;')

        return value

    def __writeLayer(self, mapTree, item):
        """Write bunch of layers to GRASS Workspace XML file"""
        self.indent += 4
        itemSelected = mapTree.GetSelections()
        while item and item.IsOk():
            type = mapTree.GetLayerInfo(item, key='type')
            if type != 'group':
                maplayer = mapTree.GetLayerInfo(item, key='maplayer')
            else:
                maplayer = None

            checked = int(item.IsChecked())
            if type == 'command':
                cmd = mapTree.GetLayerInfo(
                    item, key='maplayer').GetCmd(
                    string=True)
                self.file.write(
                    '%s<layer type="%s" name="%s" checked="%d">\n' %
                    (' ' * self.indent, type, cmd, checked))
                self.file.write('%s</layer>\n' % (' ' * self.indent))
            elif type == 'group':
                name = mapTree.GetItemText(item)
                self.file.write(
                    '%s<group name="%s" checked="%d">\n' %
                    (' ' * self.indent, name, checked))
                self.indent += 4
                subItem = mapTree.GetFirstChild(item)[0]
                self.__writeLayer(mapTree, subItem)
                self.indent -= 4
                self.file.write('%s</group>\n' % (' ' * self.indent))
            else:
                cmd = mapTree.GetLayerInfo(
                    item, key='maplayer').GetCmd(
                    string=False)
                name = mapTree.GetItemText(item).replace(os.linesep, '\\n')
                opacity = maplayer.GetOpacity()
                # remove 'opacity' part
                if opacity < 1:
                    name = name.split('(', -1)[0].strip()
                self.file.write(
                    '%s<layer type="%s" name="%s" checked="%d" opacity="%f">\n' %
                    (' ' * self.indent, type, name, checked, opacity))

                self.indent += 4
                # selected ?
                if item in itemSelected:
                    self.file.write('%s<selected />\n' % (' ' * self.indent))
                # layer properties
                self.file.write(
                    '%s<task name="%s">\n' %
                    (' ' * self.indent, cmd[0]))
                self.indent += 4
                for key, val in six.iteritems(cmd[1]):
                    if key == 'flags':
                        for f in val:
                            self.file.write('%s<flag name="%s" />\n' %
                                            (' ' * self.indent, f))
                    elif val in (True, False):
                        self.file.write('%s<flag name="%s" />\n' %
                                        (' ' * self.indent, key))
                    else:  # parameter
                        self.file.write('%s<parameter name="%s">\n' %
                                        (' ' * self.indent, key))
                        self.indent += 4
                        self.file.write(
                            '%s<value>%s</value>\n' %
                            (' ' * self.indent, self.__filterValue(val)))
                        self.indent -= 4
                        self.file.write(
                            '%s</parameter>\n' %
                            (' ' * self.indent))
                self.indent -= 4
                self.file.write('%s</task>\n' % (' ' * self.indent))
                # vector digitizer
                vdigit = mapTree.GetLayerInfo(item, key='vdigit')
                if vdigit:
                    self.file.write('%s<vdigit>\n' % (' ' * self.indent))
                    if 'geomAttr' in vdigit:
                        self.indent += 4
                        for type, val in six.iteritems(vdigit['geomAttr']):
                            units = ''
                            if val['units'] != 'mu':
                                units = ' units="%s"' % val['units']
                            self.file.write(
                                '%s<geometryAttribute type="%s" column="%s"%s />\n' %
                                (' ' * self.indent, type, val['column'], units))
                        self.indent -= 4
                    self.file.write('%s</vdigit>\n' % (' ' * self.indent))
                # nviz
                nviz = mapTree.GetLayerInfo(item, key='nviz')
                if nviz:
                    self.file.write('%s<nviz>\n' % (' ' * self.indent))
                    if maplayer.type == 'raster':
                        self.__writeNvizSurface(nviz['surface'])
                    if maplayer.type == 'raster_3d':
                        self.__writeNvizVolume(nviz['volume'])
                    elif maplayer.type == 'vector':
                        self.__writeNvizVector(nviz['vector'])
                    self.file.write('%s</nviz>\n' % (' ' * self.indent))
                self.indent -= 4
                self.file.write('%s</layer>\n' % (' ' * self.indent))
            item = mapTree.GetNextSibling(item)
        self.indent -= 4

    def __writeNvizSurface(self, data):
        """Save Nviz raster layer properties to workspace

        :param data: Nviz layer properties
        """
        if 'object' not in data:  # skip disabled
            return
        self.indent += 4
        self.file.write('%s<surface>\n' % (' ' * self.indent))
        self.indent += 4
        for attrb in six.iterkeys(data):
            if len(data[attrb]) < 1:  # skip empty attributes
                continue
            if attrb == 'object':
                continue

            for name in six.iterkeys(data[attrb]):
                # surface attribute
                if attrb == 'attribute':
                    if data[attrb][name]['map'] is None:
                        continue
                    self.file.write(
                        '%s<%s name="%s" map="%d">\n' %
                        (' ' * self.indent, attrb, name, data[attrb][name]['map']))
                    self.indent += 4
                    self.file.write(
                        '%s<value>%s</value>\n' %
                        (' ' * self.indent, data[attrb][name]['value']))
                    self.indent -= 4
                    # end tag
                    self.file.write('%s</%s>\n' % (' ' * self.indent, attrb))

            # draw mode
            if attrb == 'draw':
                self.file.write('%s<%s' % (' ' * self.indent, attrb))
                if 'mode' in data[attrb]:
                    for tag, value in six.iteritems(data[attrb]['mode']['desc']):
                        self.file.write(' %s="%s"' % (tag, value))
                self.file.write('>\n')  # <draw ...>

                if 'resolution' in data[attrb]:
                    self.indent += 4
                    for type in ('coarse', 'fine'):
                        self.file.write(
                            '%s<resolution type="%s">\n' %
                            (' ' * self.indent, type))
                        self.indent += 4
                        self.file.write(
                            '%s<value>%d</value>\n' %
                            (' ' * self.indent, data[attrb]['resolution'][type]))
                        self.indent -= 4
                        self.file.write(
                            '%s</resolution>\n' %
                            (' ' * self.indent))

                if 'wire-color' in data[attrb]:
                    self.file.write('%s<wire_color>\n' % (' ' * self.indent))
                    self.indent += 4
                    self.file.write(
                        '%s<value>%s</value>\n' %
                        (' ' * self.indent, data[attrb]['wire-color']['value']))
                    self.indent -= 4
                    self.file.write('%s</wire_color>\n' % (' ' * self.indent))
                self.indent -= 4

            # position
            elif attrb == 'position':
                self.file.write('%s<%s>\n' % (' ' * self.indent, attrb))
                for tag in ('x', 'y', 'z'):
                    self.indent += 4
                    self.file.write(
                        '%s<%s>%d</%s>\n' %
                        (' ' * self.indent, tag, data[attrb][tag], tag))
                    self.indent -= 4

            if attrb != 'attribute':
                # end tag
                self.file.write('%s</%s>\n' % (' ' * self.indent, attrb))

        self.indent -= 4
        self.file.write('%s</surface>\n' % (' ' * self.indent))
        self.indent -= 4

    def __writeNvizVolume(self, data):
        """Save Nviz volume layer properties to workspace

        :param data: Nviz layer properties
        """
        if 'object' not in data:  # skip disabled
            return
        self.indent += 4
        self.file.write('%s<volume>\n' % (' ' * self.indent))
        self.indent += 4
        for attrb in six.iterkeys(data):
            if len(data[attrb]) < 1:  # skip empty attributes
                continue
            if attrb == 'object':
                continue

            if attrb == 'attribute':
                for name in six.iterkeys(data[attrb]):
                    # surface attribute
                    if data[attrb][name]['map'] is None:
                        continue
                    self.file.write(
                        '%s<%s name="%s" map="%d">\n' %
                        (' ' * self.indent, attrb, name, data[attrb][name]['map']))
                    self.indent += 4
                    self.file.write(
                        '%s<value>%s</value>\n' %
                        (' ' * self.indent, data[attrb][name]['value']))
                    self.indent -= 4
                    # end tag
                    self.file.write('%s</%s>\n' % (' ' * self.indent, attrb))

            # draw mode
            if attrb == 'draw':
                self.file.write('%s<%s>\n' % (' ' * self.indent, attrb))

                self.indent += 4
                for att in ('resolution', 'shading'):
                    if att in data[attrb]:
                        self.file.write('%s<%s>\n' % (' ' * self.indent, att))
                        for type in ('isosurface', 'slice'):
                            self.indent += 4
                            self.file.write(
                                '%s<%s>\n' %
                                (' ' * self.indent, type))
                            self.indent += 4
                            self.file.write(
                                '%s<value>%d</value>\n' %
                                (' ' * self.indent, data[attrb][att][type]['value']))
                            if att == 'shading':
                                self.file.write(
                                    '%s<desc>%s</desc>\n' %
                                    (' ' * self.indent, data[attrb][att][type]['desc']))
                            self.indent -= 4
                            self.file.write(
                                '%s</%s>\n' %
                                (' ' * self.indent, type))
                            self.indent -= 4
                        self.file.write('%s</%s>\n' % (' ' * self.indent, att))

                if 'box' in data[attrb]:
                    self.file.write('%s<box>\n' % (' ' * self.indent))
                    self.indent += 4
                    self.file.write(
                        '%s<enabled>%d</enabled>\n' %
                        (' ' * self.indent, data[attrb]['box']['enabled']))
                    self.indent -= 4
                    self.file.write('%s</box>\n' % (' ' * self.indent))

                if 'mode' in data[attrb]:
                    self.file.write('%s<mode>\n' % (' ' * self.indent))
                    self.indent += 4
                    self.file.write(
                        '%s<desc>%s</desc>\n' %
                        (' ' * self.indent, data[attrb]['mode']['desc']))
                    self.file.write(
                        '%s<value>%d</value>\n' %
                        (' ' * self.indent, data[attrb]['mode']['value']))
                    self.indent -= 4
                    self.file.write('%s</mode>\n' % (' ' * self.indent))
                self.indent -= 4

            # position
            elif attrb == 'position':
                self.file.write('%s<%s>\n' % (' ' * self.indent, attrb))
                for tag in ('x', 'y', 'z'):
                    self.indent += 4
                    self.file.write(
                        '%s<%s>%d</%s>\n' %
                        (' ' *
                         self.indent,
                         tag,
                         data[attrb].get(
                             tag,
                             0),
                            tag))
                    self.indent -= 4
            if attrb == 'isosurface':
                for isosurface in data[attrb]:
                    self.file.write('%s<%s>\n' % (' ' * self.indent, attrb))
                    for name in six.iterkeys(isosurface):
                        self.indent += 4
                        self.file.write('%s<%s>\n' % (' ' * self.indent, name))
                        for att in six.iterkeys(isosurface[name]):
                            if isosurface[name][att] is True:
                                val = '1'
                            elif isosurface[name][att] is False:
                                val = '0'
                            else:
                                try:
                                    val = '%f' % float(isosurface[name][att])
                                except ValueError:
                                    val = '%s' % isosurface[name][att]
                                except TypeError:  # None
                                    val = ''
                            self.indent += 4
                            self.file.write(
                                ('%s<%s>' %
                                 (' ' * self.indent, att)) + val)
                            self.file.write('</%s>\n' % att)
                            self.indent -= 4
                        # end tag
                        self.file.write(
                            '%s</%s>\n' %
                            (' ' * self.indent, name))
                        self.indent -= 4
                    self.file.write('%s</%s>\n' % (' ' * self.indent, attrb))

            if attrb == 'slice':
                for slice_ in data[attrb]:
                    self.file.write('%s<%s>\n' % (' ' * self.indent, attrb))
                    for name in six.iterkeys(slice_):
                        self.indent += 4
                        self.file.write('%s<%s>\n' % (' ' * self.indent, name))
                        for att in six.iterkeys(slice_[name]):
                            if att in ('map', 'update'):
                                continue
                            val = slice_[name][att]
                            self.indent += 4
                            self.file.write(
                                ('%s<%s>' %
                                 (' ' * self.indent, att)) + str(val))
                            self.file.write('</%s>\n' % att)
                            self.indent -= 4
                        # end tag
                        self.file.write(
                            '%s</%s>\n' %
                            (' ' * self.indent, name))
                        self.indent -= 4
                    self.file.write('%s</%s>\n' % (' ' * self.indent, attrb))
            if attrb not in ('attribute', 'isosurface', 'slice'):
                # end tag
                self.file.write('%s</%s>\n' % (' ' * self.indent, attrb))

        self.indent -= 4
        self.file.write('%s</volume>\n' % (' ' * self.indent))
        self.indent -= 4

    def __writeNvizVector(self, data):
        """Save Nviz vector layer properties (lines/points) to workspace

        :param data: Nviz layer properties
        """
        self.indent += 4
        for attrb in six.iterkeys(data):
            if len(data[attrb]) < 1:  # skip empty attributes
                continue

            if 'object' not in data[attrb]:  # skip disabled
                continue
            if attrb == 'lines':
                self.file.write('%s<v%s>\n' % (' ' * self.indent, attrb))
            elif attrb == 'points':
                markerId = data[attrb]['marker']['value']
                marker = UserSettings.Get(
                    group='nviz', key='vector', subkey=[
                        'points', 'marker'], settings_type='internal')[markerId]
                self.file.write('%s<v%s marker="%s">\n' % (' ' * self.indent,
                                                           attrb,
                                                           marker))
            self.indent += 4
            for name in six.iterkeys(data[attrb]):
                if name in ('object', 'marker'):
                    continue
                if name == 'mode':
                    self.file.write(
                        '%s<%s type="%s">\n' %
                        (' ' * self.indent, name, data[attrb][name]['type']))
                    if data[attrb][name]['type'] == 'surface':
                        self.indent += 4
                        for idx, surface in enumerate(
                                data[attrb][name]['surface']['value']):
                            checked = data[attrb][name]['surface']['show'][idx]
                            self.file.write('%s<map>\n' % (' ' * self.indent))
                            self.indent += 4
                            self.file.write(
                                '%s<name>%s</name>\n' %
                                (' ' * self.indent, surface))
                            self.file.write(
                                '%s<checked>%s</checked>\n' %
                                (' ' * self.indent, int(checked)))
                            self.indent -= 4
                            self.file.write('%s</map>\n' % (' ' * self.indent))
                        self.indent -= 4
                    self.file.write('%s</%s>\n' % ((' ' * self.indent, name)))
                elif name == 'thematic':
                    self.file.write('%s<%s ' % (' ' * self.indent, name))
                    for key in six.iterkeys(data[attrb][name]):
                        if key.startswith('use'):
                            self.file.write(
                                '%s="%s" ' %
                                (key, int(data[attrb][name][key])))
                    self.file.write('>\n')
                    self.indent += 4
                    for key, value in six.iteritems(data[attrb][name]):
                        if key.startswith('use'):
                            continue
                        if value is None:
                            value = ''
                        self.file.write(
                            '%s<%s>%s</%s>\n' %
                            (' ' * self.indent, key, value, key))
                    self.indent -= 4
                    self.file.write('%s</%s>\n' % (' ' * self.indent, name))
                else:
                    self.file.write('%s<%s>\n' % (' ' * self.indent, name))
                    self.indent += 4
                    self.file.write(
                        '%s<value>%s</value>\n' %
                        (' ' * self.indent, data[attrb][name]['value']))
                    self.indent -= 4
                    self.file.write('%s</%s>\n' % (' ' * self.indent, name))
            self.indent -= 4
            self.file.write('%s</v%s>\n' % (' ' * self.indent, attrb))

        self.indent -= 4

    def __writeNvizState(self, view, iview, light, constants):
        """"Save Nviz properties (view, light) to workspace

        :param view: Nviz view properties
        :param iview: Nviz internal view properties
        :param light: Nviz light properties
        """
        self.indent += 4
        self.file.write('%s<nviz_state>\n' % (' ' * self.indent))
        #
        # view
        #
        self.indent += 4
        self.file.write('%s<view>\n' % (' ' * self.indent))
        self.indent += 4
        # position
        self.file.write('%s<v_position>\n' % (' ' * self.indent))
        self.indent += 4
        self.file.write(
            '%s<x>%.2f</x>\n' %
            (' ' * self.indent, view['position']['x']))
        self.file.write(
            '%s<y>%.2f</y>\n' %
            (' ' * self.indent, view['position']['y']))
        self.indent -= 4
        self.file.write('%s</v_position>\n' % (' ' * self.indent))
        # perspective
        self.file.write('%s<persp>\n' % (' ' * self.indent))
        self.indent += 4
        self.file.write('%s<value>%d</value>\n' %
                        (' ' * self.indent, view['persp']['value']))
        self.file.write('%s<step>%d</step>\n' %
                        (' ' * self.indent, view['persp']['step']))
        self.file.write('%s<min>%d</min>\n' %
                        (' ' * self.indent, iview['persp']['min']))
        self.file.write('%s<max>%d</max>\n' %
                        (' ' * self.indent, iview['persp']['max']))
        self.indent -= 4
        self.file.write('%s</persp>\n' % (' ' * self.indent))
        # height
        self.file.write('%s<v_height>\n' % (' ' * self.indent))
        self.indent += 4
        self.file.write('%s<value>%d</value>\n' %
                        (' ' * self.indent, iview['height']['value']))
        self.file.write('%s<min>%d</min>\n' %
                        (' ' * self.indent, iview['height']['min']))
        self.file.write('%s<max>%d</max>\n' %
                        (' ' * self.indent, iview['height']['max']))
        self.indent -= 4
        self.file.write('%s</v_height>\n' % (' ' * self.indent))
        # twist
        self.file.write('%s<twist>\n' % (' ' * self.indent))
        self.indent += 4
        self.file.write('%s<value>%d</value>\n' %
                        (' ' * self.indent, view['twist']['value']))
        self.file.write('%s<min>%d</min>\n' %
                        (' ' * self.indent, iview['twist']['min']))
        self.file.write('%s<max>%d</max>\n' %
                        (' ' * self.indent, iview['twist']['max']))
        self.indent -= 4
        self.file.write('%s</twist>\n' % (' ' * self.indent))
        # z-exag
        self.file.write('%s<z-exag>\n' % (' ' * self.indent))
        self.indent += 4
        self.file.write('%s<value>%.2f</value>\n' %
                        (' ' * self.indent, view['z-exag']['value']))
        self.file.write('%s<min>%d</min>\n' %
                        (' ' * self.indent, view['z-exag']['min']))
        self.file.write('%s<max>%d</max>\n' %
                        (' ' * self.indent, view['z-exag']['max']))
        self.file.write('%s<llRatio>%.2f</llRatio>\n' %
                        (' ' * self.indent, iview['z-exag']['llRatio']))
        self.indent -= 4
        self.file.write('%s</z-exag>\n' % (' ' * self.indent))
        # focus (look here)
        self.file.write('%s<focus>\n' % (' ' * self.indent))
        self.indent += 4
        self.file.write(
            '%s<x>%d</x>\n' %
            (' ' * self.indent, iview['focus']['x']))
        self.file.write(
            '%s<y>%d</y>\n' %
            (' ' * self.indent, iview['focus']['y']))
        self.file.write(
            '%s<z>%d</z>\n' %
            (' ' * self.indent, iview['focus']['z']))
        self.indent -= 4
        self.file.write('%s</focus>\n' % (' ' * self.indent))
        # rotation
        rotation = ','.join(
            [str(i) for i in iview['rotation']]) if iview.get(
            'rotation', '') else ''
        self.file.write(
            '%s<rotation>%s</rotation>\n' %
            (' ' * self.indent, rotation))

        # background
        self.__writeTagWithValue(
            'background_color',
            view['background']['color'][
                :3],
            format='d:%d:%d')

        self.indent -= 4
        self.file.write('%s</view>\n' % (' ' * self.indent))
        #
        # light
        #
        self.file.write('%s<light>\n' % (' ' * self.indent))
        self.indent += 4
        # position
        self.file.write('%s<l_position>\n' % (' ' * self.indent))
        self.indent += 4
        self.file.write(
            '%s<x>%.2f</x>\n' %
            (' ' * self.indent, light['position']['x']))
        self.file.write(
            '%s<y>%.2f</y>\n' %
            (' ' * self.indent, light['position']['y']))
        self.file.write(
            '%s<z>%d</z>\n' %
            (' ' * self.indent, light['position']['z']))
        self.indent -= 4
        self.file.write('%s</l_position>\n' % (' ' * self.indent))
        # bright
        self.__writeTagWithValue('bright', light['bright'])
        # ambient
        self.__writeTagWithValue('ambient', light['ambient'])
        # color
        self.__writeTagWithValue('color', light['color'][:3], format='d:%d:%d')

        self.indent -= 4
        self.file.write('%s</light>\n' % (' ' * self.indent))
        #
        # constant planes
        #
        if constants:
            self.file.write('%s<constant_planes>\n' % (' ' * self.indent))
            self.indent += 4
            for idx, plane in enumerate(constants):
                self.file.write('%s<plane>\n' % (' ' * self.indent))
                self.indent += 4
                self.__writeTagWithValue(
                    'height', constants[idx]['constant']['value'])
                self.__writeTagWithValue(
                    'fine_resolution',
                    constants[idx]['constant']['resolution'])
                self.__writeTagWithValue(
                    'color', constants[idx]['constant']['color'], format='s')
                self.indent -= 4
                self.file.write('%s</plane>\n' % (' ' * self.indent))
            self.indent -= 4
            self.file.write('%s</constant_planes>\n' % (' ' * self.indent))
        self.indent -= 4

        self.file.write('%s</nviz_state>\n' % (' ' * self.indent))
        self.indent -= 4

    def __writeTagWithValue(self, tag, data, format='d'):
        """Helper function for writing pair tag

        :param tag: written tag
        :param data: written data
        :param format: conversion type
        """
        self.file.write('%s<%s>\n' % (' ' * self.indent, tag))
        self.indent += 4
        self.file.write('%s' % (' ' * self.indent))
        self.file.write(('<value>%' + format + '</value>\n') % data)
        self.indent -= 4
        self.file.write('%s</%s>\n' % (' ' * self.indent, tag))

    def __writeOverlay(self, mapdisp):
        """Function for writing map elements (barscale, northarrow etc.)
        """
        disp_size = mapdisp.GetMapWindow().GetClientSize()
        for overlay in mapdisp.decorations.values():
            self.__writeOverlayParams(disp_size, overlay.cmd, overlay.coords)

    def __writeOverlayParams(self, disp_size, cmd, coord_px):
        """
        :param mapdisp: mapdisplay
        :param cmd: d.* command with flags and parameters
        """
        # Canvas width = display width minus 1 px on both sides
        cnvs_w = float(disp_size[0])
        # Canvas height = display height minus 1 px on bottom and height of toolbar
        cnvs_h = float(disp_size[1])

        x_prcn = round(coord_px[0] / cnvs_w * 100, 1)
        y_prcn = 100 - round(coord_px[1] / cnvs_h * 100, 1)
        self.indent += 4
        self.file.write('%s<overlay name="%s">\n' % (' ' * self.indent, cmd[0]))
        self.indent += 4
        for prm in cmd[1:]:
            if prm.startswith('-'):
                flags = []
                if prm.startswith('--'):
                    flags.append(prm[2:])
                else:
                    flags = list(prm[1:])
                for f in flags:
                    self.file.write('%s<flag name="%s" />\n' % (' ' * self.indent, f))

            elif prm.startswith("at="):
                # legend "at" argument takes 4 numbers not 2
                if cmd[0] == "d.legend":
                    leg_coord_prcn = prm.split("=", 1)[1].split(",")
                    leg_w = float(leg_coord_prcn[3]) - float(leg_coord_prcn[2])
                    leg_h = float(leg_coord_prcn[1]) - float(leg_coord_prcn[0])
                    self.file.write('%s<parameter name="at">\n' % (' ' * self.indent))
                    self.indent += 4
                    self.file.write('%s<value>%.1f,%.1f,%.1f,%.1f</value>\n' % (' ' * self.indent,
                                    y_prcn - leg_h, y_prcn, x_prcn, x_prcn + leg_w))
                    self.indent -= 4
                    self.file.write('%s</parameter>\n' % (' ' * self.indent))
                else:
                    self.file.write('%s<parameter name="at">\n' % (' ' * self.indent))
                    self.indent += 4
                    self.file.write('%s<value>%.1f,%.1f</value>\n' % (' ' * self.indent, x_prcn, y_prcn))
                    self.indent -= 4
                    self.file.write('%s</parameter>\n' % (' ' * self.indent))
            else:
                self.file.write('%s<parameter name="%s">\n' % (' ' * self.indent, prm.split("=", 1)[0]))
                self.indent += 4
                self.file.write('%s<value>%s</value>\n' % (' ' * self.indent, prm.split("=", 1)[1]))
                self.indent -= 4
                self.file.write('%s</parameter>\n' % (' ' * self.indent))
        self.indent -= 4
        self.file.write('%s</overlay>\n' % (' ' * self.indent))
        self.indent -= 4


class ProcessGrcFile(object):

    def __init__(self, filename):
        """Process GRC file"""
        self.filename = filename

        # elements
        self.inGroup = False
        self.inRaster = False
        self.inVector = False

        # list of layers
        self.layers = []

        # error message
        self.error = ''
        self.num_error = 0

    def read(self, parent):
        """Read GRC file

        :param parent: parent window

        :return: list of map layers
        """
        try:
            file = open(self.filename, "r")
        except IOError:
            wx.MessageBox(
                parent=parent,
                message=_("Unable to open file <%s> for reading.") %
                self.filename,
                caption=_("Error"),
                style=wx.OK | wx.ICON_ERROR)
            return []

        line_id = 1
        for line in file.readlines():
            self.process_line(line.rstrip('\n'), line_id)
            line_id += 1

        file.close()

        if self.num_error > 0:
            wx.MessageBox(
                parent=parent,
                message=_(
                    "Some lines were skipped when reading settings "
                    "from file <%(file)s>.\nSee 'Command output' window for details.\n\n"
                    "Number of skipped lines: %(line)d") %
                {'file': self.filename, 'line': self.num_error},
                caption=_("Warning"),
                style=wx.OK | wx.ICON_EXCLAMATION)
            parent._gconsole.WriteLog(
                'Map layers loaded from GRC file <%s>' %
                self.filename)
            parent._gconsole.WriteLog('Skipped lines:\n%s' % self.error)

        return self.layers

    def process_line(self, line, line_id):
        """Process line definition"""
        element = self._get_element(line)
        if element == 'Group':
            self.groupName = self._get_value(line)
            self.layers.append({
                "type": 'group',
                "name": self.groupName,
                "checked": None,
                "opacity": None,
                "cmd": None,
                "group": self.inGroup,
                "display": 0})
            self.inGroup = True

        elif element == '_check':
            if int(self._get_value(line)) == 1:
                self.layers[-1]['checked'] = True
            else:
                self.layers[-1]['checked'] = False

        elif element == 'End':
            if self.inRaster:
                self.inRaster = False
            elif self.inVector:
                self.inVector = False
            elif self.inGroup:
                self.inGroup = False
            elif self.inGridline:
                self.inGridline = False

        elif element == 'opacity':
            self.layers[-1]['opacity'] = float(self._get_value(line))

        # raster
        elif element == 'Raster':
            self.inRaster = True
            self.layers.append({
                "type": 'raster',
                "name": self._get_value(line),
                "checked": None,
                "opacity": None,
                "cmd": ['d.rast'],
                "group": self.inGroup,
                "display": 0})

        elif element == 'map' and self.inRaster:
            self.layers[-1]['cmd'].append('map=%s' % self._get_value(line))

        elif element == 'overlay' and self.inRaster:
            if int(self._get_value(line)) == 1:
                self.layers[-1]['cmd'].append('-o')

        elif element == 'rastquery' and self.inRaster:
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('catlist=%s' % value)

        elif element == 'bkcolor' and self.inRaster:
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('bg=%s' % value)

        # vector
        elif element == 'Vector':
            self.inVector = True
            self.layers.append({
                "type": 'vector',
                "name": self._get_value(line),
                "checked": None,
                "opacity": None,
                "cmd": ['d.vect'],
                "group": self.inGroup,
                "display": 0})

        elif element == 'vector' and self.inVector:
            self.layers[-1]['cmd'].append('map=%s' % self._get_value(line))

        elif element in ('display_shape',
                         'display_cat',
                         'display_topo',
                         'display_dir',
                         'display_attr',
                         'type_point',
                         'type_line',
                         'type_boundary',
                         'type_centroid',
                         'type_area',
                         'type_face') and self.inVector:

            if int(self._get_value(line)) == 1:
                name = element.split('_')[0]
                type = element.split('_')[1]
                paramId = self._get_cmd_param_index(
                    self.layers[-1]['cmd'], name)
                if paramId == -1:
                    self.layers[-1]['cmd'].append('%s=%s' % (name, type))
                else:
                    self.layers[-1]['cmd'][paramId] += ',%s' % type

        elif element in ('color',
                         'fcolor',
                         'lcolor') and self.inVector:
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('%s=%s' %
                                              (element, self._color_name_to_rgb(value)))

        elif element == 'rdmcolor' and self.inVector:
            if int(self._get_value(line)) == 1:
                self.layers[-1]['cmd'].append('-c')

        elif element == 'sqlcolor' and self.inVector:
            if int(self._get_value(line)) == 1:
                self.layers[-1]['cmd'].append('-a')

        elif element in ('icon',
                         'size',
                         'layer',
                         'xref',
                         'yref',
                         'lsize',
                         'where',
                         'minreg',
                         'maxreg') and self.inVector:
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('%s=%s' % (element,
                                                         value))

        elif element == 'lwidth':
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('width=%s' % value)

        elif element == 'lfield':
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('llayer=%s' % value)

        elif element == 'attribute':
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('attrcol=%s' % value)

        elif element == 'cat':
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('cats=%s' % value)

        # gridline
        elif element == 'gridline':
            self.inGridline = True
            self.layers.append({
                "type": 'grid',
                "name": self._get_value(line),
                "checked": None,
                "opacity": None,
                "cmd": ['d.grid'],
                "group": self.inGroup,
                "display": 0})

        elif element == 'gridcolor':
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('color=%s' %
                                              self._color_name_to_rgb(value))

        elif element == 'gridborder':
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('bordercolor=%s' %
                                              self._color_name_to_rgb(value))

        elif element == 'textcolor':
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('textcolor=%s' %
                                              self._color_name_to_rgb(value))

        elif element in ('gridsize',
                         'gridorigin'):
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('%s=%s' % (element[4:], value))

        elif element in 'fontsize':
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('%s=%s' % (element, value))

        elif element == 'griddraw':
            value = self._get_value(line)
            if value == '0':
                self.layers[-1]['cmd'].append('-n')

        elif element == 'gridgeo':
            value = self._get_value(line)
            if value == '1':
                self.layers[-1]['cmd'].append('-g')

        elif element == 'borderdraw':
            value = self._get_value(line)
            if value == '0':
                self.layers[-1]['cmd'].append('-b')

        elif element == 'textdraw':
            value = self._get_value(line)
            if value == '0':
                self.layers[-1]['cmd'].append('-t')

        else:
            self.error += _(' row %d:') % line_id + line + os.linesep
            self.num_error += 1

    def _get_value(self, line):
        """Get value of element"""
        try:
            return line.strip(' ').split(' ')[1].strip(' ')
        except:
            return ''

    def _get_element(self, line):
        """Get element tag"""
        return line.strip(' ').split(' ')[0].strip(' ')

    def _get_cmd_param_index(self, cmd, name):
        """Get index of parameter in cmd list

        :param cmd: cmd list
        :param name: parameter name

        :return: index
        :return: -1 if not found
        """
        i = 0
        for param in cmd:
            if '=' not in param:
                i += 1
                continue
            if param.split('=')[0] == name:
                return i

            i += 1

        return -1

    def _color_name_to_rgb(self, value):
        """Convert color name (#) to rgb values"""
        col = wx.NamedColour(value)
        return str(col.Red()) + ':' + \
            str(col.Green()) + ':' + \
            str(col.Blue())
