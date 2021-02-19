# -*- coding: utf-8 -*-
from __future__ import (nested_scopes, generators, division, absolute_import,
                        with_statement, print_function, unicode_literals)
import sys
from multiprocessing import cpu_count, Process, Queue
import time
from xml.etree.ElementTree import fromstring

from grass.exceptions import CalledModuleError, GrassError, ParameterError
from grass.script.core import Popen, PIPE, use_temp_region, del_temp_region
from grass.script.utils import encode, decode
from .docstring import docstring_property
from .parameter import Parameter
from .flag import Flag
from .typedict import TypeDict
from .read import GETFROMTAG, DOC
from .env import G_debug

if sys.version_info[0] == 2:
    from itertools import izip_longest as zip_longest
else:
    from itertools import zip_longest
    unicode = str


def _get_bash(self, *args, **kargs):
    return self.get_bash()


class ParallelModuleQueue(object):
    """This class is designed to run an arbitrary number of pygrass Module or MultiModule
    processes in parallel.

    Objects of type grass.pygrass.modules.Module or
    grass.pygrass.modules.MultiModule can be put into the
    queue using put() method. When the queue is full with the maximum
    number of parallel processes it will wait for all processes to finish,
    sets the stdout and stderr of the Module object and removes it
    from the queue when its finished.

    To finish the queue before the maximum number of parallel
    processes was reached call wait() .

    This class will raise a GrassError in case a Module process exits
    with a return code other than 0.

    Processes that were run asynchronously with the MultiModule class
    will not raise a GrassError in case of failure. This must be manually checked
    by accessing finished modules by calling get_finished_modules().

    Usage:

    Check with a queue size of 3 and 5 processes

    >>> import copy
    >>> from grass.pygrass.modules import Module, ParallelModuleQueue
    >>> mapcalc_list = []

    Setting run_ to False is important, otherwise a parallel processing is not possible

    >>> mapcalc = Module("r.mapcalc", overwrite=True, run_=False)
    >>> queue = ParallelModuleQueue(nprocs=3)
    >>> for i in range(5):
    ...     new_mapcalc = copy.deepcopy(mapcalc)
    ...     mapcalc_list.append(new_mapcalc)
    ...     m = new_mapcalc(expression="test_pygrass_%i = %i"%(i, i))
    ...     queue.put(m)
    >>> queue.wait()
    >>> mapcalc_list = queue.get_finished_modules()
    >>> queue.get_num_run_procs()
    0
    >>> queue.get_max_num_procs()
    3
    >>> for mapcalc in mapcalc_list:
    ...     print(mapcalc.popen.returncode)
    0
    0
    0
    0
    0

    Check with a queue size of 8 and 5 processes

    >>> queue = ParallelModuleQueue(nprocs=8)
    >>> mapcalc_list = []
    >>> for i in range(5):
    ...     new_mapcalc = copy.deepcopy(mapcalc)
    ...     mapcalc_list.append(new_mapcalc)
    ...     m = new_mapcalc(expression="test_pygrass_%i = %i"%(i, i))
    ...     queue.put(m)
    >>> queue.wait()
    >>> mapcalc_list = queue.get_finished_modules()
    >>> queue.get_num_run_procs()
    0
    >>> queue.get_max_num_procs()
    8
    >>> for mapcalc in mapcalc_list:
    ...     print(mapcalc.popen.returncode)
    0
    0
    0
    0
    0

    Check MultiModule approach with three by two processes running in a background process

    >>> gregion = Module("g.region", flags="p", run_=False)
    >>> queue = ParallelModuleQueue(nprocs=3)
    >>> proc_list = []
    >>> for i in range(3):
    ...     new_gregion = copy.deepcopy(gregion)
    ...     proc_list.append(new_gregion)
    ...     new_mapcalc = copy.deepcopy(mapcalc)
    ...     m = new_mapcalc(expression="test_pygrass_%i = %i"%(i, i))
    ...     proc_list.append(new_mapcalc)
    ...     mm = MultiModule(module_list=[new_gregion, new_mapcalc], sync=False, set_temp_region=True)
    ...     queue.put(mm)
    >>> queue.wait()
    >>> proc_list = queue.get_finished_modules()
    >>> queue.get_num_run_procs()
    0
    >>> queue.get_max_num_procs()
    3
    >>> for proc in proc_list:
    ...     print(proc.popen.returncode)
    0
    0
    0
    0
    0
    0

    Check with a queue size of 8 and 4 processes

    >>> queue = ParallelModuleQueue(nprocs=8)
    >>> mapcalc_list = []
    >>> new_mapcalc = copy.deepcopy(mapcalc)
    >>> mapcalc_list.append(new_mapcalc)
    >>> m = new_mapcalc(expression="test_pygrass_1 =1")
    >>> queue.put(m)
    >>> queue.get_num_run_procs()
    1
    >>> new_mapcalc = copy.deepcopy(mapcalc)
    >>> mapcalc_list.append(new_mapcalc)
    >>> m = new_mapcalc(expression="test_pygrass_2 =2")
    >>> queue.put(m)
    >>> queue.get_num_run_procs()
    2
    >>> new_mapcalc = copy.deepcopy(mapcalc)
    >>> mapcalc_list.append(new_mapcalc)
    >>> m = new_mapcalc(expression="test_pygrass_3 =3")
    >>> queue.put(m)
    >>> queue.get_num_run_procs()
    3
    >>> new_mapcalc = copy.deepcopy(mapcalc)
    >>> mapcalc_list.append(new_mapcalc)
    >>> m = new_mapcalc(expression="test_pygrass_4 =4")
    >>> queue.put(m)
    >>> queue.get_num_run_procs()
    4
    >>> queue.wait()
    >>> mapcalc_list = queue.get_finished_modules()
    >>> queue.get_num_run_procs()
    0
    >>> queue.get_max_num_procs()
    8
    >>> for mapcalc in mapcalc_list:
    ...     print(mapcalc.popen.returncode)
    0
    0
    0
    0

    Check with a queue size of 3 and 4 processes

    >>> queue = ParallelModuleQueue(nprocs=3)
    >>> mapcalc_list = []
    >>> new_mapcalc = copy.deepcopy(mapcalc)
    >>> mapcalc_list.append(new_mapcalc)
    >>> m = new_mapcalc(expression="test_pygrass_1 =1")
    >>> queue.put(m)
    >>> queue.get_num_run_procs()
    1
    >>> new_mapcalc = copy.deepcopy(mapcalc)
    >>> mapcalc_list.append(new_mapcalc)
    >>> m = new_mapcalc(expression="test_pygrass_2 =2")
    >>> queue.put(m)
    >>> queue.get_num_run_procs()
    2
    >>> new_mapcalc = copy.deepcopy(mapcalc)
    >>> mapcalc_list.append(new_mapcalc)
    >>> m = new_mapcalc(expression="test_pygrass_3 =3")
    >>> queue.put(m) # Now it will wait until all procs finish and set the counter back to 0
    >>> queue.get_num_run_procs()
    0
    >>> new_mapcalc = copy.deepcopy(mapcalc)
    >>> mapcalc_list.append(new_mapcalc)
    >>> m = new_mapcalc(expression="test_pygrass_%i = %i"%(i, i))
    >>> queue.put(m)
    >>> queue.get_num_run_procs()
    1
    >>> queue.wait()
    >>> mapcalc_list = queue.get_finished_modules()
    >>> queue.get_num_run_procs()
    0
    >>> queue.get_max_num_procs()
    3
    >>> for mapcalc in mapcalc_list:
    ...     print(mapcalc.popen.returncode)
    0
    0
    0
    0

    """

    def __init__(self, nprocs=1):
        """Constructor

        :param nprocs: The maximum number of Module processes that
                       can be run in parallel, default is 1, if None
                       then use all the available CPUs.
        :type nprocs: int
        """
        nprocs = int(nprocs) if nprocs else cpu_count()
        self._num_procs = nprocs
        self._list = nprocs * [None]
        self._proc_count = 0
        self._finished_modules = []  # Store all processed modules in a list

    def put(self, module):
        """Put the next Module or MultiModule object in the queue

        To run the Module objects in parallel the run\_ and finish\_ options
        of the Module must be set to False.

        :param module: a preconfigured Module or MultiModule object that were configured
                       with run\_ and finish\_ set to False,
        :type module: Module or MultiModule object
        """
        self._list[self._proc_count] = module
        # Force that finish is False, otherwise the execution
        # will not be parallel
        self._list[self._proc_count].finish_ = False
        self._list[self._proc_count].run()
        self._proc_count += 1

        if self._proc_count == self._num_procs:
            self.wait()

    def get(self, num):
        """Get a Module object or list of Module objects from the queue

        :param num: the number of the object in queue
        :type num: int
        :returns: the Module object or list of Module objects or None if num is not in the queue
        """
        if num < self._num_procs:
            return self._list[num]
        return None

    def get_num_run_procs(self):
        """Get the number of Module processes that are in the queue running
        or finished

        :returns: the number fo Module processes running/finished in the queue
        """
        return self._proc_count

    def get_max_num_procs(self):
        """Return the maximum number of parallel Module processes

        :returns: the maximum number of parallel Module processes
        """
        return self._num_procs

    def set_max_num_procs(self, nprocs):
        """Set the maximum number of Module processes that should run
        in parallel

        :param nprocs: The maximum number of Module processes that can be
                       run in parallel
        :type nprocs: int
        """
        self._num_procs = int(nprocs)
        self.wait()

    def get_finished_modules(self):
        """Return all finished processes that were run by this queue

        :return: A list of Module objects
        """
        return self._finished_modules

    def wait(self):
        """Wait for all Module processes that are in the list to finish
        and set the modules stdout and stderr output options

        :return: A list of modules that were run
        """
        for proc in self._list:
            if proc:
                if isinstance(proc, Module):
                    self._finished_modules.extend([proc.wait(),])
                else:
                    self._finished_modules.extend(proc.wait())

        self._list = self._num_procs * [None]
        self._proc_count = 0



