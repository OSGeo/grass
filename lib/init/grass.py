#!/usr/bin/env python
#############################################################################
#
# MODULE:   	GRASS initialization (Python)
# AUTHOR(S):	Original author unknown - probably CERL
#               Andreas Lange - Germany - andreas.lange at rhein-main.de
#   	    	Huidae Cho - Korea - grass4u at gmail.com
#   	    	Justin Hickey - Thailand - jhickey at hpcc.nectec.or.th
#   	    	Markus Neteler - Germany/Italy - neteler at itc.it
#		Hamish Bowman - New Zealand - hamish_b at yahoo,com
#		Converted to Python (based on init.sh) by Glynn Clements
#               Martin Landa - Czech Republic - landa.martin at gmail.com
# PURPOSE:  	Sets up some environment variables.
#               It also parses any remaining command line options for
#               setting the GISDBASE, LOCATION, and/or MAPSET.
#               Finally it starts GRASS with the appropriate user
#   	    	interface and cleans up after it is finished.
# COPYRIGHT:    (C) 2000-2010 by the GRASS Development Team
#
#               This program is free software under the GNU General
#   	    	Public License (>=v2). Read the file COPYING that
#   	    	comes with GRASS for details.
#
#############################################################################

import sys
import os
import atexit
import string
import subprocess
import re
import platform

# Variables substituted during build process
# Set the GISBASE variable
gisbase = "@GISBASE@"
cmd_name = "@START_UP@"
grass_version = "@GRASS_VERSION_NUMBER@"
ld_library_path_var = '@LD_LIBRARY_PATH_VAR@'
config_projshare = "@CONFIG_PROJSHARE@"
grass_config_dirname = "@GRASS_CONFIG_DIR@"

gisbase = os.path.normpath(gisbase)

### i18N
import gettext
gettext.install('grasslibs', os.path.join(gisbase, 'locale'), unicode=True)

tmpdir = None
lockfile = None
location = None
create_new = None
grass_gui = None

def warning(text):
    sys.stderr.write(_("WARNING") + ': ' + text + os.linesep)

def try_remove(path):
    try:
	os.remove(path)
    except:
	pass

def try_rmdir(path):
    try:
	os.rmdir(path)
    except:
	pass

def cleanup_dir(dir):
    if not dir:
	return

    for root, dirs, files in os.walk(dir, topdown = False):
	for name in files:
		try_remove(os.path.join(root, name))
	for name in dirs:
		try_rmdir(os.path.join(root, name))

def cleanup():
    tmpdir, lockfile
    # all exits after setting up $tmpdir should also tidy it up
    cleanup_dir(tmpdir)
    if lockfile:
	try_remove(lockfile)

def fatal(msg):
    sys.exit(msg)

def message(msg):
    sys.stderr.write(msg + "\n")
    sys.stderr.flush()
    
def readfile(path):
    f = open(path, 'r')
    s = f.read()
    f.close()
    return s

def writefile(path, s):
    f = open(path, 'w')
    f.write(s)
    f.close()

def call(cmd, **kwargs):
    if windows:
	kwargs['shell'] = True
    return subprocess.call(cmd, **kwargs)

def Popen(cmd, **kwargs):
    if windows:
	kwargs['shell'] = True
    return subprocess.Popen(cmd, **kwargs)

def gfile(*args):
    return os.path.join(gisbase, *args)

