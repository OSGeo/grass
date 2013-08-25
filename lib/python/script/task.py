"""!@package grass.script.task

@brief GRASS Python scripting module (task)

Get interface description of GRASS commands

Based on gui/wxpython/gui_modules/menuform.py

Usage:

@code
from grass.script import task as gtask

gtask.command_info('r.info')
...
@endcode

(C) 2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import types
import string
try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree # Python <= 2.4

from core import *

class grassTask:
    """!This class holds the structures needed for filling by the
    parser

    Parameter blackList is a dictionary with fixed structure, eg.

    @code
    blackList = {'items' : {'d.legend' : { 'flags' : ['m'],
                                           'params' : [] }},
                 'enabled': True}
    @endcode
    
    @param path full path
    @param blackList hide some options in the GUI (dictionary)
    """
    def __init__(self, path = None, blackList = None):
        self.path = path
        self.name = _('unknown')
        self.params = list()
        self.description = ''
        self.label = ''
        self.flags = list()
        self.keywords = list()
        self.errorMsg = ''
        self.firstParam = None
        if blackList:
            self.blackList = blackList
        else:
            self.blackList = { 'enabled' : False, 'items' : {} }
        
        if path is not None:
            try:
                processTask(tree = etree.fromstring(get_interface_description(path)),
                            task = self)
            except ScriptError, e:
                self.errorMsg = e.value
            
            self.define_first()
        
    def define_first(self):
        """!Define first parameter

        @return name of first parameter
        """
        if len(self.params) > 0:
            self.firstParam = self.params[0]['name']
        
        return self.firstParam
    
    def get_error_msg(self):
        """!Get error message ('' for no error)
        """
        return self.errorMsg
    
    def get_name(self):
        """!Get task name
        """
        if sys.platform == 'win32':
            name, ext = os.path.splitext(self.name)
            if ext in ('.py', '.sh'):
                return name
            else:
                return self.name
        
        return self.name

    def get_description(self, full = True):
        """!Get module's description

        @param full True for label + desc
        """
        if self.label:
            if full:
                return self.label + ' ' + self.description
            else:
                return self.label
        else:
            return self.description

    def get_keywords(self):
        """!Get module's keywords
        """
        return self.keywords
    
    def get_list_params(self, element = 'name'):
        """!Get list of parameters

        @param element element name
        """
        params = []
        for p in self.params:
            params.append(p[element])
        
        return params

    def get_list_flags(self, element = 'name'):
        """!Get list of flags

        @param element element name
        """
        flags = []
        for p in self.flags:
            flags.append(p[element])
        
        return flags
    
    def get_param(self, value, element = 'name', raiseError = True):
        """!Find and return a param by name

        @param value param's value
        @param element element name
        @param raiseError True for raise on error
        """
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

    def get_flag(self, aFlag):
        """!Find and return a flag by name

        Raises ValueError when the flag is not found.

        @param aFlag name of the flag
        """
        for f in self.flags:
            if f['name'] ==  aFlag:
                return f
        raise ValueError, _("Flag not found: %s") % aFlag

    def get_cmd_error(self):
        """!Get error string produced by get_cmd(ignoreErrors = False)
        
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
    
    def get_cmd(self, ignoreErrors = False, ignoreRequired = False, ignoreDefault = True):
        """!Produce an array of command name and arguments for feeding
        into some execve-like command processor.

        @param ignoreErrors True to return whatever has been built so
        far, even though it would not be a correct command for GRASS
        @param ignoreRequired True to ignore required flags, otherwise
        '@<required@>' is shown
        @param ignoreDefault True to ignore parameters with default values
        """
        cmd = [self.get_name()]
        
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
            if p.get('value', '') ==  '' and p.get('required', False):
                if p.get('default', '') !=  '':
                    cmd +=  [ '%s=%s' % (p['name'], p['default']) ]
                elif ignoreErrors and not suppress_required and not ignoreRequired:
                    cmd +=  [ '%s=%s' % (p['name'], _('<required>')) ]
            elif p.get('value', '') ==  '' and p.get('default', '') != '' and not ignoreDefault:
                cmd +=  [ '%s=%s' % (p['name'], p['default']) ]
            elif p.get('value', '') !=  '' and \
                    (p['value'] !=  p.get('default', '') or not ignoreDefault):
                # output only values that have been set, and different from defaults
                cmd +=  [ '%s=%s' % (p['name'], p['value']) ]
        
        errList = self.get_cmd_error()
        if ignoreErrors is False and errList:
            raise ValueError, '\n'.join(errList)
        
        return cmd

    def get_options(self):
        """!Get options
        """
        return { 'flags'  : self.flags,
                 'params' : self.params }
    
    def has_required(self):
        """!Check if command has at least one required paramater
        """
        for p in self.params:
            if p.get('required', False):
                return True
        
        return False
    
    def set_param(self, aParam, aValue, element = 'value'):
        """!Set param value/values.
        """
        try:
            param = self.get_param(aParam)
        except ValueError:
            return
        
        param[element] = aValue

    def set_flag(self, aFlag, aValue, element = 'value'):
        """!Enable / disable flag.
        """
        try:
            param = self.get_flag(aFlag)
        except ValueError:
            return
        
        param[element] = aValue

    def set_options(self, opts):
        """!Set flags and parameters

        @param opts list of flags and parameters"""
        for opt in opts:
            if opt[0] ==  '-': # flag
                self.set_flag(opt.lstrip('-'), True)
            else: # parameter
                key, value = opt.split('=', 1)
                self.set_param(key, value)
        
