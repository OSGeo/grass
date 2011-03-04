#!/usr/bin/env python
"""
@brief Construct simple wxPython GUI from a GRASS command interface
description.

Classes:
 - grassTask
 - processTask
 - helpPanel
 - mainFrame
 - cmdPanel
 - GrassGUIApp
 - GUI
 - FloatValidator

This program is just a coarse approach to automatically build a GUI
from a xml-based GRASS user interface description.

You need to have Python 2.4, wxPython 2.8 and python-xml.

The XML stream is read from executing the command given in the
command line, thus you may call it for instance this way:

python <this file.py> r.basins.fill

Or you set an alias or wrap the call up in a nice shell script, GUI
environment ... please contribute your idea.

Updated to wxPython 2.8 syntax and contrib widgets.  Methods added to
make it callable by gui.  Method added to automatically re-run with
pythonw on a Mac.

@todo
 - verify option value types

Copyright(C) 2000-2011 by the GRASS Development Team
This program is free software under the GPL(>=v2) Read the file
COPYING coming with GRASS for details.

@author Jan-Oliver Wagner <jan@intevation.de>
@author Bernhard Reiter <bernhard@intevation.de>
@author Michael Barton, Arizona State University
@author Daniel Calvelo <dca.gis@gmail.com>
@author Martin Landa <landa.martin@gmail.com>
"""

import sys
import re
import string
import textwrap
import os
import time
import types
import copy
import locale
import types
from threading import Thread
import Queue
import shlex
import tempfile

### i18N
import gettext
gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)

import globalvar
if not os.getenv("GRASS_WXBUNDLED"):
    globalvar.CheckForWx()

import wx
import wx.html
try:
    import wx.lib.agw.flatnotebook as FN
except ImportError:
    import wx.lib.flatnotebook as FN
import wx.lib.colourselect as csel
import wx.lib.filebrowsebutton as filebrowse
import wx.lib.scrolledpanel as scrolled
from wx.lib.expando import ExpandoTextCtrl, EVT_ETC_LAYOUT_NEEDED
from wx.lib.newevent import NewEvent

try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree # Python <= 2.4

import gdialogs
from ghelp import HelpPanel

gisbase = os.getenv("GISBASE")
if gisbase is None:
    print >>sys.stderr, "We don't seem to be properly installed, or we are being run outside GRASS. Expect glitches."
    gisbase = os.path.join(os.path.dirname(sys.argv[0]), os.path.pardir)
    wxbase = gisbase
else:
    wxbase = os.path.join(globalvar.ETCWXDIR)

sys.path.append(wxbase)
imagepath = os.path.join(wxbase, "images")
sys.path.append(imagepath)

from grass.script import core as grass

import gselect
import gcmd
import goutput
import utils
from preferences import globalSettings as UserSettings
try:
    import subprocess
except:
    from compat import subprocess

wxUpdateDialog, EVT_DIALOG_UPDATE = NewEvent()

# From lib/gis/col_str.c, except purple which is mentioned
# there but not given RGB values
str2rgb = {'aqua': (100, 128, 255),
           'black': (0, 0, 0),
           'blue': (0, 0, 255),
           'brown': (180, 77, 25),
           'cyan': (0, 255, 255),
           'gray': (128, 128, 128),
           'green': (0, 255, 0),
           'grey': (128, 128, 128),
           'indigo': (0, 128, 255),
           'magenta': (255, 0, 255),
           'orange': (255, 128, 0),
           'purple': (128, 0, 128),
           'red': (255, 0, 0),
           'violet': (128, 0, 255),
           'white': (255, 255, 255),
           'yellow': (255, 255, 0)}
rgb2str = {}
for (s,r) in str2rgb.items():
    rgb2str[ r ] = s

"""!Hide some options in the GUI"""
_blackList = { }
_ignoreBlackList = True

def color_resolve(color):
    if len(color) > 0 and color[0] in "0123456789":
        rgb = tuple(map(int, color.split(':')))
        label = color
    else:
        # Convert color names to RGB
        try:
            rgb = str2rgb[color]
            label = color
        except KeyError:
            rgb = (200, 200, 200)
            label = _('Select Color')
    return (rgb, label)

def text_beautify(someString , width = 70):
    """!Make really long texts shorter, clean up whitespace and remove
    trailing punctuation.
    """
    if width > 0:
        return escape_ampersand(string.strip(
                os.linesep.join(textwrap.wrap(utils.normalize_whitespace(someString), width)),
                ".,;:"))
    else:
        return escape_ampersand(string.strip(utils.normalize_whitespace(someString), ".,;:"))
    
def escape_ampersand(text):
    """!Escapes ampersands with additional ampersand for GUI"""
    return string.replace(text, "&", "&&")

class UpdateThread(Thread):
    """!Update dialog widgets in the thread"""
    def __init__(self, parent, event, eventId, task):
        Thread.__init__(self)
        
        self.parent = parent
        self.event = event
        self.eventId = eventId
        self.task = task
        self.setDaemon(True)
        
        # list of functions which updates the dialog
        self.data = {}
        
    def run(self):
        # get widget id
        if not self.eventId:
            for p in self.task.params:
                if p.get('gisprompt', False) == False:
                    continue
                prompt = p.get('element', '')
                if prompt == 'vector':
                    name = p.get('name', '')
                    if name in ('map', 'input'):
                        self.eventId = p['wxId'][0]
            if self.eventId is None:
                return
        
        p = self.task.get_param(self.eventId, element = 'wxId', raiseError = False)
        if not p or \
                not p.has_key('wxId-bind'):
            return
        
        # get widget prompt
        pType = p.get('prompt', '')
        if not pType:
            return
        
        # check for map/input parameter
        pMap = self.task.get_param('map', raiseError = False)
        
        if not pMap:
            pMap = self.task.get_param('input', raiseError = False)

        if pMap:
            map = pMap.get('value', '')
        else:
            map = None

        # avoid running db.describe several times
        cparams = dict()
        cparams[map] = { 'dbInfo' : None,
                         'layers' : None, }
        
        # update reference widgets
        for uid in p['wxId-bind']:
            win = self.parent.FindWindowById(uid)
            name = win.GetName()
            
            map = layer = None
            driver = db = table = None
            if name in ('LayerSelect', 'LayerNameSelect',
                        'ColumnSelect'):
                if p.get('element', '') == 'vector': # -> vector
                    # get map name
                    map = p.get('value', '')
                    
                    # get layer
                    for bid in p['wxId-bind']:
                        p = self.task.get_param(bid, element = 'wxId', raiseError = False)
                        if not p:
                            continue
                        
                        if p.get('element', '') == 'layer':
                            layer = p.get('value', '')
                            if layer != '':
                                layer = p.get('value', 1)
                            else:
                                layer = p.get('default', 1)
                            break
                        
                elif p.get('element', '') == 'layer': # -> layer
                    # get layer
                    layer = p.get('value', '')
                    if layer != '':
                        layer = p.get('value', 1)
                    else:
                        layer = p.get('default', 1)
                    
                    # get map name
                    pMap = self.task.get_param(p['wxId'][0], element = 'wxId-bind', raiseError = False)
                    if pMap:
                        map = pMap.get('value', '')
            
            if name == 'TableSelect' or \
                    (name == 'ColumnSelect' and not map):
                pDriver = self.task.get_param('dbdriver', element = 'prompt', raiseError = False)
                if pDriver:
                    driver = pDriver.get('value', '')
                pDb = self.task.get_param('dbname', element = 'prompt', raiseError = False)
                if pDb:
                    db = pDb.get('value', '')
                if name == 'ColumnSelect':
                    pTable = self.task.get_param('dbtable', element = 'element', raiseError = False)
                    if pTable:
                        table = pTable.get('value', '')
                
            if name == 'LayerSelect':
                if cparams.has_key(map) and not cparams[map]['layers']:
                    win.InsertLayers(vector = map)
                    cparams[map]['layers'] = win.GetItems()
            
            elif name == 'LayerNameSelect':
                # determine format
                native = True
                for id in pMap['wxId']:
                    winVec  = self.parent.FindWindowById(id)
                    if winVec.GetName() == 'VectorFormat' and \
                            winVec.GetSelection() != 0:
                        native = False
                        break
                if not native:
                    if map:
                        self.data[win.InsertLayers] = { 'dsn' : map.rstrip('@OGR') }
                    else:
                        self.data[win.InsertLayers] = { }
            
            elif name == 'TableSelect':
                self.data[win.InsertTables] = { 'driver' : driver,
                                                'database' : db }
                
            elif name == 'ColumnSelect':
                if map:
                    if cparams.has_key(map):
                        if not cparams[map]['dbInfo']:
                            cparams[map]['dbInfo'] = gselect.VectorDBInfo(map)
                        self.data[win.InsertColumns] = { 'vector' : map, 'layer' : layer,
                                                         'dbInfo' : cparams[map]['dbInfo'] }
                else: # table
                    if driver and db:
                        self.data[win.InsertTableColumns] = { 'table' : pTable.get('value'),
                                                              'driver' : driver,
                                                              'database' : db }
                    elif pTable:
                        self.data[win.InsertTableColumns] = { 'table'  : pTable.get('value') }
            
            elif name == 'SubGroupSelect':
                pGroup = self.task.get_param('group', element = 'element', raiseError = False)
                if pGroup:
                    self.data[win.Insert] = { 'group' : pGroup.get('value', '')}
            
            elif name == 'LocationSelect':
                pDbase = self.task.get_param('dbase', element = 'element', raiseError = False)
                if pDbase:
                    self.data[win.UpdateItems] = { 'dbase' : pDbase.get('value', '')}

            elif name == 'MapsetSelect':
                pDbase = self.task.get_param('dbase', element = 'element', raiseError = False)
                pLocation = self.task.get_param('location', element = 'element', raiseError = False)
                if pDbase and pLocation:
                    self.data[win.UpdateItems] = { 'dbase' : pDbase.get('value', ''),
                                                   'location' : pLocation.get('value', '')}

            elif name ==  'ProjSelect':
                pDbase = self.task.get_param('dbase', element = 'element', raiseError = False)
                pLocation = self.task.get_param('location', element = 'element', raiseError = False)
                pMapset = self.task.get_param('mapset', element = 'element', raiseError = False)
                if pDbase and pLocation and pMapset:
                    self.data[win.UpdateItems] = { 'dbase' : pDbase.get('value', ''),
                                                   'location' : pLocation.get('value', ''),
                                                   'mapset' : pMapset.get('value', '')}
            
def UpdateDialog(parent, event, eventId, task):
    return UpdateThread(parent, event, eventId, task)

