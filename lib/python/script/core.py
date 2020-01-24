"""
Core functions to be used in Python scripts.

Usage:

::

    from grass.script import core as grass
    grass.parser()

(C) 2008-2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Glynn Clements
.. sectionauthor:: Martin Landa <landa.martin gmail.com>
.. sectionauthor:: Michael Barton <michael.barton asu.edu>
"""
from __future__ import absolute_import, print_function

import os
import sys
import atexit
import subprocess
import shutil
import codecs
import string
import random
import pipes
import types as python_types

from .utils import KeyValue, parse_key_val, basename, encode, decode
from grass.exceptions import ScriptError, CalledModuleError

# PY2/PY3 compat
if sys.version_info.major > 2:
    unicode = str

# subprocess wrapper that uses shell on Windows
class Popen(subprocess.Popen):
    _builtin_exts = set(['.com', '.exe', '.bat', '.cmd'])

    @staticmethod
    def _escape_for_shell(arg):
        # TODO: what are cmd.exe's parsing rules?
        return arg

    def __init__(self, args, **kwargs):
        if (sys.platform == 'win32'
            and isinstance(args, list)
            and not kwargs.get('shell', False)
            and kwargs.get('executable') is None):
            cmd = shutil_which(args[0])
            if cmd is None:
                raise OSError(_("Cannot find the executable {0}")
                              .format(args[0]))
            args = [cmd] + args[1:]
            name, ext = os.path.splitext(cmd)
            if ext.lower() not in self._builtin_exts:
                kwargs['shell'] = True
                args = [self._escape_for_shell(arg) for arg in args]
        subprocess.Popen.__init__(self, args, **kwargs)

PIPE = subprocess.PIPE
STDOUT = subprocess.STDOUT


raise_on_error = False  # raise exception instead of calling fatal()
_capture_stderr = False  # capture stderr of subprocesses if possible


def call(*args, **kwargs):
    return Popen(*args, **kwargs).wait()

# GRASS-oriented interface to subprocess module

_popen_args = ["bufsize", "executable", "stdin", "stdout", "stderr",
               "preexec_fn", "close_fds", "cwd", "env",
               "universal_newlines", "startupinfo", "creationflags"]


def _make_val(val):
    """Convert value to unicode"""
    if isinstance(val, (bytes, str, unicode)):
        return decode(val)
    if isinstance(val, (int, float)):
        return unicode(val)
    try:
        return ",".join(map(_make_val, iter(val)))
    except TypeError:
        pass
    return unicode(val)


def _make_unicode(val, enc):
    """Convert value to unicode with given encoding

    :param val: value to be converted
    :param enc: encoding to be used
    """
    if val is None or enc is None:
        return val
    else:
        if enc == 'default':
            return decode(val)
        else:
            return decode(val, encoding=enc)


def get_commands():
    """Create list of available GRASS commands to use when parsing
    string from the command line

    :return: list of commands (set) and directory of scripts (collected
             by extension - MS Windows only)

    >>> cmds = list(get_commands()[0])
    >>> cmds.sort()
    >>> cmds[:5]
    ['d.barscale', 'd.colorlist', 'd.colortable', 'd.correlate', 'd.erase']

    """
    gisbase = os.environ['GISBASE']
    cmd = list()
    scripts = {'.py': list()} if sys.platform == 'win32' else {}

    def scan(gisbase, directory):
        dir_path = os.path.join(gisbase, directory)
        if os.path.exists(dir_path):
            for fname in os.listdir(os.path.join(gisbase, directory)):
                if scripts:  # win32
                    name, ext = os.path.splitext(fname)
                    if ext != '.manifest':
                        cmd.append(name)
                    if ext in scripts.keys():
                        scripts[ext].append(name)
                else:
                    cmd.append(fname)

    for directory in ('bin', 'scripts'):
        scan(gisbase, directory)

    # scan gui/scripts/
    gui_path = os.path.join(gisbase, 'etc', 'gui', 'scripts')
    if os.path.exists(gui_path):
        os.environ["PATH"] = os.getenv("PATH") + os.pathsep + gui_path
        cmd = cmd + os.listdir(gui_path)

    return set(cmd), scripts

# TODO: Please replace this function with shutil.which() before 8.0 comes out
# replacement for which function from shutil (not available in all versions)
# from http://hg.python.org/cpython/file/6860263c05b3/Lib/shutil.py#l1068
# added because of Python scripts running Python scripts on MS Windows
# see also ticket #2008 which is unrelated but same function was proposed
def shutil_which(cmd, mode=os.F_OK | os.X_OK, path=None):
    """Given a command, mode, and a PATH string, return the path which
    conforms to the given mode on the PATH, or None if there is no such
    file.

    `mode` defaults to os.F_OK | os.X_OK. `path` defaults to the result
    of os.environ.get("PATH"), or can be overridden with a custom search
    path.

    :param cmd: the command
    :param mode:
    :param path:

    """
    # Check that a given file can be accessed with the correct mode.
    # Additionally check that `file` is not a directory, as on Windows
    # directories pass the os.access check.
    def _access_check(fn, mode):
        return (os.path.exists(fn) and os.access(fn, mode)
                and not os.path.isdir(fn))

    # If we're given a path with a directory part, look it up directly rather
    # than referring to PATH directories. This includes checking relative to the
    # current directory, e.g. ./script
    if os.path.dirname(cmd):
        if _access_check(cmd, mode):
            return cmd
        return None

    if path is None:
        path = os.environ.get("PATH", os.defpath)
    if not path:
        return None
    path = path.split(os.pathsep)

    if sys.platform == "win32":
        # The current directory takes precedence on Windows.
        if not os.curdir in path:
            path.insert(0, os.curdir)

        # PATHEXT is necessary to check on Windows (force lowercase)
        pathext = list(map(lambda x: x.lower(),
                           os.environ.get("PATHEXT", "").split(os.pathsep)))
        if '.py' not in pathext:
            # we assume that PATHEXT contains always '.py'
            pathext.insert(0, '.py')
        # See if the given file matches any of the expected path extensions.
        # This will allow us to short circuit when given "python3.exe".
        # If it does match, only test that one, otherwise we have to try
        # others.
        if any(cmd.lower().endswith(ext) for ext in pathext):
            files = [cmd]
        else:
            files = [cmd + ext for ext in pathext]
    else:
        # On other platforms you don't have things like PATHEXT to tell you
        # what file suffixes are executable, so just pass on cmd as-is.
        files = [cmd]

    seen = set()
    for dir in path:
        normdir = os.path.normcase(dir)
        if not normdir in seen:
            seen.add(normdir)
            for thefile in files:
                name = os.path.join(dir, thefile)
                if _access_check(name, mode):
                    return name
    return None

