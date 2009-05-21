"""
@package grass.script.core

@brief GRASS Python scripting module

Core functions to be used in Python scripts.

Usage:

@code
from grass.script import core as grass

grass.parser()
...
@endcode

(C) 2008-2009 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Glynn Clements
@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import types
import re
import atexit
import subprocess

# subprocess wrapper that uses shell on Windows

class Popen(subprocess.Popen):
    def __init__(self, args, bufsize=0, executable=None,
                 stdin=None, stdout=None, stderr=None,
                 preexec_fn=None, close_fds=False, shell=None,
                 cwd=None, env=None, universal_newlines=False,
                 startupinfo=None, creationflags=0):

	if shell == None:
	    shell = (sys.platform == "win32")

	subprocess.Popen.__init__(self, args, bufsize, executable,
                                  stdin, stdout, stderr,
                                  preexec_fn, close_fds, shell,
                                  cwd, env, universal_newlines,
                                  startupinfo, creationflags)
        
PIPE = subprocess.PIPE
STDOUT = subprocess.STDOUT

def call(*args, **kwargs):
    return Popen(*args, **kwargs).wait()

# GRASS-oriented interface to subprocess module

_popen_args = ["bufsize", "executable", "stdin", "stdout", "stderr",
	       "preexec_fn", "close_fds", "cwd", "env",
	       "universal_newlines", "startupinfo", "creationflags"]

def _make_val(val):
    if isinstance(val, types.StringType):
	return val
    if isinstance(val, types.ListType):
	return ",".join(map(_make_val, val))
    if isinstance(val, types.TupleType):
	return _make_val(list(val))
    return str(val)

def make_command(prog, flags = "", overwrite = False, quiet = False, verbose = False, **options):
    """Return a list of strings suitable for use as the args parameter to
    Popen() or call(). Example:

    >>> grass.make_command("g.message", flags = 'w', message = 'this is a warning')
    ['g.message', '-w', 'message=this is a warning']
    """
    args = [prog]
    if overwrite:
	args.append("--o")
    if quiet:
	args.append("--q")
    if verbose:
	args.append("--v")
    if flags:
	args.append("-%s" % flags)
    for opt, val in options.iteritems():
	if val != None:
	    if opt[0] == '_':
		opt = opt[1:]
	    args.append("%s=%s" % (opt, _make_val(val)))
    return args

def start_command(prog, flags = "", overwrite = False, quiet = False, verbose = False, **kwargs):
    """Returns a Popen object with the command created by make_command.
    Accepts any of the arguments which Popen() accepts apart from "args"
    and "shell".
    """
    options = {}
    popts = {}
    for opt, val in kwargs.iteritems():
	if opt in _popen_args:
	    popts[opt] = val
	else:
	    options[opt] = val
    args = make_command(prog, flags, overwrite, quiet, verbose, **options)
    return Popen(args, **popts)

def run_command(*args, **kwargs):
    """Passes all arguments to start_command, then waits for the process to
    complete, returning its exit code. Similar to subprocess.call(), but
    with the make_command() interface.
    """
    ps = start_command(*args, **kwargs)
    return ps.wait()

def pipe_command(*args, **kwargs):
    """Passes all arguments to start_command, but also adds
    "stdout = PIPE". Returns the Popen object.
    """
    kwargs['stdout'] = PIPE
    return start_command(*args, **kwargs)

def feed_command(*args, **kwargs):
    """Passes all arguments to start_command, but also adds
    "stdin = PIPE". Returns the Popen object.
    """
    kwargs['stdin'] = PIPE
    return start_command(*args, **kwargs)

def read_command(*args, **kwargs):
    """Passes all arguments to pipe_command, then waits for the process to
    complete, returning its stdout (i.e. similar to shell `backticks`).
    """
    ps = pipe_command(*args, **kwargs)
    return ps.communicate()[0]

def parse_command(*args, **kwargs):
    """Passes all arguments to read_command, then parses the output by
    parse_key_val().

    Parsing function can be optionally given by <b>parse</b> parameter
    including its arguments, e.g.

    @code
    parse_command(..., parse = (grass.parse_key_val, { 'sep' : ':' }))
    @endcode
    """
    parse = None
    if kwargs.has_key('parse'):
        if type(kwargs['parse']) is types.TupleType:
            parse = kwargs['parse'][0]
            parse_args = kwargs['parse'][1]
        del kwargs['parse']
    
    if not parse:
        parse = parse_key_val # use default fn
        parse_args = {}
        
    res = read_command(*args, **kwargs)

    return parse(res, **parse_args)

def write_command(*args, **kwargs):
    """Passes all arguments to feed_command, with the string specified
    by the 'stdin' argument fed to the process' stdin.
    """
    stdin = kwargs['stdin']
    p = feed_command(*args, **kwargs)
    p.stdin.write(stdin)
    p.stdin.close()
    return p.wait()

def exec_command(prog, flags = "", overwrite = False, quiet = False, verbose = False, env = None, **kwargs):
    """Interface to os.execvpe(), but with the make_command() interface."""
    args = make_command(prog, flags, overwrite, quiet, verbose, **kwargs)
    if env == None:
	env = os.environ
    os.execvpe(prog, args, env)

# interface to g.message

def message(msg, flag = None):
    """Display a message using g.message"""
    run_command("g.message", flags = flag, message = msg)

def debug(msg, debug = 1):
    """Display a debugging message using g.message -d"""
    run_command("g.message", flags = 'd', message = msg, debug = debug)
    
def verbose(msg):
    """Display a verbose message using g.message -v"""
    message(msg, flag = 'v')

def info(msg):
    """Display an informational message using g.message -i"""
    message(msg, flag = 'i')

def warning(msg):
    """Display a warning message using g.message -w"""
    message(msg, flag = 'w')

def error(msg):
    """Display an error message using g.message -e"""
    message(msg, flag = 'e')

def fatal(msg):
    """Display an error message using g.message -e, then abort"""
    error(msg)
    sys.exit(1)

# interface to g.parser

def _parse_env():
    options = {}
    flags = {}
    for var, val in os.environ.iteritems():
	if var.startswith("GIS_OPT_"):
	    opt = var.replace("GIS_OPT_", "", 1).lower()
	    options[opt] = val;
	if var.startswith("GIS_FLAG_"):
	    flg = var.replace("GIS_FLAG_", "", 1).lower()
	    flags[flg] = bool(int(val));
    return (options, flags)

def parser():
    """Interface to g.parser, intended to be run from the top-level, e.g.:

	if __name__ == "__main__":
	    options, flags = grass.parser()
	    main()

    Thereafter, the global variables "options" and "flags" will be
    dictionaries containing option/flag values, keyed by lower-case
    option/flag names. The values in "options" are strings, those in
    "flags" are Python booleans.
    """
    if not os.getenv("GISBASE"):
        print >> sys.stderr, "You must be in GRASS GIS to run this program."
        sys.exit(1)

    if len(sys.argv) > 1 and sys.argv[1] == "@ARGS_PARSED@":
	return _parse_env()

    cmdline = [basename(sys.argv[0])]
    cmdline += ['"' + arg + '"' for arg in sys.argv[1:]]
    os.environ['CMDLINE'] = ' '.join(cmdline)

    argv = sys.argv[:]
    name = argv[0]
    if not os.path.isabs(name):
	if os.sep in name or (os.altsep and os.altsep in name):
	    argv[0] = os.path.abspath(name)
	else:
	    argv[0] = os.path.join(sys.path[0], name)

    if sys.platform == "win32":
       os.execvp("g.parser.exe", [name] + argv)
    else:
       os.execvp("g.parser", [name] + argv)
    raise OSError("error executing g.parser")

# interface to g.tempfile

def tempfile():
    """Returns the name of a temporary file, created with g.tempfile."""
    return read_command("g.tempfile", pid = os.getpid()).strip()

# key-value parsers

def parse_key_val(s, sep = '=', dflt = None, val_type = None, vsep = None):
    """Parse a string into a dictionary, where entries are separated
    by newlines and the key and value are separated by `sep' (default: `=')
    """
    result = {}

    if not s:
        return result
    
    if vsep:
        lines = s.split(vsep)
        try:
            lines.remove('\n')
        except ValueError:
            pass
    else:
        lines = s.splitlines()
    
    for line in lines:
	kv = line.split(sep, 1)
	k = kv[0].strip()
	if len(kv) > 1:
	    v = kv[1]
	else:
	    v = dflt
        if val_type:
            result[k] = val_type(v)
        else:
            result[k] = v
    return result

# interface to g.gisenv

def gisenv():
    """Returns the output from running g.gisenv (with no arguments), as a
    dictionary.
    """
    s = read_command("g.gisenv", flags='n')
    return parse_key_val(s)

# interface to g.region

def region():
    """Returns the output from running "g.region -g", as a dictionary."""
    s = read_command("g.region", flags='g')
    return parse_key_val(s, val_type = float)

def use_temp_region():
    """Copies the current region to a temporary region with "g.region save=",
    then sets WIND_OVERRIDE to refer to that region. Installs an atexit
    handler to delete the temporary region upon termination.
    """
    name = "tmp.%s.%d" % (os.path.basename(sys.argv[0]), os.getpid())
    run_command("g.region", save = name)
    os.environ['WIND_OVERRIDE'] = name
    atexit.register(del_temp_region)

def del_temp_region():
    """Unsets WIND_OVERRIDE and removes any region named by it."""
    try:
	name = os.environ.pop('WIND_OVERRIDE')
	run_command("g.remove", quiet = True, region = name)
    except:
	pass

# interface to g.findfile

def find_file(name, element = 'cell', mapset = None):
    """Returns the output from running g.findfile as a dictionary."""
    s = read_command("g.findfile", flags='n', element = element, file = name, mapset = mapset)
    return parse_key_val(s)

# interface to g.list

def list_grouped(type):
    """Returns the output from running g.list, as a dictionary where the keys
    are mapset names and the values are lists of maps in that mapset. 
    """
    dashes_re = re.compile("^----+$")
    mapset_re = re.compile("<(.*)>")
    result = {}
    mapset = None
    for line in read_command("g.list", type = type).splitlines():
	if line == "":
	    continue
	if dashes_re.match(line):
	    continue
	m = mapset_re.search(line)
	if m:
	    mapset = m.group(1)
	    result[mapset] = []
	    continue
        if mapset:
            result[mapset].extend(line.split())
    return result

def mlist_grouped(type, mapset = None, pattern = None):
    """Returns the output from running g.mlist, as a dictionary where the keys
    are mapset names and the values are lists of maps in that mapset. 
    """
    result = {}
    mapset_element = None
    for line in read_command("g.mlist", flags="m",
                             type = type, mapset = mapset, pattern = pattern).splitlines():
        try:
            map, mapset_element = line.split('@')
        except ValueError:
            print >> sys.stderr, "Invalid element '%s'" % line
            continue
        
        if result.has_key(mapset_element):
            result[mapset_element].append(map)
        else:
	    result[mapset_element] = [map, ]
    
    return result

def _concat(xs):
    result = []
    for x in xs:
	result.extend(x)
    return result

def list_pairs(type):
    """Returns the output from running g.list, as a list of (map, mapset)
    pairs.
    """
    return _concat([[(map, mapset) for map in maps]
		    for mapset, maps in list_grouped(type).iteritems()])

def list_strings(type):
    """Returns the output from running g.list, as a list of qualified names."""
    return ["%s@%s" % pair for pair in list_pairs(type)]

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

def parse_color(val, dflt = None):
    """Parses the string "val" as a GRASS colour, which can be either one of
    the named colours or an R:G:B tuple e.g. 255:255:255. Returns an
    (r,g,b) triple whose components are floating point values between 0
    and 1.
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
	return 0

## various utilities, not specific to GRASS

# basename inc. extension stripping

def basename(path, ext = None):
    """Remove leading directory components and an optional extension
    from the specified path
    """
    name = os.path.basename(path)
    if not ext:
	return name
    fs = name.rsplit('.', 1)
    if len(fs) > 1 and fs[1].lower() == ext:
	name = fs[0]
    return name

# find a program (replacement for "which")

def find_program(pgm, args = []):
    """Attempt to run a program, with optional arguments. Return False
    if the attempt failed due to a missing executable, True otherwise
    """
    nuldev = file(os.devnull, 'w+')
    try:
	call([pgm] + args, stdin = nuldev, stdout = nuldev, stderr = nuldev)
	found = True
    except:
	found = False
    nuldev.close()
    return found

# try to remove a file, without complaints

def try_remove(path):
    """Attempt to remove a file; no exception is generated if the
    attempt fails.
    """
    try:
	os.remove(path)
    except:
	pass

# try to remove a directory, without complaints

def try_rmdir(path):
    """Attempt to remove a directory; no exception is generated if the
    attempt fails.
    """
    try:
	os.rmdir(path)
    except:
	pass

def float_or_dms(s):
    return sum(float(x) / 60 ** n for (n, x) in enumerate(s.split(':')))
