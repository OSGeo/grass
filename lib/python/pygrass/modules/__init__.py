# -*- coding: utf-8 -*-
"""
Created on Thu Jul 12 10:23:15 2012

@author: pietro

"""
from __future__ import print_function
import subprocess

try:
    from collections import OrderedDict
except:
    from pygrass.orderdict import OrderedDict

from itertools import izip_longest
from xml.etree.ElementTree import fromstring
import numpy as np
import grass


#
# this dictionary is used to extract the value of interest from the xml
# the lambda experssion is used to define small simple functions,
# is equivalent to: ::
#
# def f(p):
#     return p.text.strip()
#
# and then we call f(p)
#
_GETFROMTAG = {
    'description': lambda p: p.text.strip(),
    'keydesc': lambda p: p.text.strip(),
    'gisprompt': lambda p: dict(p.items()),
    'default': lambda p: p.text.strip(),
    'values': lambda p: [e.text.strip() for e in p.findall('value/name')],
    'value': lambda p: None,
    'guisection': lambda p: p.text.strip(),
    'label': lambda p: p.text.strip(),
    'suppress_required': lambda p: None,
    'keywords': lambda p: p.text.strip(),
}

_GETTYPE = {
    'string': str,
    'integer': int,
    'float': float,
    'double': float,
}


def stdout2dict(stdout, sep='=', default=None, val_type=None, vsep=None):
    """Return a dictionary where entries are separated
    by newlines and the key and value are separated by `sep' (default: `=').
    Use the grass.core.parse_key_val function

    sep: key/value separator
    default: default value to be used
    val_type: value type (None for no cast)
    vsep: vertical separator (default os.linesep)
    """
    return grass.script.core.parse_key_val(stdout, sep, default,
                                           val_type, vsep)


class ParameterError(Exception):
    pass


class FlagError(Exception):
    pass


def _element2dict(xparameter):
    diz = dict(xparameter.items())
    for p in xparameter:
        if p.tag in _GETFROMTAG:
            diz[p.tag] = _GETFROMTAG[p.tag](p)
        else:
            print('New tag: %s, ignored' % p.tag)
    return diz

# dictionary used to create docstring for the objects
_DOC = {
    #------------------------------------------------------------
    # head
    'head': """{cmd_name}({cmd_params})

Parameters
----------

""",
    #------------------------------------------------------------
    # param
    'param': """{name}: {default}{required}{multi}{ptype}
    {description}{values}""",
    #------------------------------------------------------------
    # flag_head
    'flag_head': """
Flags
------
""",
    #------------------------------------------------------------
    # flag
    'flag': """{name}: {default}
    {description}""",
    #------------------------------------------------------------
    # foot
    'foot': """
Special Parameters
------------------

The Module class have some optional parameters which are distinct using a final
underscore.

run_: True, optional
    If True execute the module.
finish_: True, optional
    If True wait untill the end of the module execution, and store the module
    outputs into stdout, stderr attributes of the class.
stdin_: PIPE,
    Set the standard input
"""}


class Parameter(object):

    def __init__(self, xparameter=None, diz=None):
        self._value = None
        diz = _element2dict(xparameter) if xparameter is not None else diz
        if diz is None:
            raise TypeError('Xparameter or diz are required')
        self.name = diz['name']
        self.required = True if diz['required'] == 'yes' else False
        self.multiple = True if diz['multiple'] == 'yes' else False
        # check the type
        if diz['type'] in _GETTYPE:
            self.type = _GETTYPE[diz['type']]
            self.typedesc = diz['type']
            self._type = _GETTYPE[diz['type']]
        else:
            raise TypeError('New type: %s, ignored' % diz['type'])

        self.description = diz['description']
        self.keydesc = diz['keydesc'] if 'keydesc' in diz else None
        self.values = [self._type(
            i) for i in diz['values']] if 'values' in diz else None
        self.default = self._type(
            diz['default']) if 'default' in diz else None
        self.guisection = diz['guisection'] if 'guisection' in diz else None
        if 'gisprompt' in diz:
            self.type = diz['gisprompt']['prompt']
            self.input = False if diz['gisprompt']['age'] == 'new' else True
        else:
            self.input = True

    def _get_value(self):
        return self._value

    def _set_value(self, value):
        if isinstance(value, list) or isinstance(value, tuple):
            if self.multiple:
                # check each value
                self._value = [
                    val for val in value if isinstance(value, self._type)]
            else:
                str_err = 'The Parameter <%s>, not support multiple inputs'
                raise TypeError(str_err % self.name)
        elif isinstance(value, self._type):
            if self.values:
                if value in self.values:
                    self._value = value
                else:
                    raise ValueError('The Parameter <%s>, must be one of: %r' %
                                     (self.name, self.values))
            else:
                self._value = value
        else:
            str_err = 'The Parameter <%s>, require: %s, get: %s instead'
            raise TypeError(str_err % (self.name, self.typedesc, type(value)))

    # here the property function is used to transform value in an attribute
    # in this case we define which function must be use to get/set the value
    value = property(fget=_get_value, fset=_set_value)

    def __str__(self):
        if isinstance(self._value, list) or isinstance(self._value, tuple):
            value = ','.join([str(v) for v in self._value])
        else:
            value = str(self._value)
        return "%s=%s" % (self.name, value)

    def __repr__(self):
        str_repr = "Parameter <%s> (required:%s, type:%s, multiple:%s)"
        return str_repr % (self.name,
                           "yes" if self.required else "no",
                           self.type if self.type in (
                           'raster', 'vector') else self.typedesc,
                           "yes" if self.multiple else "no")

    # here we use property with a decorator, in this way we mask a method as
    # a class attribute
    @property
    def __doc__(self):
        """Return the docstring of the parameter

        {name}: {default}{required}{multi}{ptype}
            {description}{values}"","""
        return _DOC['param'].format(name=self.name,
                default=repr(self.default) + ', ' if self.default else '',
                required='required, ' if self.required else 'optional, ',
                multi='multi' if self.multiple else '',
                ptype=self.typedesc, description=self.description,
                values='\n    Values: {0}'.format(', '.join([repr(val)
                                                  for val in self.values]))
                       if self.values else '')


