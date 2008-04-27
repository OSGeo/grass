"""
@package workspace

@brief Open/save workspace definition file

Classes:
 - ProcessWorkspaceFile
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

        # layer manager properties
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

            self.displays.append({
                "render"         : bool(int(attrs.get('render', "0"))),
                "mode"           : int(attrs.get('mode', 0)),
                "showCompExtent" : bool(int(attrs.get('showCompExtent', "0"))),
                "pos"            : pos,
                "size"           : size})
            
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

    def characters(self, ch):
        self.my_characters(ch)

    def my_characters(self, ch):
        if self.inValue:
            self.value += ch

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