if sys.version_info.major > 2:
    shutil_which = shutil.which

# Added because of scripts calling scripts on MS Windows.
# Module name (here cmd) differs from the file name (does not have extension).
# Additionally, we don't run scripts using system executable mechanism,
# so we need the full path name.
# However, scripts are on the PATH and '.PY' in in PATHEXT, so we can use
# shutil.which to get the full file path. Addons are on PATH too.
# An alternative to which function call would be to check the script path and
# addons path. This is proposed improvement for the future.
# Another alternative is to check some global list of scripts but this list
# needs to be created first. The question is what is less expensive.
# Note that getting the full path is only part of the solution,
# the other part is to use the right Python as an executable and pass the full
# script path as a parameter.
# Nevertheless, it is unclear on which places which extensions are added.
# This function also could skip the check for platform but depends
# how will be used, this is most general but not most effective.
def get_real_command(cmd):
    """Returns the real file command for a module (cmd)

    For Python scripts on MS Windows it returns full path to the script
    and adds a '.py' extension.
    For other cases it just returns a module (name).
    So, you can just use this function for all without further check.

    >>> get_real_command('g.region')
    'g.region'

    :param cmd: the command
    """
    if sys.platform == 'win32':
        # we in fact expect pure module name (without extension)
        # so, lets remove extension
        if os.path.splitext(cmd)[1] == '.py':
            cmd = cmd[:-3]
        # PATHEXT is necessary to check on Windows (force lowercase)
        pathext = list(map(lambda x: x.lower(),
                           os.environ['PATHEXT'].split(os.pathsep)))
        if '.py' not in pathext:
            # we assume that PATHEXT contains always '.py'
            os.environ['PATHEXT'] = '.py;' + os.environ['PATHEXT']
        full_path = shutil_which(cmd + '.py')
        if full_path:
            return full_path

    return cmd


def make_command(prog, flags="", overwrite=False, quiet=False, verbose=False,
                 superquiet=False, errors=None, **options):
    """Return a list of strings suitable for use as the args parameter to
    Popen() or call(). Example:


    >>> make_command("g.message", flags = 'w', message = 'this is a warning')
    ['g.message', '-w', 'message=this is a warning']


    :param str prog: GRASS module
    :param str flags: flags to be used (given as a string)
    :param bool overwrite: True to enable overwriting the output (<tt>--o</tt>)
    :param bool quiet: True to run quietly (<tt>--q</tt>)
    :param bool verbose: True to run verbosely (<tt>--v</tt>)
    :param options: module's parameters

    :return: list of arguments
    """
    args = [_make_val(prog)]
    if overwrite:
        args.append("--o")
    if quiet:
        args.append("--q")
    if verbose:
        args.append("--v")
    if superquiet:
        args.append("--qq")
    if flags:
        flags = _make_val(flags)
        if '-' in flags:
            raise ScriptError("'-' is not a valid flag")
        args.append("-" + flags)
    for opt, val in options.items():
        if opt in _popen_args:
            continue
        # convert string to bytes
        if val is not None:
            if opt.startswith('_'):
                opt = opt[1:]
                warning(_("To run the module <%s> add underscore at the end"
                    " of the option <%s> to avoid conflict with Python"
                    " keywords. Underscore at the beginning is"
                    " depreciated in GRASS GIS 7.0 and will be removed"
                    " in version 7.1.") % (prog, opt))
            elif opt.endswith('_'):
                opt = opt[:-1]
            args.append(opt + '=' + _make_val(val))
    return args


def handle_errors(returncode, result, args, kwargs):
    if returncode == 0:
        return result
    handler = kwargs.get('errors', 'raise')
    if handler.lower() == 'ignore':
        return result
    elif handler.lower() == 'status':
        return returncode
    elif handler.lower() == 'exit':
        sys.exit(1)
    else:
        # TODO: construction of the whole command is far from perfect
        args = make_command(*args, **kwargs)
        code = ' '.join(args)
        raise CalledModuleError(module=None, code=code,
                                returncode=returncode)

def start_command(prog, flags="", overwrite=False, quiet=False,
                  verbose=False, superquiet=False, **kwargs):
    """Returns a Popen object with the command created by make_command.
    Accepts any of the arguments which Popen() accepts apart from "args"
    and "shell".

    >>> p = start_command("g.gisenv", stdout=subprocess.PIPE)
    >>> print(p)  # doctest: +ELLIPSIS
    <...Popen object at 0x...>
    >>> print(p.communicate()[0])  # doctest: +SKIP
    GISDBASE='/opt/grass-data';
    LOCATION_NAME='spearfish60';
    MAPSET='glynn';
    GUI='text';
    MONITOR='x0';

    If the module parameter is the same as Python keyword, add
    underscore at the end of the parameter. For example, use
    ``lambda_=1.6`` instead of ``lambda=1.6``.

    :param str prog: GRASS module
    :param str flags: flags to be used (given as a string)
    :param bool overwrite: True to enable overwriting the output (<tt>--o</tt>)
    :param bool quiet: True to run quietly (<tt>--q</tt>)
    :param bool verbose: True to run verbosely (<tt>--v</tt>)
    :param kwargs: module's parameters

    :return: Popen object
    """
    if 'encoding' in kwargs.keys():
        encoding = kwargs.pop('encoding')

    options = {}
    popts = {}
    for opt, val in kwargs.items():
        if opt in _popen_args:
            popts[opt] = val
        else:
            options[opt] = val

    args = make_command(prog, flags, overwrite, quiet, verbose, **options)

    if debug_level() > 0:
        sys.stderr.write("D1/{}: {}.start_command(): {}\n".format(
            debug_level(), __name__,
            ' '.join(args))
        )
        sys.stderr.flush()
    return Popen(args, **popts)


