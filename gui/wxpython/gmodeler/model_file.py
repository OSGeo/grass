"""!
@package gmodeler.model_file

@brief wxGUI Graphical Modeler - model definition file

Classes:
 - ProcessModelFile
 - WriteModelFile
 - WritePythonFile

(C) 2010-2011 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import time
import re

from gui_core.task  import GUI
from core.gcmd      import GWarning, EncodeString

class ProcessModelFile:
    """!Process GRASS model file (gxm)"""
    def __init__(self, tree):
        """!A ElementTree handler for the GXM XML file, as defined in
        grass-gxm.dtd.
        """
        self.tree = tree
        self.root = self.tree.getroot()
        
        # list of actions, data
        self.properties = dict()
        self.variables  = dict() 
        self.actions = list()
        self.data    = list()
        self.loops   = list()
        self.conditions = list()
        
        self._processWindow()
        self._processProperties()
        self._processVariables()
        self._processItems()
        self._processData()
        
    def _filterValue(self, value):
        """!Filter value
        
        @param value
        """
        value = value.replace('&lt;', '<')
        value = value.replace('&gt;', '>')
        
        return value
        
    def _getNodeText(self, node, tag, default = ''):
        """!Get node text"""
        p = node.find(tag)
        if p is not None:
            if p.text:
                return utils.normalize_whitespace(p.text)
            else:
                return ''
        
        return default
    
    def _processWindow(self):
        """!Process window properties"""
        node = self.root.find('window')
        if node is None:
            self.pos = self.size = None
            return
        
        self.pos, self.size = self._getDim(node)
        
    def _processProperties(self):
        """!Process model properties"""
        node = self.root.find('properties')
        if node is None:
            return
        for key in ('name', 'description', 'author'):
            self._processProperty(node, key)
        
        for f in node.findall('flag'):
            name = f.get('name', '')
            if name == 'overwrite':
                self.properties['overwrite'] = True
        
    def _processProperty(self, pnode, name):
        """!Process given property"""
        node = pnode.find(name)
        if node is not None:
            self.properties[name] = node.text
        else:
            self.properties[name] = ''

    def _processVariables(self):
        """!Process model variables"""
        vnode = self.root.find('variables')
        if vnode is None:
            return
        for node in vnode.findall('variable'):
            name = node.get('name', '')
            if not name:
                continue # should not happen
            self.variables[name] = { 'type' : node.get('type', 'string') }
            for key in ('description', 'value'):
                self._processVariable(node, name, key)
        
    def _processVariable(self, pnode, name, key):
        """!Process given variable"""
        node = pnode.find(key)
        if node is not None:
            if node.text:
                self.variables[name][key] = node.text

    def _processItems(self):
        """!Process model items (actions, loops, conditions)"""
        self._processActions()
        self._processLoops()
        self._processConditions()
        
    def _processActions(self):
        """!Process model file"""
        for action in self.root.findall('action'):
            pos, size = self._getDim(action)
            disabled  = False
            
            task = action.find('task')
            if task is not None:
                if task.find('disabled') is not None:
                    disabled = True
                task = self._processTask(task)
            else:
                task = None
            
            aId = int(action.get('id', -1))
            
            self.actions.append({ 'pos'      : pos,
                                  'size'     : size,
                                  'task'     : task,
                                  'id'       : aId,
                                  'disabled' : disabled })
            
    def _getDim(self, node):
        """!Get position and size of shape"""
        pos = size = None
        posAttr = node.get('pos', None)
        if posAttr:
            posVal = map(int, posAttr.split(','))
            try:
                pos = (posVal[0], posVal[1])
            except:
                pos = None
        
        sizeAttr = node.get('size', None)
        if sizeAttr:
            sizeVal = map(int, sizeAttr.split(','))
            try:
                size = (sizeVal[0], sizeVal[1])
            except:
                size = None
        
        return pos, size        
    
    def _processData(self):
        """!Process model file"""
        for data in self.root.findall('data'):
            pos, size = self._getDim(data)
            param = data.find('data-parameter')
            prompt = value = None
            if param is not None:
                prompt = param.get('prompt', None)
                value = self._filterValue(self._getNodeText(param, 'value'))
            
            if data.find('intermediate') is None:
                intermediate = False
            else:
                intermediate = True
            
            rels = list()
            for rel in data.findall('relation'):
                defrel = { 'id'  : int(rel.get('id', -1)),
                           'dir' : rel.get('dir', 'to'),
                           'name' : rel.get('name', '') }
                points = list()
                for point in rel.findall('point'):
                    x = self._filterValue(self._getNodeText(point, 'x'))
                    y = self._filterValue(self._getNodeText(point, 'y'))
                    points.append((float(x), float(y)))
                defrel['points'] = points
                rels.append(defrel)
            
            self.data.append({ 'pos' : pos,
                               'size': size,
                               'prompt' : prompt,
                               'value' : value,
                               'intermediate' : intermediate,
                               'rels' : rels })
        
    def _processTask(self, node):
        """!Process task

        @return grassTask instance
        @return None on error
        """
        cmd = list()
        parameterized = list()
        
        name = node.get('name', None)
        if not name:
            return None
        
        cmd.append(name)
        
        # flags
        for f in node.findall('flag'):
            flag = f.get('name', '')
            if f.get('parameterized', '0') == '1':
                parameterized.append(('flag', flag))
                if f.get('value', '1') == '0':
                    continue
            if len(flag) > 1:
                cmd.append('--' + flag)
            else:
                cmd.append('-' + flag)
        # parameters
        for p in node.findall('parameter'):
            name = p.get('name', '')
            if p.find('parameterized') is not None:
                parameterized.append(('param', name))
            cmd.append('%s=%s' % (name,
                                  self._filterValue(self._getNodeText(p, 'value'))))
        
        task, err = GUI(show = None, checkError = True).ParseCommand(cmd = cmd)
        if err:
            GWarning(os.linesep.join(err))
        
        for opt, name in parameterized:
            if opt == 'flag':
                task.set_flag(name, True, element = 'parameterized')
            else:
                task.set_param(name, True, element = 'parameterized')
        
        return task

    def _processLoops(self):
        """!Process model loops"""
        for node in self.root.findall('loop'):
            pos, size = self._getDim(node)
            text = self._filterValue(self._getNodeText(node, 'condition')).strip()
            aid = list()
            for anode in node.findall('item'):
                try:
                    aid.append(int(anode.text))
                except ValueError:
                    pass
            
            self.loops.append({ 'pos'     : pos,
                                'size'    : size,
                                'text'    : text,
                                'id'      : int(node.get('id', -1)),
                                'items'   : aid })
        
    def _processConditions(self):
        """!Process model conditions"""
        for node in self.root.findall('if-else'):
            pos, size = self._getDim(node)
            text = self._filterValue(self._getNodeText(node, 'condition')).strip()
            aid = { 'if'   : list(),
                    'else' : list() }
            for b in aid.keys():
                bnode = node.find(b)
                if bnode is None:
                    continue
                for anode in bnode.findall('item'):
                    try:
                        aid[b].append(int(anode.text))
                    except ValueError:
                        pass
            
            self.conditions.append({ 'pos'     : pos,
                                     'size'    : size,
                                     'text'    : text,
                                     'id'      : int(node.get('id', -1)),
                                     'items'   : aid })
        
class WriteModelFile:
    """!Generic class for writing model file"""
    def __init__(self, fd, model):
        self.fd         = fd
        self.model      = model
        self.properties = model.GetProperties()
        self.variables  = model.GetVariables()
        self.items      = model.GetItems()
        
        self.indent = 0
        
        self._header()
        
        self._window()
        self._properties()
        self._variables()
        self._items()
        
        dataList = list()
        for action in model.GetItems(objType = ModelAction):
            for rel in action.GetRelations():
                dataItem = rel.GetData()
                if dataItem not in dataList:
                    dataList.append(dataItem)
        self._data(dataList)
        
        self._footer()

    def _filterValue(self, value):
        """!Make value XML-valid"""
        value = value.replace('<', '&lt;')
        value = value.replace('>', '&gt;')
        
        return value
        
    def _header(self):
        """!Write header"""
        self.fd.write('<?xml version="1.0" encoding="UTF-8"?>\n')
        self.fd.write('<!DOCTYPE gxm SYSTEM "grass-gxm.dtd">\n')
        self.fd.write('%s<gxm>\n' % (' ' * self.indent))
        self.indent += 4
                
    def _footer(self):
        """!Write footer"""
        self.indent -= 4
        self.fd.write('%s</gxm>\n' % (' ' * self.indent))

    def _window(self):
        """!Write window properties"""
        canvas = self.model.GetCanvas()
        if canvas is None:
            return
        win  = canvas.parent
        pos  = win.GetPosition()
        size = win.GetSize()
        self.fd.write('%s<window pos="%d,%d" size="%d,%d" />\n' % \
                          (' ' * self.indent, pos[0], pos[1], size[0], size[1]))
        
    def _properties(self):
        """!Write model properties"""
        self.fd.write('%s<properties>\n' % (' ' * self.indent))
        self.indent += 4
        if self.properties['name']:
            self.fd.write('%s<name>%s</name>\n' % (' ' * self.indent, self.properties['name']))
        if self.properties['description']:
            self.fd.write('%s<description>%s</description>\n' % (' ' * self.indent,
                                                                 EncodeString(self.properties['description'])))
        if self.properties['author']:
            self.fd.write('%s<author>%s</author>\n' % (' ' * self.indent,
                                                       EncodeString(self.properties['author'])))
        
        if 'overwrite' in self.properties and \
                self.properties['overwrite']:
            self.fd.write('%s<flag name="overwrite" />\n' % (' ' * self.indent))
        self.indent -= 4
        self.fd.write('%s</properties>\n' % (' ' * self.indent))

    def _variables(self):
        """!Write model variables"""
        if not self.variables:
            return
        self.fd.write('%s<variables>\n' % (' ' * self.indent))
        self.indent += 4
        for name, values in self.variables.iteritems():
            self.fd.write('%s<variable name="%s" type="%s">\n' % \
                              (' ' * self.indent, name, values['type']))
            self.indent += 4
            if 'value' in values:
                self.fd.write('%s<value>%s</value>\n' % \
                                  (' ' * self.indent, values['value']))
            if 'description' in values:
                self.fd.write('%s<description>%s</description>\n' % \
                                  (' ' * self.indent, values['description']))
            self.indent -= 4
            self.fd.write('%s</variable>\n' % (' ' * self.indent))
        self.indent -= 4
        self.fd.write('%s</variables>\n' % (' ' * self.indent))
        
    def _items(self):
        """!Write actions/loops/conditions"""
        for item in self.items:
            if isinstance(item, ModelAction):
                self._action(item)
            elif isinstance(item, ModelLoop):
                self._loop(item)
            elif isinstance(item, ModelCondition):
                self._condition(item)
        
    def _action(self, action):
        """!Write actions"""
        self.fd.write('%s<action id="%d" name="%s" pos="%d,%d" size="%d,%d">\n' % \
                          (' ' * self.indent, action.GetId(), action.GetName(), action.GetX(), action.GetY(),
                           action.GetWidth(), action.GetHeight()))
        self.indent += 4
        self.fd.write('%s<task name="%s">\n' % (' ' * self.indent, action.GetLog(string = False)[0]))
        self.indent += 4
        if not action.IsEnabled():
            self.fd.write('%s<disabled />\n' % (' ' * self.indent))
        for key, val in action.GetParams().iteritems():
            if key == 'flags':
                for f in val:
                    if f.get('value', False) or f.get('parameterized', False):
                        if f.get('parameterized', False):
                            if f.get('value', False) == False:
                                self.fd.write('%s<flag name="%s" value="0" parameterized="1" />\n' %
                                              (' ' * self.indent, f.get('name', '')))
                            else:
                                self.fd.write('%s<flag name="%s" parameterized="1" />\n' %
                                              (' ' * self.indent, f.get('name', '')))
                        else:
                            self.fd.write('%s<flag name="%s" />\n' %
                                          (' ' * self.indent, f.get('name', '')))
            else: # parameter
                for p in val:
                    if not p.get('value', '') and not p.get('parameterized', False):
                        continue
                    self.fd.write('%s<parameter name="%s">\n' %
                                  (' ' * self.indent, p.get('name', '')))
                    self.indent += 4
                    if p.get('parameterized', False):
                        self.fd.write('%s<parameterized />\n' % (' ' * self.indent))
                    self.fd.write('%s<value>%s</value>\n' %
                                  (' ' * self.indent, self._filterValue(p.get('value', ''))))
                    self.indent -= 4
                    self.fd.write('%s</parameter>\n' % (' ' * self.indent))
        self.indent -= 4
        self.fd.write('%s</task>\n' % (' ' * self.indent))
        self.indent -= 4
        self.fd.write('%s</action>\n' % (' ' * self.indent))
                
    def _data(self, dataList):
        """!Write data"""
        for data in dataList:
            self.fd.write('%s<data pos="%d,%d" size="%d,%d">\n' % \
                              (' ' * self.indent, data.GetX(), data.GetY(),
                               data.GetWidth(), data.GetHeight()))
            self.indent += 4
            self.fd.write('%s<data-parameter prompt="%s">\n' % \
                              (' ' * self.indent, data.GetPrompt()))
            self.indent += 4
            self.fd.write('%s<value>%s</value>\n' %
                          (' ' * self.indent, self._filterValue(data.GetValue())))
            self.indent -= 4
            self.fd.write('%s</data-parameter>\n' % (' ' * self.indent))
            
            if data.IsIntermediate():
                self.fd.write('%s<intermediate />\n' % (' ' * self.indent))

            # relations
            for ft in ('from', 'to'):
                for rel in data.GetRelations(ft):
                    if ft == 'from':
                        aid = rel.GetTo().GetId()
                    else:
                        aid  = rel.GetFrom().GetId()
                    self.fd.write('%s<relation dir="%s" id="%d" name="%s">\n' % \
                                      (' ' * self.indent, ft, aid, rel.GetName()))
                    self.indent += 4
                    for point in rel.GetLineControlPoints()[1:-1]:
                        self.fd.write('%s<point>\n' % (' ' * self.indent))
                        self.indent += 4
                        x, y = point.Get()
                        self.fd.write('%s<x>%d</x>\n' % (' ' * self.indent, int(x)))
                        self.fd.write('%s<y>%d</y>\n' % (' ' * self.indent, int(y)))
                        self.indent -= 4
                        self.fd.write('%s</point>\n' % (' ' * self.indent))
                    self.indent -= 4
                    self.fd.write('%s</relation>\n' % (' ' * self.indent))
                
            self.indent -= 4
            self.fd.write('%s</data>\n' % (' ' * self.indent))

    def _loop(self, loop):
        """!Write loops"""
        self.fd.write('%s<loop id="%d" pos="%d,%d" size="%d,%d">\n' % \
                          (' ' * self.indent, loop.GetId(), loop.GetX(), loop.GetY(),
                           loop.GetWidth(), loop.GetHeight()))
        text = loop.GetText()
        self.indent += 4
        if text:
            self.fd.write('%s<condition>%s</condition>\n' %
                          (' ' * self.indent, self._filterValue(text)))
        for item in loop.GetItems():
            self.fd.write('%s<item>%d</item>\n' %
                          (' ' * self.indent, item.GetId()))
        self.indent -= 4
        self.fd.write('%s</loop>\n' % (' ' * self.indent))

    def _condition(self, condition):
        """!Write conditions"""
        bbox = condition.GetBoundingBoxMin()
        self.fd.write('%s<if-else id="%d" pos="%d,%d" size="%d,%d">\n' % \
                          (' ' * self.indent, condition.GetId(), condition.GetX(), condition.GetY(),
                           bbox[0], bbox[1]))
        text = condition.GetText()
        self.indent += 4
        if text:
            self.fd.write('%s<condition>%s</condition>\n' %
                          (' ' * self.indent, self._filterValue(text)))
        items = condition.GetItems()
        for b in items.keys():
            if len(items[b]) < 1:
                continue
            self.fd.write('%s<%s>\n' % (' ' * self.indent, b))
            self.indent += 4
            for item in items[b]:
                self.fd.write('%s<item>%d</item>\n' %
                              (' ' * self.indent, item.GetId()))
            self.indent -= 4
            self.fd.write('%s</%s>\n' % (' ' * self.indent, b))
        
        self.indent -= 4
        self.fd.write('%s</if-else>\n' % (' ' * self.indent))
        
class WritePythonFile:
    def __init__(self, fd, model):
        """!Class for exporting model to Python script

        @param fd file desciptor
        """
        self.fd     = fd
        self.model  = model
        self.indent = 4

        self._writePython()
        
    def _writePython(self):
        """!Write model to file"""
        properties = self.model.GetProperties()
        
        self.fd.write(
r"""#!/usr/bin/env python
#
############################################################################
#
# MODULE:       %s
#
# AUTHOR(S):	%s
#               
# PURPOSE:      %s
#
# DATE:         %s
#
#############################################################################
""" % (properties['name'],
       properties['author'],
       properties['description'],
       time.asctime()))
        
        self.fd.write(
r"""
import sys
import os
import atexit

