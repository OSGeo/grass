"""!@package grass.script.core

@brief GRASS Python scripting module (core functions)

Core functions to be used in Python scripts.

Usage:

@code
from grass.script import core as grass

grass.parser()
...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Glynn Clements
@author Martin Landa <landa.martin gmail.com>
@author Michael Barton <michael.barton asu.edu>
"""

import os
import sys
import types
import re
import atexit
import subprocess
import shutil
import locale
import codecs

# i18N
import gettext
gettext.install('grasslibs', os.path.join(os.getenv("GISBASE"), 'locale'), unicode=True)

# subprocess wrapper that uses shell on Windows

class Popen(subprocess.Popen):
    def __init__(self, args, bufsize = 0, executable = None,
                 stdin = None, stdout = None, stderr = None,
                 preexec_fn = None, close_fds = False, shell = None,
                 cwd = None, env = None, universal_newlines = False,
                 startupinfo = None, creationflags = 0):

	if shell == None:
	    shell = (sys.platform == "win32")

	subprocess.Popen.__init__(self, args, bufsize, executable,
                                  stdin, stdout, stderr,
                                  preexec_fn, close_fds, shell,
                                  cwd, env, universal_newlines,
                                  startupinfo, creationflags)
        
PIPE = subprocess.PIPE
STDOUT = subprocess.STDOUT

class ScriptError(Exception):
    def __init__(self, msg):
        self.value = msg
    
    def __str__(self):
        return repr(self.value)
        
raise_on_error = False # raise exception instead of calling fatal()
debug_level = 0        # DEBUG level

def call(*args, **kwargs):
    return Popen(*args, **kwargs).wait()

# GRASS-oriented interface to subprocess module

_popen_args = ["bufsize", "executable", "stdin", "stdout", "stderr",
	       "preexec_fn", "close_fds", "cwd", "env",
	       "universal_newlines", "startupinfo", "creationflags"]

def _decode(string):
    enc = locale.getdefaultlocale()[1]
    if enc:
        return string.decode(enc)
    
    return string

def _make_val(val):
    if isinstance(val, types.StringType) or \
            isinstance(val, types.UnicodeType):
	return val
    if isinstance(val, types.ListType):
	return ",".join(map(_make_val, val))
    if isinstance(val, types.TupleType):
	return _make_val(list(val))
    return str(val)

def make_command(prog, flags = "", overwrite = False, quiet = False, verbose = False, **options):
    """!Return a list of strings suitable for use as the args parameter to
    Popen() or call(). Example:

    @code
    >>> grass.make_command("g.message", flags = 'w', message = 'this is a warning')
    ['g.message', '-w', 'message=this is a warning']
    @endcode

    @param prog GRASS module
    @param flags flags to be used (given as a string)
    @param overwrite True to enable overwriting the output (<tt>--o</tt>)
    @param quiet True to run quietly (<tt>--q</tt>)
    @param verbose True to run verbosely (<tt>--v</tt>)
    @param options module's parameters

    @return list of arguments
    """
    args = [prog]
    if overwrite:
	args.append("--o")
    if quiet:
	args.append("--q")
    if verbose:
	args.append("--v")
    if flags:
        if '-' in flags:
            raise ScriptError("'-' is not a valid flag")
	args.append("-%s" % flags)
    for opt, val in options.iteritems():
	if val != None:
	    if opt[0] == '_':
		opt = opt[1:]
	    args.append("%s=%s" % (opt, _make_val(val)))
    return args