def run_command(*args, **kwargs):
    """Execute a module synchronously

    This function passes all arguments to ``start_command()``,
    then waits for the process to complete. It is similar to
    ``subprocess.check_call()``, but with the ``make_command()``
    interface.

    For backward compatibility, the function returns exit code
    by default but only if it is equal to zero. An exception is raised
    in case of an non-zero return code.

    >>> run_command('g.region', raster='elevation')
    0

    See :func:`start_command()` for details about parameters and usage.

    ..note::
        You should ignore the return value of this function unless, you
        change the default behavior using *errors* parameter.

    :param *args: unnamed arguments passed to ``start_command()``
    :param **kwargs: named arguments passed to ``start_command()``

    :returns: 0 with default parameters for backward compatibility only

    :raises: ``CalledModuleError`` when module returns non-zero return code
    """
    encoding = 'default'
    if 'encoding' in kwargs:
        encoding = kwargs['encoding']

    if _capture_stderr and 'stderr' not in kwargs.keys():
        kwargs['stderr'] = PIPE
    ps = start_command(*args, **kwargs)
    if _capture_stderr:
        stdout, stderr = ps.communicate()
        if encoding is not None:
            stdout = _make_unicode(stdout, encoding)
            stderr = _make_unicode(stderr, encoding)
        returncode = ps.poll()
        if returncode:
            sys.stderr.write(stderr)
    else:
        returncode = ps.wait()
    return handle_errors(returncode, returncode, args, kwargs)


def pipe_command(*args, **kwargs):
    """Passes all arguments to start_command(), but also adds
    "stdout = PIPE". Returns the Popen object.

    >>> p = pipe_command("g.gisenv")
    >>> print(p)  # doctest: +ELLIPSIS
    <....Popen object at 0x...>
    >>> print(p.communicate()[0])  # doctest: +SKIP
    GISDBASE='/opt/grass-data';
    LOCATION_NAME='spearfish60';
    MAPSET='glynn';
    GUI='text';
    MONITOR='x0';

    :param list args: list of unnamed arguments (see start_command() for details)
    :param list kwargs: list of named arguments (see start_command() for details)

    :return: Popen object
    """
    kwargs['stdout'] = PIPE
    return start_command(*args, **kwargs)


def feed_command(*args, **kwargs):
    """Passes all arguments to start_command(), but also adds
    "stdin = PIPE". Returns the Popen object.

    :param list args: list of unnamed arguments (see start_command() for details)
    :param list kwargs: list of named arguments (see start_command() for details)

    :return: Popen object
    """
    kwargs['stdin'] = PIPE
    return start_command(*args, **kwargs)


def read_command(*args, **kwargs):
    """Passes all arguments to pipe_command, then waits for the process to
    complete, returning its stdout (i.e. similar to shell `backticks`).

    :param list args: list of unnamed arguments (see start_command() for details)
    :param list kwargs: list of named arguments (see start_command() for details)

    :return: stdout
    """
    encoding = 'default'
    if 'encoding' in kwargs:
        encoding = kwargs['encoding']

    if _capture_stderr and 'stderr' not in kwargs.keys():
        kwargs['stderr'] = PIPE
    process = pipe_command(*args, **kwargs)
    stdout, stderr = process.communicate()
    if encoding is not None:
        stdout = _make_unicode(stdout, encoding)
        stderr = _make_unicode(stderr, encoding)
    returncode = process.poll()
    if _capture_stderr and returncode:
        sys.stderr.write(stderr)
    return handle_errors(returncode, stdout, args, kwargs)


def parse_command(*args, **kwargs):
    """Passes all arguments to read_command, then parses the output
    by parse_key_val().

    Parsing function can be optionally given by <em>parse</em> parameter
    including its arguments, e.g.

    ::

        parse_command(..., parse = (grass.parse_key_val, { 'sep' : ':' }))

    or you can simply define <em>delimiter</em>

    ::

        parse_command(..., delimiter = ':')

    :param args: list of unnamed arguments (see start_command() for details)
    :param kwargs: list of named arguments (see start_command() for details)

    :return: parsed module output
    """
    parse = None
    parse_args = {}
    if 'parse' in kwargs:
        if isinstance(kwargs['parse'], tuple):
            parse = kwargs['parse'][0]
            parse_args = kwargs['parse'][1]
        del kwargs['parse']

    if 'delimiter' in kwargs:
        parse_args = {'sep': kwargs['delimiter']}
        del kwargs['delimiter']

    if not parse:
        parse = parse_key_val  # use default fn

    res = read_command(*args, **kwargs)

    return parse(res, **parse_args)


def write_command(*args, **kwargs):
    """Execute a module with standard input given by *stdin* parameter.

    Passes all arguments to ``feed_command()``, with the string specified
    by the *stdin* argument fed to the process' standard input.

    >>> gscript.write_command(
    ...    'v.in.ascii', input='-',
    ...    stdin='%s|%s' % (635818.8, 221342.4),
    ...    output='view_point')
    0

    See ``start_command()`` for details about parameters and usage.

    :param *args: unnamed arguments passed to ``start_command()``
    :param **kwargs: named arguments passed to ``start_command()``

    :returns: 0 with default parameters for backward compatibility only

    :raises: ``CalledModuleError`` when module returns non-zero return code
    """
    encoding = 'default'
    if 'encoding' in kwargs:
        encoding = kwargs['encoding']
    # TODO: should we delete it from kwargs?
    stdin = kwargs['stdin']
    if encoding is None or encoding == 'default':
        stdin = encode(stdin)
    else:
        stdin = encode(stdin, encoding=encoding)
    if _capture_stderr and 'stderr' not in kwargs.keys():
        kwargs['stderr'] = PIPE
    process = feed_command(*args, **kwargs)
    unused, stderr = process.communicate(stdin)
    if encoding is not None:
        unused = _make_unicode(unused, encoding)
        stderr = _make_unicode(stderr, encoding)
    returncode = process.poll()
    if _capture_stderr and returncode:
        sys.stderr.write(stderr)
    return handle_errors(returncode, returncode, args, kwargs)


def exec_command(prog, flags="", overwrite=False, quiet=False, verbose=False,
                 superquiet=False, env=None, **kwargs):
    """Interface to os.execvpe(), but with the make_command() interface.

    :param str prog: GRASS module
    :param str flags: flags to be used (given as a string)
    :param bool overwrite: True to enable overwriting the output (<tt>--o</tt>)
    :param bool quiet: True to run quietly (<tt>--q</tt>)
    :param bool verbose: True to run verbosely (<tt>--v</tt>)
    :param env: directory with environmental variables
    :param list kwargs: module's parameters

    """
    args = make_command(prog, flags, overwrite, quiet, verbose, **kwargs)

    if env is None:
        env = os.environ
    os.execvpe(prog, args, env)

