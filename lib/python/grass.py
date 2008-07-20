import os
import os.path
import sys
import types
import subprocess

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