help_text = r"""
%s:
  $CMD_NAME [-h | -help | --help] [-v | --version] [-c]
	  [-text | -gui]
	  [[[<GISDBASE>/]<LOCATION_NAME>/]<MAPSET>]

%s:
  -h or -help or --help          %s
  -v or --version                %s
  -c                             %s
  -text                          %s
                                   %s
  -gui                           %s
                                   %s

%s:
  GISDBASE                       %s
  LOCATION_NAME                  %s
  MAPSET                         %s

  GISDBASE/LOCATION_NAME/MAPSET  %s

%s:
  GRASS_GUI                      %s
  GRASS_WISH                     %s
  GRASS_HTML_BROWSER             %s
  GRASS_ADDON_PATH               %s
  GRASS_BATCH_JOB                %s
  GRASS_PYTHON                   %s
""" % (_("Usage"),
       _("Flags"),
       _("print this help message"),
       _("show version information and exit"),
       _("create given mapset if it doesn't exist"),
       _("use text based interface"),
       _("and set as default"),
       _("use $DEFAULT_GUI graphical user interface"),
       _("and set as default"),
       _("Parameters"),
       _("initial database (path to GIS data)"),
       _("initial location"),
       _("initial mapset"),
       _("fully qualified initial mapset directory"),
       _("Environment variables relevant for startup"),
       _("select GUI (text, gui)"),
       _("set wish shell name to override 'wish'"),
       _("set html web browser for help pages"),
       _("set additional path(s) to local GRASS modules"),
       _("shell script to be processed as batch job"),
       _("set python shell name to override 'python'"))


def help_message():
    t = string.Template(help_text)
    s = t.substitute(CMD_NAME = cmd_name, DEFAULT_GUI = default_gui)
    sys.stderr.write(s)

def create_tmp():
    global tmpdir
    ## use $TMPDIR if it exists, then $TEMP, otherwise /tmp
    tmp = os.getenv('TMPDIR')
    if not tmp:
	tmp = os.getenv('TEMP')
    if not tmp:
	tmp = '/tmp'
    tmpdir = os.path.join(tmp, "grass7-%s-%s" % (user, gis_lock))
    try:
	os.mkdir(tmpdir, 0700)
    except:
	fatal(_("Unable to create temporary directory! Exiting."))

def create_gisrc():
    global gisrc, gisrcrc
    # Set the session grassrc file
    gisrc = os.path.join(tmpdir, "gisrc")
    os.environ['GISRC'] = gisrc
    
    # remove invalid GISRC file to avoid disturbing error messages:
    try:
	s = readfile(gisrcrc)
	if "UNKNOWN" in s:
	    try_remove(gisrcrc)
	    s = None
    except:
	s = None

    # Copy the global grassrc file to the session grassrc file
    if s:
	writefile(gisrc, s)

def read_gisrc():
    kv = {}
    f = open(gisrc, 'r')
    for line in f:
	k, v = line.split(':', 1)
	kv[k.strip()] = v.strip()
    f.close()
    return kv

def write_gisrc(kv):
    f = open(gisrc, 'w')
    for k, v in kv.iteritems():
	f.write("%s: %s\n" % (k, v))
    f.close()

def read_gui():
    global grass_gui
    # At this point the GRASS user interface variable has been set from the
    # command line, been set from an external environment variable, or is 
    # not set. So we check if it is not set
    if not grass_gui:
	# Check for a reference to the GRASS user interface in the grassrc file
	if os.access(gisrc, os.R_OK):
	    kv = read_gisrc()
	    if not kv.has_key('GRASS_GUI'):
		# Set the GRASS user interface to the default if needed
		grass_gui = default_gui
            else:
                grass_gui = kv['GRASS_GUI']
    
    if not grass_gui:
	grass_gui = default_gui

    if grass_gui == 'gui':
	grass_gui = default_gui
    
    # FIXME oldtcltk, gis.m, d.m no longer exist
    if grass_gui in ['d.m', 'gis.m', 'oldtcltk', 'tcltk']:
	grass_gui = default_gui

def get_locale():
    global locale
    locale = None
    for var in ['LC_ALL', 'LC_MESSAGES', 'LANG']:
	loc = os.getenv(var)
	if loc:
	    locale = loc[0:2]
	return

def path_prepend(dir, var):
    path = os.getenv(var)
    if path:
	path = dir + os.pathsep + path
    else:
	path = dir
    os.environ[var] = path

def path_append(dir, var):
    path = os.getenv(var)
    if path:
	path = path + os.pathsep + dir
    else:
	path = dir
    os.environ[var] = path

def set_paths():
    addon_path = os.getenv('GRASS_ADDON_PATH')
    if addon_path:
	path_prepend(addon_path, 'PATH')
    path_prepend(gfile('scripts'), 'PATH')
    path_prepend(gfile('bin'), 'PATH')

    # Set PYTHONPATH to find GRASS Python modules
    path_prepend(gfile('etc', 'python'), 'PYTHONPATH')

    # Add .py (Python) to list of executable extensions to search for in MS-Windows PATH
    if windows:
	path_append('.PY', 'PATHEXT')