class UpdateQThread(Thread):
    """!Update dialog widgets in the thread"""
    requestId = 0
    def __init__(self, parent, requestQ, resultQ, **kwds):
        Thread.__init__(self, **kwds)
        
        self.parent = parent # cmdPanel
        self.setDaemon(True)
        
        self.requestQ = requestQ
        self.resultQ = resultQ
        
        self.start()
        
    def Update(self, callable, *args, **kwds):
        UpdateQThread.requestId +=  1
        
        self.request = None
        self.requestQ.put((UpdateQThread.requestId, callable, args, kwds))
        
        return UpdateQThread.requestId
    
    def run(self):
        while True:
            requestId, callable, args, kwds = self.requestQ.get()
            
            requestTime = time.time()
            
            self.request = callable(*args, **kwds)

            self.resultQ.put((requestId, self.request.run()))
           
            if self.request:
                event = wxUpdateDialog(data = self.request.data)
                wx.PostEvent(self.parent, event)
        
class grassTask:
    """!This class holds the structures needed for both filling by the
    parser and use by the interface constructor.

    Use as either grassTask() for empty definition or
    grassTask('grass.command') for parsed filling.
    """
    def __init__(self, grassModule = None):
        self.path = grassModule
        self.name = _('unknown')
        self.params = list()
        self.description = ''
        self.label = ''
        self.flags = list()
        self.keywords = list()
        self.errorMsg = ''
        self.firstParam = None
        
        if grassModule is not None:
            try:
                processTask(tree = etree.fromstring(getInterfaceDescription(grassModule)),
                            task = self)
            except gcmd.GException, e:
                self.errorMsg = e.value
                
            self.define_first()
        
    def define_first(self):
        """!Define first parameter"""
        if len(self.params) > 0:
            self.firstParam = self.params[0]['name']
        
    def get_error_msg(self):
        """!Get error message ('' for no error)"""
        return self.errorMsg
    
    def get_name(self):
        """!Get task name"""
        return self.name
    
    def get_list_params(self, element = 'name'):
        """!Get list of parameters"""
        params = []
        for p in self.params:
            params.append(p['name'])
        
        return params

    def get_list_flags(self, element = 'name'):
        """!Get list of parameters"""
        flags = []
        for p in self.flags:
            flags.append(p['name'])
        
        return flags
    
    def get_param(self, value, element = 'name', raiseError = True):
        """!Find and return a param by name."""
        try:
            for p in self.params:
                val = p[element]
                if val is None:
                    continue
                if type(val) in (types.ListType, types.TupleType):
                    if value in val:
                        return p
                elif type(val) ==  types.StringType:
                    if p[element][:len(value)] ==  value:
                        return p
                else:
                    if p[element] ==  value:
                        return p
        except KeyError:
            pass
        
        if raiseError:
            raise ValueError, _("Parameter element '%(element)s' not found: '%(value)s'") % \
                { 'element' : element, 'value' : value }
        else:
            return None

    def set_param(self, aParam, aValue, element = 'value'):
        """!Set param value/values.
        """
        try:
            param = self.get_param(aParam)
        except ValueError:
            return
        
        param[element] = aValue
            
    def get_flag(self, aFlag):
        """!Find and return a flag by name.
        """
        for f in self.flags:
            if f['name'] ==  aFlag:
                return f
        raise ValueError, _("Flag not found: %s") % aFlag

    def set_flag(self, aFlag, aValue, element = 'value'):
        """!Enable / disable flag.
        """
        try:
            param = self.get_flag(aFlag)
        except ValueError:
            return
        
        param[element] = aValue
        
    def getCmdError(self):
        """!Get error string produced by getCmd(ignoreErrors = False)
        
        @return list of errors
        """
        errorList = list()
        # determine if suppress_required flag is given
        for f in self.flags:
            if f['value'] and f['suppress_required']:
                return errorList
        
        for p in self.params:
            if not p.get('value', '') and p.get('required', False):
                if not p.get('default', ''):
                    desc = p.get('label', '')
                    if not desc:
                        desc = p['description']
                    errorList.append(_("Parameter '%(name)s' (%(desc)s) is missing.") % \
                                         {'name' : p['name'], 'desc' : desc })
        
        return errorList
    
    def getCmd(self, ignoreErrors = False):
        """!Produce an array of command name and arguments for feeding
        into some execve-like command processor.

        If ignoreErrors =  = True then it will return whatever has been
        built so far, even though it would not be a correct command
        for GRASS.
        """
        cmd = [self.name]
        
        suppress_required = False
        for flag in self.flags:
            if flag['value']:
                if len(flag['name']) > 1: # e.g. overwrite
                    cmd +=  [ '--' + flag['name'] ]
                else:
                    cmd +=  [ '-' + flag['name'] ]
                if flag['suppress_required']:
                    suppress_required = True
        for p in self.params:
            if p.get('value','') ==  '' and p.get('required', False):
                if p.get('default', '') !=  '':
                    cmd +=  [ '%s=%s' % (p['name'], p['default']) ]
                elif ignoreErrors is True and not suppress_required:
                    cmd +=  [ '%s=%s' % (p['name'], _('<required>')) ]
            elif p.get('value','') !=  '' and p['value'] !=  p.get('default','') :
                # Output only values that have been set, and different from defaults
                cmd +=  [ '%s=%s' % (p['name'], p['value']) ]
        
        errList = self.getCmdError()
        if ignoreErrors is False and errList:
            raise ValueError, '\n'.join(errList)
        
        return cmd

    def set_options(self, opts):
        """!Set flags and parameters

        @param opts list of flags and parameters"""
        for opt in opts:
            if opt[0] ==  '-': # flag
                self.set_flag(opt.lstrip('-'), True)
            else: # parameter
                key, value = opt.split('=', 1)
                self.set_param(key, value)
        
    def get_options(self):
        """!Get options"""
        return { 'flags'  : self.flags,
                 'params' : self.params }
    
    def has_required(self):
        """!Check if command has at least one required paramater"""
        for p in self.params:
            if p.get('required', False):
                return True
        
        return False

class processTask:
    """!A ElementTree handler for the --interface-description output,
    as defined in grass-interface.dtd. Extend or modify this and the
    DTD if the XML output of GRASS' parser is extended or modified.

    @param tree root tree node
    @param task grassTask instance or None
    @return grassTask instance
    """
    def __init__(self, tree, task = None):
        if task:
            self.task = task
        else:
            self.task = grassTask()
        
        self.root = tree
        
        self.__processModule()
        self.__processParams()
        self.__processFlags()
        self.task.define_first()
        
    def __processModule(self):
        """!Process module description"""
        self.task.name = self.root.get('name', default = 'unknown')
        
        # keywords
        for keyword in self._getNodeText(self.root, 'keywords').split(','):
            self.task.keywords.append(keyword.strip())
        
        self.task.label       = self._getNodeText(self.root, 'label')
        self.task.description = self._getNodeText(self.root, 'description')
        
    def __processParams(self):
        """!Process parameters"""
        for p in self.root.findall('parameter'):
            # gisprompt
            node_gisprompt = p.find('gisprompt')
            gisprompt = False
            age = element = prompt = None
            if node_gisprompt is not None:
                gisprompt = True
                age     = node_gisprompt.get('age', '')
                element = node_gisprompt.get('element', '')
                prompt  = node_gisprompt.get('prompt', '')
            
            # value(s)
            values = []
            values_desc = []
            node_values = p.find('values')
            if node_values is not None:
                for pv in node_values.findall('value'):
                    values.append(self._getNodeText(pv, 'name'))
                    desc = self._getNodeText(pv, 'description')
                    if desc:
                        values_desc.append(desc)
            
            # keydesc
            key_desc = []
            node_key_desc = p.find('keydesc')
            if node_key_desc is not None:
                for ki in node_key_desc.findall('item'):
                    key_desc.append(ki.text)
            
            if p.get('multiple', 'no') ==  'yes':
                multiple = True
            else:
                multiple = False
            if p.get('required', 'no') ==  'yes':
                required = True
            else:
                required = False

            if not _ignoreBlackList and \
                    _blackList.has_key(self.task.name) and \
                    p.get('name') in _blackList[self.task.name]['params']:
                hidden = True
            else:
                hidden = False
            
            self.task.params.append( {
                "name"           : p.get('name'),
                "type"           : p.get('type'),
                "required"       : required,
                "multiple"       : multiple,
                "label"          : self._getNodeText(p, 'label'),
                "description"    : self._getNodeText(p, 'description'),
                'gisprompt'      : gisprompt,
                'age'            : age,
                'element'        : element,
                'prompt'         : prompt,
                "guisection"     : self._getNodeText(p, 'guisection'),
                "guidependency"  : self._getNodeText(p, 'guidependency'),
                "default"        : self._getNodeText(p, 'default'),
                "values"         : values,
                "values_desc"    : values_desc,
                "value"          : '',
                "key_desc"       : key_desc,
                "hidden"         : hidden
                })
            
    def __processFlags(self):
        """!Process flags"""
        global _ignoreBlackList
        for p in self.root.findall('flag'):
            if not _ignoreBlackList and \
                    _blackList.has_key(self.task.name) and \
                    p.get('name') in _blackList[self.task.name]['flags']:
                hidden = True
            else:
                hidden = False
            
            if p.find('suppress_required') is not None:
                suppress_required = True
            else:
                suppress_required = False
            
            self.task.flags.append( {
                    "name"              : p.get('name'),
                    "label"             : self._getNodeText(p, 'label'),
                    "description"       : self._getNodeText(p, 'description'),
                    "guisection"        : self._getNodeText(p, 'guisection'),
                    "suppress_required" : suppress_required,
                    "value"             : False,
                    "hidden"            : hidden
                    } )
        
    def _getNodeText(self, node, tag, default = ''):
        """!Get node text"""
        p = node.find(tag)
        if p is not None:
            return utils.normalize_whitespace(p.text)
        
        return default
    
    def GetTask(self):
        """!Get grassTask instance"""
        return self.task
    
    
