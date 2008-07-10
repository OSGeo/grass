"""
@package workspace

@brief Open/save workspace definition file

Classes:
 - ProcessWorkspaceFile
 - WriteWorkspaceFile
 - ProcessGrcFile

(C) 2007-2008 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>

@date 2007-2008
"""

import os

import wx

### for gxw (workspace file) parsering
# xmlproc not available on Mac OS
# from xml.parsers.xmlproc import xmlproc
# from xml.parsers.xmlproc import xmlval
# from xml.parsers.xmlproc import xmldtd
import xml.sax
import xml.sax.handler
HandlerBase=xml.sax.handler.ContentHandler
from xml.sax import make_parser

class ProcessWorkspaceFile(HandlerBase):
    """
    A SAX handler for the GXW XML file, as
    defined in grass-gxw.dtd.
    """
    def __init__(self):
        self.inGxw       = False
        self.inLayer     = False
        self.inTask      = False
        self.inParameter = False
        self.inFlag      = False
        self.inValue     = False
        self.inGroup     = False
        self.inDisplay   = False
        self.inLayerManager = False

        #
        # Nviz section
        #
        self.inNviz = False
        self.inAttribute = False

        #
        # layer manager properties
        #
        self.layerManager = {}
        self.layerManager['pos'] = None # window position
        self.layerManager['size'] = None # window size

        # list of mapdisplays
        self.displays = []
        # list of map layers
        self.layers = []

        self.cmd    = []
        self.displayIndex = -1 # first display has index '0'

    def startElement(self, name, attrs):
        if name == 'gxw':
            self.inGxw = True

        elif name == 'display':
            self.inDisplay = True
            self.displayIndex += 1

            # window position and size
            posAttr = attrs.get('dim', '')
            if posAttr:
                posVal = map(int, posAttr.split(','))
                try:
                    pos = (posVal[0], posVal[1])
                    size = (posVal[2], posVal[3])
                except:
                    pos = None
                    size = None
            else:
                pos = None
                size = None

            extentAttr = attrs.get('extent', '')
            if extentAttr:
                # w, s, e, n
                extent = map(float, extentAttr.split(','))
            else:
                extent = None

            self.displays.append({
                "render"         : bool(int(attrs.get('render', "0"))),
                "mode"           : int(attrs.get('mode', 0)),
                "showCompExtent" : bool(int(attrs.get('showCompExtent', "0"))),
                "pos"            : pos,
                "size"           : size,
                "extent"         : extent})
            
        elif name == 'group':
            self.groupName    = attrs.get('name', None)
            self.groupChecked = attrs.get('checked', None)
            self.layers.append({
                "type"    : 'group',
                "name"    : self.groupName,
                "checked" : int(self.groupChecked),
                "opacity" : None,
                "cmd"     : None,
                "group"   : self.inGroup,
                "display" : self.displayIndex})
            self.inGroup = True

        elif name == 'layer':
            self.inLayer = True
            self.layerType     = attrs.get('type', None)
            self.layerName     = attrs.get('name', None)
            self.layerChecked  = attrs.get('checked', None)
            self.layerOpacity  = attrs.get('opacity', None)
            self.layerSelected = False;
            self.cmd = []

        elif name == 'task':
            self.inTask = True;
            name = attrs.get('name', None)
            self.cmd.append(name)

        elif name == 'parameter':
            self.inParameter = True;
            self.parameterName = attrs.get('name', None)

        elif name == 'value':
            self.inValue = True
            self.value = ''

        elif name == 'flag':
            self.inFlag = True;
            name = attrs.get('name', None)
            self.cmd.append('-' + name)

        elif name == 'selected':
            if self.inLayer:
                self.layerSelected = True;

        elif name == 'layer_manager':
            self.inLayerManager = True
            posAttr = attrs.get('dim', '')
            if posAttr:
                posVal = map(int, posAttr.split(','))
                try:
                    self.layerManager['pos'] = (posVal[0], posVal[1])
                    self.layerManager['size'] = (posVal[2], posVal[3])
                except:
                    pass
            else:
                pass

        #
        # Nviz section
        #
        elif name == 'nviz':
            self.inNviz = True

        elif name == 'attribute':
            self.inAttribute = True

    def endElement(self, name):
        if name == 'gxw':
            self.inGxw = False

        elif name == 'display':
            self.inDisplay = False

        elif name == 'group':
            self.inGroup = False
            self.groupName = self.groupChecked = None

        elif name == 'layer':
            self.inLayer = False
            self.layers.append({
                    "type"     : self.layerType,
                    "name"     : self.layerName,
                    "checked"  : int(self.layerChecked),
                    "opacity"  : None,
                    "cmd"      : None,
                    "group"    : self.inGroup,
                    "display"  : self.displayIndex,
                    "selected" : self.layerSelected})

            if self.layerOpacity:
                self.layers[-1]["opacity"] = float(self.layerOpacity)
            if self.cmd:
                self.layers[-1]["cmd"] = self.cmd

            self.layerType = self.layerName = self.Checked = \
                self.Opacity = self.cmd = None

        elif name == 'task':
            self.inTask = False

        elif name == 'parameter':
            self.inParameter = False
            self.cmd.append('%s=%s' % (self.parameterName, self.value))
            self.parameterName = self.value = None

        elif name == 'value':
            self.inValue = False

        elif name == 'flag':
            self.inFlag = False

        elif name == 'layer_manager':
            self.inLayerManager = False

        #
        # Nviz section
        #
        elif name == 'nviz':
            self.inNviz = False
        
        elif name == 'attribute':
            self.inAttribute = False

    def characters(self, ch):
        self.my_characters(ch)

    def my_characters(self, ch):
        if self.inValue:
            self.value += ch