class Module(object):
    """This class is design to wrap/run/interact with the GRASS modules.

    The class during the init phase read the XML description generate using
    the ``--interface-description`` in order to understand which parameters
    are required which optionals. ::

    >>> from grass.pygrass.modules import Module
    >>> from subprocess import PIPE
    >>> import copy

    >>> region = Module("g.region")
    >>> region.flags.p = True  # set flags
    >>> region.flags.u = True
    >>> region.flags["3"].value = True  # set numeric flags
    >>> region.get_bash()
    'g.region -p -3 -u'
    >>> new_region = copy.deepcopy(region)
    >>> new_region.inputs.res = "10"
    >>> new_region.get_bash()
    'g.region res=10 -p -3 -u'

    >>> neighbors = Module("r.neighbors")
    >>> neighbors.inputs.input = "mapA"
    >>> neighbors.outputs.output = "mapB"
    >>> neighbors.inputs.size = 5
    >>> neighbors.inputs.quantile = 0.5
    >>> neighbors.get_bash()
    'r.neighbors input=mapA method=average size=5 quantile=0.5 output=mapB'

    >>> new_neighbors1 = copy.deepcopy(neighbors)
    >>> new_neighbors1.inputs.input = "mapD"
    >>> new_neighbors1.inputs.size = 3
    >>> new_neighbors1.inputs.quantile = 0.5
    >>> new_neighbors1.get_bash()
    'r.neighbors input=mapD method=average size=3 quantile=0.5 output=mapB'

    >>> new_neighbors2 = copy.deepcopy(neighbors)
    >>> new_neighbors2(input="mapD", size=3, run_=False)
    Module('r.neighbors')
    >>> new_neighbors2.get_bash()
    'r.neighbors input=mapD method=average size=3 quantile=0.5 output=mapB'

    >>> neighbors = Module("r.neighbors")
    >>> neighbors.get_bash()
    'r.neighbors method=average size=3'

    >>> new_neighbors3 = copy.deepcopy(neighbors)
    >>> new_neighbors3(input="mapA", size=3, output="mapB", run_=False)
    Module('r.neighbors')
    >>> new_neighbors3.get_bash()
    'r.neighbors input=mapA method=average size=3 output=mapB'

    >>> mapcalc = Module("r.mapcalc", expression="test_a = 1",
    ...                  overwrite=True, run_=False)
    >>> mapcalc.run()
    Module('r.mapcalc')
    >>> mapcalc.popen.returncode
    0

    >>> mapcalc = Module("r.mapcalc", expression="test_a = 1",
    ...                  overwrite=True, run_=False, finish_=False)
    >>> mapcalc.run()
    Module('r.mapcalc')
    >>> p = mapcalc.wait()
    >>> p.popen.returncode
    0
    >>> mapcalc.run()
    Module('r.mapcalc')
    >>> p = mapcalc.wait()
    >>> p.popen.returncode
    0

    >>> colors = Module("r.colors", map="test_a", rules="-",
    ...                 run_=False, stdout_=PIPE,
    ...                 stderr_=PIPE, stdin_="1 red")
    >>> colors.run()
    Module('r.colors')
    >>> p = mapcalc.wait()
    >>> p.popen.returncode
    0
    >>> colors.inputs["stdin"].value
    '1 red'
    >>> colors.outputs["stdout"].value
    ''
    >>> colors.outputs["stderr"].value.strip()
    "Color table for raster map <test_a> set to 'rules'"

    >>> colors = Module("r.colors", map="test_a", rules="-",
    ...                 run_=False, finish_=False, stdin_=PIPE)
    >>> colors.run()
    Module('r.colors')
    >>> stdout, stderr = colors.popen.communicate(input=b"1 red")
    >>> colors.popen.returncode
    0
    >>> stdout
    >>> stderr

    >>> colors = Module("r.colors", map="test_a", rules="-",
    ...                 run_=False, finish_=False,
    ...                 stdin_=PIPE, stderr_=PIPE)
    >>> colors.run()
    Module('r.colors')
    >>> stdout, stderr = colors.popen.communicate(input=b"1 red")
    >>> colors.popen.returncode
    0
    >>> stdout
    >>> stderr.strip()
    b"Color table for raster map <test_a> set to 'rules'"

    Run a second time

    >>> colors.run()
    Module('r.colors')
    >>> stdout, stderr = colors.popen.communicate(input=b"1 blue")
    >>> colors.popen.returncode
    0
    >>> stdout
    >>> stderr.strip()
    b"Color table for raster map <test_a> set to 'rules'"

    Multiple run test

    >>> colors = Module("r.colors", map="test_a",
    ...                                            color="ryb", run_=False)
    >>> colors.get_bash()
    'r.colors map=test_a color=ryb'
    >>> colors.run()
    Module('r.colors')
    >>> colors(color="gyr")
    Module('r.colors')
    >>> colors.run()
    Module('r.colors')
    >>> colors(color="ryg")
    Module('r.colors')
    >>> colors(stderr_=PIPE)
    Module('r.colors')
    >>> colors.run()
    Module('r.colors')
    >>> print(colors.outputs["stderr"].value.strip())
    Color table for raster map <test_a> set to 'ryg'
    >>> colors(color="byg")
    Module('r.colors')
    >>> colors(stdout_=PIPE)
    Module('r.colors')
    >>> colors.run()
    Module('r.colors')
    >>> print(colors.outputs["stderr"].value.strip())
    Color table for raster map <test_a> set to 'byg'

    Often in the Module class you can find ``*args`` and ``kwargs`` annotation
    in methods, like in the __call__ method.
    Python allow developers to not specify all the arguments and
    keyword arguments of a method or function. ::

        def f(*args):
            for arg in args:
                print arg

    therefore if we call the function like:

    >>> f('grass', 'gis', 'modules')                     # doctest: +SKIP
    grass
    gis
    modules

    or we can define a new list:

    >>> words = ['grass', 'gis', 'modules']              # doctest: +SKIP
    >>> f(*words)                                        # doctest: +SKIP
    grass
    gis
    modules

    we can do the same with keyword arguments, rewrite the above function: ::

        def f(*args, **kargs):
            for arg in args:
                print arg
            for key, value in kargs.items():
                print "%s = %r" % (key, value)

    now we can use the new function, with:

    >>> f('grass', 'gis', 'modules', os = 'linux', language = 'python')
    ...                                                  # doctest: +SKIP
    grass
    gis
    modules
    os = 'linux'
    language = 'python'

    or, as before we can, define a dictionary and give the dictionary to
    the function, like:

    >>> keywords = {'os' : 'linux', 'language' : 'python'}  # doctest: +SKIP
    >>> f(*words, **keywords)                            # doctest: +SKIP
    grass
    gis
    modules
    os = 'linux'
    language = 'python'

    In the Module class we heavily use this language feature to pass arguments
    and keyword arguments to the grass module.
    """

    def __init__(self, cmd, *args, **kargs):
        if isinstance(cmd, unicode):
            self.name = str(cmd)
        elif isinstance(cmd, str):
            self.name = cmd
        else:
            raise GrassError("Problem initializing the module {s}".format(s=cmd))
        try:
            # call the command with --interface-description
            get_cmd_xml = Popen([cmd, "--interface-description"], stdout=PIPE)
        except OSError as e:
            print("OSError error({0}): {1}".format(e.errno, e.strerror))
            str_err = "Error running: `%s --interface-description`."
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
                self.required.append(par.name)

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
        self.check_ = True
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
        self.time = None
        self.start_time = None            # This variable will be set in the run() function
        self._finished = False            # This variable is set True if wait() was successfully called

        if args or kargs:
            self.__call__(*args, **kargs)
        self.__call__.__func__.__doc__ = self.__doc__

    def __call__(self, *args, **kargs):
        """Set module parameters to the class and, if run_ is True execute the
        module, therefore valid parameters are all the module parameters
        plus some extra parameters that are: run_, stdin_, stdout_, stderr_,
        env_ and finish_.
        """
        if not args and not kargs:
            self.run()
            return self

        #
        # check for extra kargs, set attribute and remove from dictionary
        #
        if 'flags' in kargs:
            for flg in kargs['flags']:
                self.flags[flg].value = True
            del(kargs['flags'])

        # set attributs
        for key in ('run_', 'env_', 'finish_', 'stdout_', 'stderr_', 'check_'):
            if key in kargs:
                setattr(self, key, kargs.pop(key))

        # set inputs
        for key in ('stdin_', ):
            if key in kargs:
                self.inputs[key[:-1]].value = kargs.pop(key)

        #
        # set/update args
        #
        for param, arg in zip(self.params_list, args):
            param.value = arg
        for key, val in kargs.items():
            key = key.strip('_')
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
        # check if execute
        #
        if self.run_:
            #
            # check reqire parameters
            #
            if self.check_:
                self.check()
            return self.run()
        return self

    def get_bash(self):
        """Return a BASH representation of the Module."""
        return ' '.join(self.make_cmd())

    def get_python(self):
        """Return a Python representation of the Module."""
        prefix = self.name.split('.')[0]
        name = '_'.join(self.name.split('.')[1:])
        params = ', '.join([par.get_python() for par in self.params_list
                           if par.get_python() != ''])
        flags = ''.join([flg.get_python()
                         for flg in self.flags.values()
                         if not flg.special and flg.get_python() != ''])
        special = ', '.join([flg.get_python()
                             for flg in self.flags.values()
                             if flg.special and flg.get_python() != ''])
        #     pre name par flg special
        if flags and special:
            return "%s.%s(%s, flags=%r, %s)" % (prefix, name, params,
                                                flags, special)
        elif flags:
            return "%s.%s(%s, flags=%r)" % (prefix, name, params, flags)
        elif special:
            return "%s.%s(%s, %s)" % (prefix, name, params, special)
        else:
            return "%s.%s(%s)" % (prefix, name, params)

    def __str__(self):
        """Return the command string that can be executed in a shell"""
        return ' '.join(self.make_cmd())

    def __repr__(self):
        return "Module(%r)" % self.name

    @docstring_property(__doc__)
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
                 for line in zip_longest(*[iter(self.params_list)] * 3)]),)
        params = '\n'.join([par.__doc__ for par in self.params_list])
        flags = self.flags.__doc__
        return '\n'.join([head, params, DOC['flag_head'], flags, DOC['foot']])

    def check(self):
        """Check the correctness of the provide parameters"""
        required = True
        for flg in self.flags.values():
            if flg and flg.suppress_required:
                required = False
        if required:
            for k in self.required:
                if ((k in self.inputs and self.inputs[k].value is None) or
                        (k in self.outputs and self.outputs[k].value is None)):
                    msg = "Required parameter <%s> not set."
                    raise ParameterError(msg % k)

    def get_dict(self):
        """Return a dictionary that includes the name, all valid
        inputs, outputs and flags
        """
        dic = {}
        dic['name'] = self.name
        dic['inputs'] = [(k, v.value) for k, v in self.inputs.items()
                         if v.value]
        dic['outputs'] = [(k, v.value) for k, v in self.outputs.items()
                          if v.value]
        dic['flags'] = [flg for flg in self.flags if self.flags[flg].value]
        return dic

    def make_cmd(self):
        """Create the command string that can be executed in a shell

        :returns: the command string
        """
        skip = ['stdin', 'stdout', 'stderr']
        args = [self.name, ]
        for key in self.inputs:
            if key not in skip and self.inputs[key].value is not None and self.inputs[key].value != '':
                args.append(self.inputs[key].get_bash())
        for key in self.outputs:
            if key not in skip and self.outputs[key].value is not None and self.outputs[key].value != '':
                args.append(self.outputs[key].get_bash())
        for flg in self.flags:
            if self.flags[flg].value:
                args.append(str(self.flags[flg]))
        return args

    def run(self):
        """Run the module
        This function will wait for the process to terminate in case
        finish_==True and sets up stdout and stderr. If finish_==False this
        function will return after starting the process. Use wait() to wait for
        the started process

        :return: A reference to this object
        """
        G_debug(1, self.get_bash())
        self._finished = False
        if self.inputs['stdin'].value:
            self.stdin = self.inputs['stdin'].value
            self.stdin_ = PIPE

        cmd = self.make_cmd()
        self.start_time = time.time()
        self.popen = Popen(cmd,
                           stdin=self.stdin_,
                           stdout=self.stdout_,
                           stderr=self.stderr_,
                           env=self.env_)

        if self.finish_ is True:
            self.wait()

        return self

    def wait(self):
        """Wait for the module to finish. Call this method if
        the run() call was performed with self.false_ = False.

        :return: A reference to this object
        """
        if self._finished is False:
            if self.stdin:
                self.stdin = encode(self.stdin)
            stdout, stderr = self.popen.communicate(input=self.stdin)
            self.outputs['stdout'].value = decode(stdout) if stdout else ''
            self.outputs['stderr'].value = decode(stderr) if stderr else ''
            self.time = time.time() - self.start_time

            self._finished = True

            if self.popen.poll():
                raise CalledModuleError(returncode=self.popen.returncode,
                                        code=self.get_bash(),
                                        module=self.name, errors=stderr)

        return self