class TypeDict(OrderedDict):
    def __init__(self, dict_type, *args, **kargs):
        self.type = dict_type
        super(TypeDict, self).__init__(*args, **kargs)

    def __setitem__(self, key, value):
        if isinstance(value, self.type):
            super(TypeDict, self).__setitem__(key, value)
        else:
            cl = repr(self.type).translate(None, "'<> ").split('.')
            str_err = 'The value: %r is not a %s object'
            raise TypeError(str_err % (value, cl[-1].title()))

    @property
    def __doc__(self):
        return '\n'.join([self.__getitem__(obj).__doc__
                          for obj in self.__iter__()])

    def __call__(self):
        return [self.__getitem__(obj) for obj in self.__iter__()]


class Flag(object):
    def __init__(self, xflag=None, diz=None):
        self.value = None
        diz = _element2dict(xflag) if xflag is not None else diz
        self.name = diz['name']
        self.special = True if self.name in (
            'verbose', 'overwrite', 'quiet', 'run') else False
        self.description = diz['description']
        self.default = diz['default'] if 'default' in diz else None
        self.guisection = diz['guisection'] if 'guisection' in diz else None

    def __str__(self):
        if self.value:
            if self.special:
                return '--%s' % self.name[0]
            else:
                return '-%s' % self.name
        else:
            return ''

    def __repr__(self):
        return "Flag <%s> (%s)" % (self.name, self.description)

    @property
    def __doc__(self):
        """
        {name}: {default}
            {description}"""
        return _DOC['flag'].format(name=self.name,
                                   default=repr(self.default),
                                   description=self.description)