# interface to g.message


def message(msg, flag=None):
    """Display a message using `g.message`

    :param str msg: message to be displayed
    :param str flag: flags (given as string)
    """
    run_command("g.message", flags=flag, message=msg, errors='ignore')


def debug(msg, debug=1):
    """Display a debugging message using `g.message -d`

    :param str msg: debugging message to be displayed
    :param str debug: debug level (0-5)
    """
    if debug_level() >= debug:
        # TODO: quite a random hack here, do we need it somewhere else too?
        if sys.platform == "win32":
            msg = msg.replace('&', '^&')

        run_command("g.message", flags='d', message=msg, debug=debug)

def verbose(msg):
    """Display a verbose message using `g.message -v`

    :param str msg: verbose message to be displayed
    """
    message(msg, flag='v')


def info(msg):
    """Display an informational message using `g.message -i`

    :param str msg: informational message to be displayed
    """
    message(msg, flag='i')


def percent(i, n, s):
    """Display a progress info message using `g.message -p`

    ::

        message(_("Percent complete..."))
        n = 100
        for i in range(n):
            percent(i, n, 1)
        percent(1, 1, 1)

    :param int i: current item
    :param int n: total number of items
    :param int s: increment size
    """
    message("%d %d %d" % (i, n, s), flag='p')


def warning(msg):
    """Display a warning message using `g.message -w`

    :param str msg: warning message to be displayed
    """
    message(msg, flag='w')


def error(msg):
    """Display an error message using `g.message -e`

    This function does not end the execution of the program.
    The right action after the error is up to the caller.
    For error handling using the standard mechanism use :func:`fatal()`.

    :param str msg: error message to be displayed
    """
    message(msg, flag='e')


def fatal(msg):
    """Display an error message using `g.message -e`, then abort or raise

    Raises exception when module global raise_on_error is 'True', abort
    (calls exit) otherwise.
    Use :func:`set_raise_on_error()` to set the behavior.

    :param str msg: error message to be displayed
    """
    global raise_on_error
    if raise_on_error:
        raise ScriptError(msg)

    error(msg)
    sys.exit(1)


def set_raise_on_error(raise_exp=True):
    """Define behaviour on fatal error (fatal() called)

    :param bool raise_exp: True to raise ScriptError instead of calling
                           sys.exit(1) in fatal()

    :return: current status
    """
    global raise_on_error
    tmp_raise = raise_on_error
    raise_on_error = raise_exp
    return tmp_raise


def get_raise_on_error():
    """Return True if a ScriptError exception is raised instead of calling
       sys.exit(1) in case a fatal error was invoked with fatal()
    """
    global raise_on_error
    return raise_on_error


# TODO: solve also warnings (not printed now)
def set_capture_stderr(capture=True):
    """Enable capturing standard error output of modules and print it.

    By default, standard error output (stderr) of child processes shows
    in the same place as output of the parent process. This may not
    always be the same place as ``sys.stderr`` is written.
    After calling this function, functions in the ``grass.script``
    package will capture the stderr of child processes and pass it
    to ``sys.stderr`` if there is an error.

    .. note::

        This is advantages for interactive shells such as the one in GUI
        and interactive notebooks such as Jupyer Notebook.

    The capturing can be applied only in certain cases, for example
    in case of run_command() it is applied because run_command() nor
    its callers do not handle the streams, however feed_command()
    cannot do capturing because its callers handle the streams.

    The previous state is returned. Passing ``False`` disables the
    capturing.

    .. versionadded:: 7.4
    """
    global _capture_stderr
    tmp = _capture_stderr
    _capture_stderr = capture
    return tmp

def get_capture_stderr():
    """Return True if stderr is captured, False otherwise.

    See set_capture_stderr().
    """
    global _capture_stderr
    return _capture_stderr

# interface to g.parser


def _parse_opts(lines):
    options = {}
    flags = {}
    for line in lines:
        if not line:
            break
        try:
            [var, val] = line.split(b'=', 1)
            [var, val] = [decode(var), decode(val)]
        except:
            raise SyntaxError("invalid output from g.parser: %s" % line)

        if var.startswith('flag_'):
            flags[var[5:]] = bool(int(val))
        elif var.startswith('opt_'):
            options[var[4:]] = val
        elif var in ['GRASS_OVERWRITE', 'GRASS_VERBOSE']:
            os.environ[var] = val
        else:
            raise SyntaxError("invalid output from g.parser: %s" % line)

    return (options, flags)


def parser():
    """Interface to g.parser, intended to be run from the top-level, e.g.:

    ::

        if __name__ == "__main__":
            options, flags = grass.parser()
            main()

    Thereafter, the global variables "options" and "flags" will be
    dictionaries containing option/flag values, keyed by lower-case
    option/flag names. The values in "options" are strings, those in
    "flags" are Python booleans.

    Overview table of parser standard options:
    https://grass.osgeo.org/grass78/manuals/parser_standard_options.html
    """
    if not os.getenv("GISBASE"):
        print("You must be in GRASS GIS to run this program.", file=sys.stderr)
        sys.exit(1)

    cmdline = [basename(sys.argv[0])]
    cmdline += [pipes.quote(a) for a in sys.argv[1:]]
    os.environ['CMDLINE'] = ' '.join(cmdline)

    argv = sys.argv[:]
    name = argv[0]
    if not os.path.isabs(name):
        if os.sep in name or (os.altsep and os.altsep in name):
            argv[0] = os.path.abspath(name)
        else:
            argv[0] = os.path.join(sys.path[0], name)

    prog = "g.parser.exe" if sys.platform == "win32" else "g.parser"
    p = subprocess.Popen([prog, '-n'] + argv, stdout=subprocess.PIPE)
    s = p.communicate()[0]
    lines = s.split(b'\0')

    if not lines or lines[0] != b"@ARGS_PARSED@":
        stdout = os.fdopen(sys.stdout.fileno(), 'wb')
        stdout.write(s)
        sys.exit(p.returncode)
    return _parse_opts(lines[1:])

# interface to g.tempfile


def tempfile(create=True):
    """Returns the name of a temporary file, created with g.tempfile.

    :param bool create: True to create a file

    :return: path to a tmp file
    """
    flags = ''
    if not create:
        flags += 'd'

    return read_command("g.tempfile", flags=flags, pid=os.getpid()).strip()


