# -*- coding: utf-8 -*-
"""Specialized interfaces for invoking modules for testing framework

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

:authors: Vaclav Petras, Soeren Gebbert
"""

import subprocess
from grass.script.core import start_command
from grass.script.utils import encode, decode
from grass.exceptions import CalledModuleError
from grass.pygrass.modules import Module

from .utils import do_doctest_gettext_workaround


class SimpleModule(Module):
    """Simple wrapper around pygrass.modules.Module to make sure that
    run\_, finish\_, stdout and stderr are set correctly.

    >>> mapcalc = SimpleModule('r.mapcalc', expression='test_a = 1',
    ...                        overwrite=True)
    >>> mapcalc.run()
    Module('r.mapcalc')
    >>> mapcalc.popen.returncode
    0

    >>> colors = SimpleModule('r.colors',
    ...                       map='test_a', rules='-', stdin_='1 red')
    >>> colors.run()
    Module('r.colors')
    >>> colors.popen.returncode
    0
    >>> str(colors.inputs.stdin)
    '1 red'
    >>> str(colors.outputs.stdout)
    ''
    >>> colors.outputs.stderr.strip()
    "Color table for raster map <test_a> set to 'rules'"
    """
    def __init__(self, cmd, *args, **kargs):
        for banned in ['stdout_', 'stderr_', 'finish_', 'run_']:
            if banned in kargs:
                raise ValueError('Do not set %s parameter'
                                 ', it would be overriden' % banned)
        kargs['stdout_'] = subprocess.PIPE
        kargs['stderr_'] = subprocess.PIPE
        kargs['finish_'] = True
        kargs['run_'] = False

        Module.__init__(self, cmd, *args, **kargs)


def call_module(module, stdin=None,
                merge_stderr=False, capture_stdout=True, capture_stderr=True,
                **kwargs):
    r"""Run module with parameters given in `kwargs` and return its output.

    >>> print (call_module('g.region', flags='pg'))  # doctest: +ELLIPSIS
    projection=...
    zone=...
    n=...
    s=...
    w=...
    >>> call_module('m.proj', flags='i', input='-', stdin="50.0 41.5")
    '8642890.65|6965155.61|0.00\n'
    >>> call_module('g.region', aabbbccc='notexist')  # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
        ...
    CalledModuleError: Module run g.region ... ended with error

    If `stdin` is not set and `kwargs` contains ``input`` with value set
    to ``-`` (dash), the function raises an error.

    Note that ``input`` nor ``output`` parameters are used by this
    function itself, these are usually module parameters which this
    function just passes to it. However, when ``input`` is in parameters
    the function checks if its values is correct considering value of
    ``stdin`` parameter.

    :param str module: module name
    :param stdin: string to be used as module standard input (stdin) or `None`
    :param merge_stderr: if the standard error output should be merged with stdout
    :param kwargs: module parameters

    :returns: module standard output (stdout) as string or None if apture_stdout is False

    :raises CalledModuleError: if module return code is non-zero
    :raises ValueError: if the parameters are not correct

    .. note::
        The data read is buffered in memory, so do not use this method
        if the data size is large or unlimited.
    """
    # TODO: remove this:
    do_doctest_gettext_workaround()
    # implementation inspired by subprocess.check_output() function
    if stdin:
        if 'input' in kwargs and kwargs['input'] != '-':
            raise ValueError(_("input='-' must be used when stdin is specified"))
        if stdin == subprocess.PIPE:
            raise ValueError(_("stdin must be string or buffer, not PIPE"))
        kwargs['stdin'] = subprocess.PIPE  # to be able to send data to stdin
    elif 'input' in kwargs and kwargs['input'] == '-':
        raise ValueError(_("stdin must be used when input='-'"))
    if merge_stderr and not (capture_stdout and capture_stderr):
        raise ValueError(_("You cannot merge stdout and stderr and not capture them"))
    if 'stdout' in kwargs:
        raise TypeError(_("stdout argument not allowed, it could be overridden"))
    if 'stderr' in kwargs:
        raise TypeError(_("stderr argument not allowed, it could be overridden"))

    if capture_stdout:
        kwargs['stdout'] = subprocess.PIPE
    if capture_stderr:
        if merge_stderr:
            kwargs['stderr'] = subprocess.STDOUT
        else:
            kwargs['stderr'] = subprocess.PIPE
    process = start_command(module, **kwargs)
    # input=None means no stdin (our default)
    # for no stdout, output is None which is out interface
    # for stderr=STDOUT or no stderr, errors is None
    # which is fine for CalledModuleError
    output, errors = process.communicate(input=encode(decode(stdin)) if stdin else None)
    returncode = process.poll()
    if returncode:
        raise CalledModuleError(returncode, module, kwargs, errors)
    return decode(output) if output else None