def start_command(prog, flags = "", overwrite = False, quiet = False, verbose = False, **kwargs):
    """!Returns a Popen object with the command created by make_command.
    Accepts any of the arguments which Popen() accepts apart from "args"
    and "shell".

    \code
    >>> p = grass.start_command("g.gisenv", stdout = subprocess.PIPE)
    >>> print p
    <subprocess.Popen object at 0xb7c12f6c>
    >>> print p.communicate()[0]
    GISDBASE='/opt/grass-data';
    LOCATION_NAME='spearfish60';
    MAPSET='glynn';
    GRASS_DB_ENCODING='ascii';
    GRASS_GUI='text';
    MONITOR='x0';
    \endcode
    
    @param prog GRASS module
    @param flags flags to be used (given as a string)
    @param overwrite True to enable overwriting the output (<tt>--o</tt>)
    @param quiet True to run quietly (<tt>--q</tt>)
    @param verbose True to run verbosely (<tt>--v</tt>)
    @param kwargs module's parameters
    
    @return Popen object
    """
    options = {}
    popts = {}
    for opt, val in kwargs.iteritems():
	if opt in _popen_args:
	    popts[opt] = val
	else:
	    options[opt] = val
    args = make_command(prog, flags, overwrite, quiet, verbose, **options)

    global debug_level
    if debug_level > 0:
        sys.stderr.write("D1/%d: %s.start_command(): %s\n" % (debug_level, __name__, ' '.join(args)))
        sys.stderr.flush()
    
    return Popen(args, **popts)

def run_command(*args, **kwargs):
    """!Passes all arguments to start_command(), then waits for the process to
    complete, returning its exit code. Similar to subprocess.call(), but
    with the make_command() interface.

    @param args list of unnamed arguments (see start_command() for details)
    @param kwargs list of named arguments (see start_command() for details)

    @return exit code (0 for success)
    """
    ps = start_command(*args, **kwargs)
    return ps.wait()

def pipe_command(*args, **kwargs):
    """!Passes all arguments to start_command(), but also adds
    "stdout = PIPE". Returns the Popen object.

    \code
    >>> p = grass.pipe_command("g.gisenv")
    >>> print p
    <subprocess.Popen object at 0xb7c12f6c>
    >>> print p.communicate()[0]
    GISDBASE='/opt/grass-data';
    LOCATION_NAME='spearfish60';
    MAPSET='glynn';
    GRASS_DB_ENCODING='ascii';
    GRASS_GUI='text';
    MONITOR='x0';
    \endcode
    
    @param args list of unnamed arguments (see start_command() for details)
    @param kwargs list of named arguments (see start_command() for details)

    @return Popen object
    """
    kwargs['stdout'] = PIPE
    return start_command(*args, **kwargs)

def feed_command(*args, **kwargs):
    """!Passes all arguments to start_command(), but also adds
    "stdin = PIPE". Returns the Popen object.

    @param args list of unnamed arguments (see start_command() for details)
    @param kwargs list of named arguments (see start_command() for details)
    
    @return Popen object
    """
    kwargs['stdin'] = PIPE
    return start_command(*args, **kwargs)

def read_command(*args, **kwargs):
    """!Passes all arguments to pipe_command, then waits for the process to
    complete, returning its stdout (i.e. similar to shell `backticks`).

    @param args list of unnamed arguments (see start_command() for details)
    @param kwargs list of named arguments (see start_command() for details)
    
    @return stdout
    """
    ps = pipe_command(*args, **kwargs)
    return ps.communicate()[0]