class mainFrame(wx.Frame):
    """!This is the Frame containing the dialog for options input.

    The dialog is organized in a notebook according to the guisections
    defined by each GRASS command.

    If run with a parent, it may Apply, Ok or Cancel; the latter two
    close the dialog.  The former two trigger a callback.

    If run standalone, it will allow execution of the command.

    The command is checked and sent to the clipboard when clicking
    'Copy'.
    """
    def __init__(self, parent, ID, task_description,
                 get_dcmd = None, layer = None):
        self.get_dcmd = get_dcmd
        self.layer    = layer
        self.task     = task_description
        self.parent   = parent            # LayerTree | Modeler | None | ...
        if parent and parent.GetName() ==  'Modeler':
            self.modeler = self.parent
        else:
            self.modeler = None
        
        # module name + keywords
        if self.task.name.split('.')[-1] in ('py', 'sh'):
            title = str(self.task.name.rsplit('.',1)[0])
        else:
            title = self.task.name
        try:
            if self.task.keywords !=  ['']:
                title +=   " [" + ', '.join(self.task.keywords) + "]"
        except ValueError:
            pass
        
        wx.Frame.__init__(self, parent = parent, id = ID, title = title,
                          pos = wx.DefaultPosition, style = wx.DEFAULT_FRAME_STYLE | wx.TAB_TRAVERSAL,
                          name = "MainFrame")
        
        self.locale = wx.Locale(language = wx.LANGUAGE_DEFAULT)
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        # statusbar
        self.CreateStatusBar()
        
        # icon
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass_dialog.ico'), wx.BITMAP_TYPE_ICO))
        
        guisizer = wx.BoxSizer(wx.VERTICAL)
        
        # set apropriate output window
        if self.parent:
            self.standalone = False
        else:
            self.standalone = True
        
        # logo + description
        topsizer = wx.BoxSizer(wx.HORIZONTAL)
        
        # GRASS logo
        self.logo = wx.StaticBitmap(parent = self.panel,
                                    bitmap = wx.Bitmap(name = os.path.join(imagepath,
                                                                       'grass_form.png'),
                                                     type = wx.BITMAP_TYPE_PNG))
        topsizer.Add(item = self.logo, proportion = 0, border = 3,
                     flag = wx.ALL | wx.ALIGN_CENTER_VERTICAL)
        
        # add module description
        if self.task.label:
            module_desc = self.task.label + ' ' + self.task.description
        else:
            module_desc = self.task.description
        self.description = gdialogs.StaticWrapText(parent = self.panel,
                                                   label = module_desc)
        topsizer.Add(item = self.description, proportion = 1, border = 5,
                     flag = wx.ALL | wx.ALIGN_CENTER_VERTICAL | wx.EXPAND)
        
        guisizer.Add(item = topsizer, proportion = 0, flag = wx.EXPAND)
        
        self.panel.SetSizerAndFit(guisizer)
        self.Layout()
        
        # notebooks
        self.notebookpanel = cmdPanel(parent = self.panel, task = self.task,
                                      mainFrame = self)
        self.goutput = self.notebookpanel.goutput
        self.notebookpanel.OnUpdateValues = self.updateValuesHook
        guisizer.Add(item = self.notebookpanel, proportion = 1, flag = wx.EXPAND)
        
        # status bar
        status_text = _("Enter parameters for '") + self.task.name + "'"
        try:
            self.task.getCmd()
            self.updateValuesHook()
        except ValueError:
            self.SetStatusText(status_text)
        
        # buttons
        btnsizer = wx.BoxSizer(orient = wx.HORIZONTAL)
        # cancel
        self.btn_cancel = wx.Button(parent = self.panel, id = wx.ID_CLOSE)
        self.btn_cancel.SetToolTipString(_("Close this window without executing the command (Ctrl+Q)"))
        btnsizer.Add(item = self.btn_cancel, proportion = 0, flag = wx.ALL | wx.ALIGN_CENTER, border = 10)
        self.btn_cancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        if self.get_dcmd is not None: # A callback has been set up
            btn_apply = wx.Button(parent = self.panel, id = wx.ID_APPLY)
            btn_ok = wx.Button(parent = self.panel, id = wx.ID_OK)
            btn_ok.SetDefault()
            
            btnsizer.Add(item = btn_apply, proportion = 0,
                         flag = wx.ALL | wx.ALIGN_CENTER,
                         border = 10)
            btnsizer.Add(item = btn_ok, proportion = 0,
                         flag = wx.ALL | wx.ALIGN_CENTER,
                         border = 10)
            
            btn_apply.Bind(wx.EVT_BUTTON, self.OnApply)
            btn_ok.Bind(wx.EVT_BUTTON, self.OnOK)
        else: # We're standalone
            # run
            self.btn_run = wx.Button(parent = self.panel, id = wx.ID_OK, label =  _("&Run"))
            self.btn_run.SetToolTipString(_("Run the command (Ctrl+R)"))
            self.btn_run.SetDefault()
            # copy
            self.btn_clipboard = wx.Button(parent = self.panel, id = wx.ID_COPY, label = _("C&opy"))
            self.btn_clipboard.SetToolTipString(_("Copy the current command string to the clipboard (Ctrl+C)"))
            
            btnsizer.Add(item = self.btn_run, proportion = 0,
                         flag = wx.ALL | wx.ALIGN_CENTER,
                         border = 10)
            
            btnsizer.Add(item = self.btn_clipboard, proportion = 0,
                         flag = wx.ALL | wx.ALIGN_CENTER,
                         border = 10)
            
            self.btn_run.Bind(wx.EVT_BUTTON, self.OnRun)
            self.btn_clipboard.Bind(wx.EVT_BUTTON, self.OnCopy)
        # help
        self.btn_help = wx.Button(parent = self.panel, id = wx.ID_HELP)
        self.btn_help.SetToolTipString(_("Show manual page of the command (Ctrl+H)"))
        self.btn_help.Bind(wx.EVT_BUTTON, self.OnHelp)
        if not hasattr(self.notebookpanel, "manual_tab_id"):
            self.btn_help.Hide()

        # add help button
        btnsizer.Add(item = self.btn_help, proportion = 0, flag = wx.ALL | wx.ALIGN_CENTER, border = 10)
        
        guisizer.Add(item = btnsizer, proportion = 0, flag = wx.ALIGN_CENTER | wx.LEFT | wx.RIGHT,
                     border = 30)
        
        if self.parent and not self.modeler:
            addLayer = False
            for p in self.task.params:
                if p.get('age', 'old') ==  'new' and \
                   p.get('prompt', '') in ('raster', 'vector', '3d-raster'):
                    addLayer = True
            
            if addLayer:
                # add newly created map into layer tree
                self.addbox = wx.CheckBox(parent = self.panel,
                                          label = _('Add created map(s) into layer tree'), style = wx.NO_BORDER)
                self.addbox.SetValue(UserSettings.Get(group = 'cmd', key = 'addNewLayer', subkey = 'enabled'))
                guisizer.Add(item = self.addbox, proportion = 0,
                             flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                             border = 5)
        
        hasNew = False
        for p in self.task.params:
            if p.get('age', 'old') ==  'new':
                hasNew = True
                break
        
        if self.get_dcmd is None and hasNew:
            # close dialog when command is terminated
            self.closebox = wx.CheckBox(parent = self.panel,
                                        label = _('Close dialog on finish'), style = wx.NO_BORDER)
            self.closebox.SetValue(UserSettings.Get(group = 'cmd', key = 'closeDlg', subkey = 'enabled'))
            self.closebox.SetToolTipString(_("Close dialog when command is successfully finished. "
                                             "Change this settings in Preferences dialog ('Command' tab)."))
            guisizer.Add(item = self.closebox, proportion = 0,
                         flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                         border = 5)
        
        self.Bind(wx.EVT_CLOSE,  self.OnCancel)
        self.Bind(wx.EVT_KEY_UP, self.OnKeyUp)
        
        # do layout
        # called automatically by SetSizer()
        self.panel.SetAutoLayout(True) 
        self.panel.SetSizerAndFit(guisizer)
        
        sizeFrame = self.GetBestSize()
        self.SetMinSize(sizeFrame)
        
        if hasattr(self, "closebox"):
            scale = 0.33
        else:
            scale = 0.50
        self.SetSize(wx.Size(sizeFrame[0], sizeFrame[1] + scale * max(self.notebookpanel.panelMinHeight,
                                                                      self.notebookpanel.constrained_size[1])))
        
        # thread to update dialog
        # create queues
        self.requestQ = Queue.Queue()
        self.resultQ = Queue.Queue()
        self.updateThread = UpdateQThread(self.notebookpanel, self.requestQ, self.resultQ)
        
        self.Layout()
        
        # keep initial window size limited for small screens
        width, height = self.GetSizeTuple()
        self.SetSize(wx.Size(min(width, 650),
                             min(height, 500)))
        
        # fix goutput's pane size
        if self.goutput:
            self.goutput.SetSashPosition(int(self.GetSize()[1] * .75))

    def updateValuesHook(self, event = None):
        """!Update status bar data"""
        self.SetStatusText(' '.join(self.notebookpanel.createCmd(ignoreErrors = True)))
        if event:
            event.Skip()
        
    def OnKeyUp(self, event):
        """!Key released (check hot-keys)"""
        try:
            kc = chr(event.GetKeyCode())
        except ValueError:
            event.Skip()
            return
        
        if not event.ControlDown():
            event.Skip()
            return
        
        if kc ==  'Q':
            self.OnCancel(None)
        elif kc ==  'S':
            self.OnAbort(None)
        elif kc ==  'H':
            self.OnHelp(None)
        elif kc ==  'R':
            self.OnRun(None)
        elif kc ==  'C':
            self.OnCopy(None)
        
        event.Skip()

    def OnDone(self, cmd, returncode):
        """!This function is launched from OnRun() when command is
        finished

        @param returncode command's return code (0 for success)
        """
        if not self.parent or returncode !=  0:
            return
        if self.parent.GetName() not in ('LayerTree', 'LayerManager'):
            return
        
        if self.parent.GetName() ==  'LayerTree':
            display = self.parent.GetMapDisplay()
        else: # Layer Manager
            display = self.parent.GetLayerTree().GetMapDisplay()
            
        if not display or not display.IsAutoRendered():
            return
        
        mapLayers = map(lambda x: x.GetName(),
                        display.GetRender().GetListOfLayers(l_type = 'raster') +
                        display.GetRender().GetListOfLayers(l_type = 'vector'))
        
        task = GUI(show = None).ParseCommand(cmd)
        for p in task.get_options()['params']:
            if p.get('prompt', '') not in ('raster', 'vector'):
                continue
            mapName = p.get('value', '')
            if '@' not in mapName:
                mapName = mapName + '@' + grass.gisenv()['MAPSET']
            if mapName in mapLayers:
                display.GetWindow().UpdateMap(render = True)
                return
        
    def OnOK(self, event):
        """!OK button pressed"""
        cmd = self.OnApply(event)
        if cmd is not None and self.get_dcmd is not None:
            self.OnCancel(event)

    def OnApply(self, event):
        """!Apply the command"""
        if self.modeler:
            cmd = self.createCmd(ignoreErrors = True)
        else:
            cmd = self.createCmd()
            
        if cmd is not None and self.get_dcmd is not None:
            # return d.* command to layer tree for rendering
            self.get_dcmd(cmd, self.layer, {"params": self.task.params, 
                                            "flags" : self.task.flags},
                          self)
            # echo d.* command to output console
            # self.parent.writeDCommand(cmd)

        return cmd

    def OnRun(self, event):
        """!Run the command"""
        cmd = self.createCmd()
        
        if not cmd or len(cmd) < 1:
            return
        
        if self.standalone or cmd[0][0:2] !=  "d.":
            # Send any non-display command to parent window (probably wxgui.py)
            # put to parents switch to 'Command output'
            if self.notebookpanel.notebook.GetSelection() !=  self.notebookpanel.goutputId:
                self.notebookpanel.notebook.SetSelection(self.notebookpanel.goutputId)
            
            try:
                if self.task.path:
                    cmd[0] = self.task.path # full path
                
                self.goutput.RunCmd(cmd, onDone = self.OnDone)
            except AttributeError, e:
                print >> sys.stderr, "%s: Propably not running in wxgui.py session?" % (e)
                print >> sys.stderr, "parent window is: %s" % (str(self.parent))
        else:
            gcmd.Command(cmd)
        
        # update buttons status
        for btn in (self.btn_run,
                    self.btn_cancel,
                    self.btn_clipboard,
                    self.btn_help):
            btn.Enable(False)
        
    def OnAbort(self, event):
        """!Abort running command"""
        event = goutput.wxCmdAbort(aborted = True)
        wx.PostEvent(self.goutput, event)

    def OnCopy(self, event):
        """!Copy the command"""
        cmddata = wx.TextDataObject()
        # list -> string
        cmdstring = ' '.join(self.createCmd(ignoreErrors = True))
        cmddata.SetText(cmdstring)
        if wx.TheClipboard.Open():