class MultiModule(object):
    """This class is designed to run a list of modules in serial in the provided order
    within a temporary region environment.

    Module can be run in serial synchronously or asynchronously.

    - Synchronously:  When calling run() all modules will run in serial order
                      until they are finished, The run() method will return until all modules finished.
                      The modules objects can be accessed by calling get_modules() to check their return
                      values.
    - Asynchronously: When calling run() all modules will run in serial order in a background process.
                      Method run() will return after starting the modules without waiting for them to finish.
                      The user must call the wait() method to wait for the modules to finish.
                      Asynchronously called module can be optionally run in a temporary region
                      environment, hence invokeing g.region will not alter the current
                      region or the region of other MultiModule runs.

                      Note:

                          Modules run in asynchronous mode can only be accessed via the wait() method.
                          The wait() method will return all finished module objects as list.

    Objects of this class can be passed to the ParallelModuleQueue to run serial stacks
    of modules in parallel. This is meaningful if region settings must be applied
    to each parallel module run.

    >>> from grass.pygrass.modules import Module
    >>> from grass.pygrass.modules import MultiModule
    >>> from multiprocessing import Process
    >>> import copy

    Synchronous module run

    >>> region_1 = Module("g.region", run_=False)
    >>> region_1.flags.p = True
    >>> region_2 = copy.deepcopy(region_1)
    >>> region_2.flags.p = True
    >>> mm = MultiModule(module_list=[region_1, region_2])
    >>> mm.run()
    >>> m_list = mm.get_modules()
    >>> m_list[0].popen.returncode
    0
    >>> m_list[1].popen.returncode
    0

    Asynchronous module run, setting finish = False

    >>> region_1 = Module("g.region", run_=False)  # doctest: +SKIP
    >>> region_1.flags.p = True  # doctest: +SKIP
    >>> region_2 = copy.deepcopy(region_1)  # doctest: +SKIP
    >>> region_2.flags.p = True  # doctest: +SKIP
    >>> region_3 = copy.deepcopy(region_1)  # doctest: +SKIP
    >>> region_3.flags.p = True  # doctest: +SKIP
    >>> region_4 = copy.deepcopy(region_1)  # doctest: +SKIP
    >>> region_4.flags.p = True  # doctest: +SKIP
    >>> region_5 = copy.deepcopy(region_1)  # doctest: +SKIP
    >>> region_5.flags.p = True  # doctest: +SKIP
    >>> mm = MultiModule(module_list=[region_1, region_2, region_3, region_4, region_5],
    ...                  sync=False)  # doctest: +SKIP
    >>> t = mm.run()  # doctest: +SKIP
    >>> isinstance(t, Process)  # doctest: +SKIP
    True
    >>> m_list = mm.wait()  # doctest: +SKIP
    >>> m_list[0].popen.returncode  # doctest: +SKIP
    0
    >>> m_list[1].popen.returncode  # doctest: +SKIP
    0
    >>> m_list[2].popen.returncode  # doctest: +SKIP
    0
    >>> m_list[3].popen.returncode  # doctest: +SKIP
    0
    >>> m_list[4].popen.returncode  # doctest: +SKIP
    0

    Asynchronous module run, setting finish = False and using temporary region

    >>> mm = MultiModule(module_list=[region_1, region_2, region_3, region_4, region_5],
    ...                  sync=False, set_temp_region=True)
    >>> str(mm)
    'g.region -p ; g.region -p ; g.region -p ; g.region -p ; g.region -p'
    >>> t = mm.run()
    >>> isinstance(t, Process)
    True
    >>> m_list = mm.wait()
    >>> m_list[0].popen.returncode
    0
    >>> m_list[1].popen.returncode
    0
    >>> m_list[2].popen.returncode
    0
    >>> m_list[3].popen.returncode
    0
    >>> m_list[4].popen.returncode
    0

    """

    def __init__(self, module_list, sync=True, set_temp_region=False):
        """Constructor of the multi module class

        :param module_list: A list of pre-configured Module objects that should be run
        :param sync: If set True the run() method will wait for all processes to finish -> synchronously run.
                     If set False, the run() method will return after starting the processes -> asynchronously run.
                     The wait() method must be called to finish the modules.
        :param set_temp_region: Set a temporary region in which the modules should be run, hence
                                region settings in the process list will not affect the current
                                computation region.

                                Note:

                                    This flag is only available in asynchronous mode!
        :return:
        """
        self.module_list = module_list
        self.set_temp_region = set_temp_region
        self.finish_ = sync      # We use the same variable name a Module
        self.p = None
        self.q = Queue()

    def __str__(self):
        """Return the command string that can be executed in a shell"""
        return ' ; '.join(str(string) for string in self.module_list)

    def get_modules(self):
        """Return the list of modules that have been run in synchronous mode

        Note: Asynchronously run module can only be accessed via the wait() method.

        :return: The list of modules
        """
        return self.module_list

    def run(self):
        """Start the modules in the list. If self.finished_ is set True
        this method will return after all processes finished.

        If self.finish_ is set False, this method will return
        after the process list was started for execution.
        In a background process, the processes in the list will
        be run one after the another.

        :return: None in case of self.finish_ is True,
                 otherwise a multiprocessing.Process object that invokes the modules
        """

        if self.finish_ is True:
            for module in self.module_list:
                module.finish_ = True
                module.run()
            return None
        else:
            if self.set_temp_region is True:
                self.p = Process(target=run_modules_in_temp_region,
                                 args=[self.module_list, self.q])
            else:
                self.p = Process(target=run_modules,
                                 args=[self.module_list, self.q])
            self.p.start()

            return self.p

    def wait(self):
        """Wait for all processes to finish. Call this method
        in asynchronous mode, hence if finished was set False.

        :return: The process list with finished processes to check their return states
        """
        if self.p:
            proc_list = self.q.get()
            self.p.join()

            return proc_list


def run_modules_in_temp_region(module_list, q):
    """Run the modules in a temporary region environment

    This function is the argument for multiprocessing.Process class
    in the MultiModule asynchronous execution.

    :param module_list: The list of modules to run in serial
    :param q: The process queue to put the finished process list
    """
    use_temp_region()
    try:
        for proc in module_list:
            proc.run()
            proc.wait()
    except:
        raise
    finally:
        q.put(module_list)
        del_temp_region()


def run_modules(module_list, q):
    """Run the modules

    This function is the argument for multiprocessing.Process class
    in the MultiModule asynchronous execution.

    :param module_list: The list of modules to run in serial
    :param q: The process queue to put the finished process list
    """
    try:
        for proc in module_list:
            proc.run()
            proc.wait()
    except:
        raise
    finally:
        q.put(module_list)

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