import grass.script as grass
""")
        
        # cleanup()
        rast, vect, rast3d, msg = self.model.GetIntermediateData()
        self.fd.write(
r"""
def cleanup():
""")
        if rast:
            self.fd.write(
r"""    grass.run_command('g.remove',
                      rast=%s)
""" % ','.join(map(lambda x: "'" + x + "'", rast)))
        if vect:
            self.fd.write(
r"""    grass.run_command('g.remove',
                      vect = %s)
""" % ','.join(map(lambda x: "'" + x + "'", vect)))
        if rast3d:
            self.fd.write(
r"""    grass.run_command('g.remove',
                      rast3d = %s)
""" % ','.join(map(lambda x: "'" + x + "'", rast3d)))
        if not rast and not vect and not rast3d:
            self.fd.write('    pass\n')
        
        self.fd.write("\ndef main():\n")
        for item in self.model.GetItems():
            self._writePythonItem(item)
        
        self.fd.write("\n    return 0\n")
        
        self.fd.write(
r"""
if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    sys.exit(main())
""")
        
    def _writePythonItem(self, item, ignoreBlock = True, variables = []):
        """!Write model object to Python file"""
        if isinstance(item, ModelAction):
            if ignoreBlock and item.GetBlockId(): # ignore items in loops of conditions
                return
            self._writePythonAction(item, variables = variables)
        elif isinstance(item, ModelLoop) or isinstance(item, ModelCondition):
            # substitute condition
            variables = self.model.GetVariables()
            cond = item.GetText()
            for variable in variables:
                pattern = re.compile('%' + variable)
                if pattern.search(cond):
                    value = variables[variable].get('value', '')
                    if variables[variable].get('type', 'string') == 'string':
                        value = '"' + value + '"'
                    cond = pattern.sub(value, cond)
            if isinstance(item, ModelLoop):
                condVar, condText = map(lambda x: x.strip(), re.split('\s*in\s*', cond))
                cond = "%sfor %s in " % (' ' * self.indent, condVar)
                if condText[0] == '`' and condText[-1] == '`':
                    task = GUI(show = None).ParseCommand(cmd = utils.split(condText[1:-1]))
                    cond += "grass.read_command("
                    cond += self._getPythonActionCmd(task, len(cond), variables = [condVar]) + ".splitlines()"
                else:
                    cond += condText
                self.fd.write('%s:\n' % cond)
                self.indent += 4
                for action in item.GetItems():
                    self._writePythonItem(action, ignoreBlock = False, variables = [condVar])
                self.indent -= 4
            else: # ModelCondition
                self.fd.write('%sif %s:\n' % (' ' * self.indent, cond))
                self.indent += 4
                condItems = item.GetItems()
                for action in condItems['if']:
                    self._writePythonItem(action, ignoreBlock = False)
                if condItems['else']:
                    self.indent -= 4
                    self.fd.write('%selse:\n' % (' ' * self.indent))
                    self.indent += 4
                    for action in condItems['else']:
                        self._writePythonItem(action, ignoreBlock = False)
                self.indent += 4
        
    def _writePythonAction(self, item, variables = []):
        """!Write model action to Python file"""
        task = GUI(show = None).ParseCommand(cmd = item.GetLog(string = False))
        strcmd = "%sgrass.run_command(" % (' ' * self.indent)
        self.fd.write(strcmd + self._getPythonActionCmd(task, len(strcmd), variables) + '\n')
        
    def _getPythonActionCmd(self, task, cmdIndent, variables = []):
        opts = task.get_options()
        
        ret = ''
        flags = ''
        params = list()
        
        for f in opts['flags']:
            if f.get('value', False):
                name = f.get('name', '')
                if len(name) > 1:
                    params.append('%s = True' % name)
                else:
                    flags += name
            
        for p in opts['params']:
            name = p.get('name', None)
            value = p.get('value', None)
            if name and value:
                ptype = p.get('type', 'string')
                if value[0] == '%':
                    params.append("%s = %s" % (name, value[1:]))
                elif ptype == 'string':
                    params.append('%s = "%s"' % (name, value))
                else:
                    params.append("%s = %s" % (name, value))
        
        ret += '"%s"' % task.get_name()
        if flags:
            ret += ",\n%sflags = '%s'" % (' ' * cmdIndent, flags)
        if len(params) > 0:
            ret += ",\n"
            for opt in params[:-1]:
                ret += "%s%s,\n" % (' ' * cmdIndent, opt)
            ret += "%s%s)" % (' ' * cmdIndent, params[-1])
        else:
            ret += ")"
        
        return ret
