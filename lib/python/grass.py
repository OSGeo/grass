import os
import os.path
import sys
import types
import subprocess
import re
import atexit

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
	args.append("%s=%s" % (opt, _make_val(val)))
    return args

def start_command(prog, flags = "", overwrite = False, quiet = False, verbose = False, **kwargs):
    options = {}
    popts = {}
    for opt, val in kwargs.iteritems():
	if opt in _popen_args:
	    popts[opt] = val
	else:
	    options[opt] = val
    args = make_command(prog, flags, overwrite, quiet, verbose, **options)
    return subprocess.Popen(args, **popts)

def run_command(*args, **kwargs):
    ps = start_command(*args, **kwargs)
    return ps.wait()

def read_command(*args, **kwargs):
    kwargs['stdout'] = subprocess.PIPE
    ps = start_command(*args, **kwargs)
    return ps.communicate()[0]

# interface to g.message

def message(msg, flag = None):
    run_command("g.message", flags = flag, message = msg)

def debug(msg):
    message(msg, flag = 'd')

def verbose(msg):
    message(msg, flag = 'v')

def info(msg):
    message(msg, flag = 'i')

def warning(msg):
    message(msg, flag = 'w')

def error(msg):
    message(msg, flag = 'e')

def fatal(msg):
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
    if not os.getenv("GISBASE"):
        print >> sys.stderr, "You must be in GRASS GIS to run this program."
        sys.exit(1)

    if len(sys.argv) > 1 and sys.argv[1] == "@ARGS_PARSED@":
	return _parse_env()

    argv = sys.argv[:]
    name = argv[0]
    if not os.path.isabs(name):
	if os.sep in name or (os.altsep and os.altsep in name):
	    argv[0] = os.path.abspath(name)
	else:
	    argv[0] = os.path.join(sys.path[0], name)

    os.execvp("g.parser", [name] + argv)
    raise OSError("error executing g.parser")

# interface to g.tempfile

def tempfile():
    return read_command("g.tempfile", pid = os.getpid()).strip()

# interface to g.gisenv

_kv_regex = re.compile("([^=]+)='(.*)';?")

def gisenv():
    lines = read_command("g.gisenv").splitlines()
    return dict([_kv_regex.match(line).groups() for line in lines])

# interface to g.region

def region():
    lines = read_command("g.region", flags='g').splitlines()
    return dict([line.split('=',1) for line in lines])

def use_temp_region():
    name = "tmp.%s.%d" % (os.path.basename(sys.argv[0]), os.getpid())
    run_command("g.region", save = name)
    os.environ['WIND_OVERRIDE'] = name
    atexit.register(del_temp_region)

def del_temp_region():
    try:
	name = os.environ.pop('WIND_OVERRIDE')
	run_command("g.remove", quiet = True, region = name)
    except:
	pass

# interface to g.findfile

def find_file(name, element = 'cell'):
    lines = read_command("g.findfile", element = element, file = name).splitlines()
    return dict([_kv_regex.match(line).groups() for line in lines])

# interface to g.list

def list_grouped(type):
    dashes_re = re.compile("^----+$")
    mapset_re = re.compile("<(.*)>:$")
    result = {}
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
	result[mapset].extend(line.split())
    return result

def _concat(xs):
    result = []
    for x in xs:
	result.extend(x)
    return result

def list_pairs(type):
    return _concat([[(map, mapset) for map in maps]
		    for mapset, maps in list_grouped(type).iteritems()])

def list_strings(type):
    return ["%s@%s" % pair for pair in list_pairs(type)]