def find_exe(pgm):
    for dir in os.getenv('PATH').split(os.pathsep):
	path = os.path.join(dir, pgm)
	if os.access(path, os.X_OK):
	    return path
    return None

def set_defaults():
    # GRASS_PAGER
    if not os.getenv('GRASS_PAGER'):
	if find_exe("more"):
	    pager = "more"
	elif find_exe("less"):
	    pager = "less"
	elif windows:
	    pager = "more"
	else:
	    pager = "cat"
	os.environ['GRASS_PAGER'] = pager

    # GRASS_WISH
    if not os.getenv('GRASS_WISH'):
	os.environ['GRASS_WISH'] = "wish"

    # GRASS_PYTHON
    if not os.getenv('GRASS_PYTHON'):
	if windows:
	    os.environ['GRASS_PYTHON'] = "python.exe"
	else:
	    os.environ['GRASS_PYTHON'] = "python"

    # GRASS_GNUPLOT
    if not os.getenv('GRASS_GNUPLOT'):
	os.environ['GRASS_GNUPLOT'] = "gnuplot -persist"

    # GRASS_PROJSHARE
    if not os.getenv('GRASS_PROJSHARE'):
	os.environ['GRASS_PROJSHARE'] = config_projshare

def set_browser():
    # GRASS_HTML_BROWSER
    browser = os.getenv('GRASS_HTML_BROWSER')
    if not browser:
	if macosx:
	    # OSX doesn't execute browsers from the shell PATH - route thru a script
	    browser = gfile('etc', "html_browser_mac.sh")
	    os.environ['GRASS_HTML_BROWSER_MACOSX'] = "-b com.apple.helpviewer"

	if windows or cygwin:
	    # MinGW startup moved to into init.bat
	    browser = "explorer"
	else:
	    # the usual suspects
	    browsers = [ "xdg-open", "htmlview", "konqueror", "mozilla", "mozilla-firefox",
                         "firefox", "iceweasel", "opera", "netscape", "dillo", "lynx", "links", "w3c" ]
	    for b in browsers:
		if find_exe(b):
		    browser = b
		    break
    
    elif macosx:
	# OSX doesn't execute browsers from the shell PATH - route thru a script
	os.environ['GRASS_HTML_BROWSER_MACOSX'] = "-b %s" % browser
	browser = gfile('etc', "html_browser_mac.sh")

    if not browser:
	warning(_("Searched for a web browser, but none found"))
	# even so we set konqueror to make lib/gis/parser.c happy:
	browser = "konqueror"
    
    os.environ['GRASS_HTML_BROWSER'] = browser

def grass_intro():
    if locale:
	path = gfile("locale", locale, "etc", "grass_intro")
	if not os.access(path, os.R_OK):
	    path = gfile("etc", "grass_intro")
    else:
	path = gfile("etc", "grass_intro")
    f = open(path, 'r')
    for line in f:
	sys.stderr.write(line)
    f.close()

    sys.stderr.write("\n")
    sys.stderr.write(_("Hit RETURN to continue"))
    sys.stdin.readline()

    #for convenience, define pwd as GISDBASE:
    s = r"""GISDBASE: %s
LOCATION_NAME: <UNKNOWN>
MAPSET: <UNKNOWN>
""" % os.getcwd()
    writefile(gisrc, s)