def tempdir():
    """Returns the name of a temporary dir, created with g.tempfile."""
    tmp = tempfile(create=False)
    os.mkdir(tmp)

    return tmp


def tempname(length, lowercase=False):
    """Generate a GRASS and SQL compliant random name starting with tmp_
    followed by a random part of length "length"

    :param int length: length of the random part of the name to generate
    :param bool lowercase: use only lowercase characters to generate name
    :returns: String with a random name of length "length" starting with a letter
    :rtype: str

    :Example:

    >>> tempname(12)
    'tmp_MxMa1kAS13s9'
    """

    chars = string.ascii_lowercase + string.digits
    if not lowercase:
        chars += string.ascii_uppercase
    random_part = ''.join(random.choice(chars) for _ in range(length))
    randomname = 'tmp_' + random_part

    return randomname


def _compare_projection(dic):
    """Check if projection has some possibility of duplicate names like
    Universal Transverse Mercator and Universe Transverse Mercator and
    unify them

    :param dic: The dictionary containing information about projection

    :return: The dictionary with the new values if needed

    """
    # the lookup variable is a list of list, each list contains all the
    # possible name for a projection system
    lookup = [['Universal Transverse Mercator', 'Universe Transverse Mercator']]
    for lo in lookup:
        for n in range(len(dic['name'])):
            if dic['name'][n] in lo:
                dic['name'][n] = lo[0]
    return dic


def _compare_units(dic):
    """Check if units has some possibility of duplicate names like
    meter and metre and unify them

    :param dic: The dictionary containing information about units

    :return: The dictionary with the new values if needed

    """
    # the lookup variable is a list of list, each list contains all the
    # possible name for a units
    lookup = [['meter', 'metre'], ['meters', 'metres'], ['kilometer',
              'kilometre'], ['kilometers', 'kilometres']]
    for l in lookup:
        for n in range(len(dic['unit'])):
            if dic['unit'][n].lower() in l:
                dic['unit'][n] = l[0]
        for n in range(len(dic['units'])):
            if dic['units'][n].lower() in l:
                dic['units'][n] = l[0]
    return dic


def _text_to_key_value_dict(filename, sep=":", val_sep=",", checkproj=False,
                            checkunits=False):
    """Convert a key-value text file, where entries are separated by newlines
    and the key and value are separated by `sep', into a key-value dictionary
    and discover/use the correct data types (float, int or string) for values.

    :param str filename: The name or name and path of the text file to convert
    :param str sep: The character that separates the keys and values, default
                    is ":"
    :param str val_sep: The character that separates the values of a single
                        key, default is ","
    :param bool checkproj: True if it has to check some information about
                           projection system
    :param bool checkproj: True if it has to check some information about units

    :return: The dictionary

    A text file with this content:
    ::

        a: Hello
        b: 1.0
        c: 1,2,3,4,5
        d : hello,8,0.1

    Will be represented as this dictionary:

    ::

        {'a': ['Hello'], 'c': [1, 2, 3, 4, 5], 'b': [1.0], 'd': ['hello', 8, 0.1]}

    """
    text = open(filename, "r").readlines()
    kvdict = KeyValue()

    for line in text:
        if line.find(sep) >= 0:
            key, value = line.split(sep)
            key = key.strip()
            value = value.strip()
        else:
            # Jump over empty values
            continue
        values = value.split(val_sep)
        value_list = []

        for value in values:
            not_float = False
            not_int = False

            # Convert values into correct types
            # We first try integer then float
            try:
                value_converted = int(value)
            except:
                not_int = True
            if not_int:
                try:
                    value_converted = float(value)
                except:
                    not_float = True

            if not_int and not_float:
                value_converted = value.strip()

            value_list.append(value_converted)

        kvdict[key] = value_list
    if checkproj:
        kvdict = _compare_projection(kvdict)
    if checkunits:
        kvdict = _compare_units(kvdict)
    return kvdict


def compare_key_value_text_files(filename_a, filename_b, sep=":",
                                 val_sep=",", precision=0.000001,
                                 proj=False, units=False):
    """Compare two key-value text files

    This method will print a warning in case keys that are present in the first
    file are not present in the second one.
    The comparison method tries to convert the values into their native format
    (float, int or string) to allow correct comparison.

    An example key-value text file may have this content:

    ::

        a: Hello
        b: 1.0
        c: 1,2,3,4,5
        d : hello,8,0.1

    :param str filename_a: name of the first key-value text file
    :param str filenmae_b: name of the second key-value text file
    :param str sep: character that separates the keys and values, default is ":"
    :param str val_sep: character that separates the values of a single key, default is ","
    :param double precision: precision with which the floating point values are compared
    :param bool proj: True if it has to check some information about projection system
    :param bool units: True if it has to check some information about units

    :return: True if full or almost identical, False if different
    """
    dict_a = _text_to_key_value_dict(filename_a, sep, checkproj=proj,
                                     checkunits=units)
    dict_b = _text_to_key_value_dict(filename_b, sep, checkproj=proj,
                                     checkunits=units)

    if sorted(dict_a.keys()) != sorted(dict_b.keys()):
        return False

    # We compare matching keys
    for key in dict_a.keys():
        # Floating point values must be handled separately
        if isinstance(dict_a[key], float) and isinstance(dict_b[key], float):
            if abs(dict_a[key] - dict_b[key]) > precision:
                return False
        elif isinstance(dict_a[key], float) or isinstance(dict_b[key], float):
            warning(_("Mixing value types. Will try to compare after "
                      "integer conversion"))
            return int(dict_a[key]) == int(dict_b[key])
        elif key == "+towgs84":
            # We compare the sum of the entries
            if abs(sum(dict_a[key]) - sum(dict_b[key])) > precision:
                return False
        else:
            if dict_a[key] != dict_b[key]:
                return False
    return True

# interface to g.gisenv


def gisenv(env=None):
    """Returns the output from running g.gisenv (with no arguments), as a
    dictionary. Example:

    >>> env = gisenv()
    >>> print(env['GISDBASE'])  # doctest: +SKIP
    /opt/grass-data

    :param env run with different environment
    :return: list of GRASS variables
    """
    s = read_command("g.gisenv", flags='n', env=env)
    return parse_key_val(s)

# interface to g.region