#            wx.TheClipboard.UsePrimarySelection(True)
            wx.TheClipboard.SetData(cmddata)
            wx.TheClipboard.Close()
            self.SetStatusText(_("'%s' copied to clipboard") % \
                                    (cmdstring))

    def OnCancel(self, event):
        """!Cancel button pressed"""
        self.MakeModal(False)
        
        if self.get_dcmd and \
                self.parent and \
                self.parent.GetName() in ('LayerTree',
                                          'MapWindow'):
            # display decorations and 
            # pressing OK or cancel after setting layer properties
            if self.task.name in ['d.barscale','d.legend','d.histogram'] \
                or len(self.parent.GetPyData(self.layer)[0]['cmd']) >=  1:
                self.Hide()
            # canceled layer with nothing set
            elif len(self.parent.GetPyData(self.layer)[0]['cmd']) < 1:
                self.parent.Delete(self.layer)
                self.Destroy()
        else:
            # cancel for non-display commands
            self.Destroy()

    def OnHelp(self, event):
        """!Show manual page (switch to the 'Manual' notebook page)"""
        if hasattr(self.notebookpanel, "manual_tab_id"):
            self.notebookpanel.notebook.SetSelection(self.notebookpanel.manual_tab_id)
            self.notebookpanel.OnPageChange(None)
        
        if event:    
            event.Skip()
        
    def createCmd(self, ignoreErrors = False):
        """!Create command string (python list)"""
        return self.notebookpanel.createCmd(ignoreErrors = ignoreErrors)