def check_gui():
    global grass_gui, wxpython_base
    # Check if we are running X windows by checking the DISPLAY variable
    if os.getenv('DISPLAY') or windows:
	# Check if python is working properly
	if grass_gui == 'wxpython':
	    nul = open(os.devnull, 'w')
	    p = Popen([os.environ['GRASS_PYTHON']],
                      stdin = subprocess.PIPE,
                      stdout = nul, stderr = nul)
	    nul.close()
	    p.stdin.write("variable=True")
	    p.stdin.close()
	    p.wait()
	    if p.returncode == 0:
		# Set the wxpython base directory
		wxpython_base = gfile("etc", "gui", "wxpython")
	    else:
		# Python was not found - switch to text interface mode
		warning(_("The python command does not work as expected!\n"
                          "Please check your GRASS_PYTHON environment variable.\n"
                          "Use the -help option for details.\n"
                          "Switching to text based interface mode.\n\n"
                          "Hit RETURN to continue.\n"))
		sys.stdin.readline()
		grass_gui = 'text'

    else:
	# Display a message if a graphical interface was expected
	if grass_gui != 'text':
	    # Set the interface mode to text
	    warning(_("It appears that the X Windows system is not active.\n"
                      "A graphical based user interface is not supported.\n"
                      "Switching to text based interface mode.\n\n"
                      "Hit RETURN to continue"""))
	    sys.stdin.readline()
	    grass_gui = 'text'

    # Save the user interface variable in the grassrc file - choose a temporary
    # file name that should not match another file
    if os.access(gisrc, os.F_OK):
	kv = read_gisrc()
	kv['GRASS_GUI'] = grass_gui
	write_gisrc(kv)

def non_interactive(arg):
    global gisdbase, location_name, mapset, location
    # Try non-interactive startup
    l = None

    if arg == '-':
	if location:
	    l = location
    else:
	l = arg

    if l:
	if l == '.':
	    l = os.getcwd()
	elif not os.path.isabs(l):
	    l = os.path.abspath(l)

	l, mapset = os.path.split(l)
	if not mapset:
	    l, mapset = os.path.split(l)
	l, location_name = os.path.split(l)
	gisdbase = l

    if gisdbase and location_name and mapset:
	location = os.path.join(gisdbase, location_name, mapset)

	if not os.access(os.path.join(location, "WIND"), os.R_OK):
	    if location_name == "PERMANENT":
		fatal(_("<%s> is not a valid GRASS location") % location)
	    else:
		# the user wants to create mapset on the fly
		if create_new:
		    if not os.access(os.path.join(os.path.join(gisdbase, location_name, "PERMANENT", "DEFAULT_WIND")), os.F_OK):
			fatal(_("The location <%s> does not exist. Please create it first.") % location_name)
		    else:
			os.mkdir(location)
			# copy PERMANENT/DEFAULT_WIND to <mapset>/WIND
			s = readfile(os.path.join(gisdbase, location_name, "PERMANENT", "DEFAULT_WIND"))
			writefile(os.path.join(location, "WIND"), s)
			message(_("Missing WIND file fixed"))
		else:
		    fatal(_("<%s> is not a valid GRASS location") % location)

	if os.access(gisrc, os.R_OK):
	    kv = read_gisrc()
	else:
	    kv = {}

	kv['GISDBASE'] = gisdbase
	kv['LOCATION_NAME'] = location_name
	kv['MAPSET'] = mapset
	write_gisrc(kv)
    else:
	fatal(_("GISDBASE, LOCATION_NAME and MAPSET variables not set properly.\n"
                "Interactive startup needed."))

def set_data():
    # User selects LOCATION and MAPSET if not set
    if not location:
	# Check for text interface
	if grass_gui == 'text':
	    pass
	# Check for GUI
	elif grass_gui == 'wxpython':
	    gui_startup()
	else:
	    # Shouldn't need this but you never know
	    fatal(_("Invalid user interface specified - <%s>.\n" 
                    "Use the --help option to see valid interface names.") % grass_gui)

def gui_startup():
    if grass_gui == 'wxpython':
	thetest = call([os.getenv('GRASS_PYTHON'),
                        gfile(wxpython_base, "gis_set.py")])
    
    if thetest == 0:
	pass
    elif thetest == 1:
	# The startup script printed an error message so wait
	# for user to read it
	message(_("Error in GUI startup. If necessary, please "
                  "report this error to the GRASS developers.\n"
                  "Switching to text mode now.\n\n"
                  "Hit RETURN to continue..."))
	sys.stdin.readline()

	os.execlp(cmd_name, "-text")
	sys.exit(1)
    elif thetest == 2:
	# User wants to exit from GRASS
	message(_("Received EXIT message from GUI.\nGRASS is not started. Bye."))
	sys.exit(0)
    else:
	fatal(_("Invalid return code from GUI startup script.\n"
                "Please advise GRASS developers of this error."))