def locn_is_latlong():
    """Tests if location is lat/long. Value is obtained
    by checking the "g.region -pu" projection code.

    :return: True for a lat/long region, False otherwise
    """
    s = read_command("g.region", flags='pu')
    kv = parse_key_val(s, ':')
    if kv['projection'].split(' ')[0] == '3':
        return True
    else:
        return False


def region(region3d=False, complete=False, env=None):
    """Returns the output from running "g.region -gu", as a
    dictionary. Example:

    :param bool region3d: True to get 3D region
    :param bool complete:
    :param env env

    >>> curent_region = region()
    >>> # obtain n, s, e and w values
    >>> [curent_region[key] for key in "nsew"]  # doctest: +ELLIPSIS
    [..., ..., ..., ...]
    >>> # obtain ns and ew resulutions
    >>> (curent_region['nsres'], curent_region['ewres'])  # doctest: +ELLIPSIS
    (..., ...)

    :return: dictionary of region values
    """
    flgs = 'gu'
    if region3d:
        flgs += '3'
    if complete:
        flgs += 'cep'

    s = read_command("g.region", flags=flgs, env=env)
    reg = parse_key_val(s, val_type=float)
    for k in ['projection', 'zone', 'rows',  'cols',  'cells',
              'rows3', 'cols3', 'cells3', 'depths']:
        if k not in reg:
            continue
        reg[k] = int(reg[k])

    return reg


def region_env(region3d=False, flags=None, env=None, **kwargs):
    """Returns region settings as a string which can used as
    GRASS_REGION environmental variable.

    If no 'kwargs' are given then the current region is used. Note
    that this function doesn't modify the current region!

    See also :func:`use_temp_region()` for alternative method how to define
    temporary region used for raster-based computation.

    :param bool region3d: True to get 3D region
    :param string flags: for example 'a'
    :param env: different environment than current
    :param kwargs: g.region's parameters like 'raster', 'vector' or 'region'

    ::

        os.environ['GRASS_REGION'] = grass.region_env(region='detail')
        grass.mapcalc('map=1', overwrite=True)
        os.environ.pop('GRASS_REGION')

    :return: string with region values
    :return: empty string on error
    """
    # read proj/zone from WIND file
    gis_env = gisenv(env)
    windfile = os.path.join(gis_env['GISDBASE'], gis_env['LOCATION_NAME'],
                            gis_env['MAPSET'], "WIND")
    with open(windfile, "r") as fd:
        grass_region = ''
        for line in fd.readlines():
            key, value = map(lambda x: x.strip(), line.split(":", 1))
            if kwargs and key not in ('proj', 'zone'):
                continue
            if not kwargs and not region3d and \
                    key in ('top', 'bottom', 'cols3', 'rows3',
                            'depths', 'e-w resol3', 'n-s resol3', 't-b resol'):
                continue

            grass_region += '%s: %s;' % (key, value)

    if not kwargs:  # return current region
        return grass_region

    # read other values from `g.region -gu`
    flgs = 'ug'
    if region3d:
        flgs += '3'
    if flags:
        flgs += flags

    s = read_command('g.region', flags=flgs, env=env, **kwargs)
    if not s:
        return ''
    reg = parse_key_val(s)

    kwdata = [('north',     'n'),
              ('south',     's'),
              ('east',      'e'),
              ('west',      'w'),
              ('cols',      'cols'),
              ('rows',      'rows'),
              ('e-w resol', 'ewres'),
              ('n-s resol', 'nsres')]
    if region3d:
        kwdata += [('top',        't'),
                   ('bottom',     'b'),
                   ('cols3',      'cols3'),
                   ('rows3',      'rows3'),
                   ('depths',     'depths'),
                   ('e-w resol3', 'ewres3'),
                   ('n-s resol3', 'nsres3'),
                   ('t-b resol',  'tbres')]

    for wkey, rkey in kwdata:
        grass_region += '%s: %s;' % (wkey, reg[rkey])

    return grass_region


def use_temp_region():
    """Copies the current region to a temporary region with "g.region save=",
    then sets WIND_OVERRIDE to refer to that region. Installs an atexit
    handler to delete the temporary region upon termination.
    """
    name = "tmp.%s.%d" % (os.path.basename(sys.argv[0]), os.getpid())
    run_command("g.region", save=name, overwrite=True)
    os.environ['WIND_OVERRIDE'] = name
    atexit.register(del_temp_region)


def del_temp_region():
    """Unsets WIND_OVERRIDE and removes any region named by it."""
    try:
        name = os.environ.pop('WIND_OVERRIDE')
        run_command("g.remove", flags='f', quiet=True, type='region', name=name)
    except:
        pass

# interface to g.findfile


def find_file(name, element='cell', mapset=None):
    """Returns the output from running g.findfile as a
    dictionary. Example:

    >>> result = find_file('elevation', element='cell')
    >>> print(result['fullname'])
    elevation@PERMANENT
    >>> print(result['file'])  # doctest: +ELLIPSIS
    /.../PERMANENT/cell/elevation


    :param str name: file name
    :param str element: element type (default 'cell')
    :param str mapset: mapset name (default all mapsets in search path)

    :return: parsed output of g.findfile
    """
    if element == 'raster' or element == 'rast':
        verbose(_('Element type should be "cell" and not "%s"') % element)
        element = 'cell'
    # g.findfile returns non-zero when file was not found
    # se we ignore return code and just focus on stdout
    process = start_command('g.findfile', flags='n',
                            element=element, file=name, mapset=mapset,
                            stdout=PIPE)
    stdout = process.communicate()[0]
    return parse_key_val(stdout)

# interface to g.list


def list_strings(type, pattern=None, mapset=None, exclude=None, flag=''):
    """List of elements as strings.

    Returns the output from running g.list, as a list of qualified
    names.

    :param str type: element type (raster, vector, raster_3d, region, ...)
    :param str pattern: pattern string
    :param str mapset: mapset name (if not given use search path)
    :param str exclude: pattern string to exclude maps from the research
    :param str flag: pattern type: 'r' (basic regexp), 'e' (extended regexp),
                     or '' (glob pattern)

    :return: list of elements
    """
    if type == 'cell':
        verbose(_('Element type should be "raster" and not "%s"') % type)

    result = list()
    for line in read_command("g.list",
                             quiet=True,
                             flags='m' + flag,
                             type=type,
                             pattern=pattern,
                             exclude=exclude,
                             mapset=mapset).splitlines():
        result.append(line.strip())

    return result