def parse_command(*args, **kwargs):
    """!Passes all arguments to read_command, then parses the output by
    parse_key_val().

    Parsing function can be optionally given by <b>parse</b> parameter
    including its arguments, e.g.

    @code
    parse_command(..., parse = (grass.parse_key_val, { 'sep' : ':' }))
    @endcode

    @param args list of unnamed arguments (see start_command() for details)
    @param kwargs list of named arguments (see start_command() for details)
    
    @return parsed module output
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
    """!Passes all arguments to feed_command, with the string specified
    by the 'stdin' argument fed to the process' stdin.

    @param args list of unnamed arguments (see start_command() for details)
    @param kwargs list of named arguments (see start_command() for details)

    @return return code
    """
    stdin = kwargs['stdin']
    p = feed_command(*args, **kwargs)
    p.stdin.write(stdin)
    p.stdin.close()
    return p.wait()

def exec_command(prog, flags = "", overwrite = False, quiet = False, verbose = False, env = None, **kwargs):
    """!Interface to os.execvpe(), but with the make_command() interface.

    @param prog GRASS module
    @param flags flags to be used (given as a string)
    @param overwrite True to enable overwriting the output (<tt>--o</tt>)
    @param quiet True to run quietly (<tt>--q</tt>)
    @param verbose True to run verbosely (<tt>--v</tt>)
    @param env directory with enviromental variables
    @param kwargs module's parameters

    """
    args = make_command(prog, flags, overwrite, quiet, verbose, **kwargs)
    if env == None:
	env = os.environ
    os.execvpe(prog, args, env)

# interface to g.message

def message(msg, flag = None):
    """!Display a message using `g.message`

    @param msg message to be displayed
    @param flag flags (given as string)
    """
    run_command("g.message", flags = flag, message = msg)

def debug(msg, debug = 1):
    """!Display a debugging message using `g.message -d`

    @param msg debugging message to be displayed
    @param debug debug level (0-5)
    """
    run_command("g.message", flags = 'd', message = msg, debug = debug)
    
def verbose(msg):
    """!Display a verbose message using `g.message -v`
    
    @param msg verbose message to be displayed
    """
    message(msg, flag = 'v')

def info(msg):
    """!Display an informational message using `g.message -i`

    @param msg informational message to be displayed
    """
    message(msg, flag = 'i')

def percent(i, n, s):
    """!Display a progress info message using `g.message -p`
    
    @code
    message(_("Percent complete..."))
    n = 100
    for i in range(n):
        percent(i, n, 1)
    percent(1, 1, 1)
    @endcode
    
    @param i current item
    @param n total number of items
    @param s increment size
    """
    message("%d %d %d" % (i, n, s), flag = 'p')

def warning(msg):
    """!Display a warning message using `g.message -w`

    @param msg warning message to be displayed
    """
    message(msg, flag = 'w')

def error(msg):
    """!Display an error message using `g.message -e`

    Raise exception when on_error is 'raise'.
    
    @param msg error message to be displayed
    """
    global raise_on_error
    if raise_on_error:
        raise ScriptError(msg)
    else:
        message(msg, flag = 'e')

def fatal(msg):
    """!Display an error message using `g.message -e`, then abort
    
    @param msg error message to be displayed
    """
    error(msg)
    sys.exit(1)
    
def set_raise_on_error(raise_exp = True):
    """!Define behaviour on error (error() called)

    @param raise_exp True to raise ScriptError instead of calling
    error()
    
    @return current status
    """
    global raise_on_error
    tmp_raise = raise_on_error
    raise_on_error = raise_exp

# interface to g.parser

def _parse_opts(lines):
    options = {}
    flags = {}
    for line in lines:
	line = line.rstrip('\r\n')
	if not line:
	    break
	try:
	    [var, val] = line.split('=', 1)
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
    """!Interface to g.parser, intended to be run from the top-level, e.g.:

    @code
	if __name__ == "__main__":
	    options, flags = grass.parser()
	    main()
    @endcode

    Thereafter, the global variables "options" and "flags" will be
    dictionaries containing option/flag values, keyed by lower-case
    option/flag names. The values in "options" are strings, those in
    "flags" are Python booleans.
    """
    if not os.getenv("GISBASE"):
        print >> sys.stderr, "You must be in GRASS GIS to run this program."
        sys.exit(1)
    
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
    
    p = Popen(['g.parser', '-s'] + argv, stdout = PIPE)
    s = p.communicate()[0]
    lines = s.splitlines()
    
    if not lines or lines[0].rstrip('\r\n') != "@ARGS_PARSED@":
	sys.stdout.write(s)
	sys.exit(1)

    return _parse_opts(lines[1:])

# interface to g.tempfile

def tempfile():
    """!Returns the name of a temporary file, created with g.tempfile."""
    return read_command("g.tempfile", pid = os.getpid()).strip()

def tempdir():
    """!Returns the name of a temporary dir, created with g.tempfile."""
    tmp = read_command("g.tempfile", pid = os.getpid()).strip()
    try_remove(tmp)
    os.mkdir(tmp)
    
    return tmp

# key-value parsers

def parse_key_val(s, sep = '=', dflt = None, val_type = None, vsep = None):
    """!Parse a string into a dictionary, where entries are separated
    by newlines and the key and value are separated by `sep' (default: `=')

    @param s string to be parsed
    @param sep key/value separator
    @param dflt default value to be used
    @param val_type value type (None for no cast)
    @param vsep vertical separator (default os.linesep)

    @return parsed input (dictionary of keys/values)
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
    """!Returns the output from running g.gisenv (with no arguments), as a
    dictionary. Example:

    \code
    >>> env = grass.gisenv()
    >>> print env['GISDBASE']
    /opt/grass-data
    \endcode

    @return list of GRASS variables
    """
    s = read_command("g.gisenv", flags='n')
    return parse_key_val(s)

