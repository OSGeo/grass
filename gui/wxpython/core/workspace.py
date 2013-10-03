"""!
@package core.workspace

@brief Open/save workspace definition file

Classes:
 - workspace::ProcessWorkspaceFile
 - workspace::WriteWorkspaceFile
 - workspace::ProcessGrcFile

(C) 2007-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os

import wx

from core.utils     import normalize_whitespace, _
from core.settings  import UserSettings
from core.gcmd      import EncodeString, GetDefaultEncoding
from nviz.main      import NvizSettings

class ProcessWorkspaceFile:
    def __init__(self, tree):
        """!A ElementTree handler for the GXW XML file, as defined in
        grass-gxw.dtd.
        """
        self.tree = tree
        self.root = self.tree.getroot()
        
        #
        # layer manager properties
        #
        self.layerManager = { 'pos' : None,  # window position
                              'size' : None, # window size
                              'cwd' : None } # current working directory
        
        #
        # list of mapdisplays
        #
        self.displays = []
        #
        # list of map layers
        #
        self.layers = []
        #
        # nviz state
        #
        self.nviz_state = {}
        
        self.displayIndex = -1 # first display has index '0'
        
        self.__processFile()

        if NvizSettings:
            self.nvizDefault = NvizSettings()
        else:
            self.nvizDefault = None
        
    def __filterValue(self, value):
        """!Filter value
        
        @param value
        """
        value = value.replace('&lt;', '<')
        value = value.replace('&gt;', '>')
        
        return value

    def __getNodeText(self, node, tag, default = ''):
        """!Get node text"""
        p = node.find(tag)
        if p is not None:
            # if empty text is inside tag,
            # etree returns None
            if p.text is None:
                return ''

            return normalize_whitespace(p.text)
        
        return default
    
    def __processFile(self):
        """!Process workspace file"""
        #
        # layer manager
        #
        node_lm = self.root.find('layer_manager')
        if node_lm is not None:
            posAttr = node_lm.get('dim', '')
            if posAttr:
                posVal = map(int, posAttr.split(','))
                try:
                    self.layerManager['pos']  = (posVal[0], posVal[1])
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
                posVal = map(int, posAttr.split(','))
                try:
                    pos  = (posVal[0], posVal[1])
                    size = (posVal[2], posVal[3])
                except:
                    pos  = None
                    size = None
            else:
                pos  = None
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
                projection = { 'enabled' : True,
                               'epsg' : node_projection.get('epsg', ''),
                               'proj' : self.__getNodeText(node_projection, 'value') }
            else:
                projection = { 'enabled' : False }
            
            self.displays.append( {
                    "name"           : display.get('name'),
                    "render"         : bool(int(display.get('render', "0"))),
                    "mode"           : int(display.get('mode', 0)),
                    "showCompExtent" : bool(int(display.get('showCompExtent', "0"))),
                    "pos"            : pos,
                    "size"           : size,
                    "extent"         : extent,
                    "alignExtent"    : bool(int(display.get('alignExtent', "0"))),
                    "constrainRes"   : bool(int(display.get('constrainRes', "0"))),
                    "projection"     : projection,
                    "viewMode"       : display.get('viewMode', '2d')} )
            
            # process all layers/groups in the display
            self.__processLayers(display)
            # process nviz_state
            self.__processNvizState(display)

    def __processLayers(self, node, inGroup = -1):
        """!Process layers/groups of selected display
        
        @param node display tree node
        @param inGroup in group -> index of group item otherwise -1
        """
        for item in node.getchildren():
            if item.tag == 'group':
                # -> group
                self.layers.append( {
                        "type"    : 'group',
                        "name"    : item.get('name', ''),
                        "checked" : bool(int(item.get('checked', "0"))),
                        "opacity" : None,
                        "cmd"     : None,
                        "group"   : inGroup,
                        "display" : self.displayIndex,
                        "vdigit"  : None,
                        "nviz"    : None})
                
                self.__processLayers(item, inGroup = len(self.layers) - 1) # process items in group
                
            elif item.tag == 'layer':
                cmd, selected, vdigit, nviz = self.__processLayer(item)
                lname = item.get('name', None)
                if lname and '\\n' in lname:
                    lname = lname.replace('\\n', os.linesep)
                
                self.layers.append( {
                        "type"     : item.get('type', None),
                        "name"     : lname,
                        "checked"  : bool(int(item.get('checked', "0"))),
                        "opacity"  : float(item.get('opacity', '1.0')),
                        "cmd"      : cmd,
                        "group"    : inGroup,
                        "display"  : self.displayIndex,
                        "selected" : selected,
                        "vdigit"   : vdigit,
                        "nviz"     : nviz } )
            
    def __processLayer(self, layer):
        """!Process layer item

        @param layer tree node
        """
        cmd = list()
        
        #
        # layer attributes (task) - 2D settings
        #
        node_task = layer.find('task')
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
                                  self.__filterValue(self.__getNodeText(p, 'value'))))
        
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

    def __processLayerVdigit(self, node_vdigit):
        """!Process vector digitizer layer settings

        @param node_vdigit vdigit node
        """
        # init nviz layer properties
        vdigit = dict()
        for node in node_vdigit.findall('geometryAttribute'):
            if 'geomAttr' not in vdigit:
                vdigit['geomAttr'] = dict()
            type = node.get('type')
            vdigit['geomAttr'][type] = dict()
            vdigit['geomAttr'][type]['column'] = node.get('column') # required
            # default map units
            vdigit['geomAttr'][type]['units'] = node.get('units', 'mu')
        
        return vdigit
    
    def __processLayerNviz(self, node_nviz):
        """!Process 3D layer settings

        @param node_nviz nviz node
        """
        # init nviz layer properties
        nviz = {}
        if node_nviz.find('surface') is not None: # -> raster
            nviz['surface'] = {}
            for sec in ('attribute', 'draw', 'position'):
                nviz['surface'][sec] = {}
        elif node_nviz.find('vlines') is not None or \
                node_nviz.find('vpoints') is not None: # -> vector
            nviz['vector'] = {}
            for sec in ('lines', 'points'):
                nviz['vector'][sec] = {}
        
        if 'surface' in nviz:
            node_surface = node_nviz.find('surface')
            # attributes
            for attrb in node_surface.findall('attribute'):
                tagName = str(attrb.tag)
                attrbName = attrb.get('name', '')
                dc = nviz['surface'][tagName][attrbName] = {}
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
            node_draw = node_surface.find('draw')
            if node_draw is not None:
                tagName = str(node_draw.tag)
                nviz['surface'][tagName]['all'] = False
                nviz['surface'][tagName]['mode'] = {}
                nviz['surface'][tagName]['mode']['value'] = -1 # to be calculated
                nviz['surface'][tagName]['mode']['desc'] = {}
                nviz['surface'][tagName]['mode']['desc']['shading'] = \
                    str(node_draw.get('shading', ''))
                nviz['surface'][tagName]['mode']['desc']['style'] = \
                    str(node_draw.get('style', ''))
                nviz['surface'][tagName]['mode']['desc']['mode'] = \
                    str(node_draw.get('mode', ''))
                
                # resolution
                for node_res in node_draw.findall('resolution'):
                    resType = str(node_res.get('type', ''))
                    if 'resolution' not in nviz['surface']['draw']:
                        nviz['surface']['draw']['resolution'] = {}
                    value = int(self.__getNodeText(node_res, 'value'))
                    nviz['surface']['draw']['resolution'][resType] = value
                
                # wire-color
                node_wire_color = node_draw.find('wire_color')
                if node_wire_color is not None:
                    nviz['surface']['draw']['wire-color'] = {}
                    value = str(self.__getNodeText(node_wire_color, 'value'))
                    nviz['surface']['draw']['wire-color']['value'] = value
                
            # position
            node_pos = node_surface.find('position')
            if node_pos is not None:
                dc = nviz['surface']['position'] = {}
                for coor in ['x', 'y', 'z']:
                    node = node_pos.find(coor)
                    if node is None:
                        continue
                    value = int(self.__getNodeText(node_pos, coor))
                    dc[coor] = value
            
        elif 'vector' in nviz:
            # vpoints
            node_vpoints = node_nviz.find('vpoints')
            if node_vpoints is not None:
                marker = str(node_vpoints.get('marker', ''))
                markerId = list(UserSettings.Get(group='nviz', key='vector',
                                                 subkey=['points', 'marker'], internal=True)).index(marker)
                nviz['vector']['points']['marker'] = { 'value' : markerId }
                
                node_mode = node_vpoints.find('mode')
                if node_mode is not None:
                    nviz['vector']['points']['mode'] = {}
                    nviz['vector']['points']['mode']['type'] = str(node_mode.get('type', 'surface'))
                    nviz['vector']['points']['mode']['surface'] = {}
                    nviz['vector']['points']['mode']['surface']['value'] = []
                    nviz['vector']['points']['mode']['surface']['show'] = []
                    
                    # map
                    for node_map in node_mode.findall('map'):
                        nviz['vector']['points']['mode']['surface']['value'].append(
                            self.__processLayerNvizNode(node_map, 'name', str))
                        nviz['vector']['points']['mode']['surface']['show'].append(bool(
                            self.__processLayerNvizNode(node_map, 'checked', int)))
                
                # color
                self.__processLayerNvizNode(node_vpoints, 'color', str,
                                            nviz['vector']['points'])
                
                # width
                self.__processLayerNvizNode(node_vpoints, 'width', int,
                                            nviz['vector']['points'])
                
                # height
                self.__processLayerNvizNode(node_vpoints, 'height', float,
                                            nviz['vector']['points'])
                
                # height
                self.__processLayerNvizNode(node_vpoints, 'size', float,
                                            nviz['vector']['points'])
                
                # thematic
                node_thematic = node_vpoints.find('thematic')
                thematic = nviz['vector']['points']['thematic'] = {}
                thematic['rgbcolumn'] = self.__processLayerNvizNode(node_thematic, 'rgbcolumn', str)
                thematic['sizecolumn'] = self.__processLayerNvizNode(node_thematic, 'sizecolumn', str)
                for col in ('rgbcolumn', 'sizecolumn'):
                    if thematic[col] == 'None':
                        thematic[col] = None
                thematic['layer'] = self.__processLayerNvizNode(node_thematic, 'layer', int)
                for use in ('usecolor', 'usesize', 'usewidth'):
                    if node_thematic.get(use, ''):
                        thematic[use] = int(node_thematic.get(use, '0'))
                
            # vlines
            node_vlines = node_nviz.find('vlines')
            if node_vlines is not None:
                node_mode = node_vlines.find('mode')
                if node_mode is not None:
                    nviz['vector']['lines']['mode'] = {}
                    nviz['vector']['lines']['mode']['type'] = str(node_mode.get('type', ''))
                    nviz['vector']['lines']['mode']['surface'] = {}
                    nviz['vector']['lines']['mode']['surface']['value'] = []
                    nviz['vector']['lines']['mode']['surface']['show'] = []
                    
                    # map
                    for node_map in node_mode.findall('map'):
                        nviz['vector']['lines']['mode']['surface']['value'].append(
                            self.__processLayerNvizNode(node_map, 'name', str))
                        nviz['vector']['lines']['mode']['surface']['show'].append(bool(
                            self.__processLayerNvizNode(node_map, 'checked', int)))
                
                # color
                self.__processLayerNvizNode(node_vlines, 'color', str,
                                            nviz['vector']['lines'])
                
                # width
                self.__processLayerNvizNode(node_vlines, 'width', int,
                                            nviz['vector']['lines'])
                
                # height
                self.__processLayerNvizNode(node_vlines, 'height', int,
                                            nviz['vector']['lines'])
                
                # thematic
                node_thematic = node_vlines.find('thematic')
                thematic = nviz['vector']['lines']['thematic'] = {}
                thematic['rgbcolumn'] = self.__processLayerNvizNode(node_thematic, 'rgbcolumn', str)
                thematic['sizecolumn'] = self.__processLayerNvizNode(node_thematic, 'sizecolumn', str)
                for col in ('rgbcolumn', 'sizecolumn'):
                    if thematic[col] == 'None':
                        thematic[col] = None
                thematic['layer'] = self.__processLayerNvizNode(node_thematic, 'layer', int)
                for use in ('usecolor', 'usesize', 'usewidth'):
                    if node_thematic.get(use, ''):
                        thematic[use] = int(node_thematic.get(use, '0'))
            
        return nviz
    
    def __processLayerNvizNode(self, node, tag, cast, dc = None):
        """!Process given tag nviz/vector"""
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
        """!Process tag nviz_state"""
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
        view['position']['x'] = self.__processLayerNvizNode(node_position, 'x', float)
        view['position']['y'] = self.__processLayerNvizNode(node_position, 'y', float)
        node_persp = node_view.find('persp')
        view['persp'] = {}
        iview['persp'] = {}
        view['persp']['value'] = self.__processLayerNvizNode(node_persp, 'value', int)
        view['persp']['step'] = self.__processLayerNvizNode(node_persp, 'step', int)
        iview['persp']['min'] = self.__processLayerNvizNode(node_persp, 'min', int)
        iview['persp']['max'] = self.__processLayerNvizNode(node_persp, 'max', int)
        node_height = node_view.find('v_height')
        iview['height'] = {}
        iview['height']['value'] = self.__processLayerNvizNode(node_height, 'value', int)
        iview['height']['min'] = self.__processLayerNvizNode(node_height, 'min', int)
        iview['height']['max'] = self.__processLayerNvizNode(node_height, 'max', int)
        node_twist = node_view.find('twist')
        view['twist'] = {}
        iview['twist'] = {}
        view['twist']['value'] = self.__processLayerNvizNode(node_twist, 'value', int)
        iview['twist']['min'] = self.__processLayerNvizNode(node_twist, 'min', int)
        iview['twist']['max'] = self.__processLayerNvizNode(node_twist, 'max', int)
        node_zexag = node_view.find('z-exag')
        view['z-exag'] = {}
        iview['z-exag'] = {}
        view['z-exag']['value'] = self.__processLayerNvizNode(node_zexag, 'value', float)
        view['z-exag']['min'] = self.__processLayerNvizNode(node_zexag, 'min', int)
        view['z-exag']['max'] = self.__processLayerNvizNode(node_zexag, 'max', int)
        iview['z-exag']['llRatio'] = self.__processLayerNvizNode(node_zexag, 'llRatio', float)
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
        light['position']['x'] = self.__processLayerNvizNode(node_position, 'x', float)
        light['position']['y'] = self.__processLayerNvizNode(node_position, 'y', float)
        light['position']['z'] = self.__processLayerNvizNode(node_position, 'z', int)
        
        light['bright'] = self.__processLayerNvizNode(node_light, 'bright', int) 
        light['ambient'] = self.__processLayerNvizNode(node_light, 'ambient', int)
        color = self.__processLayerNvizNode(node_light, 'color', str)
        light['color'] = tuple(map(int, color.split(':')))
        
        self.nviz_state['light'] = light
        
        node_constants = node_state.find('constant_planes')
        constants = []
        if node_constants:
            for node_plane in node_constants.findall('plane'):
                plane = {}
                plane['color'] = self.__processLayerNvizNode(node_plane, 'color', str)                
                plane['resolution'] = self.__processLayerNvizNode(node_plane, 'fine_resolution', int)
                plane['value'] = self.__processLayerNvizNode(node_plane, 'height', int)
                plane['object'] = {}
                constants.append({'constant': plane})
        self.nviz_state['constants'] = constants    

class WriteWorkspaceFile(object):
    """!Generic class for writing workspace file"""
    def __init__(self, lmgr, file):
        self.file =  file
        self.lmgr = lmgr
        self.indent = 0
        
        # write header

        self.file.write('<?xml version="1.0" encoding="%s"?>\n' % GetDefaultEncoding(forceUTF8 = True))
        self.file.write('<!DOCTYPE gxw SYSTEM "grass-gxw.dtd">\n')
        self.file.write('%s<gxw>\n' % (' ' * self.indent))
        
        self.indent =+ 4
        
        # layer manager
        windowPos = self.lmgr.GetPosition()
        windowSize = self.lmgr.GetSize()
        file.write('%s<layer_manager dim="%d,%d,%d,%d">\n' % (' ' * self.indent,
                                                              windowPos[0],
                                                              windowPos[1],
                                                              windowSize[0],
                                                              windowSize[1]
                                                              ))
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
                       'dim="%d,%d,%d,%d" '
                       'extent="%f,%f,%f,%f" '
                       'viewMode="%s" >\n' % (' ' * self.indent,
                                              dispName.encode('utf8'),
                                              int(mapdisp.mapWindowProperties.autoRender),
                                              mapdisp.statusbarManager.GetMode(),
                                              int(mapdisp.mapWindowProperties.showRegion),
                                              int(mapdisp.mapWindowProperties.alignExtent),
                                              int(mapdisp.mapWindowProperties.resolution),
                                              displayPos[0],
                                              displayPos[1],
                                              displaySize[0],
                                              displaySize[1],
                                              region['w'],
                                              region['s'],
                                              region['e'],
                                              region['n'],
                                              viewmode
                                              ))
            # projection statusbar info
            if mapdisp.GetProperty('projection') and \
                    UserSettings.Get(group='display', key='projection', subkey='proj4'):
                self.indent += 4
                file.write('%s<projection' % (' ' * self.indent))
                epsg = UserSettings.Get(group='display', key='projection', subkey='epsg')
                if epsg:
                    file.write(' epsg="%s"' % epsg)
                file.write('>\n')
                proj = UserSettings.Get(group='display', key='projection', subkey='proj4')
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
                self.__writeNvizState(view = nvizDisp.view, iview =  nvizDisp.iview, 
                                      light = nvizDisp.light, constants = nvizDisp.constants)
            
            file.write('%s</display>\n' % (' ' * self.indent))
        
        self.indent =- 4
        file.write('%s</gxw>\n' % (' ' * self.indent))

    def __filterValue(self, value):
        """!Make value XML-valid"""
        value = value.replace('<', '&lt;')
        value = value.replace('>', '&gt;')
        
        return value
    
    def __writeLayer(self, mapTree, item):
        """!Write bunch of layers to GRASS Workspace XML file"""
        self.indent += 4
        itemSelected = mapTree.GetSelections()
        while item and item.IsOk():
            type = mapTree.GetLayerInfo(item, key = 'type')
            if type != 'group':
                maplayer = mapTree.GetLayerInfo(item, key = 'maplayer')
            else:
                maplayer = None

            checked = int(item.IsChecked())
            if type == 'command':
                cmd = mapTree.GetLayerInfo(item, key = 'maplayer').GetCmd(string=True)
                self.file.write('%s<layer type="%s" name="%s" checked="%d">\n' % \
                               (' ' * self.indent, type, EncodeString(cmd), checked))
                self.file.write('%s</layer>\n' % (' ' * self.indent))
            elif type == 'group':
                name = mapTree.GetItemText(item)
                self.file.write('%s<group name="%s" checked="%d">\n' % \
                               (' ' * self.indent, EncodeString(name), checked))
                self.indent += 4
                subItem = mapTree.GetFirstChild(item)[0]
                self.__writeLayer(mapTree, subItem)
                self.indent -= 4
                self.file.write('%s</group>\n' % (' ' * self.indent));
            else:
                cmd = mapTree.GetLayerInfo(item, key = 'maplayer').GetCmd(string = False)
                name = mapTree.GetItemText(item).replace(os.linesep, '\\n')
                opacity = maplayer.GetOpacity(float = True)
                # remove 'opacity' part
                if opacity < 1:
                    name = name.split('(', -1)[0].strip()
                self.file.write('%s<layer type="%s" name="%s" checked="%d" opacity="%f">\n' % \
                                    (' ' * self.indent, type, EncodeString(name), checked, opacity));
                
                self.indent += 4
                # selected ?
                if item in itemSelected:
                    self.file.write('%s<selected />\n' % (' ' * self.indent))
                # layer properties
                self.file.write('%s<task name="%s">\n' % (' ' * self.indent, cmd[0]))
                self.indent += 4
                for key, val in cmd[1].iteritems():
                    if key == 'flags':
                        for f in val:
                            self.file.write('%s<flag name="%s" />\n' %
                                            (' ' * self.indent, f))
                    elif val in (True, False):
                        self.file.write('%s<flag name="%s" />\n' %
                                        (' ' * self.indent, key))
                    else: # parameter
                        self.file.write('%s<parameter name="%s">\n' %
                                        (' ' * self.indent, key))
                        self.indent += 4
                        self.file.write('%s<value>%s</value>\n' %
                                        (' ' * self.indent, self.__filterValue(val)))
                        self.indent -= 4
                        self.file.write('%s</parameter>\n' % (' ' * self.indent))
                self.indent -= 4
                self.file.write('%s</task>\n' % (' ' * self.indent))
                # vector digitizer
                vdigit = mapTree.GetLayerInfo(item, key = 'vdigit')
                if vdigit:
                    self.file.write('%s<vdigit>\n' % (' ' * self.indent))
                    if 'geomAttr' in vdigit:
                        self.indent += 4
                        for type, val in vdigit['geomAttr'].iteritems():
                            units = ''
                            if val['units'] != 'mu':
                                units = ' units="%s"' % val['units']
                            self.file.write('%s<geometryAttribute type="%s" column="%s"%s />\n' % \
                                                (' ' * self.indent, type, val['column'], units))
                        self.indent -= 4
                    self.file.write('%s</vdigit>\n' % (' ' * self.indent))
                # nviz
                nviz = mapTree.GetLayerInfo(item, key = 'nviz')
                if nviz:
                    self.file.write('%s<nviz>\n' % (' ' * self.indent))
                    if maplayer.type == 'raster':
                        self.__writeNvizSurface(nviz['surface'])
                    elif maplayer.type == 'vector':
                        self.__writeNvizVector(nviz['vector'])
                    self.file.write('%s</nviz>\n' % (' ' * self.indent))
                self.indent -= 4
                self.file.write('%s</layer>\n' % (' ' * self.indent))
            item = mapTree.GetNextSibling(item)
        self.indent -= 4
        
    def __writeNvizSurface(self, data):
        """!Save Nviz raster layer properties to workspace

        @param data Nviz layer properties
        """
        if 'object' not in data: # skip disabled
            return
        self.indent += 4
        self.file.write('%s<surface>\n' % (' ' * self.indent))
        self.indent += 4
        for attrb in data.iterkeys():
            if len(data[attrb]) < 1: # skip empty attributes
                continue
            if attrb == 'object':
                continue
            
            for name in data[attrb].iterkeys():
                # surface attribute
                if attrb == 'attribute':
                    if data[attrb][name]['map'] is None:
                        continue
                    self.file.write('%s<%s name="%s" map="%d">\n' % \
                                   (' ' * self.indent, attrb, name, data[attrb][name]['map']))
                    self.indent += 4
                    self.file.write('%s<value>%s</value>\n' % (' ' * self.indent, data[attrb][name]['value']))
                    self.indent -= 4
                    # end tag
                    self.file.write('%s</%s>\n' % (' ' * self.indent, attrb))

            # draw mode
            if attrb == 'draw':
                self.file.write('%s<%s' %(' ' * self.indent, attrb))
                if 'mode' in data[attrb]:
                    for tag, value in data[attrb]['mode']['desc'].iteritems():
                        self.file.write(' %s="%s"' % (tag, value))
                self.file.write('>\n') # <draw ...>

                if 'resolution' in data[attrb]:
                    self.indent += 4
                    for type in ('coarse', 'fine'):
                        self.file.write('%s<resolution type="%s">\n' % (' ' * self.indent, type))
                        self.indent += 4
                        self.file.write('%s<value>%d</value>\n' % (' ' * self.indent,
                                                                   data[attrb]['resolution'][type]))
                        self.indent -= 4
                        self.file.write('%s</resolution>\n' % (' ' * self.indent))

                if 'wire-color' in data[attrb]:
                    self.file.write('%s<wire_color>\n' % (' ' * self.indent))
                    self.indent += 4
                    self.file.write('%s<value>%s</value>\n' % (' ' * self.indent,
                                                               data[attrb]['wire-color']['value']))
                    self.indent -= 4
                    self.file.write('%s</wire_color>\n' % (' ' * self.indent))
                self.indent -= 4
            
            # position
            elif attrb == 'position':
                self.file.write('%s<%s>\n' %(' ' * self.indent, attrb))
                for tag in ('x', 'y', 'z'):
                    self.indent += 4
                    self.file.write('%s<%s>%d</%s>\n' % (' ' * self.indent, tag,
                                                        data[attrb][tag], tag))
                    self.indent -= 4

            if attrb != 'attribute':
                # end tag
                self.file.write('%s</%s>\n' % (' ' * self.indent, attrb))

        self.indent -= 4
        self.file.write('%s</surface>\n' % (' ' * self.indent))
        self.indent -= 4

    def __writeNvizVector(self, data):
        """!Save Nviz vector layer properties (lines/points) to workspace

        @param data Nviz layer properties
        """
        self.indent += 4
        for attrb in data.iterkeys():
            if len(data[attrb]) < 1: # skip empty attributes
                continue

            if 'object' not in data[attrb]: # skip disabled
                continue
            if attrb == 'lines':
                self.file.write('%s<v%s>\n' % (' ' * self.indent, attrb))
            elif attrb == 'points':
                markerId = data[attrb]['marker']['value']
                marker = UserSettings.Get(group = 'nviz', key = 'vector',
                                          subkey = ['points', 'marker'], internal = True)[markerId]
                self.file.write('%s<v%s marker="%s">\n' % (' ' * self.indent,
                                                           attrb,
                                                           marker))
            self.indent += 4
            for name in data[attrb].iterkeys():
                if name in ('object', 'marker'):
                    continue
                if name == 'mode':
                    self.file.write('%s<%s type="%s">\n' % (' ' * self.indent, name,
                                                          data[attrb][name]['type']))
                    if data[attrb][name]['type'] == 'surface':
                        self.indent += 4
                        for idx, surface in enumerate(data[attrb][name]['surface']['value']):
                            checked = data[attrb][name]['surface']['show'][idx]
                            self.file.write('%s<map>\n' % (' ' * self.indent))
                            self.indent += 4
                            self.file.write('%s<name>%s</name>\n' % (' ' * self.indent, surface))
                            self.file.write('%s<checked>%s</checked>\n' % (' ' * self.indent, int(checked)))
                            self.indent -= 4
                            self.file.write('%s</map>\n' % (' ' * self.indent))
                        self.indent -= 4
                    self.file.write('%s</%s>\n' % ((' ' * self.indent, name)))
                elif name == 'thematic':
                    self.file.write('%s<%s ' % (' ' * self.indent, name))
                    for key in data[attrb][name].iterkeys():
                        if key.startswith('use'):
                            self.file.write('%s="%s" ' % (key, int(data[attrb][name][key])))
                    self.file.write('>\n')
                    self.indent += 4
                    for key, value in data[attrb][name].iteritems():
                        if key.startswith('use'):
                            continue
                        if value is None:
                            value = ''
                        self.file.write('%s<%s>%s</%s>\n' % (' ' * self.indent, key, value, key))
                    self.indent -= 4
                    self.file.write('%s</%s>\n' % (' ' * self.indent, name))
                else:
                    self.file.write('%s<%s>\n' % (' ' * self.indent, name))
                    self.indent += 4
                    self.file.write('%s<value>%s</value>\n' % (' ' * self.indent, data[attrb][name]['value']))
                    self.indent -= 4
                    self.file.write('%s</%s>\n' % (' ' * self.indent, name))
            self.indent -= 4
            self.file.write('%s</v%s>\n' % (' ' * self.indent, attrb))

        self.indent -= 4

    def __writeNvizState(self, view, iview, light, constants):
        """"!Save Nviz properties (view, light) to workspace

        @param view Nviz view properties
        @param iview Nviz internal view properties
        @param light Nviz light properties
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
        self.file.write('%s<x>%.2f</x>\n' % (' ' * self.indent, view['position']['x']))
        self.file.write('%s<y>%.2f</y>\n' % (' ' * self.indent, view['position']['y']))
        self.indent -= 4
        self.file.write('%s</v_position>\n' % (' ' * self.indent))
        # perspective
        self.file.write('%s<persp>\n' % (' ' * self.indent))
        self.indent += 4
        self.file.write('%s<value>%d</value>\n' % (' ' * self.indent, view['persp']['value']))
        self.file.write('%s<step>%d</step>\n' % (' ' * self.indent, view['persp']['step']))
        self.file.write('%s<min>%d</min>\n' % (' ' * self.indent, iview['persp']['min']))
        self.file.write('%s<max>%d</max>\n' % (' ' * self.indent, iview['persp']['max']))
        self.indent -= 4
        self.file.write('%s</persp>\n' % (' ' * self.indent))
        # height
        self.file.write('%s<v_height>\n' % (' ' * self.indent))
        self.indent += 4
        self.file.write('%s<value>%d</value>\n' % (' ' * self.indent, iview['height']['value']))
        self.file.write('%s<min>%d</min>\n' % (' ' * self.indent, iview['height']['min']))
        self.file.write('%s<max>%d</max>\n' % (' ' * self.indent, iview['height']['max']))
        self.indent -= 4
        self.file.write('%s</v_height>\n' % (' ' * self.indent))
        # twist
        self.file.write('%s<twist>\n' % (' ' * self.indent))
        self.indent += 4
        self.file.write('%s<value>%d</value>\n' % (' ' * self.indent, view['twist']['value']))
        self.file.write('%s<min>%d</min>\n' % (' ' * self.indent, iview['twist']['min']))
        self.file.write('%s<max>%d</max>\n' % (' ' * self.indent, iview['twist']['max']))
        self.indent -= 4
        self.file.write('%s</twist>\n' % (' ' * self.indent))
        # z-exag
        self.file.write('%s<z-exag>\n' % (' ' * self.indent))
        self.indent += 4
        self.file.write('%s<value>%.2f</value>\n' % (' ' * self.indent, view['z-exag']['value']))
        self.file.write('%s<min>%d</min>\n' % (' ' * self.indent, view['z-exag']['min']))
        self.file.write('%s<max>%d</max>\n' % (' ' * self.indent, view['z-exag']['max']))
        self.file.write('%s<llRatio>%.2f</llRatio>\n' % (' ' * self.indent, iview['z-exag']['llRatio']))
        self.indent -= 4
        self.file.write('%s</z-exag>\n' % (' ' * self.indent))
        # focus (look here)
        self.file.write('%s<focus>\n' % (' ' * self.indent))
        self.indent += 4
        self.file.write('%s<x>%d</x>\n' % (' ' * self.indent, iview['focus']['x']))
        self.file.write('%s<y>%d</y>\n' % (' ' * self.indent, iview['focus']['y']))
        self.file.write('%s<z>%d</z>\n' % (' ' * self.indent, iview['focus']['z']))
        self.indent -= 4
        self.file.write('%s</focus>\n' % (' ' * self.indent))
        # background
        self.__writeTagWithValue('background_color', view['background']['color'][:3], format = 'd:%d:%d')
        
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
        self.file.write('%s<x>%.2f</x>\n' % (' ' * self.indent, light['position']['x']))
        self.file.write('%s<y>%.2f</y>\n' % (' ' * self.indent, light['position']['y']))
        self.file.write('%s<z>%d</z>\n' % (' ' * self.indent, light['position']['z']))
        self.indent -= 4
        self.file.write('%s</l_position>\n' % (' ' * self.indent))
        # bright
        self.__writeTagWithValue('bright', light['bright'])
        # ambient
        self.__writeTagWithValue('ambient', light['ambient'])
        # color
        self.__writeTagWithValue('color', light['color'][:3], format = 'd:%d:%d')
        
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
                self.__writeTagWithValue('height', constants[idx]['constant']['value'])
                self.__writeTagWithValue('fine_resolution', constants[idx]['constant']['resolution'])
                self.__writeTagWithValue('color', constants[idx]['constant']['color'], format = 's')
                self.indent -= 4
                self.file.write('%s</plane>\n' % (' ' * self.indent))
            self.indent -= 4
            self.file.write('%s</constant_planes>\n' % (' ' * self.indent))
        self.indent -= 4
        
        self.file.write('%s</nviz_state>\n' % (' ' * self.indent))
        self.indent -= 4
    
    def __writeTagWithValue(self, tag, data, format = 'd'):
        """!Helper function for writing pair tag
        
        @param tag written tag
        @param data written data
        @param format conversion type
        """
        self.file.write('%s<%s>\n' % (' ' * self.indent, tag))
        self.indent += 4
        self.file.write('%s' % (' ' * self.indent))
        self.file.write(('<value>%' + format + '</value>\n') % data)
        self.indent -= 4
        self.file.write('%s</%s>\n' % (' ' * self.indent, tag))
        