class processTask:
    """!A ElementTree handler for the --interface-description output,
    as defined in grass-interface.dtd. Extend or modify this and the
    DTD if the XML output of GRASS' parser is extended or modified.

    @param tree root tree node
    @param task grassTask instance or None
    @param blackList list of flags/params to hide
    
    @return grassTask instance
    """
    def __init__(self, tree, task = None, blackList = None):
        if task:
            self.task = task
        else:
            self.task = grassTask()
        if blackList:
            self.task.blackList = blackList
        
        self.root = tree
        
        self._process_module()
        self._process_params()
        self._process_flags()
        self.task.define_first()
        
    def _process_module(self):
        """!Process module description
        """
        self.task.name = self.root.get('name', default = 'unknown')
        
        # keywords
        for keyword in self._get_node_text(self.root, 'keywords').split(','):
            self.task.keywords.append(keyword.strip())
        
        self.task.label       = self._get_node_text(self.root, 'label')
        self.task.description = self._get_node_text(self.root, 'description')
        
    def _process_params(self):
        """!Process parameters
        """
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
                    values.append(self._get_node_text(pv, 'name'))
                    desc = self._get_node_text(pv, 'description')
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
            
            if self.task.blackList['enabled'] and \
                    self.task.name in self.task.blackList['items'] and \
                    p.get('name') in self.task.blackList['items'][self.task.name].get('params', []):
                hidden = True
            else:
                hidden = False
            
            self.task.params.append( {
                "name"           : p.get('name'),
                "type"           : p.get('type'),
                "required"       : required,
                "multiple"       : multiple,
                "label"          : self._get_node_text(p, 'label'),
                "description"    : self._get_node_text(p, 'description'),
                'gisprompt'      : gisprompt,
                'age'            : age,
                'element'        : element,
                'prompt'         : prompt,
                "guisection"     : self._get_node_text(p, 'guisection'),
                "guidependency"  : self._get_node_text(p, 'guidependency'),
                "default"        : self._get_node_text(p, 'default'),
                "values"         : values,
                "values_desc"    : values_desc,
                "value"          : '',
                "key_desc"       : key_desc,
                "hidden"         : hidden
                })
            
    def _process_flags(self):
        """!Process flags
        """
        for p in self.root.findall('flag'):
            if self.task.blackList['enabled'] and \
                    self.task.name in self.task.blackList['items'] and \
                    p.get('name') in self.task.blackList['items'][self.task.name].get('flags', []):
                hidden = True
            else:
                hidden = False
            
            if p.find('suppress_required') is not None:
                suppress_required = True
            else:
                suppress_required = False
            
            self.task.flags.append( {
                    "name"              : p.get('name'),
                    "label"             : self._get_node_text(p, 'label'),
                    "description"       : self._get_node_text(p, 'description'),
                    "guisection"        : self._get_node_text(p, 'guisection'),
                    "suppress_required" : suppress_required,
                    "value"             : False,
                    "hidden"            : hidden
                    } )
        
    def _get_node_text(self, node, tag, default = ''):
        """!Get node text"""
        p = node.find(tag)
        if p is not None:
            return string.join(string.split(p.text), ' ')
        
        return default
    
    def get_task(self):
        """!Get grassTask instance"""
        return self.task

def convert_xml_to_utf8(xml_text):
    # enc = locale.getdefaultlocale()[1]
    
    # modify: fetch encoding from the interface description text(xml)
    # e.g. <?xml version="1.0" encoding="GBK"?>
    pattern = re.compile('<\?xml[^>]*\Wencoding="([^"]*)"[^>]*\?>')
    m = re.match(pattern, xml_text)
    if m == None:
        return xml_text
    #
    enc = m.groups()[0]
    
    # modify: change the encoding to "utf-8", for correct parsing
    xml_text_utf8 = xml_text.decode(enc).encode("utf-8")
    p = re.compile('encoding="' + enc + '"', re.IGNORECASE)
    xml_text_utf8 = p.sub('encoding="utf-8"', xml_text_utf8)
    
    return xml_text_utf8