class cmdPanel(wx.Panel):
    """!A panel containing a notebook dividing in tabs the different
    guisections of the GRASS cmd.
    """
    def __init__(self, parent, task, id = wx.ID_ANY, mainFrame = None, *args, **kwargs):
        if mainFrame:
            self.parent = mainFrame
        else:
            self.parent = parent
        self.task = task
        
        wx.Panel.__init__(self, parent, id = id, *args, **kwargs)
        
        # Determine tab layout
        sections = []
        is_section = {}
        not_hidden = [ p for p in self.task.params + self.task.flags if not p.get('hidden', False) ==  True ]

        self.label_id = [] # wrap titles on resize

        self.Bind(wx.EVT_SIZE, self.OnSize)
        
        for task in not_hidden:
            if task.get('required', False):
                # All required go into Main, even if they had defined another guisection
                task['guisection'] = _('Required')
            if task.get('guisection','') ==  '':
                # Undefined guisections end up into Options
                task['guisection'] = _('Optional')
            if not is_section.has_key(task['guisection']):
                # We do it like this to keep the original order, except for Main which goes first
                is_section[task['guisection']] = 1
                sections.append(task['guisection'])
            else:
                is_section[ task['guisection'] ] +=  1
        del is_section

        # 'Required' tab goes first, 'Optional' as the last one
        for (newidx,content) in [ (0,_('Required')), (len(sections)-1,_('Optional')) ]:
            if content in sections:
                idx = sections.index(content)
                sections[idx:idx+1] = []
                sections[newidx:newidx] =  [content]

        panelsizer = wx.BoxSizer(orient = wx.VERTICAL)

        # Build notebook
        nbStyle = globalvar.FNPageStyle
        if globalvar.hasAgw:
            self.notebook = FN.FlatNotebook(self, id = wx.ID_ANY, agwStyle = nbStyle)
        else:
            self.notebook = FN.FlatNotebook(self, id = wx.ID_ANY, style = nbStyle)
        self.notebook.SetTabAreaColour(globalvar.FNPageColor)
        self.notebook.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CHANGED, self.OnPageChange)

        tab = {}
        tabsizer = {}
        for section in sections:
            tab[section] = scrolled.ScrolledPanel(parent = self.notebook)
            tab[section].SetScrollRate(10, 10)
            tabsizer[section] = wx.BoxSizer(orient = wx.VERTICAL)
            self.notebook.AddPage(tab[section], text = section)

        # are we running from command line?
        ### add 'command output' tab regardless standalone dialog
        if self.parent.GetName() ==  "MainFrame" and self.parent.get_dcmd is None:
            self.goutput = goutput.GMConsole(parent = self, margin = False,
                                             pageid = self.notebook.GetPageCount())
            self.goutputId = self.notebook.GetPageCount()
            self.outpage = self.notebook.AddPage(self.goutput, text = _("Command output"))
        else:
            self.goutput = None
            self.goutputId = -1
        
        self.manual_tab = HelpPanel(parent = self, grass_command = self.task.name)
        if not self.manual_tab.IsFile():
            self.manual_tab.Hide()
        else:
            self.notebook.AddPage(self.manual_tab, text = _("Manual"))
            self.manual_tab_id = self.notebook.GetPageCount() - 1
        
        self.notebook.SetSelection(0)

        panelsizer.Add(item = self.notebook, proportion = 1, flag = wx.EXPAND)

        #
        # flags
        #
        text_style = wx.FONTWEIGHT_NORMAL
        visible_flags = [ f for f in self.task.flags if not f.get('hidden', False) ==  True ]
        for f in visible_flags:
            which_sizer = tabsizer[ f['guisection'] ]
            which_panel = tab[ f['guisection'] ]
            # if label is given: description -> tooltip
            if f.get('label','') !=  '':
                title = text_beautify(f['label'])
                tooltip = text_beautify(f['description'], width = -1)
            else:
                title = text_beautify(f['description'])
                tooltip = None
            title_sizer = wx.BoxSizer(wx.HORIZONTAL)
            rtitle_txt = wx.StaticText(parent = which_panel,
                                       label = '(' + f['name'] + ')')
            chk = wx.CheckBox(parent = which_panel, label = title, style = wx.NO_BORDER)
            self.label_id.append(chk.GetId())
            if tooltip:
                chk.SetToolTipString(tooltip)
            chk.SetValue(f.get('value', False))
            title_sizer.Add(item = chk, proportion = 1,
                            flag = wx.EXPAND)
            title_sizer.Add(item = rtitle_txt, proportion = 0,
                            flag = wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
            which_sizer.Add(item = title_sizer, proportion = 0,
                            flag = wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT, border = 5)
            f['wxId'] = [ chk.GetId(), ]
            chk.Bind(wx.EVT_CHECKBOX, self.OnSetValue)
            
            if self.parent.GetName() ==  'MainFrame' and self.parent.modeler:
                parChk = wx.CheckBox(parent = which_panel, id = wx.ID_ANY,
                                     label = _("Parameterized in model"))
                parChk.SetName('ModelParam')
                parChk.SetValue(f.get('parameterized', False))
                if f.has_key('wxId'):
                    f['wxId'].append(parChk.GetId())
                else:
                    f['wxId'] = [ parChk.GetId() ]
                parChk.Bind(wx.EVT_CHECKBOX, self.OnSetValue)
                which_sizer.Add(item = parChk, proportion = 0,
                                flag = wx.LEFT, border = 20)
            
            if f['name'] in ('verbose', 'quiet'):
                chk.Bind(wx.EVT_CHECKBOX, self.OnVerbosity)
                vq = UserSettings.Get(group = 'cmd', key = 'verbosity', subkey = 'selection')
                if f['name'] ==  vq:
                    chk.SetValue(True)
                    f['value'] = True
            elif f['name'] ==  'overwrite' and not f.has_key('value'):
                chk.SetValue(UserSettings.Get(group = 'cmd', key = 'overwrite', subkey = 'enabled'))
                f['value'] = UserSettings.Get(group = 'cmd', key = 'overwrite', subkey = 'enabled')
                
        #
        # parameters
        #
        visible_params = [ p for p in self.task.params if not p.get('hidden', False) ==  True ]
        
        try:
            first_param = visible_params[0]
        except IndexError:
            first_param = None
        
        for p in visible_params:
            which_sizer = tabsizer[ p['guisection'] ]
            which_panel = tab[ p['guisection'] ]
            # if label is given -> label and description -> tooltip
            # otherwise description -> lavel
            if p.get('label','') !=  '':
                title = text_beautify(p['label'])
                tooltip = text_beautify(p['description'], width = -1)
            else:
                title = text_beautify(p['description'])
                tooltip = None
            txt = None

            # text style (required -> bold)
            if not p.get('required', False):
                text_style = wx.FONTWEIGHT_NORMAL
            else:
                text_style = wx.FONTWEIGHT_BOLD

            # title sizer (description, name, type)
            if (len(p.get('values', [])) > 0) and \
                    p.get('multiple', False) and \
                    p.get('gisprompt',False) ==  False and \
                    p.get('type', '') ==  'string':
                title_txt = wx.StaticBox(parent = which_panel, id = wx.ID_ANY)
            else:
                title_sizer = wx.BoxSizer(wx.HORIZONTAL)
                title_txt = wx.StaticText(parent = which_panel)
                if p['key_desc']:
                    ltype = ','.join(p['key_desc'])
                else:
                    ltype = p['type']
                rtitle_txt = wx.StaticText(parent = which_panel,
                                           label = '(' + p['name'] + '=' + ltype + ')')
                title_sizer.Add(item = title_txt, proportion = 1,
                                flag = wx.LEFT | wx.TOP | wx.EXPAND, border = 5)
                title_sizer.Add(item = rtitle_txt, proportion = 0,
                                flag = wx.ALIGN_RIGHT | wx.RIGHT | wx.TOP, border = 5)
                which_sizer.Add(item = title_sizer, proportion = 0,
                                flag = wx.EXPAND)
            self.label_id.append(title_txt.GetId())

            # title expansion
            if p.get('multiple', False) and len(p.get('values','')) ==  0:
                title = _("[multiple]") + " " + title
                if p.get('value','') ==   '' :
                    p['value'] = p.get('default','')

            if (len(p.get('values', [])) > 0):
                valuelist      = map(str, p.get('values',[]))
                valuelist_desc = map(unicode, p.get('values_desc',[]))

                if p.get('multiple', False) and \
                        p.get('gisprompt',False) ==  False and \
                        p.get('type', '') ==  'string':
                    title_txt.SetLabel(" %s: (%s, %s) " % (title, p['name'], p['type']))
                    if valuelist_desc:
                        hSizer = wx.StaticBoxSizer(box = title_txt, orient = wx.VERTICAL)
                    else:
                        hSizer = wx.StaticBoxSizer(box = title_txt, orient = wx.HORIZONTAL)
                    isEnabled = {}
                    # copy default values
                    if p['value'] ==  '':
                        p['value'] = p.get('default', '')
                        
                    for defval in p.get('value', '').split(','):
                        isEnabled[ defval ] = 'yes'
                        # for multi checkboxes, this is an array of all wx IDs
                        # for each individual checkbox
                        p[ 'wxId' ] = list()
                    idx = 0
                    for val in valuelist:
                        try:
                            label = valuelist_desc[idx]
                        except IndexError:
                            label = val
                        
                        chkbox = wx.CheckBox(parent = which_panel,
                                             label = text_beautify(label))
                        p[ 'wxId' ].append(chkbox.GetId())
                        if isEnabled.has_key(val):
                            chkbox.SetValue(True)
                        hSizer.Add(item = chkbox, proportion = 0,
                                    flag = wx.ADJUST_MINSIZE | wx.ALL, border = 1)
                        chkbox.Bind(wx.EVT_CHECKBOX, self.OnCheckBoxMulti)
                        idx +=  1
                        
                    which_sizer.Add(item = hSizer, proportion = 0,
                                    flag = wx.EXPAND | wx.TOP | wx.RIGHT | wx.LEFT, border = 5)
                elif p.get('gisprompt', False) ==  False:
                    if len(valuelist) ==  1: # -> textctrl
                        title_txt.SetLabel("%s (%s %s):" % (title, _('valid range'),
                                                            str(valuelist[0])))
                        
                        if p.get('type', '') ==  'integer' and \
                                not p.get('multiple', False):

                            # for multiple integers use textctrl instead of spinsctrl
                            try:
                                minValue, maxValue = map(int, valuelist[0].split('-'))
                            except ValueError:
                                minValue = -1e6
                                maxValue = 1e6
                            txt2 = wx.SpinCtrl(parent = which_panel, id = wx.ID_ANY, size = globalvar.DIALOG_SPIN_SIZE,
                                               min = minValue, max = maxValue)
                            txt2.SetName("SpinCtrl")
                            style = wx.BOTTOM | wx.LEFT
                        else:
                            txt2 = wx.TextCtrl(parent = which_panel, value = p.get('default',''))
                            txt2.SetName("TextCtrl")
                            style = wx.EXPAND | wx.BOTTOM | wx.LEFT
                        
                        value = self._getValue(p)
                        # parameter previously set
                        if value:
                            if txt2.GetName() ==  "SpinCtrl":
                                txt2.SetValue(int(value))
                            else:
                                txt2.SetValue(value)
                        
                        which_sizer.Add(item = txt2, proportion = 0,
                                        flag = style, border = 5)

                        p['wxId'] = [ txt2.GetId(), ]
                        txt2.Bind(wx.EVT_TEXT, self.OnSetValue)
                    else:
                        # list of values (combo)
                        title_txt.SetLabel(title + ':')
                        cb = wx.ComboBox(parent = which_panel, id = wx.ID_ANY, value = p.get('default',''),
                                         size = globalvar.DIALOG_COMBOBOX_SIZE,
                                         choices = valuelist, style = wx.CB_DROPDOWN)
                        value = self._getValue(p)
                        if value:
                            cb.SetValue(value) # parameter previously set
                        which_sizer.Add(item = cb, proportion = 0,
                                        flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT, border = 5)
                        p['wxId'] = [cb.GetId(),]
                        cb.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                        cb.Bind(wx.EVT_TEXT, self.OnSetValue)
            
            # text entry
            if (p.get('type','string') in ('string','integer','float')
                and len(p.get('values',[])) ==  0
                and p.get('gisprompt',False) ==  False
                and p.get('prompt','') !=  'color'):

                title_txt.SetLabel(title + ':')
                if p.get('multiple', False) or \
                        p.get('type', 'string') ==  'string' or \
                        len(p.get('key_desc', [])) > 1:
                    txt3 = wx.TextCtrl(parent = which_panel, value = p.get('default',''))
                    
                    value = self._getValue(p)
                    if value:
                        # parameter previously set
                        txt3.SetValue(str(value))
                    
                    txt3.Bind(wx.EVT_TEXT, self.OnSetValue)
                    style = wx.EXPAND | wx.BOTTOM | wx.LEFT | wx.RIGHT
                else:
                    minValue = -1e9
                    maxValue = 1e9
                    if p.get('type', '') ==  'integer':
                        txt3 = wx.SpinCtrl(parent = which_panel, value = p.get('default',''),
                                           size = globalvar.DIALOG_SPIN_SIZE,
                                           min = minValue, max = maxValue)
                        style = wx.BOTTOM | wx.LEFT | wx.RIGHT
                        
                        value = self._getValue(p)
                        if value:
                            txt3.SetValue(int(value)) # parameter previously set
                        
                        txt3.Bind(wx.EVT_SPINCTRL, self.OnSetValue)
                    else:
                        txt3 = wx.TextCtrl(parent = which_panel, value = p.get('default',''),
                                           validator = FloatValidator())
                        style = wx.EXPAND | wx.BOTTOM | wx.LEFT | wx.RIGHT
                        
                        value = self._getValue(p)
                        if value:
                            txt3.SetValue(str(value)) # parameter previously set
                    
                txt3.Bind(wx.EVT_TEXT, self.OnSetValue)
                
                which_sizer.Add(item = txt3, proportion = 0,
                                flag = style, border = 5)
                p['wxId'] = [ txt3.GetId(), ]

            #
            # element selection tree combobox (maps, icons, regions, etc.)
            #
            if p.get('gisprompt', False) ==  True:
                title_txt.SetLabel(title + ':')
                # GIS element entry
                if p.get('prompt','') not in ('color',
                                              'color_none',
                                              'subgroup',
                                              'dbdriver',
                                              'dbname',
                                              'dbtable',
                                              'dbcolumn',
                                              'layer',
                                              'layer_all',
                                              'location',
                                              'mapset',
                                              'dbase') and \
                       p.get('element', '') !=  'file':
                    multiple = p.get('multiple', False)
                    if p.get('age', '') ==  'new':
                        mapsets = [grass.gisenv()['MAPSET'],]
                    else:
                        mapsets = None
                    if self.task.name in ('r.proj', 'v.proj') \
                            and p.get('name', '') ==  'input':
                        if self.task.name ==  'r.proj':
                            isRaster = True
                        else:
                            isRaster = False
                        selection = gselect.ProjSelect(parent = which_panel,
                                                       isRaster = isRaster)
                        p['wxId'] = [ selection.GetId(), ]
                        selection.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                        formatSelector = False
                        selection.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                    else:
                        selection = gselect.Select(parent = which_panel, id = wx.ID_ANY,
                                                   size = globalvar.DIALOG_GSELECT_SIZE,
                                                   type = p.get('element', ''),
                                                   multiple = multiple, mapsets = mapsets)
                        value = self._getValue(p)
                        if value:
                            selection.SetValue(value)
                        
                        formatSelector = True
                        # A select.Select is a combobox with two children: a textctl and a popupwindow;
                        # we target the textctl here
                        textWin = selection.GetTextCtrl()
                        p['wxId'] = [ textWin.GetId(), ]
                        textWin.Bind(wx.EVT_TEXT, self.OnSetValue)
                    
                    if p.get('prompt', '') ==  'vector':
                        selection.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                        
                        if formatSelector and p.get('age', 'old') ==  'old':
                            # OGR supported (read-only)
                            self.hsizer = wx.BoxSizer(wx.HORIZONTAL)
                            
                            self.hsizer.Add(item = selection,
                                            flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_TOP,
                                            border = 5)
                            
                            # format (native / ogr)
                            rbox = wx.RadioBox(parent = which_panel, id = wx.ID_ANY,
                                               label = " %s " % _("Format"),
                                               style = wx.RA_SPECIFY_ROWS,
                                               choices = [_("Native / Linked OGR"), _("Direct OGR")])
                            if p.get('value', '').lower().rfind('@ogr') > -1:
                                rbox.SetSelection(1)
                            rbox.SetName('VectorFormat')
                            rbox.Bind(wx.EVT_RADIOBOX, self.OnVectorFormat)
                            
                            self.hsizer.Add(item = rbox,
                                            flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT |
                                            wx.RIGHT | wx.ALIGN_TOP,
                                            border = 5)
                            
                            ogrSelection = gselect.GdalSelect(parent = self, panel = which_panel, ogr = True,
                                                              default = 'dir',
                                                              exclude = ['file'])
                            self.Bind(gselect.EVT_GDALSELECT, self.OnUpdateSelection)
                            self.Bind(gselect.EVT_GDALSELECT, self.OnSetValue)
                            
                            ogrSelection.SetName('OgrSelect')
                            ogrSelection.Hide()
                            
                            which_sizer.Add(item = self.hsizer, proportion = 0)
                            
                            p['wxId'].append(rbox.GetId())
                            p['wxId'].append(ogrSelection.GetId())
                            for win in ogrSelection.GetDsnWin():
                                p['wxId'].append(win.GetId())
                        else:
                            which_sizer.Add(item = selection, proportion = 0,
                                            flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                                            border = 5)
                    elif p.get('prompt', '') ==  'group':
                        selection.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                        which_sizer.Add(item = selection, proportion = 0,
                                        flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                                        border = 5)
                    else:
                        which_sizer.Add(item = selection, proportion = 0,
                                        flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                                        border = 5)
                # subgroup
                elif p.get('prompt', '') ==  'subgroup':
                    selection = gselect.SubGroupSelect(parent = which_panel)
                    p['wxId'] = [ selection.GetId() ]
                    selection.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                    which_sizer.Add(item = selection, proportion = 0,
                                    flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                                    border = 5)

                # layer, dbdriver, dbname, dbcolumn, dbtable entry
                elif p.get('prompt', '') in ('dbdriver',
                                             'dbname',
                                             'dbtable',
                                             'dbcolumn',
                                             'layer',
                                             'layer_all',
                                             'location',
                                             'mapset',
                                             'dbase'):
                    if p.get('multiple', 'no') ==  'yes':
                        win = wx.TextCtrl(parent = which_panel, value = p.get('default',''),
                                          size = globalvar.DIALOG_TEXTCTRL_SIZE)
                        win.Bind(wx.EVT_TEXT, self.OnSetValue)
                    else:
                        value = p.get('value', '')
                        if not value:
                            value = p.get('default', '')
                        
                        if p.get('prompt', '') in ('layer',
                                                   'layer_all'):
                            if p.get('prompt', '') ==  'layer_all':
                                all = True
                            else:
                                all = False
                            win = wx.BoxSizer(wx.HORIZONTAL)
                            if p.get('age', 'old') ==  'old':
                                win1 = gselect.LayerSelect(parent = which_panel,
                                                          all = all,
                                                          default = p['default'])
                                win1.Bind(wx.EVT_CHOICE, self.OnUpdateSelection)
                                win1.Bind(wx.EVT_CHOICE, self.OnSetValue)
                            else:
                                win1 = wx.SpinCtrl(parent = which_panel, id = wx.ID_ANY,
                                                  min = 1, max = 100, initial = int(p['default']))
                                win1.Bind(wx.EVT_SPINCTRL, self.OnSetValue)
                            win2 = gselect.LayerNameSelect(parent = which_panel)
                            if p.get('value','') !=  '':
                                win2.SetItems([p['value']])
                                win2.SetSelection(0)
                            
                            win2.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                            p['wxId'] = [ win1.GetId(), win2.GetId() ]
                            win.Add(item = win1, proportion = 0)
                            win.Add(item = win2, proportion = 0,
                                    flag = wx.LEFT | wx.ALIGN_CENTER_VERTICAL,
                                    border = 5)

                        elif p.get('prompt', '') ==  'dbdriver':
                            win = gselect.DriverSelect(parent = which_panel,
                                                       choices = p.get('values', []),
                                                       value = value)
                            win.Bind(wx.EVT_COMBOBOX, self.OnUpdateSelection)
                            win.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                        elif p.get('prompt', '') ==  'dbname':
                            win = gselect.DatabaseSelect(parent = which_panel,
                                                         value = value)
                            win.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                            win.Bind(wx.EVT_TEXT, self.OnSetValue)
                        elif p.get('prompt', '') ==  'dbtable':
                            if p.get('age', 'old') ==  'old':
                                win = gselect.TableSelect(parent = which_panel)
                                win.Bind(wx.EVT_COMBOBOX, self.OnUpdateSelection)
                                win.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                            else:
                                win = wx.TextCtrl(parent = which_panel, value = p.get('default',''),
                                                  size = globalvar.DIALOG_TEXTCTRL_SIZE)
                                win.Bind(wx.EVT_TEXT, self.OnSetValue)
                        elif p.get('prompt', '') ==  'dbcolumn':
                            win = gselect.ColumnSelect(parent = which_panel,
                                                       value = value,
                                                       param = p)
                            win.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                            win.Bind(wx.EVT_TEXT,     self.OnSetValue)

                        elif p.get('prompt', '') ==  'location':
                            win = gselect.LocationSelect(parent = which_panel,
                                                         value = value)
                            win.Bind(wx.EVT_COMBOBOX,     self.OnUpdateSelection)
                            win.Bind(wx.EVT_COMBOBOX,     self.OnSetValue)
                        
                        elif p.get('prompt', '') ==  'mapset':
                            win = gselect.MapsetSelect(parent = which_panel,
                                                       value = value)
                            win.Bind(wx.EVT_COMBOBOX,     self.OnUpdateSelection)
                            win.Bind(wx.EVT_COMBOBOX,     self.OnSetValue)
                            
                        elif p.get('prompt', '') ==  'dbase':
                            win = gselect.DbaseSelect(parent = which_panel,
                                                      changeCallback = self.OnSetValue)
                            win.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                            p['wxId'] = [ win.GetChildren()[1].GetId() ]
                            
                    if not p.has_key('wxId'):
                        try:
                            p['wxId'] = [ win.GetId(), ]
                        except AttributeError:
                            pass
                    
                    which_sizer.Add(item = win, proportion = 0,
                                    flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT, border = 5)
                # color entry
                elif p.get('prompt', '') in ('color',
                                             'color_none'):
                    default_color = (200,200,200)
                    label_color = _("Select Color")
                    if p.get('default','') !=  '':
                        default_color, label_color = color_resolve(p['default'])
                    if p.get('value','') !=  '': # parameter previously set
                        default_color, label_color = color_resolve(p['value'])
                    if p.get('prompt', '') ==  'color_none':
                        this_sizer = wx.BoxSizer(orient = wx.HORIZONTAL)
                    else:
                        this_sizer = which_sizer
                    btn_colour = csel.ColourSelect(parent = which_panel, id = wx.ID_ANY,
                                                   label = label_color, colour = default_color,
                                                   pos = wx.DefaultPosition, size = (150,-1))
                    this_sizer.Add(item = btn_colour, proportion = 0,
                                   flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT, border = 5)
                    # For color selectors, this is a two-member array, holding the IDs of
                    # the selector proper and either a "transparent" button or None
                    p['wxId'] = [btn_colour.GetId(),]
                    btn_colour.Bind(csel.EVT_COLOURSELECT,  self.OnColorChange)
                    if p.get('prompt', '') ==  'color_none':
                        none_check = wx.CheckBox(which_panel, wx.ID_ANY, _("Transparent"))
                        if p.get('value','') !=  '' and p.get('value',[''])[0] ==  "none":
                            none_check.SetValue(True)
                        else:
                            none_check.SetValue(False)
                        this_sizer.Add(item = none_check, proportion = 0,
                                       flag = wx.ADJUST_MINSIZE | wx.LEFT | wx.RIGHT | wx.TOP, border = 5)
                        which_sizer.Add(this_sizer)
                        none_check.Bind(wx.EVT_CHECKBOX, self.OnColorChange)
                        p['wxId'].append(none_check.GetId())
                    else:
                        p['wxId'].append(None)
                # file selector
                elif p.get('prompt','') !=  'color' and p.get('element', '') ==  'file':
                    fbb = filebrowse.FileBrowseButton(parent = which_panel, id = wx.ID_ANY, fileMask = '*',
                                                      size = globalvar.DIALOG_GSELECT_SIZE, labelText = '',
                                                      dialogTitle = _('Choose %s') % \
                                                          p.get('description',_('File')),
                                                      buttonText = _('Browse'),
                                                      startDirectory = os.getcwd(), fileMode = 0,
                                                      changeCallback = self.OnSetValue)
                    value = self._getValue(p)
                    if value:
                        fbb.SetValue(value) # parameter previously set
                    which_sizer.Add(item = fbb, proportion = 0,
                                    flag = wx.EXPAND | wx.RIGHT, border = 5)
                    
                    # A file browse button is a combobox with two children:
                    # a textctl and a button;
                    # we have to target the button here
                    p['wxId'] = [ fbb.GetChildren()[1].GetId() ]
                    if p.get('age', 'new') ==  'old' and \
                            UserSettings.Get(group = 'cmd', key = 'interactiveInput', subkey = 'enabled'):
                        # widget for interactive input
                        ifbb = wx.TextCtrl(parent = which_panel, id = wx.ID_ANY,
                                           style = wx.TE_MULTILINE,
                                           size = (-1, 75))
                        if p.get('value', '') and os.path.isfile(p['value']):
                            f = open(p['value'])
                            ifbb.SetValue(''.join(f.readlines()))
                            f.close()
                    
                        ifbb.Bind(wx.EVT_TEXT, self.OnFileText)
                        which_sizer.Add(item = wx.StaticText(parent = which_panel, id = wx.ID_ANY,
                                                             label = _('or enter values interactively')),
                                        proportion = 0,
                                        flag = wx.EXPAND | wx.RIGHT | wx.LEFT | wx.BOTTOM, border = 5)
                        which_sizer.Add(item = ifbb, proportion = 1,
                                        flag = wx.EXPAND | wx.RIGHT | wx.LEFT, border = 5)
                        p['wxId'].append(ifbb.GetId())
            
            if self.parent.GetName() ==  'MainFrame' and self.parent.modeler:
                parChk = wx.CheckBox(parent = which_panel, id = wx.ID_ANY,
                                     label = _("Parameterized in model"))
                parChk.SetName('ModelParam')
                parChk.SetValue(p.get('parameterized', False))
                if p.has_key('wxId'):
                    p['wxId'].append(parChk.GetId())
                else:
                    p['wxId'] = [ parChk.GetId() ]
                parChk.Bind(wx.EVT_CHECKBOX, self.OnSetValue)
                which_sizer.Add(item = parChk, proportion = 0,
                                flag = wx.LEFT, border = 20)
                
            if title_txt is not None:
                # create tooltip if given
                if len(p['values_desc']) > 0:
                    if tooltip:
                        tooltip +=  2 * os.linesep
                    else:
                        tooltip = ''
                    if len(p['values']) ==  len(p['values_desc']):
                        for i in range(len(p['values'])):
                            tooltip +=  p['values'][i] + ': ' + p['values_desc'][i] + os.linesep
                    tooltip.strip(os.linesep)
                if tooltip:
                    title_txt.SetToolTipString(tooltip)

            if p ==  first_param:
                if p.has_key('wxId') and len(p['wxId']) > 0:
                    win = self.FindWindowById(p['wxId'][0])
                    win.SetFocus()
        
        #
        # set widget relations for OnUpdateSelection
        #
        pMap = None
        pLayer = []
        pDriver = None
        pDatabase = None
        pTable = None
        pColumn = []
        pGroup = None
        pSubGroup = None
        pDbase = None
        pLocation = None
        pMapset = None
        for p in self.task.params:
            if p.get('gisprompt', False) ==  False:
                continue
            
            guidep = p.get('guidependency', '')
            
            if guidep:
                # fixed options dependency defined
                options = guidep.split(',')
                for opt in options:
                    pOpt = self.task.get_param(opt, element = 'name', raiseError = False)
                    if id:
                        if not p.has_key('wxId-bind'):
                            p['wxId-bind'] = list()
                        p['wxId-bind'] +=  pOpt['wxId']
                continue
            
            prompt = p.get('element', '')
            if prompt in ('cell', 'vector'):
                name = p.get('name', '')
                if name in ('map', 'input'):
                    pMap = p
            elif prompt ==  'layer':
                pLayer.append(p)
            elif prompt ==  'dbcolumn':
                pColumn.append(p)
            elif prompt ==  'dbdriver':
                pDriver = p
            elif prompt ==  'dbname':
                pDatabase = p
            elif prompt ==  'dbtable':
                pTable = p
            elif prompt ==  'group':
                pGroup = p
            elif prompt ==  'subgroup':
                pSubGroup = p
            elif prompt ==  'dbase':
                pDbase = p
            elif prompt ==  'location':
                pLocation = p
            elif prompt ==  'mapset':
                pMapset = p
        
        # collect ids
        pColumnIds = []
        for p in pColumn:
            pColumnIds +=  p['wxId']
        pLayerIds = []
        for p in pLayer:
            pLayerIds +=  p['wxId']
        
        # set wxId-bindings
        if pMap:
            pMap['wxId-bind'] = copy.copy(pColumnIds)
            if pLayer:
                pMap['wxId-bind'] +=  pLayerIds
        if pLayer:
            for p in pLayer:
                p['wxId-bind'] = copy.copy(pColumnIds)
        
        if pDriver and pTable:
            pDriver['wxId-bind'] = pTable['wxId']

        if pDatabase and pTable:
            pDatabase['wxId-bind'] = pTable['wxId']

        if pTable and pColumnIds:
            pTable['wxId-bind'] = pColumnIds
        
        if pGroup and pSubGroup:
            pGroup['wxId-bind'] = pSubGroup['wxId']

        if pDbase and pLocation:
            pDbase['wxId-bind'] = pLocation['wxId']

        if pLocation and pMapset:
            pLocation['wxId-bind'] = pMapset['wxId']
        
        if pLocation and pMapset and pMap:
            pLocation['wxId-bind'] +=  pMap['wxId']
            pMapset['wxId-bind'] = pMap['wxId']
        
	#
	# determine panel size
	#
        maxsizes = (0, 0)
        for section in sections:
            tab[section].SetSizer(tabsizer[section])
            tabsizer[section].Fit(tab[section])
            tab[section].Layout()
            minsecsizes = tabsizer[section].GetSize()
            maxsizes = map(lambda x: max(maxsizes[x], minsecsizes[x]), (0, 1))

        # TODO: be less arbitrary with these 600
        self.panelMinHeight = 100
        self.constrained_size = (min(600, maxsizes[0]) + 25, min(400, maxsizes[1]) + 25)
        for section in sections:
            tab[section].SetMinSize((self.constrained_size[0], self.panelMinHeight))
        
        if self.manual_tab.IsLoaded():
            self.manual_tab.SetMinSize((self.constrained_size[0], self.panelMinHeight))
        
        self.SetSizer(panelsizer)
        panelsizer.Fit(self.notebook)
        
        self.Bind(EVT_DIALOG_UPDATE, self.OnUpdateDialog)

    def _getValue(self, p):
        """!Get value or default value of given parameter

        @param p parameter directory
        """
        if p.get('value', '') !=  '':
            return p['value']
        return p.get('default', '')
        
    def OnFileText(self, event):
        """File input interactively entered"""
        text = event.GetString()
        p = self.task.get_param(value = event.GetId(), element = 'wxId', raiseError = False)
        if not p:
            return # should not happen
        win = self.FindWindowById(p['wxId'][0])
        if text:
            filename = win.GetValue()
            if not filename:
                # outFile = tempfile.NamedTemporaryFile(mode = 'w+b')
                filename = grass.tempfile()
                win.SetValue(filename)
                
            f = open(filename, "w")
            try:
                f.write(text)
            finally:
                f.close()
        else:
            win.SetValue('')
        
    def OnVectorFormat(self, event):
        """!Change vector format (native / ogr)"""
        sel = event.GetSelection()
        idEvent = event.GetId()
        p = self.task.get_param(value = idEvent, element = 'wxId', raiseError = False)
        if not p:
            return # should not happen
        
        # detect windows
        winNative = None
        winOgr = None
        for id in p['wxId']:
            if id ==  idEvent:
                continue
            name = self.FindWindowById(id).GetName()
            if name ==  'Select':
                winNative = self.FindWindowById(id + 1)  # fix the mystery (also in nviz_tools.py)
            elif name ==  'OgrSelect':
                winOgr = self.FindWindowById(id)
        
        # enable / disable widgets & update values
        rbox = self.FindWindowByName('VectorFormat')
        self.hsizer.Remove(rbox)
        if sel ==  0:   # -> native
            winOgr.Hide()
            self.hsizer.Remove(winOgr)
            
            self.hsizer.Add(item = winNative,
                            flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_TOP,
                            border = 5)
            winNative.Show()
            p['value'] = winNative.GetValue()
        
        elif sel ==  1: # -> OGR
            sizer = wx.BoxSizer(wx.VERTICAL)
            
            winNative.Hide()
            self.hsizer.Remove(winNative)

            sizer.Add(item = winOgr)
            winOgr.Show()
            p['value'] = winOgr.GetDsn()
            
            self.hsizer.Add(item = sizer,
                            flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_TOP,
                            border = 5)
        
        self.hsizer.Add(item = rbox,
                        flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT |
                        wx.RIGHT | wx.ALIGN_TOP,
                        border = 5)
        
        self.hsizer.Layout()
        self.Layout()
        self.OnUpdateValues()
        self.OnUpdateSelection(event)
        
    def OnUpdateDialog(self, event):
        for fn, kwargs in event.data.iteritems():
            fn(**kwargs)
        
        self.parent.updateValuesHook()
        
    def OnVerbosity(self, event):
        """!Verbosity level changed"""
        verbose = self.FindWindowById(self.task.get_flag('verbose')['wxId'][0])
        quiet = self.FindWindowById(self.task.get_flag('quiet')['wxId'][0])
        if event.IsChecked():
            if event.GetId() ==  verbose.GetId():
                if quiet.IsChecked():
                    quiet.SetValue(False)
                    self.task.get_flag('quiet')['value'] = False
            else:
                if verbose.IsChecked():
                    verbose.SetValue(False)
                    self.task.get_flag('verbose')['value'] = False

        event.Skip()

    def OnPageChange(self, event):
        if not event:
            sel = self.notebook.GetSelection()
        else:
            sel = event.GetSelection()

        if hasattr(self, "manual_tab_id") and \
                sel ==  self.manual_tab_id:
            # calling LoadPage() is strangely time-consuming (only first call)
            # FIXME: move to helpPage.__init__()
            if not self.manual_tab.IsLoaded():
                wx.Yield()
                self.manual_tab.LoadPage()

        self.Layout()

    def OnColorChange(self, event):
        myId = event.GetId()
        for p in self.task.params:
            if 'wxId' in p and myId in p['wxId']:
                has_button = p['wxId'][1] is not None
                if has_button and wx.FindWindowById(p['wxId'][1]).GetValue() ==  True:
                    p[ 'value' ] = 'none'
                else:
                    colorchooser = wx.FindWindowById(p['wxId'][0])
                    new_color = colorchooser.GetValue()[:]
                    # This is weird: new_color is a 4-tuple and new_color[:] is a 3-tuple
                    # under wx2.8.1
                    new_label = rgb2str.get(new_color, ':'.join(map(str,new_color)))
                    colorchooser.SetLabel(new_label)
                    colorchooser.SetColour(new_color)
                    colorchooser.Refresh()
                    p[ 'value' ] = colorchooser.GetLabel()
        self.OnUpdateValues()

    def OnUpdateValues(self, event = None):
        """!If we were part of a richer interface, report back the
        current command being built.

        This method should be set by the parent of this panel if
        needed. It's a hook, actually.  Beware of what is 'self' in
        the method def, though. It will be called with no arguments.
        """
        pass

    def OnCheckBoxMulti(self, event):
        """!Fill the values as a ','-separated string according to
        current status of the checkboxes.
        """
        me = event.GetId()
        theParam = None
        for p in self.task.params:
            if 'wxId' in p and me in p['wxId']:
                theParam = p
                myIndex = p['wxId'].index(me)

        # Unpack current value list
        currentValues = {}
        for isThere in theParam.get('value', '').split(','):
            currentValues[isThere] = 1
        theValue = theParam['values'][myIndex]

        if event.Checked():
            currentValues[ theValue ] = 1
        else:
            del currentValues[ theValue ]

        # Keep the original order, so that some defaults may be recovered
        currentValueList = [] 
        for v in theParam['values']:
            if currentValues.has_key(v):
                currentValueList.append(v)

        # Pack it back
        theParam['value'] = ','.join(currentValueList)

        self.OnUpdateValues()

    def OnSetValue(self, event):
        """!Retrieve the widget value and set the task value field
        accordingly.

        Use for widgets that have a proper GetValue() method, i.e. not
        for selectors.
        """
        myId = event.GetId()
        me = wx.FindWindowById(myId)
        name = me.GetName()

        found = False
        for porf in self.task.params + self.task.flags:
            if not porf.has_key('wxId'):
                continue
            if myId in porf['wxId']:
                found = True
                break
        
        if not found:
            return
        
        if name in ('LayerSelect', 'DriverSelect', 'TableSelect',
                    'LocationSelect', 'MapsetSelect', 'ProjSelect'):
            porf['value'] = me.GetStringSelection()
        elif name ==  'GdalSelect':
            porf['value'] = event.dsn
        elif name ==  'ModelParam':
            porf['parameterized'] = me.IsChecked()
        else:
            porf['value'] = me.GetValue()
        
        self.OnUpdateValues(event)
        
        event.Skip()
        
    def OnUpdateSelection(self, event):
        """!Update dialog (layers, tables, columns, etc.)
        """
        if not hasattr(self.parent, "updateThread"):
            if event:
                event.Skip()
            return
        if event:
            self.parent.updateThread.Update(UpdateDialog,
                                            self,
                                            event,
                                            event.GetId(),
                                            self.task)
        else:
            self.parent.updateThread.Update(UpdateDialog,
                                            self,
                                            None,
                                            None,
                                            self.task)
            
    def createCmd(self, ignoreErrors = False):
        """!Produce a command line string (list) or feeding into GRASS.

        If ignoreErrors == True then it will return whatever has been
        built so far, even though it would not be a correct command
        for GRASS.
        """
        try:
            cmd = self.task.getCmd(ignoreErrors = ignoreErrors)
        except ValueError, err:
            dlg = wx.MessageDialog(parent = self,
                                   message = unicode(err),
                                   caption = _("Error in %s") % self.task.name,
                                   style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
            dlg.ShowModal()
            dlg.Destroy()
            cmd = None
        
        return cmd
    
    def OnSize(self, event):
        width = event.GetSize()[0]
        fontsize = self.GetFont().GetPointSize()
        text_width = max(width / (fontsize - 3), 70)
        
        for id in self.label_id:
            win = self.FindWindowById(id)
            label = win.GetLabel()
            label_new = '\n'.join(textwrap.wrap(label, text_width))
            win.SetLabel(label_new)
            
        event.Skip()
        
def getInterfaceDescription(cmd):
    """!Returns the XML description for the GRASS cmd.

    The DTD must be located in $GISBASE/etc/grass-interface.dtd,
    otherwise the parser will not succeed.

    @param cmd command (name of GRASS module)
    """
    try:
        cmdout, cmderr = grass.Popen([cmd, '--interface-description'], stdout = grass.PIPE,
                                     stderr = grass.PIPE).communicate()
    except OSError, e:
        raise gcmd.GException, _("Unable to fetch interface description for command '%s'. "
                                 "Details: %s") % (cmd, e.value)
    if cmderr and cmderr[:7] != 'WARNING':
        raise gcmd.GException, _("Unable to fetch interface description for command '%s'. "
                                 "Details: %s") % (cmd, cmderr)
    
    return cmdout.replace('grass-interface.dtd', os.path.join(globalvar.ETCDIR, 'grass-interface.dtd'))
    
class GrassGUIApp(wx.App):
    """!Stand-alone GRASS command GUI
    """
    def __init__(self, grass_task):
        self.grass_task = grass_task
        wx.App.__init__(self, False)
        
    def OnInit(self):
        msg = self.grass_task.get_error_msg()
        if msg:
            gcmd.GError(msg + '\n\nTry to set up GRASS_ADDON_PATH variable.')
            return True
        
        self.mf = mainFrame(parent = None, ID = wx.ID_ANY, task_description = self.grass_task)
        self.mf.CentreOnScreen()
        self.mf.Show(True)
        self.SetTopWindow(self.mf)
        
        return True

class GUI:
    def __init__(self, parent = None, show = True, modal = False,
                 centreOnParent = False, checkError = False):
        """!Parses GRASS commands when module is imported and used from
        Layer Manager.
        """
        self.parent = parent
        self.show   = show
        self.modal  = modal
        self.centreOnParent = centreOnParent
        self.checkError     = checkError
        
        self.grass_task = None
        self.cmd = list()
        
        global _ignoreBlackList
        _ignoreBlackList = False if self.parent else True
        
    def GetCmd(self):
        """!Get validated command"""
        return self.cmd
    
    def ParseInterface(self, cmd, parser = processTask):
        """!Parse interface

        @param cmd command to be parsed (given as list)
        """
        # enc = locale.getdefaultlocale()[1]
        # if enc and enc.lower() not in ("utf8", "utf-8"):
        #     tree = etree.fromstring(getInterfaceDescription(cmd[0]).decode(enc).encode("utf-8"))
        # else:
        tree = etree.fromstring(getInterfaceDescription(cmd[0]))
        
        return processTask(tree).GetTask()
    
    def ParseCommand(self, cmd, gmpath = None, completed = None):
        """!Parse command
        
        Note: cmd is given as list
        
        If command is given with options, return validated cmd list:
         - add key name for first parameter if not given
         - change mapname to mapname@mapset
        """
        start = time.time()
        dcmd_params = {}
        if completed ==  None:
            get_dcmd = None
            layer = None
            dcmd_params = None
        else:
            get_dcmd = completed[0]
            layer = completed[1]
            if completed[2]:
                dcmd_params.update(completed[2])
        
        # parse the interface decription
        self.grass_task = self.ParseInterface(cmd)
        
        # if layer parameters previously set, re-insert them into dialog
        if completed is not None:
            if 'params' in dcmd_params:
                self.grass_task.params = dcmd_params['params']
            if 'flags' in dcmd_params:
                self.grass_task.flags = dcmd_params['flags']
        
        err = list()
        # update parameters if needed && validate command
        if len(cmd) > 1:
            i = 0
            cmd_validated = [cmd[0]]
            for option in cmd[1:]:
                if option[0] ==  '-': # flag
                    if option[1] ==  '-':
                        self.grass_task.set_flag(option[2:], True)
                    else:
                        self.grass_task.set_flag(option[1], True)
                    cmd_validated.append(option)
                else: # parameter
                    try:
                        key, value = option.split('=', 1)
                    except:
                        if i == 0: # add key name of first parameter if not given
                            key = self.grass_task.firstParam
                            value = option
                        else:
                            raise gcmd.GException, _("Unable to parse command '%s'") % ' '.join(cmd)
                    
                    element = self.grass_task.get_param(key, raiseError = False)
                    if not element:
                        err.append(_("%(cmd)s: parameter '%(key)s' not available") % \
                                       { 'cmd' : cmd[0],
                                         'key' : key })
                        continue
                    element = element['element']
                    
                    if element in ['cell', 'vector']:
                        # mapname -> mapname@mapset
                        try:
                            name, mapset = value.split('@')
                        except ValueError:
                            mapset = grass.find_file(value, element)['mapset']
                            curr_mapset = grass.gisenv()['MAPSET']
                            if mapset and mapset !=  curr_mapset:
                                value = value + '@' + mapset
                        
                    self.grass_task.set_param(key, value)
                    cmd_validated.append(key + '=' + value)
                    i += 1
            
            # update original command list
            cmd = cmd_validated
        
        if self.show is not None:
            self.mf = mainFrame(parent = self.parent, ID = wx.ID_ANY,
                                task_description = self.grass_task,
                                get_dcmd = get_dcmd, layer = layer)
        else:
            self.mf = None
        
        if get_dcmd is not None:
            # update only propwin reference
            get_dcmd(dcmd = None, layer = layer, params = None,
                     propwin = self.mf)
        
        if self.show is not None:
            self.mf.notebookpanel.OnUpdateSelection(None)
            if self.show is True:
                if self.parent and self.centreOnParent:
                    self.mf.CentreOnParent()
                else:
                    self.mf.CenterOnScreen()
                self.mf.Show(self.show)
                self.mf.MakeModal(self.modal)
            else:
                self.mf.OnApply(None)
        
        self.cmd = cmd
        
        if self.checkError:
            return self.grass_task, err
        else:
            return self.grass_task
    
    def GetCommandInputMapParamKey(self, cmd):
        """!Get parameter key for input raster/vector map
        
        @param cmd module name
        
        @return parameter key
        @return None on failure
        """
        # parse the interface decription
        if not self.grass_task:
            tree = etree.fromstring(getInterfaceDescription(cmd))
            self.grass_task = processTask(tree).GetTask()
            
            for p in self.grass_task.params:
                if p.get('name', '') in ('input', 'map'):
                    age = p.get('age', '')
                    prompt = p.get('prompt', '')
                    element = p.get('element', '') 
                    if age ==  'old' and \
                            element in ('cell', 'grid3', 'vector') and \
                            prompt in ('raster', '3d-raster', 'vector'):
                        return p.get('name', None)
        return None

class FloatValidator(wx.PyValidator):
    """!Validator for floating-point input"""
    def __init__(self):
        wx.PyValidator.__init__(self)
        
        self.Bind(wx.EVT_TEXT, self.OnText) 
        
    def Clone(self):
        """!Clone validator"""
        return FloatValidator()

    def Validate(self):
        """Validate input"""
        textCtrl = self.GetWindow()
        text = textCtrl.GetValue()

        if text:
            try:
                float(text)
            except ValueError:
                textCtrl.SetBackgroundColour("grey")
                textCtrl.SetFocus()
                textCtrl.Refresh()
                return False
        
        sysColor = wx.SystemSettings_GetColour(wx.SYS_COLOUR_WINDOW)
        textCtrl.SetBackgroundColour(sysColor)
        
        textCtrl.Refresh()
        
        return True

    def OnText(self, event):
        """!Do validation"""
        self.Validate()
        
        event.Skip()
        
    def TransferToWindow(self):
        return True # Prevent wxDialog from complaining.
    
    def TransferFromWindow(self):
        return True # Prevent wxDialog from complaining.
     
if __name__ ==  "__main__":

    if len(sys.argv) ==  1:
        sys.exit(_("usage: %s <grass command>") % sys.argv[0])
    if sys.argv[1] !=  'test':
        q = wx.LogNull()
        cmd = shlex.split(sys.argv[1])
        task = grassTask(cmd[0])
        task.set_options(cmd[1:])
        app = GrassGUIApp(task)
        app.MainLoop()
    else: #Test
        # Test grassTask from within a GRASS session
        if os.getenv("GISBASE") is not None:
            task = grassTask("d.vect")
            task.get_param('map')['value'] = "map_name"
            task.get_flag('v')['value'] = True
            task.get_param('layer')['value'] = 1
            task.get_param('bcolor')['value'] = "red"
            assert ' '.join(task.getCmd()) ==  "d.vect -v map = map_name layer = 1 bcolor = red"
        # Test interface building with handmade grassTask,
        # possibly outside of a GRASS session.
        task = grassTask()
        task.name = "TestTask"
        task.description = "This is an artificial grassTask() object intended for testing purposes."
        task.keywords = ["grass","test","task"]
        task.params = [
            {
            "name" : "text",
            "description" : "Descriptions go into tooltips if labels are present, like this one",
            "label" : "Enter some text",
            },{
            "name" : "hidden_text",
            "description" : "This text should not appear in the form",
            "hidden" : True
            },{
            "name" : "text_default",
            "description" : "Enter text to override the default",
            "default" : "default text"
            },{
            "name" : "text_prefilled",
            "description" : "You should see a friendly welcome message here",
            "value" : "hello, world"
            },{
            "name" : "plain_color",
            "description" : "This is a plain color, and it is a compulsory parameter",
            "required" : False,
            "gisprompt" : True,
            "prompt" : "color"
            },{
            "name" : "transparent_color",
            "description" : "This color becomes transparent when set to none",
            "guisection" : "tab",
            "gisprompt" : True,
            "prompt" : "color"
            },{
            "name" : "multi",
            "description" : "A multiple selection",
            'default': u'red,green,blue',
            'gisprompt': False,
            'guisection': 'tab',
            'multiple': u'yes',
            'type': u'string',
            'value': '',
            'values': ['red', 'green', u'yellow', u'blue', u'purple', u'other']
            },{
            "name" : "single",
            "description" : "A single multiple-choice selection",
            'values': ['red', 'green', u'yellow', u'blue', u'purple', u'other'],
            "guisection" : "tab"
            },{
            "name" : "large_multi",
            "description" : "A large multiple selection",
            "gisprompt" : False,
            "multiple" : "yes",
            # values must be an array of strings
            "values" : str2rgb.keys() + map(str, str2rgb.values())
            },{
            "name" : "a_file",
            "description" : "A file selector",
            "gisprompt" : True,
            "element" : "file"
            }
            ]
        task.flags = [
            {
            "name" : "a",
            "description" : "Some flag, will appear in Main since it is required",
            "required" : True
            },{
            "name" : "b",
            "description" : "pre-filled flag, will appear in options since it is not required",
            "value" : True
            },{
            "name" : "hidden_flag",
            "description" : "hidden flag, should not be changeable",
            "hidden" : "yes",
            "value" : True
            }
            ]
        q = wx.LogNull()
        GrassGUIApp(task).MainLoop()