def load_gisrc():
    global gisdbase, location_name, mapset, location
    kv = read_gisrc()
    gisdbase = kv.get('GISDBASE')
    location_name = kv.get('LOCATION_NAME')
    mapset = kv.get('MAPSET')
    if not gisdbase or not location_name or not mapset:
	fatal(_("Error reading data path information from g.gisenv.\n"
                "GISDBASE=%(gisbase)s\n"
                "LOCATION_NAME=%(location)s\n"
                "MAPSET=%(mapset)s\n\n"
                "Check the <%s(file)> file." % \
                    { 'gisbase' : gisdbase, 'location' : location_name,
                      'mapset' : mapset, 'file' : gisrcrc }))
    
    location = os.path.join(gisdbase, location_name, mapset)

def check_lock():
    global lockfile
    # Check for concurrent use
    lockfile = os.path.join(location, ".gislock")
    ret = call([gfile("etc", "lock"),
		lockfile,
		"%d" % os.getpid()])
    
    if ret == 0:
	msg = None
    elif ret == 2:
	msg = _("%(user)s is currently running GRASS in selected mapset (file %(file)s found). "
                "Concurrent use not allowed." % \
                    { 'user' : user, 'file' : lockfile })
    else:
	msg = _("Unable to properly access \"%s\"\nPlease notify system personel.") % lockfile
        
    if msg:
        if grass_gui == "wxpython":
            thetest = call([os.getenv('GRASS_PYTHON'), os.path.join(wxpython_base, "gis_set_error.py"), msg])
        else:
            fatal(msg)
    
def make_fontcap():
    fc = os.getenv('GRASS_FONT_CAP')
    if fc and not os.access(fc, os.R_OK):
	message(_("Building user fontcap..."))
	call(["g.mkfontcap"])

def check_shell():
    global sh, shellname
    # cygwin has many problems with the shell setup
    # below, so i hardcoded everything here.
    if os.getenv('CYGWIN'):
        sh = "cygwin"
        shellname = "GNU Bash (Cygwin)"
	os.environ['SHELL'] = "/usr/bin/bash.exe"
        os.environ['OSTYPE'] = "cygwin"
    else:
        sh = os.path.basename(os.getenv('SHELL'))
	if sh == "ksh":
	    shellname = "Korn Shell"
        elif sh == "csh":
	    shellname = "C Shell" 
        elif sh == "tcsh":
	    shellname = "TC Shell" 
        elif sh == "bash":
	    shellname = "Bash Shell" 
        elif sh == "sh":
	    shellname = "Bourne Shell"
	else:
	    shellname = "shell"

    # check for SHELL
    if not os.getenv('SHELL'):
        fatal(_("The SHELL variable is not set"))

def check_batch_job():
    global batch_job
    # hack to process batch jobs:
    batch_job = os.getenv('GRASS_BATCH_JOB')
    if batch_job:
	# defined, but ...
	if not os.access(batch_job, os.F_OK):
          # wrong file
          fatal(_("Job file '%s' has been defined in "
                  "the 'GRASS_BATCH_JOB' variable but not found. Exiting.\n\n"
                  "Use 'unset GRASS_BATCH_JOB' to disable batch job processing.") % batch_job)
        elif not os.access(batch_job, os.X_OK):
	    # right file, but ...
	    fatal(_("Change file permission to 'executable' for '%s'") % batch_job)
	else:
	    message(_("Executing '%s' ...") % batch_job)
	    grass_gui = "text"
	    shell = batch_job
	    bj = Popen(shell,shell=True)
	    bj.wait()
	    message(_("Execution of '%s' finished.") % batch_job)

def start_gui():
    # Start the chosen GUI but ignore text
    if grass_debug:
	message(_("GRASS GUI should be '%s'") % grass_gui)
    
    # Check for gui interface
    if grass_gui == "wxpython":
        Popen([os.getenv('GRASS_PYTHON'),
               gfile(wxpython_base, "wxgui.py")])
    