def list_pairs(type, pattern=None, mapset=None, exclude=None, flag=''):
    """List of elements as pairs

    Returns the output from running g.list, as a list of
    (name, mapset) pairs

    :param str type: element type (raster, vector, raster_3d, region, ...)
    :param str pattern: pattern string
    :param str mapset: mapset name (if not given use search path)
    :param str exclude: pattern string to exclude maps from the research
    :param str flag: pattern type: 'r' (basic regexp), 'e' (extended regexp),
                     or '' (glob pattern)

    :return: list of elements
    """
    return [tuple(map.split('@', 1)) for map in list_strings(type, pattern,
                                                              mapset, exclude,
                                                              flag)]


def list_grouped(type, pattern=None, check_search_path=True, exclude=None,
                 flag=''):
    """List of elements grouped by mapsets.

    Returns the output from running g.list, as a dictionary where the
    keys are mapset names and the values are lists of maps in that
    mapset. Example:

    >>> list_grouped('vect', pattern='*roads*')['PERMANENT']
    ['railroads', 'roadsmajor']

    :param str type: element type (raster, vector, raster_3d, region, ...) or list of elements
    :param str pattern: pattern string
    :param str check_search_path: True to add mapsets for the search path
                                  with no found elements
    :param str exclude: pattern string to exclude maps from the research
    :param str flag: pattern type: 'r' (basic regexp), 'e' (extended regexp),
                                    or '' (glob pattern)

    :return: directory of mapsets/elements
    """
    if isinstance(type, str) or len(type) == 1:
        types = [type]
        store_types = False
    else:
        types = type
        store_types = True
        flag += 't'
    for i in range(len(types)):
        if types[i] == 'cell':
            verbose(_('Element type should be "raster" and not "%s"') % types[i])
            types[i] = 'raster'
    result = {}
    if check_search_path:
        for mapset in mapsets(search_path=True):
            if store_types:
                result[mapset] = {}
            else:
                result[mapset] = []

    mapset = None
    for line in read_command("g.list", quiet=True, flags="m" + flag,
                             type=types, pattern=pattern, exclude=exclude).splitlines():
        try:
            name, mapset = line.split('@')
        except ValueError:
            warning(_("Invalid element '%s'") % line)
            continue

        if store_types:
            type_, name = name.split('/')
            if mapset in result:
                if type_ in result[mapset]:
                    result[mapset][type_].append(name)
                else:
                    result[mapset][type_] = [name, ]
            else:
                result[mapset] = {type_: [name, ]}
        else:
            if mapset in result:
                result[mapset].append(name)
            else:
                result[mapset] = [name, ]

    return result

# color parsing

named_colors = {
    "white":   (1.00, 1.00, 1.00),
    "black":   (0.00, 0.00, 0.00),
    "red":     (1.00, 0.00, 0.00),
    "green":   (0.00, 1.00, 0.00),
    "blue":    (0.00, 0.00, 1.00),
    "yellow":  (1.00, 1.00, 0.00),
    "magenta": (1.00, 0.00, 1.00),
    "cyan":    (0.00, 1.00, 1.00),
    "aqua":    (0.00, 0.75, 0.75),
    "grey":    (0.75, 0.75, 0.75),
    "gray":    (0.75, 0.75, 0.75),
    "orange":  (1.00, 0.50, 0.00),
    "brown":   (0.75, 0.50, 0.25),
    "purple":  (0.50, 0.00, 1.00),
    "violet":  (0.50, 0.00, 1.00),
    "indigo":  (0.00, 0.50, 1.00)}


def parse_color(val, dflt=None):
    """Parses the string "val" as a GRASS colour, which can be either one of
    the named colours or an R:G:B tuple e.g. 255:255:255. Returns an
    (r,g,b) triple whose components are floating point values between 0
    and 1. Example:

    >>> parse_color("red")
    (1.0, 0.0, 0.0)
    >>> parse_color("255:0:0")
    (1.0, 0.0, 0.0)

    :param val: color value
    :param dflt: default color value

    :return: tuple RGB
    """
    if val in named_colors:
        return named_colors[val]

    vals = val.split(':')
    if len(vals) == 3:
        return tuple(float(v) / 255 for v in vals)

    return dflt

# check GRASS_OVERWRITE


def overwrite():
    """Return True if existing files may be overwritten"""
    owstr = 'GRASS_OVERWRITE'
    return owstr in os.environ and os.environ[owstr] != '0'

# check GRASS_VERBOSE


def verbosity():
    """Return the verbosity level selected by GRASS_VERBOSE"""
    vbstr = os.getenv('GRASS_VERBOSE')
    if vbstr:
        return int(vbstr)
    else:
        return 2

## various utilities, not specific to GRASS

def find_program(pgm, *args):
    """Attempt to run a program, with optional arguments.

    You must call the program in a way that will return a successful
    exit code. For GRASS modules this means you need to pass it some
    valid CLI option, like "--help". For other programs a common
    valid do-little option is usually "--version".

    Example:

    >>> find_program('r.sun', '--help')
    True
    >>> find_program('ls', '--version')
    True

    :param str pgm: program name
    :param args: list of arguments

    :return: False if the attempt failed due to a missing executable
            or non-zero return code
    :return: True otherwise
    """
    nuldev = open(os.devnull, 'w+')
    try:
        # TODO: the doc or impl is not correct, any return code is accepted
        call([pgm] + list(args), stdin = nuldev, stdout = nuldev, stderr = nuldev)
        found = True
    except:
        found = False
    nuldev.close()

    return found

# interface to g.mapsets


def mapsets(search_path=False):
    """List available mapsets

    :param bool search_path: True to list mapsets only in search path

    :return: list of mapsets
    """
    if search_path:
        flags = 'p'
    else:
        flags = 'l'
    mapsets = read_command('g.mapsets',
                           flags=flags,
                           sep='newline',
                           quiet=True)
    if not mapsets:
        fatal(_("Unable to list mapsets"))

    return mapsets.splitlines()

# interface to `g.proj -c`