# interface to g.region

def region():
    """!Returns the output from running "g.region -g", as a
    dictionary. Example:

    \code
    >>> region = grass.region()
    >>> [region[key] for key in "nsew"]
    [228500.0, 215000.0, 645000.0, 630000.0]
    >>> (region['nsres'], region['ewres'])
    (10.0, 10.0)
    \endcode

    @return dictionary of region values
    """
    s = read_command("g.region", flags='g')
    reg = parse_key_val(s, val_type = float)
    for k in ['rows', 'cols']:
	reg[k] = int(reg[k])
    return reg

def use_temp_region():
    """!Copies the current region to a temporary region with "g.region save=",
    then sets WIND_OVERRIDE to refer to that region. Installs an atexit
    handler to delete the temporary region upon termination.
    """
    name = "tmp.%s.%d" % (os.path.basename(sys.argv[0]), os.getpid())
    run_command("g.region", save = name, overwrite = True)
    os.environ['WIND_OVERRIDE'] = name
    atexit.register(del_temp_region)

def del_temp_region():
    """!Unsets WIND_OVERRIDE and removes any region named by it."""
    try:
	name = os.environ.pop('WIND_OVERRIDE')
	run_command("g.remove", quiet = True, region = name)
    except:
	pass

# interface to g.findfile

def find_file(name, element = 'cell', mapset = None):
    """!Returns the output from running g.findfile as a
    dictionary. Example:

    \code
    >>> result = grass.find_file('fields', element = 'vector')
    >>> print result['fullname']
    fields@PERMANENT
    >>> print result['file']
    /opt/grass-data/spearfish60/PERMANENT/vector/fields
    \endcode
    
    @param name file name
    @param element element type (default 'cell')
    @param mapset mapset name (default all mapsets in search path)

    @return parsed output of g.findfile
    """
    s = read_command("g.findfile", flags='n', element = element, file = name, mapset = mapset)
    return parse_key_val(s)

# interface to g.list