def clear_screen():
    if windows:
	pass
    # TODO: uncomment when PDCurses works.
    #	cls
    else:
	if not os.getenv('GRASS_BATCH_JOB') and not grass_debug:
	    call(["tput", "clear"])

def show_banner():
    sys.stderr.write(r"""
          __________  ___   __________    _______________
         / ____/ __ \/   | / ___/ ___/   / ____/  _/ ___/
        / / __/ /_/ / /| | \__ \\_  \   / / __ / / \__ \ 
       / /_/ / _, _/ ___ |___/ /__/ /  / /_/ // / ___/ / 
       \____/_/ |_/_/  |_/____/____/   \____/___//____/  

""")


def say_hello():
    if locale:
	path = gfile("locale", locale, "etc", "welcome")
	if not os.access(path, os.R_OK):
	    path = gfile("etc", "welcome")
    else:
	path = gfile("etc", "welcome")
    s = readfile(path)
    sys.stderr.write(s)

def show_info():
    sys.stderr.write(
r"""
%-41shttp://grass.osgeo.org
%-41s%s (%s)
%-41sg.manual -i
%-41sg.version -c
""" % (_("GRASS homepage:"),
       _("This version running through:"),
       shellname, os.getenv('SHELL'),
       _("Help is available with the command:"),
       _("See the licence terms with:")))

    if grass_gui == 'wxpython':
        message("%-41sg.gui wxpython" % _("If required, restart the GUI with:"))
    else:
        message("%-41sg.gui %s" % (_("Start the GUI with:"), default_gui))

    message("%-41sexit" % _("When ready to quit enter:"))
    message("")

def csh_startup():
    global exit_val

    userhome = os.getenv('HOME')      # save original home
    home = location
    os.environ['HOME'] = home

    cshrc = os.path.join(home, ".cshrc")
    tcshrc = os.path.join(home, ".tcshrc")
    try_remove(cshrc)
    try_remove(tcshrc)

    f = open(cshrc, 'w')
    f.write("set home = %s" % userhome)
    f.write("set history = 3000 savehist = 3000  noclobber ignoreeof")
    f.write("set histfile = %s" % os.path.join(os.getenv('HOME'), ".history"))

    f.write("set prompt = '\\")
    f.write("Mapset <%s> in Location <%s> \\" % (mapset, location_name))
    f.write("GRASS %s > '" % grass_version)
    f.write("set BOGUS=``;unset BOGUS")

    path = os.path.join(userhome, ".grass.cshrc")
    if os.access(path, os.R_OK):
	f.write(readfile(path))

    mail_re = re.compile(r"^ *set  *mail *= *")

    for filename in [".cshrc", ".tcshrc", ".login"]:
	path = os.path.join(userhome, filename)
	if os.access(path, os.R_OK):
	    s = readfile(path)
	    lines = s.splitlines()
	    for l in lines:
		if mail_re.match(l):
		    f.write(l)

    path = os.getenv('PATH').split(':')
    f.write("set path = ( %s ) " % ' '.join(path))

    f.close()
    writefile(tcshrc, readfile(cshrc))

    exit_val = call([gfile("etc", "run"), os.getenv('SHELL')])

    os.environ['HOME'] = userhome

def bash_startup():
    global exit_val
    
    # save command history in mapset dir and remember more
    os.environ['HISTFILE'] = os.path.join(location, ".bash_history")
    if not os.getenv('HISTSIZE') and not os.getenv('HISTFILESIZE'):
	os.environ['HISTSIZE'] = "3000"
    
    # instead of changing $HOME, start bash with: --rcfile "$LOCATION/.bashrc" ?
    #   if so, must care be taken to explicity call .grass.bashrc et al for
    #   non-interactive bash batch jobs?
    userhome = os.getenv('HOME')      # save original home
    home = location		      # save .bashrc in $LOCATION
    os.environ['HOME'] = home
    
    bashrc = os.path.join(home, ".bashrc")
    try_remove(bashrc)
    
    f = open(bashrc, 'w')
    f.write("test -r ~/.alias && . ~/.alias\n")
    f.write("PS1='GRASS %s (%s):\w > '\n" % (grass_version, location_name))
    
    path = os.path.join(userhome, ".grass.bashrc")
    if os.access(path, os.R_OK):
	f.write(readfile(path) + '\n')
    
    f.write("export PATH=\"%s\"\n" % os.getenv('PATH'))
    f.write("export HOME=\"%s\"\n" % userhome) # restore user home path
    
    for env, value in os.environ.iteritems():
        if env.find('GRASS_') < 0:
            continue
        f.write("export %s=\"%s\"\n" % (env, value))
    
    f.close()
    
    exit_val = call([gfile("etc", "run"), os.getenv('SHELL')])
    
    os.environ['HOME'] = userhome