class ProcessGrcFile(object):
    def __init__(self, filename):
        """!Process GRC file"""
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
        """!Read GRC file

        @param parent parent window

        @return list of map layers
        """
        try:
            file = open(self.filename, "r")
        except IOError:
            wx.MessageBox(parent=parent,
                          message=_("Unable to open file <%s> for reading.") % self.filename,
                          caption=_("Error"), style=wx.OK | wx.ICON_ERROR)
            return []

        line_id = 1
        for line in file.readlines():
            self.process_line(line.rstrip('\n'), line_id)
            line_id +=1

        file.close()

        if self.num_error > 0:
            wx.MessageBox(parent=parent,
                          message=_("Some lines were skipped when reading settings "
                                    "from file <%(file)s>.\nSee 'Command output' window for details.\n\n"
                                    "Number of skipped lines: %(line)d") % \
                                        { 'file' : self.filename, 'line' : self.num_error },
                          caption=_("Warning"), style=wx.OK | wx.ICON_EXCLAMATION)
            parent._gconsole.WriteLog('Map layers loaded from GRC file <%s>' % self.filename)
            parent._gconsole.WriteLog('Skipped lines:\n%s' % self.error)

        return self.layers

    def process_line(self, line, line_id):
        """!Process line definition"""
        element = self._get_element(line)
        if element == 'Group':
            self.groupName = self._get_value(line)
            self.layers.append({
                    "type"    : 'group',
                    "name"    : self.groupName,
                    "checked" : None,
                    "opacity" : None,
                    "cmd"     : None,
                    "group"   : self.inGroup,
                    "display" : 0 })
            self.inGroup = True

        elif element == '_check':
            if int(self._get_value(line)) ==  1:
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
                    "type"    : 'raster',
                    "name"    : self._get_value(line),
                    "checked" : None,
                    "opacity" : None,
                    "cmd"     : ['d.rast'],
                    "group"   : self.inGroup,
                    "display" : 0})

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
                    "type"    : 'vector',
                    "name"    : self._get_value(line),
                    "checked" : None,
                    "opacity" : None,
                    "cmd"     : ['d.vect'],
                    "group"   : self.inGroup,
                    "display" : 0})

        elif element == 'vect' and self.inVector:
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
                paramId = self._get_cmd_param_index(self.layers[-1]['cmd'], name)
                if paramId == -1:
                    self.layers[-1]['cmd'].append('%s=%s' % (name, type))
                else:
                    self.layers[-1]['cmd'][paramId] += ',%s' % type

        elif element in ('color',
                         'fcolor',
                         'lcolor') and self.inVector:
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('%s=%s' % (element,
                                                         self._color_name_to_rgb(value)))

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
                    "type"    : 'grid',
                    "name"    : self._get_value(line),
                    "checked" : None,
                    "opacity" : None,
                    "cmd"     : ['d.grid'],
                    "group"   : self.inGroup,
                    "display" : 0})

        elif element == 'gridcolor':
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('color=%s' % self._color_name_to_rgb(value))

        elif element == 'gridborder':
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('bordercolor=%s' % self._color_name_to_rgb(value))

        elif element == 'textcolor':
            value = self._get_value(line)
            if value != '':
                self.layers[-1]['cmd'].append('textcolor=%s' % self._color_name_to_rgb(value))

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
        """!Get value of element"""
        try:
            return line.strip(' ').split(' ')[1].strip(' ')
        except:
            return ''

    def _get_element(self, line):
        """!Get element tag"""
        return line.strip(' ').split(' ')[0].strip(' ')

    def _get_cmd_param_index(self, cmd, name):
        """!Get index of parameter in cmd list

        @param cmd cmd list
        @param name parameter name

        @return index
        @return -1 if not found
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
        """!Convert color name (#) to rgb values"""
        col = wx.NamedColour(value)
        return str(col.Red()) + ':' + \
            str(col.Green()) + ':' + \
            str(col.Blue())