class Module(object):
    """

    Python allow developers to not specify all the arguments and
    keyword arguments of a method or function.

    ::

        def f(*args):
            for arg in args:
                print arg

    therefore if we call the function like: ::

        >>> f('grass', 'gis', 'modules')
        grass
        gis
        modules

    or we can define a new list: ::

        >>> words = ['grass', 'gis', 'modules']
        >>> f(*words)
        grass
        gis
        modules

    we can do the same with keyword arguments, rewrite the above function: ::

        def f(*args, **kargs):
            for arg in args:
                print arg
            for key, value in kargs.items():
                print "%s = %r" % (key, value)

    now we can use the new function, with: ::

        >>> f('grass', 'gis', 'modules', os = 'linux', language = 'python')
        grass
        gis
        modules
        os = 'linux'
        language = 'python'

    or, as before we can, define a dictionary and give the dictionary to
    the function, like: ::

        >>> keywords = {'os' : 'linux', 'language' : 'python'}
        >>> f(*words, **keywords)
        grass
        gis
        modules
        os = 'linux'
        language = 'python'

    In the Module class we heavily use this language feature to pass arguments
    and keyword arguments to the grass module.
    """
    def __init__(self, cmd, *args, **kargs):
        self.name = cmd
        # call the command with --interface-description
        get_cmd_xml = subprocess.Popen([cmd, "--interface-description"],
                                       stdout=subprocess.PIPE)
        # get the xml of the module
        self.xml = get_cmd_xml.communicate()[0]
        # transform and parse the xml into an Element class:
        # http://docs.python.org/library/xml.etree.elementtree.html
        tree = fromstring(self.xml)

        for e in tree:
            if e.tag not in ('parameter', 'flag'):
                self.__setattr__(e.tag, _GETFROMTAG[e.tag](e))

        #
        # extract parameters from the xml
        #
        self.params_list = [Parameter(p) for p in tree.findall("parameter")]
        self.inputs = TypeDict(Parameter)
        self.outputs = TypeDict(Parameter)
        self.required = []
        # Insert parameters into input/output and required
        for par in self.params_list:
            if par.input:
                self.inputs[par.name] = par
            else:
                self.outputs[par.name] = par
            if par.required:
                self.required.append(par)

        #
        # extract flags from the xml
        #
        flags_list = [Flag(f) for f in tree.findall("flag")]
        self.flags_dict = TypeDict(Flag)
        for flag in flags_list:
            self.flags_dict[flag.name] = flag

        #
        # Add new attributes to the class
        #
        self._flags = ''
        self.run_ = True
        self.finish_ = True
        self.stdin_ = subprocess.PIPE
        self.stdout = subprocess.PIPE
        self.stderr = subprocess.PIPE
        self.popen = None

        if args or kargs:
            self.__call__(*args, **kargs)

    def _get_flags(self):
        return self._flags

    def _set_flags(self, value):
        if isinstance(value, str):
            flgs = [flg for flg in self.flags_dict
                    if not self.flags_dict[flg].special]
            # we need to check if the flag is valid, special flags are not
            # allow
            if value in flgs:
                self._flags = value
            else:
                raise ValueError('Flag not valid, valid flag are: %r' % flgs)
        else:
            raise TypeError('The flags attribute must be a string')

    flags = property(fget=_get_flags, fset=_set_flags)

    def __call__(self, *args, **kargs):
        if not args and not kargs:
            self.run()
            return
        #
        # check for extra kargs, set attribute and remove from dictionary
        #
        if 'flags' in kargs:
            self.flags = kargs['flags']
            del(kargs['flags'])
        if 'run_' in kargs:
            self.run_ = kargs['run_']
            del(kargs['run_'])
        if 'stdin_' in kargs:
            self.stdin_ = kargs['stdin_']
            del(kargs['stdin_'])
        if 'finish_' in kargs:
            self.finish_ = kargs['finish_']
            del(kargs['finish_'])

        #
        # check args
        #
        for param, arg in zip(self.params_list, args):
            param.value = arg
        for key, val in kargs.items():
            if key in self.inputs:
                self.inputs[key].value = val
            elif key in self.outputs:
                self.outputs[key].value = val
            elif key in self.flags_dict:
                # we need to add this, because some parameters (overwrite,
                # verbose and quiet) work like parameters
                self.flags_dict[key].value = val
            else:
                raise ParameterError('Parameter not found')

        #
        # check reqire parameters
        #
        for par in self.required:
            if par.value is None:
                raise ParameterError(
                    "Required parameter <%s> not set." % par.name)

        #
        # check flags parameters
        #
        if self.flags:
            # check each character in flags
            for flag in self.flags:
                if flag in self.flags_dict:
                    self.flags_dict[flag].value = True
                else:
                    raise FlagError('Flag "%s" not valid.')

        #
        # check if execute
        #
        if self.run_:
            self.run()

    def __str__(self):
        return ' '.join(self.make_cmd())

    @property
    def __doc__(self):
        """{cmd_name}({cmd_params})
        """
        head = _DOC['head'].format(cmd_name=self.name,
             cmd_params=('\n' +  # go to a new line
             # give space under the function name
             (' ' * (len(self.name) + 1))).join([', '.join(
             # transform each parameter in string
             [str(param) for param in line if param is not None])
             # make a list of parameters with only 3 param per line
             for line in izip_longest(*[iter(self.params_list)] * 3)]),)
        params = '\n'.join([par.__doc__ for par in self.params_list])
        flags = self.flags_dict.__doc__
        return '\n'.join([head, params, _DOC['flag_head'], flags])

    def make_cmd(self):
        args = [self.name, ]
        for par in self.params_list:
            if par.value is not None:
                args.append(str(par))
        for flg in self.flags_dict:
            if self.flags_dict[flg].value is not None:
                args.append(str(self.flags_dict[flg]))
        return args

    def run(self, node=None):
        cmd = self.make_cmd()
        #print(repr(cmd))
        self.popen = subprocess.Popen(cmd, stdin=self.stdin_,
                                      stdout=subprocess.PIPE,
                                      stderr=subprocess.PIPE)
        if self.finish_:
            self.popen.wait()
            self.stdout, self.stderr = self.popen.communicate()