def default_startup():
    global exit_val
    
    if windows:
	os.environ['PS1'] = "GRASS %s> " % (grass_version)
	# "$ETC/run" doesn't work at all???
        exit_val = subprocess.call([os.getenv('SHELL')])
	cleanup_dir(os.path.join(location, ".tmp"))  # remove GUI session files from .tmp
    else:
	os.environ['PS1'] = "GRASS %s (%s):\w > " % (grass_version, location_name)
	exit_val = call([gfile("etc", "run"), os.getenv('SHELL')])

    if exit_val != 0:
        fatal(_("Failed to start shell '%s'") % os.getenv('SHELL'))
    
def done_message():
    if batch_job and os.access(batch_job, os.X_OK):
	message(_("Batch job '%s' (defined in GRASS_BATCH_JOB variable) was executed.") % batch_job)
        message(_("Goodbye from GRASS GIS"))
	sys.exit(exit_val)
    else:
        message(_("Done."))
        message("")
        message(_("Goodbye from GRASS GIS"))
        message("")

def clean_temp():
    message(_("Cleaning up temporary files..."))
    nul = open(os.devnull, 'w')
    call([gfile("etc", "clean_temp")], stdout = nul, stderr = nul)
    nul.close()

def get_username():
    global user
    if windows:
	user = os.getenv('USERNAME')
	if not user:
	    user = "user_name"
    else:
	user = os.getenv('USER')
	if not user:
	    user = os.getenv('LOGNAME')
	if not user:
	    try:
		p = Popen(['whoami'], stdout = subprocess.PIPE)
		s = p.stdout.read()
		p.wait()
		user = s.strip()
	    except:
		pass
	if not user:
	    user = "user_%d" % os.getuid()

def parse_cmdline():
    global args, grass_gui, create_new
    args = []
    for i in sys.argv[1:]:
	# Check if the user asked for the version
	if i in ["-v","--version"]:
	    message('\n' + readfile(gfile("etc", "license")))
	    sys.exit()
	# Check if the user asked for help
	elif i in ["help","-h","-help","--help"]:
	    help_message()
	    sys.exit()
	# Check if the -text flag was given
	elif i == "-text":
	    grass_gui = 'text'
	# Check if the -gui flag was given
	elif i == "-gui":
	    grass_gui = default_gui
	# Check if the -wxpython flag was given
	elif i in ["-wxpython","-wx"]:
	    grass_gui = 'wxpython'
	# Check if the user wants to create a new mapset
	elif i == "-c":
	    create_new = True
	else:
	    args.append(i)

### MAIN script starts here

# Get the system name
windows = sys.platform == 'win32'
cygwin = "cygwin" in sys.platform
macosx = "darwin" in sys.platform

# Set GISBASE
os.environ['GISBASE'] = gisbase

# set HOME
if windows and not os.getenv('HOME'):
    os.environ['HOME'] = os.path.join(os.getenv('HOMEDRIVE'), os.getenv('HOMEPATH'))

# set SHELL
if windows:
    if os.getenv('GRASS_SH'):
        os.environ['SHELL'] = os.getenv('GRASS_SH')
    if not os.getenv('SHELL'):
        os.environ['SHELL'] = os.getenv('COMSPEC', 'cmd.exe')

grass_config_dir = os.path.join(os.getenv('HOME'), grass_config_dirname)

atexit.register(cleanup)