def create_location(dbase, location, epsg=None, proj4=None, filename=None,
                    wkt=None, datum=None, datum_trans=None, desc=None,
                    overwrite=False):
    """Create new location

    Raise ScriptError on error.

    :param str dbase: path to GRASS database
    :param str location: location name to create
    :param epsg: if given create new location based on EPSG code
    :param proj4: if given create new location based on Proj4 definition
    :param str filename: if given create new location based on georeferenced file
    :param str wkt: if given create new location based on WKT definition
                    (path to PRJ file)
    :param datum: GRASS format datum code
    :param datum_trans: datum transformation parameters (used for epsg and proj4)
    :param desc: description of the location (creates MYNAME file)
    :param bool overwrite: True to overwrite location if exists(WARNING:
                           ALL DATA from existing location ARE DELETED!)
    """
    gisdbase = None
    if epsg or proj4 or filename or wkt:
        # FIXME: changing GISDBASE mid-session is not background-job safe
        gisdbase = gisenv()['GISDBASE']
        run_command('g.gisenv', set='GISDBASE=%s' % dbase)
    # create dbase if not exists
    if not os.path.exists(dbase):
            os.mkdir(dbase)

    # check if location already exists
    if os.path.exists(os.path.join(dbase, location)):
        if not overwrite:
            warning(_("Location <%s> already exists. Operation canceled.") % location)
            return
        else:
            warning(_("Location <%s> already exists and will be overwritten") % location)
            shutil.rmtree(os.path.join(dbase, location))

    kwargs = dict()
    if datum:
        kwargs['datum'] = datum
    if datum_trans:
        kwargs['datum_trans'] = datum_trans

    if epsg:
        ps = pipe_command('g.proj', quiet=True, flags='t', epsg=epsg,
                          location=location, stderr=PIPE, **kwargs)
    elif proj4:
        ps = pipe_command('g.proj', quiet=True, flags='t', proj4=proj4,
                          location=location, stderr=PIPE, **kwargs)
    elif filename:
        ps = pipe_command('g.proj', quiet=True, georef=filename,
                          location=location, stderr=PIPE)
    elif wkt:
        ps = pipe_command('g.proj', quiet=True, wkt=wkt, location=location,
                          stderr=PIPE)
    else:
        _create_location_xy(dbase, location)

    if epsg or proj4 or filename or wkt:
        error = ps.communicate()[1]
        run_command('g.gisenv', set='GISDBASE=%s' % gisdbase)

        if ps.returncode != 0 and error:
            raise ScriptError(repr(error))

    try:
        fd = codecs.open(os.path.join(dbase, location, 'PERMANENT', 'MYNAME'),
                         encoding='utf-8', mode='w')
        if desc:
            fd.write(desc + os.linesep)
        else:
            fd.write(os.linesep)
        fd.close()
    except OSError as e:
        raise ScriptError(repr(e))


def _create_location_xy(database, location):
    """Create unprojected location

    Raise ScriptError on error.

    :param database: GRASS database where to create new location
    :param location: location name
    """
    cur_dir = os.getcwd()
    try:
        os.chdir(database)
        os.mkdir(location)
        os.mkdir(os.path.join(location, 'PERMANENT'))

        # create DEFAULT_WIND and WIND files
        regioninfo = ['proj:       0',
                      'zone:       0',
                      'north:      1',
                      'south:      0',
                      'east:       1',
                      'west:       0',
                      'cols:       1',
                      'rows:       1',
                      'e-w resol:  1',
                      'n-s resol:  1',
                      'top:        1',
                      'bottom:     0',
                      'cols3:      1',
                      'rows3:      1',
                      'depths:     1',
                      'e-w resol3: 1',
                      'n-s resol3: 1',
                      't-b resol:  1']

        defwind = open(os.path.join(location,
                                    "PERMANENT", "DEFAULT_WIND"), 'w')
        for param in regioninfo:
            defwind.write(param + '%s' % os.linesep)
        defwind.close()

        shutil.copy(os.path.join(location, "PERMANENT", "DEFAULT_WIND"),
                    os.path.join(location, "PERMANENT", "WIND"))

        os.chdir(cur_dir)
    except OSError as e:
        raise ScriptError(repr(e))

# interface to g.version


def version():
    """Get GRASS version as dictionary

    ::

        >>> print(version())
        {'proj4': '4.8.0', 'geos': '3.3.5', 'libgis_revision': '52468',
         'libgis_date': '2012-07-27 22:53:30 +0200 (Fri, 27 Jul 2012)',
         'version': '7.0.svn', 'date': '2012', 'gdal': '2.0dev',
         'revision': '53670'}

    """
    data = parse_command('g.version', flags='rge', errors='ignore')
    for k, v in data.items():
        data[k.strip()] = v.replace('"', '').strip()

    return data

# get debug_level
_debug_level = None


def debug_level(force=False):
    global _debug_level
    if not force and _debug_level is not None:
        return _debug_level
    _debug_level = 0
    if find_program('g.gisenv', '--help'):
        try:
            _debug_level = int(gisenv().get('DEBUG', 0))
            if _debug_level < 0 or _debug_level > 5:
                raise ValueError(_("Debug level {0}").format(_debug_level))
        except ValueError as e:
            _debug_level = 0
            sys.stderr.write(_("WARNING: Ignoring unsupported debug level (must be >=0 and <=5). {0}\n").format(e))

    return _debug_level


def legal_name(s):
    """Checks if the string contains only allowed characters.

    This is the Python implementation of :func:`G_legal_filename()` function.

    ..note::

        It is not clear when exactly use this function, but it might be
        useful anyway for checking map names and column names.
    """
    if not s or s[0] == '.':
        warning(_("Illegal filename <%s>. Cannot be 'NULL' or start with " \
                  "'.'.") % s)
        return False

    illegal = [c
               for c in s
               if c in '/"\'@,=*~' or c <= ' ' or c >= '\177']
    if illegal:
        illegal = ''.join(sorted(set(illegal)))
        warning(_("Illegal filename <%(s)s>. <%(il)s> not allowed.\n") % {
        's': s, 'il': illegal})
        return False

    return True


def create_environment(gisdbase, location, mapset):
    """Creates environment to be passed in run_command for example.
    Returns tuple with temporary file path and the environment. The user
    of this function is responsile for deleting the file."""
    tmp_gisrc_file = tempfile()
    with open(tmp_gisrc_file, 'w') as f:
        f.write('MAPSET: {mapset}\n'.format(mapset=mapset))
        f.write('GISDBASE: {g}\n'.format(g=gisdbase))
        f.write('LOCATION_NAME: {l}\n'.format(l=location))
        f.write('GUI: text\n')
    env = os.environ.copy()
    env['GISRC'] = tmp_gisrc_file
    return tmp_gisrc_file, env


if __name__ == '__main__':
    import doctest
    doctest.testmod()
