# -*- coding: utf-8 -*-
"""
Created on Tue Apr  2 18:41:27 2013

@author: pietro
"""

from __future__ import print_function
import subprocess
from itertools import izip_longest
from xml.etree.ElementTree import fromstring


from grass.pygrass.errors import GrassError, ParameterError
from parameter import Parameter
from flag import Flag
from typedict import TypeDict
from read import GETFROMTAG, DOC


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
        try:
            # call the command with --interface-description
            get_cmd_xml = subprocess.Popen([cmd, "--interface-description"],
                                           stdout=subprocess.PIPE)
        except OSError:
            str_err = "Module %r not found, please check that the module exist"
            raise GrassError(str_err % self.name)
        # get the xml of the module
        self.xml = get_cmd_xml.communicate()[0]
        # transform and parse the xml into an Element class:
        # http://docs.python.org/library/xml.etree.elementtree.html
        tree = fromstring(self.xml)

        for e in tree:
            if e.tag not in ('parameter', 'flag'):
                self.__setattr__(e.tag, GETFROMTAG[e.tag](e))

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
        self.flags = TypeDict(Flag)
        for flag in flags_list:
            self.flags[flag.name] = flag

        #
        # Add new attributes to the class
        #
        self.run_ = True
        self.finish_ = True
        self.env_ = None
        self.stdin_ = None
        self.stdin = None
        self.stdout_ = None
        self.stderr_ = None
        diz = {'name': 'stdin', 'required': False,
               'multiple': False, 'type': 'all',
               'value': None}
        self.inputs['stdin'] = Parameter(diz=diz)
        diz['name'] = 'stdout'
        self.outputs['stdout'] = Parameter(diz=diz)
        diz['name'] = 'stderr'
        self.outputs['stderr'] = Parameter(diz=diz)
        self.popen = None

        if args or kargs:
            self.__call__(*args, **kargs)

    def __call__(self, *args, **kargs):
        if not args and not kargs:
            self.run()
            return
        #
        # check for extra kargs, set attribute and remove from dictionary
        #
        if 'flags' in kargs:
            for flg in kargs['flags']:
                self.flags[flg].value = True
            del(kargs['flags'])
        if 'run_' in kargs:
            self.run_ = kargs['run_']
            del(kargs['run_'])
        if 'stdin_' in kargs:
            self.inputs['stdin'].value = kargs['stdin_']
            del(kargs['stdin_'])
        if 'stdout_' in kargs:
            self.outputs['stdout'].value = kargs['stdout_']
            del(kargs['stdout_'])
        if 'stderr_' in kargs:
            self.outputs['stderr'].value = kargs['stderr_']
            del(kargs['stderr_'])
        if 'env_' in kargs:
            self.env_ = kargs['env_']
            del(kargs['env_'])
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
            elif key in self.flags:
                # we need to add this, because some parameters (overwrite,
                # verbose and quiet) work like parameters
                self.flags[key].value = val
            else:
                raise ParameterError('%s is not a valid parameter.' % key)

        #
        # check reqire parameters
        #
        for par in self.required:
            if par.value is None:
                raise ParameterError(
                    "Required parameter <%s> not set." % par.name)

        #
        # check if execute
        #
        if self.run_:
            self.run()

    def get_bash(self):
        return ' '.join(self.make_cmd())

    def get_python(self):
        prefix = self.name.split('.')[0]
        name = '_'.join(self.name.split('.')[1:])
        params = ', '.join([par.get_python() for par in self.params_list
                           if par.get_python() != ''])
        special = ', '.join([flg.get_python()
                             for flg in self.flags.values()
                             if flg.special and flg.get_python() != ''])
        #     pre name par flg special
        if self.flags and special:
            return "%s.%s(%s, flags=%r, %s)" % (prefix, name, params,
                                                self.flags, special)
        elif self.flags:
            return "%s.%s(%s, flags=%r)" % (prefix, name, params, self.flags)
        elif special:
            return "%s.%s(%s, %s)" % (prefix, name, params, special)
        else:
            return "%s.%s(%s)" % (prefix, name, params)

    def __str__(self):
        return ' '.join(self.make_cmd())

    def __repr__(self):
        return "Module(%r)" % self.name

    @property
    def __doc__(self):
        """{cmd_name}({cmd_params})
        """
        head = DOC['head'].format(cmd_name=self.name,
             cmd_params=('\n' +  # go to a new line
             # give space under the function name
             (' ' * (len(self.name) + 1))).join([', '.join(
             # transform each parameter in string
             [str(param) for param in line if param is not None])
             # make a list of parameters with only 3 param per line
             for line in izip_longest(*[iter(self.params_list)] * 3)]),)
        params = '\n'.join([par.__doc__ for par in self.params_list])
        flags = self.flags.__doc__
        return '\n'.join([head, params, DOC['flag_head'], flags, DOC['foot']])

    def get_dict(self):
        dic = {}
        dic['name'] = self.name
        dic['inputs'] = [(k, v.value) for k, v in self.inputs.items()
                         if v.value]
        dic['outputs'] = [(k, v.value) for k, v in self.outputs.items()
                          if v.value]
        dic['flags'] = [flg for flg in self.flags if self.flags[flg].value]
        return dic

    def make_cmd(self):
        args = [self.name, ]
        for par in self.params_list:
            if par.value is not None:
                args.append(str(par))
        for flg in self.flags:
            if self.flags[flg].value:
                args.append(str(self.flags[flg]))
        return args

    def run(self, node=None):
        if self.inputs['stdin'].value:
            self.stdin = self.inputs['stdin'].value
            self.stdin_ = subprocess.PIPE
        if self.outputs['stdout'].value:
            self.stdout_ = self.outputs['stdout'].value
        if self.outputs['stderr'].value:
            self.stderr_ = self.outputs['stderr'].value
        cmd = self.make_cmd()
        self.popen = subprocess.Popen(cmd,
                                      stdin=self.stdin_,
                                      stdout=self.stdout_,
                                      stderr=self.stderr_,
                                      env=self.env_)
        if self.finish_:
            self.popen.wait()

        stdout, stderr = self.popen.communicate(input=self.stdin)
        self.outputs['stdout'].value = stdout if stdout else ''
        self.outputs['stderr'].value = stderr if stderr else ''