class WriteWorkspaceFile(object):
    """Generic class for writing workspace file"""
    def __init__(self, lmgr, file):
        self.file =  file
        self.lmgr = lmgr
        self.indent = 0

        # write header
        self.file.write('<?xml version="1.0" encoding="UTF-8"?>\n')
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
        
        file.write('%s</layer_manager>\n' % (' ' * self.indent))
        
        # list of displays
        for page in range(0, self.lmgr.gm_cb.GetPageCount()):
            mapTree = self.lmgr.gm_cb.GetPage(page).maptree
            region = mapTree.Map.region
            
            displayPos = mapTree.mapdisplay.GetPosition()
            displaySize = mapTree.mapdisplay.GetSize()
            
            file.write('%s<display render="%d" '
                       'mode="%d" showCompExtent="%d" '
                       'dim="%d,%d,%d,%d" '
                       'extent="%f,%f,%f,%f">\n' % (' ' * self.indent,
                                                    int(mapTree.mapdisplay.autoRender.IsChecked()),
                                                    mapTree.mapdisplay.toggleStatus.GetSelection(),
                                                    int(mapTree.mapdisplay.showRegion.IsChecked()),
                                                    displayPos[0],
                                                    displayPos[1],
                                                    displaySize[0],
                                                    displaySize[1],
                                                    region['w'],
                                                    region['s'],
                                                    region['e'],
                                                    region['n']
                                                    ))
            
            # list of layers
            item = mapTree.GetFirstChild(mapTree.root)[0]
            self.__writeLayer(mapTree, item)
            file.write('%s</display>\n' % (' ' * self.indent))

        self.indent =- 4
        file.write('%s</gxw>\n' % (' ' * self.indent))

    def __writeLayer(self, mapTree, item):
        """Write bunch of layers to GRASS Workspace XML file"""
        self.indent += 4
        while item and item.IsOk():
            type = mapTree.GetPyData(item)[0]['type']
            if type != 'group':
                maplayer = mapTree.GetPyData(item)[0]['maplayer']
            else:
                maplayer = None

            checked = int(item.IsChecked())
            cmd = mapTree.GetPyData(item)[0]['cmd']
            if type == 'command':
                self.file.write('%s<layer type="%s" name="%s" checked="%d">\n' % \
                               (' ' * self.indent, type, ' '.join(cmd), checked));
                self.file.write('%s</layer>\n' % (' ' * self.indent));
            elif type == 'group':
                name = mapTree.GetItemText(item)
                self.file.write('%s<group name="%s" checked="%d">\n' % \
                               (' ' * self.indent, name, checked));
                self.indent += 4
                subItem = mapTree.GetFirstChild(item)[0]
                self.WriteLayer(subItem)
                self.indent -= 4
                self.file.write('%s</group>\n' % (' ' * self.indent));
            else:
                name = mapTree.GetItemText(item)
                opacity = maplayer.GetOpacity(float=True)
                self.file.write('%s<layer type="%s" name="%s" checked="%d" opacity="%f">\n' % \
                               (' ' * self.indent, type, name, checked, opacity));

                self.indent += 4
                # selected ?
                if item == mapTree.layer_selected:
                    self.file.write('%s<selected />\n' % (' ' * self.indent))
                # layer properties
                self.file.write('%s<task name="%s">\n' % (' ' * self.indent, cmd[0]))
                self.indent += 4
                for option in cmd[1:]:
                    if option[0] == '-': # flag
                        self.file.write('%s<flag name="%s" />\n' %
                                   (' ' * self.indent, option[1]))
                    else: # parameter
                        key, value = option.split('=', 1)
                        self.file.write('%s<parameter name="%s">\n' %
                                   (' ' * self.indent, key))
                        self.indent += 4
                        self.file.write('%s<value>%s</value>\n' %
                                   (' ' * self.indent, value))
                        self.indent -= 4
                        self.file.write('%s</parameter>\n' % (' ' * self.indent));
                self.indent -= 4
                self.file.write('%s</task>\n' % (' ' * self.indent));
                nviz = mapTree.GetPyData(item)[0]['nviz']
                if nviz:
                    self.file.write('%s<nviz>\n' % (' ' * self.indent));
                    if maplayer.type == 'raster':
                        self.__writeNvizSurface(nviz)
                    elif maplayer.type == 'vector':
                        self.__writeNvizVector(nviz)
                    self.file.write('%s</nviz>\n' % (' ' * self.indent));
                self.indent -= 4
                self.file.write('%s</layer>\n' % (' ' * self.indent));
            item = mapTree.GetNextSibling(item)
        self.indent -= 4

    def __writeNvizSurface(self, data):
        """Save Nviz raster layer properties to workspace

        @param data Nviz layer properties
        """
        self.indent += 4
        for attrb in data.iterkeys():
            if len(data[attrb]) < 1: # skip empty attributes
                continue
            for name in data[attrb].iterkeys():
                # surface attribute
                if attrb == 'attribute':
                    self.file.write('%s<%s name="%s" map="%d">\n' % \
                                   (' ' * self.indent, attrb, name, data[attrb][name]['map']))
                    self.indent += 4
                    self.file.write('%s<value>%s</value>\n' % (' ' * self.indent, data[attrb][name]['value']))
                    self.indent -= 4
            # draw mode
            if attrb == 'draw':
                self.file.write('%s<%s' %(' ' * self.indent, attrb))
                resTag = None
                for name in data[attrb]:
                    if name == 'resolution':
                        self.indent += 4
                        resTag = ''
                        for type, value in (('coarse', data[attrb][name][0]),
                                            ('fine', data[attrb][name][1])):
                            resTag += '%s<resolution type="%s">\n' % (' ' * self.indent, type)
                            self.indent += 4
                            resTag += '%s<value>%s</value>\n' % (' ' * self.indent, value)
                            self.indent -= 4
                            resTag += '%s</resolution>\n' % (' ' * self.indent)
                        self.indent -= 4
                    else:
                        # note: second argument is 'all' -> skip
                        self.file.write(' %s="%s"' % (name, data[attrb][name][0]))
                self.file.write('>\n') # <draw ...>
                if resTag:
                    self.file.write(resTag)
            # position
            elif attrb == 'position':
                self.file.write('%s<%s>\n' %(' ' * self.indent, attrb))
                i = 0
                for tag in ('x', 'y', 'z'):
                    self.indent += 4
                    self.file.write('%s<%s>%s<%s>\n' % (' ' * self.indent, tag,
                                                   data[attrb][name][i], tag))
                    i += 1
                    self.indent -= 4
            # end tag
            self.file.write('%s<%s>\n' % (' ' * self.indent, attrb))
        self.indent -= 4

    def __writeNvizVector(self, data):
        """Save Nviz vector layer properties to workspace

        @param data Nviz layer properties
        """
        self.indent += 4
        for attrb in data.iterkeys():
            if len(data[attrb]) < 1: # skip empty attributes
                continue
            self.file.write('%s<%s>\n' % (' ' * self.indent, attrb))
            self.indent += 4
            for name in data[attrb].iterkeys():
                self.file.write('%s<%s>' % (' ' * self.indent, name))
                if type(data[attrb][name]) == type({}):
                    self.file.write('\n')
                    self.indent += 4
                    for subname in data[attrb][name].iterkeys():
                        if subname == 'flat':
                            self.file.write('%s<style flat="%d">%s</style>\n' % (' ' * self.indent, 
                                                                            data[attrb][name][subname][0],
                                                                            data[attrb][name][subname][1]))
                        else:
                            self.file.write('%s<%s>%s</%s>\n' % (' ' * self.indent, subname,
                                                            data[attrb][name][subname], subname))
                    self.indent -= 4
                    self.file.write('%s</%s>\n' % (' ' * self.indent, name))
                else:
                    self.file.write('%s</%s>\n' % (data[attrb][name], name))
            self.indent -= 4
            self.file.write('%s</%s>\n' % (' ' * self.indent, attrb))

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
                                    "from file <%s>.\nSee 'Command output' window for details.\n\n"
                                    "Number of skipped lines: %d" % \
                                        (self.filename, self.num_error)),
                          caption=_("Warning"), style=wx.OK | wx.ICON_EXCLAMATION)
            parent.goutput.WriteLog('Map layers loaded from GRC file <%s>' % self.filename)
            parent.goutput.WriteLog('Skipped lines:\n%s' % self.error)

        return self.layers

    def process_line(self, line, line_id):
        """Process line definition"""
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
        """Convert color name (#) to rgb values"""
        col = wx.NamedColour(value)
        return str(col.Red()) + ':' + \
            str(col.Green()) + ':' + \
            str(col.Blue())