def list_grouped(type):
    """!List elements grouped by mapsets.

    Returns the output from running g.list, as a dictionary where the
    keys are mapset names and the values are lists of maps in that
    mapset. Example:

    @code
    >>> grass.list_grouped('rast')['PERMANENT']
    ['aspect', 'erosion1', 'quads', 'soils', 'strm.dist', ...
    @endcode
    
    @param type element type (rast, vect, rast3d, region, ...)

    @return directory of mapsets/elements
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

def _concat(xs):
    result = []
    for x in xs:
	result.extend(x)
    return result

def list_pairs(type):
    """!List of elements as tuples.

    Returns the output from running g.list, as a list of (map, mapset)
    pairs. Example:

    @code
    >>> grass.list_pairs('rast')
    [('aspect', 'PERMANENT'), ('erosion1', 'PERMANENT'), ('quads', 'PERMANENT'), ...
    @endcode
    
    @param type element type (rast, vect, rast3d, region, ...)
    
    @return list of tuples (map, mapset)
    """
    return _concat([[(map, mapset) for map in maps]
		    for mapset, maps in list_grouped(type).iteritems()])

def list_strings(type):
    """!List of elements as strings.

    Returns the output from running g.list, as a list of qualified
    names. Example:

    @code
    >>> grass.list_strings('rast')
    ['aspect@PERMANENT', 'erosion1@PERMANENT', 'quads@PERMANENT', 'soils@PERMANENT', ...
    @endcode

    @param type element type
    
    @return list of strings ('map@@mapset')
    """
    return ["%s@%s" % pair for pair in list_pairs(type)]

# interface to g.mlist

def mlist(type, pattern = None, mapset = None):
    """!List of elements

    @param type element type (rast, vect, rast3d, region, ...)
    @param pattern pattern string
    @param mapset mapset name (if not given use search path)

    @return list of elements
    """
    result = list()
    for line in read_command("g.mlist",
                             type = type,
                             pattern = pattern,
                             mapset = mapset).splitlines():
        result.append(line.strip())
    
    return result
    
def mlist_grouped(type, pattern = None):
    """!List of elements grouped by mapsets.

    Returns the output from running g.mlist, as a dictionary where the
    keys are mapset names and the values are lists of maps in that
    mapset. Example:

    @code
    >>> grass.mlist_grouped('rast', pattern='r*')['PERMANENT']
    ['railroads', 'roads', 'rstrct.areas', 'rushmore']
    @endcode
    
    @param type element type (rast, vect, rast3d, region, ...)
    @param pattern pattern string

    @return directory of mapsets/elements
    """
    result = dict()
    mapset_element = None
    for line in read_command("g.mlist", flags = "m",
                             type = type, pattern = pattern).splitlines():
        try:
            map, mapset_element = line.split('@')
        except ValueError:
            warning(_("Invalid element '%s'") % line)
            continue
        
        if result.has_key(mapset_element):
            result[mapset_element].append(map)
        else:
	    result[mapset_element] = [map, ]
    
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

def parse_color(val, dflt = None):
    """!Parses the string "val" as a GRASS colour, which can be either one of
    the named colours or an R:G:B tuple e.g. 255:255:255. Returns an
    (r,g,b) triple whose components are floating point values between 0
    and 1. Example:

    \code
    >>> grass.parse_color("red")
    (1.0, 0.0, 0.0)
    >>> grass.parse_color("255:0:0")
    (1.0, 0.0, 0.0)
    \endcode

    @param val color value
    @param dflt default color value

    @return tuple RGB
    """
    if val in named_colors:
        return named_colors[val]

    vals = val.split(':')
    if len(vals) == 3:
        return tuple(float(v) / 255 for v in vals)

    return dflt

# check GRASS_OVERWRITE

def overwrite():
    """!Return True if existing files may be overwritten"""
    owstr = 'GRASS_OVERWRITE'
    return owstr in os.environ and os.environ[owstr] != '0'

# check GRASS_VERBOSE

def verbosity():
    """!Return the verbosity level selected by GRASS_VERBOSE"""
    vbstr = os.getenv('GRASS_VERBOSE')
    if vbstr:
	return int(vbstr)
    else:
	return 2

## various utilities, not specific to GRASS

# basename inc. extension stripping

def basename(path, ext = None):
    """!Remove leading directory components and an optional extension
    from the specified path

    @param path path
    @param ext extension
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
    """!Attempt to run a program, with optional arguments. 

    @param pgm program name
    @param args list of arguments

    @return False if the attempt failed due to a missing executable
    @return True otherwise
    """
    nuldev = file(os.devnull, 'w+')
    try:
	ret = call([pgm] + args, stdin = nuldev, stdout = nuldev, stderr = nuldev)
        if ret == 0:
            found = True
        else:
            found = False
    except:
	found = False
    nuldev.close()
    
    return found

# try to remove a file, without complaints

def try_remove(path):
    """!Attempt to remove a file; no exception is generated if the
    attempt fails.

    @param path path to file to remove
    """
    try:
	os.remove(path)
    except:
	pass

# try to remove a directory, without complaints

def try_rmdir(path):
    """!Attempt to remove a directory; no exception is generated if the
    attempt fails.

    @param path path to directory to remove
    """
    try:
	os.rmdir(path)
    except:
	shutil.rmtree(path, ignore_errors = True)

def float_or_dms(s):
    """!Convert DMS to float.

    @param s DMS value

    @return float value
    """
    return sum(float(x) / 60 ** n for (n, x) in enumerate(s.split(':')))

# interface to g.mapsets

def mapsets(accessible = True):
    """!List accessible mapsets (mapsets in search path)

    @param accessible False to list all mapsets in the location

    @return list of mapsets
    """
    if accessible:
        flags = 'p'
    else:
        flags = 'l'
    mapsets = read_command('g.mapsets',
                           flags = flags,
                           fs = 'newline',
                           quiet = True)
    if not mapsets:
        fatal(_("Unable to list mapsets"))
        
    return mapsets.splitlines()

# interface to `g.proj -c`

def create_location(dbase, location,
                    epsg = None, proj4 = None, filename = None, wkt = None,
                    datum = None, desc = None):
    """!Create new location

    Raise ScriptError on error.
    
    @param dbase path to GRASS database
    @param location location name to create
    @param epgs if given create new location based on EPSG code
    @param proj4 if given create new location based on Proj4 definition
    @param filename if given create new location based on georeferenced file
    @param wkt if given create new location based on WKT definition (path to PRJ file)
    @param datum datum transformation parameters (used for epsg and proj4)
    @param desc description of the location (creates MYNAME file)
    """
    gisdbase = None
    if epsg or proj4 or filename or wkt:
        gisdbase = gisenv()['GISDBASE']
        run_command('g.gisenv',
                    set = 'GISDBASE=%s' % dbase)
    if not os.path.exists(dbase):
            os.mkdir(dbase)
    
    kwargs = dict()
    if datum:
        kwargs['datum'] = datum
    
    if epsg:
        ps = pipe_command('g.proj',
                          quiet = True,
                          flags = 'c',
                          epsg = epsg,
                          location = location,
                          stderr = PIPE,
                          **kwargs)
    elif proj4:
        ps = pipe_command('g.proj',
                          quiet = True,
                          flags = 'c',
                          proj4 = proj4,
                          location = location,
                          stderr = PIPE,
                          **kwargs)
    elif filename:
        ps = pipe_command('g.proj',
                          quiet = True,
                          flags = 'c',
                          georef = filename,
                          location = location,
                          stderr = PIPE)
    elif wkt:
        ps = pipe_command('g.proj',
                          quiet = True,
                          flags = 'c',
                          wkt = wktfile,
                          location = location,
                          stderr = PIPE)
    else:
        _create_location_xy(dbase, location)
    
    if epsg or proj4 or filename or wkt:
        error = ps.communicate()[1]
        run_command('g.gisenv',
                    set = 'GISDBASE=%s' % gisdbase)
        
        if ps.returncode != 0 and error:
            raise ScriptError(repr(error))

    try:
        fd = codecs.open(os.path.join(dbase, location,
                                      'PERMANENT', 'MYNAME'),
                         encoding = 'utf-8', mode = 'w')
        if desc:
            fd.write(desc + os.linesep)
        else:
            fd.write(os.linesep)
        fd.close()
    except OSError, e:
        raise ScriptError(repr(e))
        
def _create_location_xy(database, location):
    """!Create unprojected location

    Raise ScriptError on error.
    
    @param database GRASS database where to create new location
    @param location location name
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
    except OSError, e:
        raise ScriptError(repr(e))

# interface to g.version

def version():
    """!Get GRASS version as dictionary

    @code
    version()

    {'date': '2011', 'libgis_revision': '45093 ', 'version': '7.0.svn',
     'libgis_date': '2011-01-20 13:10:50 +0100 (Thu, 20 Jan 2011) ', 'revision': '45136M'}
    @endcode
    """
    return parse_command('g.version',
                         flags = 'rg')

# get debug_level
if find_program('g.gisenv', ['--help']):
    debug_level = int(gisenv().get('DEBUG', 0))