def get_interface_description(cmd):
    """!Returns the XML description for the GRASS cmd (force text encoding to "utf-8").

    The DTD must be located in $GISBASE/etc/grass-interface.dtd,
    otherwise the parser will not succeed.

    @param cmd command (name of GRASS module)
    """
    try:
        p = Popen([cmd, '--interface-description'], stdout = PIPE,
                  stderr = PIPE)
        cmdout, cmderr = p.communicate()
        
        # TODO: replace ugly hack bellow
        if not cmdout and sys.platform == 'win32':
            if os.path.splitext(cmd)[1] != '.py':
                cmd += '.py'
            
            if cmd == 'd.rast3d.py':
                os.chdir(os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'scripts'))
            else:
                os.chdir(os.path.join(os.getenv('GISBASE'), 'scripts'))
            p = Popen([sys.executable, cmd, '--interface-description'],
                      stdout = PIPE, stderr = PIPE)
            cmdout, cmderr = p.communicate()
        
        if p.returncode != 0:
            raise ScriptError, _("Unable to fetch interface description for command '%(cmd)s'."
                                 "\n\nDetails: %(det)s") % { 'cmd' : cmd, 'det' : decode(cmderr) }
    
    except OSError, e:
        raise ScriptError, _("Unable to fetch interface description for command '%(cmd)s'."
                             "\n\nDetails: %(det)s") % { 'cmd' : cmd, 'det' : e }
    
    # if cmderr and cmderr[:7] != 'WARNING':
    # raise ScriptError, _("Unable to fetch interface description for command '%(cmd)s'."
    # "\n\nDetails: %(det)s") % { 'cmd': cmd, 'det' : decode(cmderr) }
    
    desc = cmdout.replace('grass-interface.dtd', os.path.join(os.getenv('GISBASE'), 'etc', 'grass-interface.dtd'))
    return convert_xml_to_utf8(desc)

def parse_interface(name, parser = processTask, blackList = None):
    """!Parse interface of given GRASS module
    
    @param name name of GRASS module to be parsed
    """
    tree = etree.fromstring(get_interface_description(name))
    return parser(tree, blackList = blackList).get_task()

def command_info(cmd):
    """!Returns meta information for any GRASS command as dictionary
    with entries for description, keywords, usage, flags, and
    parameters, e.g.
    
    @code
    >>> gtask.command_info('g.tempfile')
    
    {'keywords': ['general', 'map management'],
     'params': [{'gisprompt': False, 'multiple': False, 'name': 'pid', 'guidependency': '',
                'default': '', 'age': None, 'required': True, 'value': '',
                'label': '', 'guisection': '', 'key_desc': [], 'values': [], 'values_desc': [],
                'prompt': None, 'hidden': False, 'element': None, 'type': 'integer',
                'description': 'Process id to use when naming the tempfile'}],
     'flags': [{'description': 'Verbose module output', 'value': False, 'label': '', 'guisection': '',
                'suppress_required': False, 'hidden': False, 'name': 'verbose'}, {'description': 'Quiet module output',
                'value': False, 'label': '', 'guisection': '', 'suppress_required': False, 'hidden': False, 'name': 'quiet'}],
     'description': 'Creates a temporary file and prints the file name.',
     'usage': 'g.tempfile pid=integer [--verbose] [--quiet]'
    }

    >>> gtask.command_info('v.buffer')['keywords']
    
    ['vector', 'geometry', 'buffer']
    @endcode
    """
    task = parse_interface(cmd)
    cmdinfo = {}
    
    cmdinfo['description'] = task.get_description()
    cmdinfo['keywords']    = task.get_keywords()
    cmdinfo['flags']       = flags = task.get_options()['flags']
    cmdinfo['params']      = params = task.get_options()['params']
    
    usage = task.get_name()
    flags_short = list()
    flags_long  = list()
    for f in flags:
        fname = f.get('name', 'unknown')
        if len(fname) > 1:
            flags_long.append(fname)
        else:
            flags_short.append(fname)
    
    if len(flags_short) > 1:
        usage += ' [-' + ''.join(flags_short) + ']'
    
    for p in params:
        ptype = ','.join(p.get('key_desc', []))
        if not ptype:
            ptype = p.get('type', '')
        req =  p.get('required', False)
        if not req:
           usage += ' ['
        else:
            usage += ' '
        usage += p['name'] + '=' + ptype
        if p.get('multiple', False):
            usage += '[,' + ptype + ',...]'
        if not req:
            usage += ']'
        
    for key in flags_long:
        usage += ' [--' + key + ']'
    
    cmdinfo['usage'] = usage
    
    return cmdinfo