# Set default GUI
default_gui = "wxpython"

# the following is only meant to be an internal variable for debugging this script.
#  use 'g.gisenv set="DEBUG=[0-5]"' to turn GRASS debug mode on properly.
grass_debug = os.getenv('GRASS_DEBUG')

# Set GRASS version number for R interface etc (must be an env_var for MS-Windows)
os.environ['GRASS_VERSION'] = grass_version

# Set the GIS_LOCK variable to current process id
gis_lock = str(os.getpid())
os.environ['GIS_LOCK'] = gis_lock

# Set the global grassrc file
batch_job = os.getenv('GRASS_BATCH_JOB')
if batch_job:
    gisrcrc = os.path.join(grass_config_dir, "rc.%s" % platform.node())
    if not os.access(gisrcrc, os.R_OK):
	gisrcrc = os.path.join(grass_config_dir, "rc")
else:
    gisrcrc = os.path.join(grass_config_dir, "rc")

# Set the username and working directory
get_username()

# Parse the command-line options
parse_cmdline()

# Create the temporary directory and session grassrc file
create_tmp()

# Create the session grassrc file
create_gisrc()

# Ensure GRASS_GUI is set
read_gui()

# Get Locale name
get_locale()

# Set PATH, PYTHONPATH
set_paths()

# Set LD_LIBRARY_PATH (etc) to find GRASS shared libraries
path_prepend(gfile("lib"), ld_library_path_var)

# Set GRASS_PAGER, GRASS_WISH, GRASS_PYTHON, GRASS_GNUPLOT, GRASS_PROJSHARE
set_defaults()

# Set GRASS_HTML_BROWSER
set_browser()

#predefine monitor size for certain architectures
if os.getenv('HOSTTYPE') == 'arm':
    # small monitor on ARM (iPAQ, zaurus... etc)
    os.environ['GRASS_HEIGHT'] = "320"
    os.environ['GRASS_WIDTH']  = "240"

# First time user - GISRC is defined in the GRASS script
if not os.access(gisrc, os.F_OK):
    if grass_gui == 'text' and len(args) == 0:
        fatal(_("Unable to start GRASS. You can:\n"
                " - Launch GRASS with '-gui' switch (`grass70 -gui`)\n"
                " - Create manually GISRC file (%s)\n"
                " - Launch GRASS with path to "
                "the location/mapset as an argument (`grass70 /path/to/location/mapset`)") % gisrcrc)
    grass_intro()
else:
    clean_temp()

message(_("Starting GRASS GIS..."))

# Check that the GUI works
check_gui()

# Parsing argument to get LOCATION
if args == []:
    # Try interactive startup
    location = None
else:
    non_interactive(args[0])

# User selects LOCATION and MAPSET if not set
set_data()

# Set GISDBASE, LOCATION_NAME, MAPSET, LOCATION from $GISRC
load_gisrc()

# Check .gislock file
check_lock()

# build user fontcap if specified but not present
make_fontcap()

# predefine default driver if DB connection not defined
#  is this really needed?? Modules should call this when/if required.
if not os.access(os.path.join(location, "VAR"), os.F_OK):
    call(['db.connect', '-c', '--quiet'])

check_shell()

check_batch_job()

if not batch_job:	
    start_gui()

clear_screen()

# Display the version and license info
if batch_job:
    say_hello()
    grass_gui = 'text'
    clear_screen()
    clean_temp()
    try_remove(lockfile)
    sys.exit(0)
else:
    show_banner()
    say_hello()
    show_info()
    if grass_gui == "wxpython":
        message(_("Launching '%s' GUI in the background, please wait...") % grass_gui)
    
if sh in ['csh', 'tcsh']:
    csh_startup()
elif sh in ['bash', 'msh', 'cygwin']:
    bash_startup()
else:
    default_startup()

clear_screen()

clean_temp()

try_remove(lockfile)

# Save GISRC
s = readfile(gisrc)
if not os.path.exists(grass_config_dir):
    os.mkdir(grass_config_dir)
writefile(gisrcrc, s)

cleanup()

#### after this point no more grass modules may be called ####

done_message()
