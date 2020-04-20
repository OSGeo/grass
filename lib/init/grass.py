#!/usr/bin/env python3
#############################################################################
#
# MODULE:       GRASS initialization script
# AUTHOR(S):    Original author unknown - probably CERL
#               Andreas Lange <andreas.lange rhein-main.de>
#               Huidae Cho <grass4u gmail.com>
#               Justin Hickey <jhickey hpcc.nectec.or.th>
#               Markus Neteler <neteler osgeo.org>
#               Hamish Bowman <hamish_b yahoo.com>
#
#               GRASS 7: converted to Python (based on init.sh shell
#               script from GRASS 6) by Glynn Clements
#               Martin Landa <landa.martin gmail.com>
#               Luca Delucchi <lucadeluge gmail.com>
#               Vaclav Petras <wenzeslaus gmail.com> (refactoring and exec)
# PURPOSE:      Sets up environment variables, parses any remaining
#               command line options for setting the GISDBASE, LOCATION,
#               and/or MAPSET. Finally it starts GRASS with the appropriate
#               user interface and cleans up after it is finished.
# COPYRIGHT:    (C) 2000-2019 by the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

"""
Script to run GRASS session.

Some of the functions could be used separately but import from this module
is not safe, i.e. it has side effects (this should be changed in the future).
"""

# we allow for long file because we want to avoid imports if possible
# (this makes it more stable since we have to set up paths first)
# pylint: disable=too-many-lines

from __future__ import print_function
import sys
import os
import errno
import atexit
import gettext
import shutil
import signal
import string
import subprocess
import types
import re
import six
import platform
import tempfile
import locale


# mechanism meant for debugging this script (only)
# private global to store if we are debugging
_DEBUG = None

# for wxpath
_WXPYTHON_BASE = None

ENCODING = locale.getdefaultlocale()[1]
if ENCODING is None:
    ENCODING = 'UTF-8'
    print("Default locale not found, using UTF-8")  # intentionally not translatable

# The "@...@" variables are being substituted during build process
#
# TODO: should GISBASE be renamed to something like GRASS_PATH?
# GISBASE marks complete runtime, so no need to get it here when
# setting it up, possible scenario: existing runtime and starting
# GRASS in that, we want to overwrite the settings, not to take it
# possibly same for GRASS_PROJSHARE and others but maybe not
#
# We need to simultaneously make sure that:
# - we get GISBASE from os.environ if it is defined (doesn't this mean that we are already
#   inside a GRASS session? If we are, why do we need to run this script again???).
# - GISBASE exists as an ENV variable
#
# pmav99: Ugly as hell, but that's what the code before the refactoring was doing.
if 'GISBASE' in os.environ and len(os.getenv('GISBASE')) > 0:
    GISBASE = os.path.normpath(os.environ["GISBASE"])
else:
    GISBASE = os.path.normpath("@GISBASE@")
    os.environ['GISBASE'] = GISBASE
CMD_NAME = "@START_UP@"
GRASS_VERSION = "@GRASS_VERSION_NUMBER@"
LD_LIBRARY_PATH_VAR = '@LD_LIBRARY_PATH_VAR@'
CONFIG_PROJSHARE = os.environ.get('GRASS_PROJSHARE', "@CONFIG_PROJSHARE@")

# Get the system name
WINDOWS = sys.platform == 'win32'
CYGWIN = "cygwin" in sys.platform
MACOSX = "darwin" in sys.platform


def decode(bytes_, encoding=ENCODING):
    """Decode bytes with default locale and return (unicode) string
    Adapted from lib/python/core/utils.py

    No-op if parameter is not bytes (assumed unicode string).

    :param bytes bytes_: the bytes to decode
    :param encoding: encoding to be used, default value is the system's default
        encoding or, if that cannot be determined, 'UTF-8'.
    """
    if sys.version_info.major >= 3:
        unicode = str
    if isinstance(bytes_, unicode):
        return bytes_
    elif isinstance(bytes_, bytes):
        return bytes_.decode(encoding)
    else:
        # if something else than text
        raise TypeError("can only accept types str and bytes")


def encode(string, encoding=ENCODING):
    """Encode string with default locale and return bytes with that encoding
    Adapted from lib/python/core/utils.py

    No-op if parameter is bytes (assumed already encoded).
    This ensures garbage in, garbage out.

    :param str string: the string to encode
    :param encoding: encoding to be used, default value is the system's default
        encoding or, if that cannot be determined, 'UTF-8'.
    """
    if sys.version_info.major >= 3:
        unicode = str
    if isinstance(string, bytes):
        return string
    # this also tests str in Py3:
    elif isinstance(string, unicode):
        return string.encode(encoding)
    else:
        # if something else than text
        raise TypeError("can only accept types str and bytes")


# see https://trac.osgeo.org/grass/ticket/3508
def to_text_string(obj, encoding=ENCODING):
    """Convert `obj` to (unicode) text string"""
    if six.PY2:
        # Python 2
        return encode(obj, encoding=encoding)
    else:
        # Python 3
        return decode(obj, encoding=encoding)


def try_remove(path):
    try:
        os.remove(path)
    except:
        pass


def clean_env():
    gisrc = os.environ['GISRC']
    env_curr = read_gisrc(gisrc)
    env_new = {}
    for k, v in env_curr.items():
        if k.endswith('PID') or k.startswith('MONITOR'):
            continue
        env_new[k] = v
    write_gisrc(env_new, gisrc)


def is_debug():
    """Returns True if we are in debug mode

    For debug messages use ``debug()``.
    """
    global _DEBUG
    if _DEBUG is not None:
        return _DEBUG
    _DEBUG = os.getenv('GRASS_DEBUG')
    # translate to bool (no or empty variable means false)
    if _DEBUG:
        _DEBUG = True
    else:
        _DEBUG = False
    return _DEBUG


def debug(msg):
    """Print a debug message if in debug mode

    Do not use translatable strings for debug messages.
    """
    if is_debug():
        sys.stderr.write("DEBUG: %s\n" % msg)
        sys.stderr.flush()


def message(msg):
    sys.stderr.write(msg + "\n")
    sys.stderr.flush()


def warning(text):
    sys.stderr.write(_("WARNING") + ': ' + text + os.linesep)


def fatal(msg):
    sys.stderr.write("%s: " % _('ERROR') + msg + os.linesep)
    sys.exit(_("Exiting..."))


def readfile(path):
    debug("Reading %s" % path)
    f = open(path, 'r')
    s = f.read()
    f.close()
    return s


def writefile(path, s):
    debug("Writing %s" % path)
    f = open(path, 'w')
    f.write(s)
    f.close()


def call(cmd, **kwargs):
    """Wrapper for subprocess.call to deal with platform-specific issues"""
    if WINDOWS:
        kwargs['shell'] = True
    return subprocess.call(cmd, **kwargs)


def Popen(cmd, **kwargs):  # pylint: disable=C0103
    """Wrapper for subprocess.Popen to deal with platform-specific issues"""
    if WINDOWS:
        kwargs['shell'] = True
    return subprocess.Popen(cmd, **kwargs)


def gpath(*args):
    """Costruct path to file or directory in GRASS GIS installation

    Can be called only after GISBASE was set.
    """
    return os.path.join(GISBASE, *args)


def wxpath(*args):
    """Costruct path to file or directory in GRASS wxGUI

    Can be called only after GISBASE was set.

    This function does not check if the directories exist or if GUI works
    this must be done by the caller if needed.
    """
    global _WXPYTHON_BASE
    if not _WXPYTHON_BASE:
        # this can be called only after GISBASE was set
        _WXPYTHON_BASE = gpath("gui", "wxpython")
    return os.path.join(_WXPYTHON_BASE, *args)


# using format for most but leaving usage of template for the dynamic ones
# two different methods are easy way to implement two phase construction
HELP_TEXT = r"""GRASS GIS $VERSION_NUMBER
Geographic Resources Analysis Support System (GRASS GIS).

{usage}:
  $CMD_NAME [-h | --help] [-v | --version]
          [-c | -c geofile | -c EPSG:code[:datum_trans] | -c XY]
          [-e] [-f] [--text | --gtext | --gui] [--config param]
          [[[GISDBASE/]LOCATION/]MAPSET]
  $CMD_NAME [FLAG]... GISDBASE/LOCATION/MAPSET --exec EXECUTABLE [EPARAM]...
  $CMD_NAME --tmp-location [geofile | EPSG | XY] --exec EXECUTABLE [EPARAM]...

{flags}:
  -h or --help                   {help_flag}
  -v or --version                {version_flag}
  -c                             {create}
  -e                             {exit_after}
  -f                             {force_removal}
  --text                         {text}
                                   {text_detail}
  --gtext                        {gtext}
                                   {gtext_detail}
  --gui                          {gui}
                                   {gui_detail}
  --config                       {config}
                                   {config_detail}
  --exec EXECUTABLE              {exec_}
                                   {exec_detail}
  --tmp-location                 {tmp_location}

{params}:
  GISDBASE                       {gisdbase}
                                   {gisdbase_detail}
  LOCATION                       {location}
                                   {location_detail}
  MAPSET                         {mapset}

  GISDBASE/LOCATION/MAPSET       {full_mapset}

  EXECUTABLE                     {executable}
  EPARAM                         {executable_params}
  FLAG                           {standard_flags}

{env_vars}:
  GRASS_GUI                      {gui_var}
  GRASS_HTML_BROWSER             {html_var}
  GRASS_ADDON_PATH               {addon_path_var}
  GRASS_ADDON_BASE               {addon_base_var}
  GRASS_BATCH_JOB                {batch_var}
  GRASS_PYTHON                   {python_var}
"""


def help_message(default_gui):
    t = string.Template(
        HELP_TEXT.format(
            usage=_("Usage"),
            flags=_("Flags"),
            help_flag=_("print this help message"),
            version_flag=_("show version information and exit"),
            create=_("create given database, location or mapset if it doesn't exist"),
            exit_after=_("exit after creation of location or mapset. Only with -c flag"),
            force_removal=_("force removal of .gislock if exists (use with care!). Only with --text flag"),
            text=_("use text based interface (skip graphical welcome screen)"),
            text_detail=_("and set as default"),
            gtext=_("use text based interface (show graphical welcome screen)"),
            gtext_detail=_("and set as default"),
            gui=_("use $DEFAULT_GUI graphical user interface"),
            gui_detail=_("and set as default"),
            config=_("print GRASS configuration parameters"),
            config_detail=_("options: arch,build,compiler,date,path,revision,svn_revision,version"),
            params=_("Parameters"),
            gisdbase=_("initial GRASS database directory"),
            gisdbase_detail=_("directory containing Locations"),
            location=_("initial GRASS Location"),
            location_detail=_("directory containing Mapsets with one common coordinate system (projection)"),
            mapset=_("initial GRASS Mapset"),
            full_mapset=_("fully qualified initial Mapset directory"),
            env_vars=_("Environment variables relevant for startup"),
            gui_var=_("select GUI (text, gui, gtext)"),
            html_var=_("set html web browser for help pages"),
            addon_path_var=_("set additional path(s) to local GRASS modules or user scripts"),
            addon_base_var=_("set additional GISBASE for locally installed GRASS Addons"),
            batch_var=_("shell script to be processed as batch job"),
            python_var=_("set Python interpreter name to override 'python'"),
            exec_=_("execute GRASS module or script"),
            exec_detail=_("provided executable will be executed in GRASS session"),
            executable=_("GRASS module, script or any other executable"),
            executable_params=_("parameters of the executable"),
            standard_flags=_("standard flags"),
            tmp_location=_("create temporary location (use with the --exec flag)"),
        )
    )
    s = t.substitute(CMD_NAME=CMD_NAME, DEFAULT_GUI=default_gui,
                     VERSION_NUMBER=GRASS_VERSION)
    sys.stderr.write(s)


def get_grass_config_dir():
    """Get configuration directory

    Determines path of GRASS GIS user configuration directory and creates
    it if it does not exist.

    Configuration directory is for example used for grass env file
    (the one which caries mapset settings from session to session).
    """
    if sys.platform == 'win32':
        grass_config_dirname = "GRASS7"
        win_conf_path = os.getenv('APPDATA')
        # this can happen with some strange settings
        if not win_conf_path:
            fatal(_("The APPDATA variable is not set, ask your operating"
                    " system support"))
        if not os.path.exists(win_conf_path):
            fatal(_("The APPDATA variable points to directory which does"
                    " not exist, ask your operating system support"))
        directory = os.path.join(win_conf_path, grass_config_dirname)
    else:
        grass_config_dirname = ".grass7"
        directory = os.path.join(os.getenv('HOME'), grass_config_dirname)
    if not os.path.isdir(directory) :
        try:
            os.mkdir(directory)
        except OSError as e:
            # Can happen as a race condition
            if not e.errno == errno.EEXIST or not os.path.isdir(directory):
                fatal(
                    _("Failed to create configuration directory '%s' with error: %s")
                    % (directory, e.strerror))
    return directory


def create_tmp(user, gis_lock):
    """Create temporary directory

    :param user: user name to be used in the directory name
    :param gis_lock: session lock filename to be used in the directory name
    """
    # use $TMPDIR if it exists, then $TEMP, otherwise /tmp
    tmp = os.getenv('TMPDIR')
    if not tmp:
        tmp = os.getenv('TEMP')
    if not tmp:
        tmp = os.getenv('TMP')
    if not tmp:
        tmp = tempfile.gettempdir()

    if tmp:
        tmpdir = os.path.join(
            tmp, "grass7-%(user)s-%(lock)s" % {'user': user,
                                               'lock': gis_lock})
        try:
            os.mkdir(tmpdir, 0o700)
        except:
            tmp = None

    if not tmp:
        for ttmp in ("/tmp", "/var/tmp", "/usr/tmp"):
            tmp = ttmp
            tmpdir = os.path.join(
                tmp, "grass7-%(user)s-%(lock)s" % {'user': user,
                                                   'lock': gis_lock})
            try:
                os.mkdir(tmpdir, 0o700)
            except:
                tmp = None
            if tmp:
                break

    if not tmp:
        fatal(_("Unable to create temporary directory <grass7-%(user)s-"
                "%(lock)s>! Exiting.") % {'user': user, 'lock': gis_lock})

    # promoting the variable even if it was not defined before
    os.environ['TMPDIR'] = tmpdir

    debug("Tmp directory '{tmpdir}' created for user '{user}'".format(
        tmpdir=tmpdir, user=user))
    return tmpdir


def get_gisrc_from_config_dir(grass_config_dir, batch_job):
    """Set the global grassrc file (aka grassrcrc)"""
    if batch_job:
        # use individual GISRCRC files when in batch mode (r33174)
        filename = os.path.join(grass_config_dir, "rc.%s" % platform.node())
        if os.access(filename, os.R_OK):
            return filename
    # use standard file if in normal mode or the special file is not available
    return os.path.join(grass_config_dir, "rc")


def create_gisrc(tmpdir, gisrcrc):
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
    return gisrc


def read_gisrc(filename):
    kv = {}
    try:
        f = open(filename, 'r')
    except IOError:
        return kv

    for line in f:
        try:
            k, v = line.split(':', 1)
        except ValueError as e:
            warning(_("Invalid line in RC file ({file}):"
                      " '{line}' ({error})\n").format(
                          line=line, error=e, file=filename))
            continue
        kv[k.strip()] = v.strip()
    if not kv:
        warning(_("Empty RC file ({file})").format(file=filename))
    f.close()

    return kv


def read_env_file(path):
    kv = {}
    f = open(path, 'r')
    for line in f:
        k, v = line.split(':', 1)
        kv[k.strip()] = v.strip()
    f.close()
    return kv


def write_gisrc(kv, filename):
    f = open(filename, 'w')
    for k, v in kv.items():
        f.write("%s: %s\n" % (k, v))
    f.close()


def read_gui(gisrc, default_gui):
    grass_gui = None
    # At this point the GRASS user interface variable has been set from the
    # command line, been set from an external environment variable,
    # or is not set. So we check if it is not set
    # Check for a reference to the GRASS user interface in the grassrc file
    if os.access(gisrc, os.R_OK):
        kv = read_gisrc(gisrc)
        if 'GRASS_GUI' in os.environ:
            grass_gui = os.environ['GRASS_GUI']
        elif 'GUI' in kv:
            grass_gui = kv['GUI']
        elif 'GRASS_GUI' in kv:
            # For backward compatibility (GRASS_GUI renamed to GUI)
            grass_gui = kv['GRASS_GUI']
        else:
            # Set the GRASS user interface to the default if needed
            grass_gui = default_gui

    if not grass_gui:
        grass_gui = default_gui

    if grass_gui == 'gui':
        grass_gui = default_gui

    # FIXME oldtcltk, gis.m, d.m no longer exist (remove this around 7.2)
    if grass_gui in ['d.m', 'gis.m', 'oldtcltk', 'tcltk']:
        warning(_("GUI <%s> not supported in this version") % grass_gui)
        grass_gui = default_gui

    return grass_gui


def path_prepend(directory, var):
    path = os.getenv(var)
    if path:
        path = directory + os.pathsep + path
    else:
        path = directory
    os.environ[var] = path


def path_append(directory, var):
    path = os.getenv(var)
    if path:
        path = path + os.pathsep + directory
    else:
        path = directory
    os.environ[var] = path


def set_paths(grass_config_dir):
    # addons (path)
    addon_path = os.getenv('GRASS_ADDON_PATH')
    if addon_path:
        for path in addon_path.split(os.pathsep):
            path_prepend(addon_path, 'PATH')

    # addons (base)
    addon_base = os.getenv('GRASS_ADDON_BASE')
    if not addon_base:
        addon_base = os.path.join(grass_config_dir, 'addons')
        os.environ['GRASS_ADDON_BASE'] = addon_base
    if not WINDOWS:
        path_prepend(os.path.join(addon_base, 'scripts'), 'PATH')
    path_prepend(os.path.join(addon_base, 'bin'), 'PATH')

    # standard installation
    if not WINDOWS:
        path_prepend(gpath('scripts'), 'PATH')
    path_prepend(gpath('bin'), 'PATH')

    # Set PYTHONPATH to find GRASS Python modules
    if os.path.exists(gpath('etc', 'python')):
        pythonpath = gpath('etc', 'python')
        path_prepend(pythonpath, 'PYTHONPATH')
        # the env var PYTHONPATH is only evaluated when python is started,
        # thus:
        sys.path.append(pythonpath)
        # now we can import stuff from GRASS lib/python

    # set path for the GRASS man pages
    grass_man_path = gpath('docs', 'man')
    addons_man_path = os.path.join(addon_base, 'docs', 'man')
    man_path = os.getenv('MANPATH')
    sys_man_path = None
    if man_path:
        path_prepend(addons_man_path, 'MANPATH')
        path_prepend(grass_man_path, 'MANPATH')
    else:
        try:
            nul = open(os.devnull, 'w')
            p = Popen(['manpath'], stdout=subprocess.PIPE, stderr=nul)
            nul.close()
            s = p.stdout.read()
            p.wait()
            sys_man_path = s.strip()
        except:
            pass

        if sys_man_path:
            os.environ['MANPATH'] = to_text_string(sys_man_path)
            path_prepend(addons_man_path, 'MANPATH')
            path_prepend(grass_man_path, 'MANPATH')
        else:
            os.environ['MANPATH'] = to_text_string(addons_man_path)
            path_prepend(grass_man_path, 'MANPATH')

    # Set LD_LIBRARY_PATH (etc) to find GRASS shared libraries
    # this works for subprocesses but won't affect the current process
    path_prepend(gpath("lib"), LD_LIBRARY_PATH_VAR)


def find_exe(pgm):
    for directory in os.getenv('PATH').split(os.pathsep):
        path = os.path.join(directory, pgm)
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
        elif WINDOWS:
            pager = "more"
        else:
            pager = "cat"
        os.environ['GRASS_PAGER'] = pager

    # GRASS_PYTHON
    if not os.getenv('GRASS_PYTHON'):
        if WINDOWS:
            os.environ['GRASS_PYTHON'] = "python3.exe"
        else:
            os.environ['GRASS_PYTHON'] = "python3"

    # GRASS_GNUPLOT
    if not os.getenv('GRASS_GNUPLOT'):
        os.environ['GRASS_GNUPLOT'] = "gnuplot -persist"

    # GRASS_PROJSHARE
    if not os.getenv('GRASS_PROJSHARE'):
        os.environ['GRASS_PROJSHARE'] = CONFIG_PROJSHARE


def set_display_defaults():
    """ Predefine monitor size for certain architectures"""
    if os.getenv('HOSTTYPE') == 'arm':
        # small monitor on ARM (iPAQ, zaurus... etc)
        os.environ['GRASS_RENDER_HEIGHT'] = "320"
        os.environ['GRASS_RENDER_WIDTH'] = "240"


def set_browser():
    # GRASS_HTML_BROWSER
    browser = os.getenv('GRASS_HTML_BROWSER')
    if not browser:
        if MACOSX:
            # OSX doesn't execute browsers from the shell PATH - route through a
            # script
            browser = gpath('etc', "html_browser_mac.sh")
            os.environ['GRASS_HTML_BROWSER_MACOSX'] = "-b com.apple.helpviewer"

        if WINDOWS:
            browser = "start"
        elif CYGWIN:
            browser = "explorer"
        else:
            # the usual suspects
            browsers = ["xdg-open", "x-www-browser", "htmlview", "konqueror", "mozilla",
                        "mozilla-firefox", "firefox", "iceweasel", "opera", "google-chrome",
                        "chromium", "netscape", "dillo", "lynx", "links", "w3c"]
            for b in browsers:
                if find_exe(b):
                    browser = b
                    break

    elif MACOSX:
        # OSX doesn't execute browsers from the shell PATH - route through a
        # script
        os.environ['GRASS_HTML_BROWSER_MACOSX'] = "-b %s" % browser
        browser = gpath('etc', "html_browser_mac.sh")

    if not browser:
        # even so we set to 'xdg-open' as a generic fallback
        browser = "xdg-open"

    os.environ['GRASS_HTML_BROWSER'] = browser


def ensure_home():
    """Set HOME if not set on MS Windows"""
    if WINDOWS and not os.getenv('HOME'):
        os.environ['HOME'] = os.path.join(os.getenv('HOMEDRIVE'),
                                          os.getenv('HOMEPATH'))


def create_initial_gisrc(filename):
    # for convenience, define GISDBASE as pwd:
    s = r"""GISDBASE: %s
LOCATION_NAME: <UNKNOWN>
MAPSET: <UNKNOWN>
""" % os.getcwd()
    writefile(filename, s)


def check_gui(expected_gui):
    grass_gui = expected_gui
    # Check if we are running X windows by checking the DISPLAY variable
    if os.getenv('DISPLAY') or WINDOWS or MACOSX:
        # Check if python is working properly
        if expected_gui in ('wxpython', 'gtext'):
            nul = open(os.devnull, 'w')
            p = Popen([os.environ['GRASS_PYTHON']], stdin=subprocess.PIPE,
                      stdout=nul, stderr=nul)
            nul.close()
            p.stdin.write("variable=True".encode(ENCODING))
            p.stdin.close()
            p.wait()
            msg = None
            if p.returncode != 0:
                # Python was not found - switch to text interface mode
                msg = _("The python command does not work as expected!\n"
                        "Please check your GRASS_PYTHON environment variable.\n"
                        "Use the -help option for details.\n")
            if not os.path.exists(wxpath("wxgui.py")):
                msg = _("GRASS GUI not found. Please check your installation.")
            if msg:
                warning(_("{}\nSwitching to text based interface mode.\n\n"
                          "Hit RETURN to continue.\n").format(msg))
                sys.stdin.readline()
                grass_gui = 'text'

    else:
        # Display a message if a graphical interface was expected
        if expected_gui != 'text':
            # Set the interface mode to text
            warning(_("It appears that the X Windows system is not active.\n"
                      "A graphical based user interface is not supported.\n"
                      "(DISPLAY variable is not set.)\n"
                      "Switching to text based interface mode.\n\n"
                      "Hit RETURN to continue.\n"))
            sys.stdin.readline()
            grass_gui = 'text'
    return grass_gui


def save_gui(gisrc, grass_gui):
    """Save the user interface variable in the grassrc file"""
    if os.access(gisrc, os.F_OK):
        kv = read_gisrc(gisrc)
        kv['GUI'] = grass_gui
        write_gisrc(kv, gisrc)


def create_location(gisdbase, location, geostring):
    """Create GRASS Location using georeferenced file or EPSG

    EPSG code format is ``EPSG:code`` or ``EPSG:code:datum_trans``.

    :param gisdbase: Path to GRASS GIS database directory
    :param location: name of new Location
    :param geostring: path to a georeferenced file or EPSG code
    """
    if gpath('etc', 'python') not in sys.path:
        sys.path.append(gpath('etc', 'python'))
    from grass.script import core as gcore  # pylint: disable=E0611

    try:
        if geostring and geostring.upper().find('EPSG:') > -1:
            # create location using EPSG code
            epsg = geostring.split(':', 1)[1]
            if ':' in epsg:
                epsg, datum_trans = epsg.split(':', 1)
            else:
                datum_trans = None
            gcore.create_location(gisdbase, location,
                                  epsg=epsg, datum_trans=datum_trans)
        elif geostring == 'XY':
            # create an XY location
            gcore.create_location(gisdbase, location)
        else:
            # create location using georeferenced file
            gcore.create_location(gisdbase, location,
                                  filename=geostring)
    except gcore.ScriptError as err:
        fatal(err.value.strip('"').strip("'").replace('\\n', os.linesep))


# TODO: distinguish between valid for getting maps and usable as current
# https://lists.osgeo.org/pipermail/grass-dev/2016-September/082317.html
# interface created according to the current usage
def is_mapset_valid(full_mapset):
    """Return True if GRASS Mapset is valid"""
    # WIND is created from DEFAULT_WIND by `g.region -d` and functions
    # or modules which create a new mapset. Most modules will fail if
    # WIND doesn't exist (assuming that neither GRASS_REGION nor
    # WIND_OVERRIDE environmental variables are set).
    return os.access(os.path.join(full_mapset, "WIND"), os.R_OK)


def is_location_valid(gisdbase, location):
    """Return True if GRASS Location is valid

    :param gisdbase: Path to GRASS GIS database directory
    :param location: name of a Location
    """
    # DEFAULT_WIND file should not be required until you do something
    # that actually uses them. The check is just a heuristic; a directory
    # containing a PERMANENT/DEFAULT_WIND file is probably a GRASS
    # location, while a directory lacking it probably isn't.
    return os.access(os.path.join(gisdbase, location,
                                  "PERMANENT", "DEFAULT_WIND"), os.F_OK)


# basically checking location, possibly split into two functions
# (mapset one can call location one)
def get_mapset_invalid_reason(gisdbase, location, mapset):
    """Returns a message describing what is wrong with the Mapset

    The goal is to provide the most suitable error message
    (rather than to do a quick check).

    :param gisdbase: Path to GRASS GIS database directory
    :param location: name of a Location
    :param mapset: name of a Mapset
    :returns: translated message
    """
    full_location = os.path.join(gisdbase, location)
    full_permanent = os.path.join(full_location, 'PERMANENT')
    full_mapset = os.path.join(full_location, mapset)
    # first checking the location validity
    if not os.path.exists(full_location):
        return _("Location <%s> doesn't exist") % full_location
    elif 'PERMANENT' not in os.listdir(full_location):
        return _("<%s> is not a valid GRASS Location"
                 " because PERMANENT Mapset is missing") % full_location
    elif not os.path.isdir(full_permanent):
        return _("<%s> is not a valid GRASS Location"
                 " because PERMANENT is not a directory") % full_location
    # partially based on the is_location_valid() function
    elif not os.path.isfile(os.path.join(full_permanent,
                                         'DEFAULT_WIND')):
        return _("<%s> is not a valid GRASS Location"
                 " because PERMANENT Mapset does not have a DEFAULT_WIND file"
                 " (default computational region)") % full_location
    # if location is valid, check mapset
    elif mapset not in os.listdir(full_location):
        return _("Mapset <{mapset}> doesn't exist in GRASS Location <{loc}>. "
                 "A new mapset can be created by '-c' switch.").format(
                     mapset=mapset, loc=location)
    elif not os.path.isdir(full_mapset):
        return _("<%s> is not a GRASS Mapset"
                 " because it is not a directory") % mapset
    elif not os.path.isfile(os.path.join(full_mapset, 'WIND')):
        return _("<%s> is not a valid GRASS Mapset"
                 " because it does not have a WIND file") % mapset
    # based on the is_mapset_valid() function
    elif not os.access(os.path.join(full_mapset, "WIND"), os.R_OK):
        return _("<%s> is not a valid GRASS Mapset"
                 " because its WIND file is not readable") % mapset
    else:
        return _("Mapset <{mapset}> or Location <{location}> is"
                 " invalid for an unknown reason").format(
                     mapset=mapset, loc=location)


def can_create_location(gisdbase, location):
    """Checks if location can be created"""
    path = os.path.join(gisdbase, location)
    if os.path.exists(path):
        return False
    return True


def cannot_create_location_reason(gisdbase, location):
    """Returns a message describing why location cannot be created

    The goal is to provide the most suitable error message
    (rather than to do a quick check).

    :param gisdbase: Path to GRASS GIS database directory
    :param location: name of a Location
    :returns: translated message
    """
    path = os.path.join(gisdbase, location)
    if is_location_valid(gisdbase, location):
        return _("Unable to create new location because"
                 " the location <{location}>"
                 " already exists.").format(**locals())
    elif os.path.isfile(path):
        return _("Unable to create new location <{location}> because"
                 " <{path}> is a file.").format(**locals())
    elif os.path.isdir(path):
        return _("Unable to create new location <{location}> because"
                 " the directory <{path}>"
                 " already exists.").format(**locals())
    else:
        return _("Unable to create new location in"
                 " the directory <{path}>"
                 " for an unknown reason.").format(**locals())


def set_mapset(gisrc, arg=None, geofile=None, create_new=False,
               tmp_location=False, tmpdir=None):
    """Selected Location and Mapset are checked and created if requested

    The gisrc (GRASS environment file) is written at the end
    (nothing is returned).

    tmp_location requires tmpdir (which is used as gisdbase)
    """
    # TODO: arg param seems to be always the mapset parameter (or a dash
    # in a distant past), refactor
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

    # all was None for tmp loc so that case goes here quickly
    # TODO: but the above code needs review anyway
    if tmp_location:
        # set gisdbase to temporary directory
        gisdbase = tmpdir
        # we are already in a unique directory, so we can use fixed name
        location_name = "tmploc"
        # we need only one mapset
        mapset = "PERMANENT"
        debug("Using temporary location <{gdb}{sep}{lc}>".format(
            gdb=gisdbase, lc=location_name, sep=os.path.sep))

    if gisdbase and location_name and mapset:
        path = os.path.join(gisdbase, location_name, mapset)
        # check if 'path' is a valid GRASS location/mapset
        path_is_valid_mapset = is_mapset_valid(path)

        if path_is_valid_mapset and create_new:
            warning(_("Mapset <{}> already exists. Ignoring the"
                      " request to create it. Note that this warning"
                      " may become an error in future versions.")
                    .format(path))

        if not path_is_valid_mapset:
            if not create_new:
                # 'path' is not a valid mapset and user does not
                # want to create anything new
                fatal(get_mapset_invalid_reason(gisdbase, location_name, mapset))
            else:
                # 'path' is not valid and the user wants to create
                # mapset on the fly
                # check if 'location_name' is a valid GRASS location
                if not is_location_valid(gisdbase, location_name):
                    if not tmp_location:
                        # 'location_name' is not a valid GRASS location
                        # and user requested its creation, so we parsed
                        # the path wrong and need to move one level
                        # and use 'PERMANENT' mapset
                        # (we already got that right in case of tmploc)
                        gisdbase = os.path.join(gisdbase, location_name)
                        location_name = mapset
                        mapset = "PERMANENT"
                    if not can_create_location(gisdbase, location_name):
                        fatal(cannot_create_location_reason(
                            gisdbase, location_name))
                    # create new location based on the provided EPSG/...
                    message(_("Creating new GRASS GIS location <{}>...")
                            .format(location_name))
                    create_location(gisdbase, location_name, geofile)
                else:
                    # 'location_name' is a valid GRASS location,
                    # create new mapset
                    message(_("Creating new GRASS GIS mapset <{}>...")
                            .format(mapset))
                    if os.path.isfile(path):
                        # not a valid mapset, but dir exists, assuming
                        # broken/incomplete mapset
                        fatal(_("Unable to create new mapset <{mapset}>"
                                " because <{path}> is a file.")
                              .format(mapset=mapset, path=path))
                    elif os.path.isdir(path):
                        # not a valid mapset, but dir exists, assuming
                        # broken/incomplete mapset
                        warning(_("The mapset <{}> is missing the WIND file"
                                  " (computational region). It will be"
                                  " fixed now. Note that this warning"
                                  " may become an error in future versions.")
                                .format(mapset))
                    else:
                        # create mapset directory
                        os.mkdir(path)
                    # make directory a mapset, add the region
                    # copy PERMANENT/DEFAULT_WIND to <mapset>/WIND
                    s = readfile(os.path.join(gisdbase, location_name,
                                              "PERMANENT", "DEFAULT_WIND"))
                    writefile(os.path.join(path, "WIND"), s)

        if os.access(gisrc, os.R_OK):
            kv = read_gisrc(gisrc)
        else:
            kv = {}

        kv['GISDBASE'] = gisdbase
        kv['LOCATION_NAME'] = location_name
        kv['MAPSET'] = mapset
        write_gisrc(kv, gisrc)
    else:
        fatal(_("GRASS GIS database directory, location and mapset"
                " not set properly."
                " Use GUI or command line to set them."))


def set_mapset_interactive(grass_gui):
    """User selects Location and Mapset in an interative way

    The gisrc (GRASS environment file) is written at the end.
    """
    if not os.path.exists(wxpath("gis_set.py")) and grass_gui != 'text':
        debug("No GUI available, switching to text mode")
        return False

    # Check for text interface
    if grass_gui == 'text':
        # TODO: maybe this should be removed and solved from outside
        # this depends on what we expect from this function
        # should gisrc be ok after running or is it allowed to be still not set
        pass
    # Check for GUI
    elif grass_gui in ('gtext', 'wxpython'):
        gui_startup(grass_gui)
    else:
        # Shouldn't need this but you never know
        fatal(_("Invalid user interface specified - <%s>. "
                "Use the --help option to see valid interface names.") % grass_gui)

    return True


def gui_startup(grass_gui):
    """Start GUI for startup (setting gisrc file)"""
    if grass_gui in ('wxpython', 'gtext'):
        ret = call([os.getenv('GRASS_PYTHON'), wxpath("gis_set.py")])

    # this if could be simplified to three branches (0, 5, rest)
    # if there is no need to handle unknown code separately
    if ret == 0:
        pass
    elif ret in [1, 2]:
        # 1 probably error coming from gis_set.py
        # 2 probably file not found from python interpreter
        # formerly we were starting in text mode instead, now we just fail
        # which is more straightforward for everybody
        fatal(_("Error in GUI startup. See messages above (if any)"
                " and if necessary, please"
                " report this error to the GRASS developers.\n"
                "On systems with package manager, make sure you have the right"
                " GUI package, probably named grass-gui, installed.\n"
                "To run GRASS GIS in text mode use the --text flag.\n"
                "Use '--help' for further options\n"
                "     {cmd_name} --help\n"
                "See also: https://grass.osgeo.org/{cmd_name}/manuals/helptext.html").format(
                    cmd_name=CMD_NAME))
    elif ret == 5:  # defined in gui/wxpython/gis_set.py
        # User wants to exit from GRASS
        message(_("Exit was requested in GUI.\nGRASS GIS will not start. Bye."))
        sys.exit(0)
    else:
        fatal(_("Invalid return code from GUI startup script.\n"
                "Please advise GRASS developers of this error."))


# we don't follow the LOCATION_NAME legacy naming here but we have to still
# translate to it, so always double check
class MapsetSettings(object):
    """Holds GRASS GIS database directory, Location and Mapset

    Provides few convenient functions.
    """
    def __init__(self):
        self.gisdbase = None
        self.location = None
        self.mapset = None
        self._full_mapset = None

    # TODO: perhaps full_mapset would be better as mapset_path
    # TODO: works only when set for the first time
    # this follows the current usage but we must invalidate when
    # the others are changed (use properties for that)
    @property
    def full_mapset(self):
        if self._full_mapset is None:
            self._full_mapset = os.path.join(self.gisdbase, self.location,
                                             self.mapset)
        return self._full_mapset

    # TODO: perhaps conversion to bool would be nicer
    def is_valid(self):
        return self.gisdbase and self.location and self.mapset


# TODO: does it really makes sense to tell user about gisrcrc?
# anything could have happened in between loading from gisrcrc and now
# (we do e.g. GUI or creating loctation)
def load_gisrc(gisrc, gisrcrc):
    """Get the settings of Location and Mapset from the gisrc file

    :returns: MapsetSettings object
    """
    mapset_settings = MapsetSettings()
    kv = read_gisrc(gisrc)
    mapset_settings.gisdbase = kv.get('GISDBASE')
    mapset_settings.location = kv.get('LOCATION_NAME')
    mapset_settings.mapset = kv.get('MAPSET')
    if not mapset_settings.is_valid():
        fatal(_("Error reading data path information from g.gisenv.\n"
                "GISDBASE={gisdbase}\n"
                "LOCATION_NAME={location}\n"
                "MAPSET={mapset}\n\n"
                "Check the <{file}> file.").format(
                    gisdbase=mapset_settings.gisdbase,
                    location=mapset_settings.location,
                    mapset=mapset_settings.mapset,
                    file=gisrcrc))
    return mapset_settings


# load environmental variables from grass_env_file
def load_env(grass_env_file):
    if not os.access(grass_env_file, os.R_OK):
        return

    # Regular expression for lines starting with "export var=val" (^export
    # lines below). Environment variables should start with a-zA-Z or _.
    # \1 and \2 are a variable name and its value, respectively.
    export_re = re.compile('^export[ \t]+([a-zA-Z_]+[a-zA-Z0-9_]*)=(.*?)[ \t]*$')

    for line in readfile(grass_env_file).split(os.linesep):
        # match ^export lines
        m = export_re.match(line)
        # if not ^export lines, skip
        if not m:
            continue

        # k is the variable name and v is its value
        k = m.group(1)
        v = m.group(2)
        # let's try to expand any $var's in v
        expand = True
        if v.startswith("'") and v.endswith("'"):
            # we're parsing
            #   export var='value'
            # and need to strip out starting and ending quotes from the value
            v = v.strip("'")
            # we don't want to expand any $var's inside 'value' because they
            # are within single quotes
            expand = False
        elif v.startswith('"') and v.endswith('"'):
            # in this case, we're parsing
            #   export var="value"
            # and again need to strip out starting and ending quotes from the
            # value
            v = v.strip('"')
            # we'll keep expand=True to expand $var's inside "value" because
            # they are within double quotes
        elif v.startswith("'") or v.endswith("'") or v.startswith('"') or v.endswith('"'):
            # here, let's try to ignore unmatching single/double quotes, which
            # might be a multi-line variable or just a user error
            debug("Ignoring multi-line environmental variable {0}".format(k))
            continue
        if expand:
            # finally, expand $var's within a non-single quoted value
            v = os.path.expanduser(os.path.expandvars(v.replace('\\$', '\0')).replace('\0', '$'))
        debug("Environmental variable set {0}={1}".format(k, v))
        # create a new environment variable
        os.environ[k] = v

    # Allow for mixed ISIS-GRASS Environment
    if os.getenv('ISISROOT'):
        isis = os.getenv('ISISROOT')
        os.environ['ISIS_LIB'] = isis + os.sep + "lib"
        os.environ['ISIS_3RDPARTY'] = isis + os.sep + "3rdParty" + os.sep + "lib"
        os.environ['QT_PLUGIN_PATH'] = isis + os.sep + "3rdParty" + os.sep + "plugins"
        # os.environ['ISIS3DATA'] = isis + "$ISIS3DATA"
        libpath = os.getenv('LD_LIBRARY_PATH', '')
        isislibpath = os.getenv('ISIS_LIB')
        isis3rdparty = os.getenv('ISIS_3RDPARTY')
        os.environ['LD_LIBRARY_PATH'] = libpath + os.pathsep + isislibpath + os.pathsep + isis3rdparty


def install_notranslation():
    # If locale is not supported, _ function might be missing
    # This function just installs _ as a pass-through function
    # See trac #3875 for details
    import builtins
    builtins.__dict__['_'] = lambda x: x


def set_language(grass_config_dir):
    # This function is used to override system default language and locale
    # Such override can be requested only from wxGUI
    # An override if user has provided correct environmental variables as
    # LC_MESSAGES or LANG is not necessary.
    # Unfortunately currently a working solution for Windows is lacking
    # thus it always on Vista and XP will print an error.
    # See discussion for Windows not following its own documentation and
    # not accepting ISO codes as valid locale identifiers http://bugs.python.org/issue10466
    language = 'None'  # Such string sometimes is present in wx file
    encoding = None

    # Override value is stored in wxGUI preferences file.
    # As it's the only thing required, we'll just grep it out.
    try:
        fd = open(os.path.join(grass_config_dir, 'wx'), 'r')
    except:
        # Even if there is no override, we still need to set locale.
        pass
    else:
        for line in fd:
            if re.search('^language', line):
                line = line.rstrip(' %s' % os.linesep)
                language = ''.join(line.split(';')[-1:])
                break
        fd.close()

    # Backwards compatibility with old wx preferences files
    if language == 'C':
        language = 'en'

    if language == 'None' or language == '' or not language:
        # Language override is disabled (system language specified)
        # As by default program runs with C locale, but users expect to
        # have their default locale, we'll just set default locale
        try:
            locale.setlocale(locale.LC_ALL, '')
        except locale.Error as e:
            # If we get here, system locale settings are terribly wrong
            # There is no point to continue as GRASS/Python will fail
            # in some other unpredictable way.
            sys.stderr.write(
                "System locale is not usable (LC_ALL variable not defined)."
                " Most likely it indicates misconfigured environment.\n")
            sys.stderr.write("Reported error message: %s\n" % e)
            # it would be too drastic to exit
            # sys.exit("Fix system locale settings and then try again.")
            locale.setlocale(locale.LC_ALL, 'C')
            sys.stderr.write("Default locale settings are missing. GRASS running with C locale.\n")

        language, encoding = locale.getdefaultlocale()
        if not language:
            sys.stderr.write("Default locale settings are missing. GRASS running with C locale.\n")
            install_notranslation()
            return

    else:
        debug("A language override has been requested."
              " Trying to switch GRASS into '%s'..." % language)
        try:
            locale.setlocale(locale.LC_ALL, language)
        except locale.Error as e:
            try:
                # Locale lang.encoding might be missing. Let's try
                # UTF-8 encoding before giving up as on Linux systems
                # lang.UTF-8 locales are more common than legacy
                # ISO-8859 ones.
                encoding = 'UTF-8'
                normalized = locale.normalize('%s.%s' % (language, encoding))
                locale.setlocale(locale.LC_ALL, normalized)
            except locale.Error as e:
                if language == 'en':
                    # A workaround for Python Issue30755
                    # https://bugs.python.org/issue30755
                    if locale.normalize('C.UTF-8') == 'en_US.UTF-8':
                        locale.setlocale(locale.LC_ALL, 'C')
                        os.environ['LANGUAGE'] = 'C'
                        os.environ['LANG'] = 'C'
                        os.environ['LC_MESSAGES'] = 'C'
                        os.environ['LC_NUMERIC'] = 'C'
                        os.environ['LC_TIME'] = 'C'
                        sys.stderr.write(
                            "To avoid Unicode errors in GUI, install"
                            " en_US.UTF-8 locale and restart GRASS.\n"
                            "Also consider upgrading your Python version"
                            " to one containing fix for Python Issue 30755.\n")
                        install_notranslation()
                        return
                    # en_US locale might be missing, still all messages in
                    # GRASS are already in en_US language.
                    # Using plain C as locale forces encodings to ascii
                    # thus lets try our luck with C.UTF-8 first.
                    # See bugs #3441 and #3423
                    try:
                        locale.setlocale(locale.LC_ALL, 'C.UTF-8')
                    except locale.Error as e:
                        # All lost. Setting to C as much as possible.
                        # We can not call locale.normalize on C as it
                        # will transform it to en_US and we already know
                        # it doesn't work.
                        locale.setlocale(locale.LC_ALL, 'C')
                        os.environ['LANGUAGE'] = 'C'
                        os.environ['LANG'] = 'C'
                        os.environ['LC_MESSAGES'] = 'C'
                        os.environ['LC_NUMERIC'] = 'C'
                        os.environ['LC_TIME'] = 'C'
                        gettext.install('grasslibs', gpath('locale'))
                        sys.stderr.write(
                            "All attempts to enable English language have"
                            " failed. GRASS running with C locale.\n"
                            "If you observe UnicodeError in Python,"
                            " install en_US.UTF-8"
                            " locale and restart GRASS.\n")
                        install_notranslation()
                        return
                else:
                    # The last attempt...
                    try:
                        encoding = locale.getpreferredencoding()
                        normalized = locale.normalize('%s.%s' % (language, encoding))
                        locale.setlocale(locale.LC_ALL, normalized)
                    except locale.Error as e:
                        # If we got so far, attempts to set up language and locale have failed
                        # on this system
                        sys.stderr.write("Failed to enforce user specified language '%s' with error: '%s'\n" % (language, e))
                        sys.stderr.write("A LANGUAGE environmental variable has been set.\nPart of messages will be displayed in the requested language.\n")
                        # Even if setting locale will fail, let's set LANG in a hope,
                        # that UI will use it GRASS texts will be in selected language,
                        # system messages (i.e. OK, Cancel etc.) - in system default
                        # language
                        os.environ['LANGUAGE'] = language
                        os.environ['LANG'] = language
                        install_notranslation()
                        return

    # Set up environment for subprocesses
    os.environ['LANGUAGE'] = language
    os.environ['LANG'] = language

    if language == 'ko_KR' and encoding == 'cp949':
        # The default encoding for the Korean language in Windows is cp949,
        # Microsoft's proprietary extension to euc-kr, but gettext prints no
        # translated messages at all in the Command Prompt window if LC_CTYPE
        # is set to ko_KR.cp949. Here, force LC_CTYPE to be euc-kr.
        normalized = 'euc-kr'
        encoding = None
    elif encoding:
        normalized = locale.normalize('%s.%s' % (language, encoding))
    else:
        normalized = language

    for lc in ('LC_CTYPE', 'LC_MESSAGES', 'LC_TIME', 'LC_COLLATE',
               'LC_MONETARY', 'LC_PAPER', 'LC_NAME', 'LC_ADDRESS',
               'LC_TELEPHONE', 'LC_MEASUREMENT', 'LC_IDENTIFICATION'):
        os.environ[lc] = normalized

    # Some code in GRASS might not like other decimal separators than .
    # Other potential sources for problems are: LC_TIME LC_CTYPE
    locale.setlocale(locale.LC_NUMERIC, 'C')
    os.environ['LC_NUMERIC'] = 'C'
    if os.getenv('LC_ALL'):
        del os.environ['LC_ALL']  # Remove LC_ALL to not override LC_NUMERIC

    # From now on enforce the new language
    if encoding:
        gettext.install('grasslibs', gpath('locale'), codeset=encoding)
    else:
        gettext.install('grasslibs', gpath('locale'))


# TODO: grass_gui parameter is a hack and should be removed, see below
def lock_mapset(mapset_path, force_gislock_removal, user, grass_gui):
    """Lock the mapset and return name of the lock file

    Behavior on error must be changed somehow; now it fatals but GUI case is
    unresolved.
    """
    if not os.path.exists(mapset_path):
        fatal(_("Path '%s' doesn't exist") % mapset_path)
    if not os.access(mapset_path, os.W_OK):
        error = _("Path '%s' not accessible.") % mapset_path
        stat_info = os.stat(mapset_path)
        mapset_uid = stat_info.st_uid
        if mapset_uid != os.getuid():
            # GTC %s is mapset's folder path
            error = "%s\n%s" % (error, _("You are not the owner of '%s'.") % mapset_path)
        fatal(error)
    # Check for concurrent use
    lockfile = os.path.join(mapset_path, ".gislock")
    ret = call([gpath("etc", "lock"), lockfile, "%d" % os.getpid()])
    msg = None
    if ret == 2:
        if not force_gislock_removal:
            msg = _("%(user)s is currently running GRASS in selected mapset"
                    " (file %(file)s found). Concurrent use not allowed.\n"
                    "You can force launching GRASS using -f flag"
                    " (note that you need permission for this operation)."
                    " Have another look in the processor "
                    "manager just to be sure..." % {
                        'user': user, 'file': lockfile})
        else:
            try_remove(lockfile)
            message(_("%(user)s is currently running GRASS in selected mapset"
                      " (file %(file)s found). Forcing to launch GRASS..." % {
                          'user': user, 'file': lockfile}))
    elif ret != 0:
        msg = _("Unable to properly access '%s'.\n"
                "Please notify system personnel.") % lockfile

    # TODO: the gui decision should be done by the caller
    # this needs some change to the function interface, return tuple or
    # use exceptions (better option)
    if msg:
        if grass_gui == "wxpython":
            call([os.getenv('GRASS_PYTHON'), wxpath("gis_set_error.py"), msg])
            # TODO: here we probably miss fatal or exit, needs to be added
        else:
            fatal(msg)
    debug("Mapset <{mapset}> locked using '{lockfile}'".format(
        mapset=mapset_path, lockfile=lockfile))
    return lockfile


# TODO: the gisrcrc here does not make sense, remove it from load_gisrc
def unlock_gisrc_mapset(gisrc, gisrcrc):
    """Unlock mapset from the gisrc file"""
    settings = load_gisrc(gisrc, gisrcrc)
    lockfile = os.path.join(settings.full_mapset, ".gislock")
    # this fails silently, perhaps a warning would be helpful to
    # catch cases when removal was not possible due to e.g. another
    # session force-removing the file (unlocking the mapset)
    try_remove(lockfile)


def make_fontcap():
    # TODO: is GRASS_FONT_CAP ever defined? It seems it must be defined in system
    fc = os.getenv('GRASS_FONT_CAP')
    if fc and not os.access(fc, os.R_OK):
        message(_("Building user fontcap..."))
        call(["g.mkfontcap"])


def ensure_db_connected(mapset):
    """Predefine default driver if DB connection not defined

    :param mapset: full path to the mapset
    """
    if not os.access(os.path.join(mapset, "VAR"), os.F_OK):
        call(['db.connect', '-c', '--quiet'])


def get_shell():
    # set SHELL on ms windowns
    # this was at the very beginning of the script but it can be anywhere
    if WINDOWS:
        if os.getenv('GRASS_SH'):
            os.environ['SHELL'] = os.getenv('GRASS_SH')
        if not os.getenv('SHELL'):
            os.environ['SHELL'] = os.getenv('COMSPEC', 'cmd.exe')

    # cygwin has many problems with the shell setup
    # below, so i hardcoded everything here.
    if sys.platform == 'cygwin':
        sh = "CYGWIN"
        shellname = "GNU Bash (Cygwin)"
        os.environ['SHELL'] = "/usr/bin/bash.exe"
        os.environ['OSTYPE'] = "cygwin"
    else:
        # in docker the 'SHELL' variable may not be
        # visible in a Python session unless 'ENV SHELL /bin/bash' is set in Dockerfile
        try:
            sh = os.path.basename(os.getenv('SHELL'))
        except:
            sh = 'sh'
            os.environ['SHELL'] = sh

        if WINDOWS and sh:
            sh = os.path.splitext(sh)[0]

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
        elif sh == "zsh":
            shellname = "Z Shell"
        elif sh == "cmd":
            shellname = "Command Prompt"
        elif sh == "powershell":
            shellname = "Windows PowerShell"
        else:
            shellname = "shell"
    # check for SHELL
    if not os.getenv('SHELL'):
        fatal(_("The SHELL variable is not set"))
    return sh, shellname


def get_grass_env_file(sh, grass_config_dir):
    """Get name of the shell-specific GRASS environment (rc) file"""
    if sh in ['csh', 'tcsh']:
        grass_env_file = os.path.join(grass_config_dir, 'cshrc')
    elif sh in ['bash', 'msh', 'cygwin', 'sh']:
        grass_env_file = os.path.join(grass_config_dir, 'bashrc')
    elif sh == 'zsh':
        grass_env_file = os.path.join(grass_config_dir, 'zshrc')
    elif sh in ['cmd', 'powershell']:
        grass_env_file = os.path.join(grass_config_dir, 'env.bat')
    else:
        grass_env_file = os.path.join(grass_config_dir, 'bashrc')
        warning(_("Unsupported shell <{sh}>: {env_file}").format(
            sh=sh, env_file=grass_env_file))
    return grass_env_file


def get_batch_job_from_env_variable():
    """Get script to execute from batch job variable if available

    Fails with fatal if variable is set but content unusable.
    """
    # hack to process batch jobs:
    batch_job = os.getenv('GRASS_BATCH_JOB')
    # variable defined, but user might not have been careful enough
    if batch_job:
        if not os.access(batch_job, os.F_OK):
            # wrong file
            fatal(_("Job file <%s> has been defined in "
                    "the 'GRASS_BATCH_JOB' variable but not found. Exiting."
                    "\n\n"
                    "Use 'unset GRASS_BATCH_JOB' to disable "
                    "batch job processing.") % batch_job)
        elif not os.access(batch_job, os.X_OK):
            # right file, but ...
            fatal(_("Change file permission to 'executable' for <%s>")
                  % batch_job)
    return batch_job


def run_batch_job(batch_job):
    """Runs script, module or any command

    If *batch_job* is a string (insecure) shell=True is used for execution.

    :param batch_job: executable and parameters in a list or a string
    """
    batch_job_string = batch_job
    if not isinstance(batch_job, six.string_types):
        # for messages only
        batch_job_string = ' '.join(batch_job)
    message(_("Executing <%s> ...") % batch_job_string)
    if isinstance(batch_job, six.string_types):
        # shell=True is keeping the original GRASS_BATCH_JOB behavior
        def quote(string):
            if '"' in string:
                return "'%s'" % batch_job
            else:
                return '"%s"' % batch_job
        batch_job = quote(batch_job)
        proc = Popen(batch_job, shell=True)
    else:
        try:
            proc = Popen(batch_job, shell=False)
        except OSError as error:
            fatal(_("Execution of <{cmd}> failed:\n"
                    "{error}").format(cmd=batch_job_string, error=error))
    returncode = proc.wait()
    message(_("Execution of <%s> finished.") % batch_job_string)
    return returncode


def start_gui(grass_gui):
    """Start specified GUI

    :param grass_gui: GUI name (allowed values: 'wxpython')
    """
    # Start the chosen GUI but ignore text
    debug("GRASS GUI should be <%s>" % grass_gui)
    # Check for gui interface
    if grass_gui == "wxpython":
        Popen([os.getenv('GRASS_PYTHON'), wxpath("wxgui.py")])


def close_gui():
    """Close GUI if running"""
    if gpath('etc', 'python') not in sys.path:
        sys.path.append(gpath('etc', 'python'))
    from grass.script import core as gcore  # pylint: disable=E0611
    env = gcore.gisenv()
    if 'GUI_PID' not in env:
        return
    for pid in env['GUI_PID'].split(','):
        debug("Exiting GUI with pid={0}".format(pid))
        try:
            os.kill(int(pid), signal.SIGTERM)
        except OSError as e:
            message(_("Unable to close GUI. {0}").format(e))


def show_banner():
    """Write GRASS GIS ASCII name to stderr"""
    sys.stderr.write(r"""
          __________  ___   __________    _______________
         / ____/ __ \/   | / ___/ ___/   / ____/  _/ ___/
        / / __/ /_/ / /| | \__ \\_  \   / / __ / / \__ \
       / /_/ / _, _/ ___ |___/ /__/ /  / /_/ // / ___/ /
       \____/_/ |_/_/  |_/____/____/   \____/___//____/

""")


def say_hello():
    """Write welcome to stderr including code revision if in git copy"""
    sys.stderr.write(_("Welcome to GRASS GIS %s") % GRASS_VERSION)
    if GRASS_VERSION.endswith('dev'):
        try:
            filerev = open(gpath('etc', 'VERSIONNUMBER'))
            linerev = filerev.readline().rstrip('\n')
            filerev.close()

            revision = linerev.split(' ')[1]
            sys.stderr.write(' (' + revision + ')')
        except:
            pass


INFO_TEXT = r"""
%-41shttps://grass.osgeo.org
%-41s%s (%s)
%-41sg.manual -i
%-41sg.version -c
%-41sg.version -x
"""


def show_info(shellname, grass_gui, default_gui):
    """Write basic info about GRASS GIS and GRASS session to stderr"""
    sys.stderr.write(INFO_TEXT % (
        _("GRASS GIS homepage:"),
        # GTC Running through: SHELL NAME
        _("This version running through:"),
        shellname, os.getenv('SHELL'),
        _("Help is available with the command:"),
        _("See the licence terms with:"),
        _("See citation options with:")))

    if grass_gui == 'wxpython':
        message("%-41sg.gui wxpython" % _("If required, restart the GUI with:"))
    else:
        message("%-41sg.gui %s" % (_("Start the GUI with:"), default_gui))

    message("%-41sexit" % _("When ready to quit enter:"))
    message("")


def csh_startup(location, location_name, mapset, grass_env_file):
    userhome = os.getenv('HOME')      # save original home
    home = location
    os.environ['HOME'] = home

    cshrc = os.path.join(home, ".cshrc")
    tcshrc = os.path.join(home, ".tcshrc")
    try_remove(cshrc)
    try_remove(tcshrc)

    f = open(cshrc, 'w')
    f.write("set home = %s\n" % userhome)
    f.write("set history = 3000 savehist = 3000  noclobber ignoreeof\n")
    f.write("set histfile = %s\n" % os.path.join(os.getenv('HOME'),
                                                 ".history"))

    f.write("set prompt = '\\\n")
    f.write("Mapset <%s> in Location <%s> \\\n" % (mapset, location_name))
    f.write("GRASS GIS %s > '\n" % GRASS_VERSION)
    f.write("set BOGUS=``;unset BOGUS\n")

    # csh shell rc file left for backward compatibility
    path = os.path.join(userhome, ".grass.cshrc")
    if os.access(path, os.R_OK):
        f.write(readfile(path) + '\n')
    if os.access(grass_env_file, os.R_OK):
        f.write(readfile(grass_env_file) + '\n')

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
    f.write("set path = ( %s ) \n" % ' '.join(path))
    f.close()
    writefile(tcshrc, readfile(cshrc))

    process = Popen([gpath("etc", "run"), os.getenv('SHELL')])
    os.environ['HOME'] = userhome
    return process


def bash_startup(location, location_name, grass_env_file):
    # save command history in mapset dir and remember more
    os.environ['HISTFILE'] = os.path.join(location, ".bash_history")
    if not os.getenv('HISTSIZE') and not os.getenv('HISTFILESIZE'):
        os.environ['HISTSIZE'] = "3000"

    # instead of changing $HOME, start bash with:
    #   --rcfile "$LOCATION/.bashrc" ?
    #   if so, must care be taken to explicitly call .grass.bashrc et al
    #   for non-interactive bash batch jobs?
    userhome = os.getenv('HOME')      # save original home
    home = location                   # save .bashrc in $LOCATION
    os.environ['HOME'] = home

    bashrc = os.path.join(home, ".bashrc")
    try_remove(bashrc)

    f = open(bashrc, 'w')
    f.write("test -r ~/.alias && . ~/.alias\n")

    if os.getenv('ISISROOT'):
        # GRASS GIS and ISIS blend
        grass_name = "ISIS-GRASS"
    else:
        grass_name = "GRASS"
    f.write("PS1='{name} {version} ({location}):\\w > '\n".format(
        name=grass_name, version=GRASS_VERSION, location=location_name))

    # TODO: have a function and/or module to test this
    mask2d_test = 'test -f "$MAPSET_PATH/cell/MASK"'
    mask3d_test = 'test -d "$MAPSET_PATH/grid3/RASTER3D_MASK"'

    # double curly brackets means single one for format function
    # setting LOCATION for backwards compatibility
    f.write(
        """grass_prompt() {{
    MAPSET_PATH="`g.gisenv get=GISDBASE,LOCATION_NAME,MAPSET separator='/'`"
    LOCATION="$MAPSET_PATH"
    if {mask2d_test} && {mask3d_test} ; then
        echo [{both_masks}]
    elif {mask2d_test} ; then
        echo [{mask2d}]
    elif {mask3d_test} ; then
        echo [{mask3d}]
    fi
}}
PROMPT_COMMAND=grass_prompt\n""".format(
            both_masks=_("2D and 3D raster MASKs present"),
            mask2d=_("Raster MASK present"),
            mask3d=_("3D raster MASK present"),
            mask2d_test=mask2d_test, mask3d_test=mask3d_test
            ))

    # this line was moved here from below .grass.bashrc to allow ~ and $HOME in
    # .grass.bashrc
    f.write("export HOME=\"%s\"\n" % userhome)  # restore user home path

    # read other settings (aliases, ...) since environmental variables
    # have been already set by load_env(), see #3462
    for env_file in [os.path.join(userhome, ".grass.bashrc"),
                     grass_env_file]:
        if not os.access(env_file, os.R_OK):
            continue
        for line in readfile(env_file).splitlines():
            # Bug related to OS X "SIP", see
            # https://trac.osgeo.org/grass/ticket/3462#comment:13
            if MACOSX or not line.startswith('export'):
                f.write(line + '\n')

    f.write("export PATH=\"%s\"\n" % os.getenv('PATH'))
    f.close()

    process = Popen([gpath("etc", "run"), os.getenv('SHELL')])
    os.environ['HOME'] = userhome
    return process


def default_startup(location, location_name):
    if WINDOWS:
        os.environ['PS1'] = "GRASS %s> " % (GRASS_VERSION)
        # "$ETC/run" doesn't work at all???
        process = subprocess.Popen([os.getenv('SHELL')])
    else:
        os.environ['PS1'] = "GRASS %s (%s):\\w > " % (GRASS_VERSION, location_name)
        process = Popen([gpath("etc", "run"), os.getenv('SHELL')])

    return process


def done_message():
    # here was something for batch job but it was never called
    message(_("Done."))
    message("")
    message(_("Goodbye from GRASS GIS"))
    message("")


def clean_temp():
    message(_("Cleaning up temporary files..."))
    nul = open(os.devnull, 'w')
    call([gpath("etc", "clean_temp")], stdout=nul)
    nul.close()


def clean_all():
    from grass.script import setup as gsetup
    # clean default sqlite db
    gsetup.clean_default_db()
    # remove leftover temp files
    clean_temp()
    # save 'last used' GISRC after removing variables which shouldn't
    # be saved, e.g. d.mon related
    clean_env()


def grep(pattern, lines):
    """Search lines (list of strings) and return them when beginning matches.

    >>> grep("a", ['abc', 'cab', 'sdr', 'aaa', 'sss'])
    ['abc', 'aaa']
    """
    expr = re.compile(pattern)
    return [elem for elem in lines if expr.match(elem)]


def print_params():
    """Write compile flags and other configuration to stderr"""
    params = sys.argv[2:]
    if not params:
        params = ['arch', 'build', 'compiler', 'path', 'revision', 'version', 'date']

    # check if we are dealing with parameters which require dev files
    dev_params = ["arch", "compiler", "build", "date"]
    if any([param in dev_params for param in params]):
        plat = gpath('include', 'Make', 'Platform.make')
        if not os.path.exists(plat):
            fatal(_("Please install the GRASS GIS development package"))
        fileplat = open(plat)
        # this is in fact require only for some, but prepare it anyway
        linesplat = fileplat.readlines()
        fileplat.close()

    for arg in params:
        if arg == 'path':
            sys.stdout.write("%s\n" % GISBASE)
        elif arg == 'arch':
            val = grep('ARCH', linesplat)
            sys.stdout.write("%s\n" % val[0].split('=')[1].strip())
        elif arg == 'build':
            build = gpath('include', 'grass', 'confparms.h')
            filebuild = open(build)
            val = filebuild.readline()
            filebuild.close()
            sys.stdout.write("%s\n" % val.strip().strip('"').strip())
        elif arg == 'compiler':
            val = grep('CC', linesplat)
            sys.stdout.write("%s\n" % val[0].split('=')[1].strip())
        elif arg == 'revision':
            sys.stdout.write("@GRASS_VERSION_GIT@\n")
        elif arg == 'svn_revision':
            filerev = open(gpath('etc', 'VERSIONNUMBER'))
            linerev = filerev.readline().rstrip('\n')
            filerev.close()
            try:
               revision = linerev.split(' ')[1]
               sys.stdout.write("%s\n" % revision[1:])
            except:
               sys.stdout.write("No SVN revision defined\n")
        elif arg == 'version':
            sys.stdout.write("%s\n" % GRASS_VERSION)
        elif arg == 'date':
            date_str = "#define GRASS_HEADERS_DATE "
            gdate = gpath('include', 'grass', 'version.h')
            with open(gdate) as filegdate:
                for line in filegdate.readlines():
                    if line.startswith(date_str):
                        sys.stdout.write('{}\n'.format(
                            line.replace(date_str, '').lstrip()[1:-2]) # remove quotes
                        )
                        break
        else:
            message(_("Parameter <%s> not supported") % arg)


def get_username():
    """Get name of the current user"""
    if WINDOWS:
        user = os.getenv('USERNAME')
        if not user:
            user = "user_name"
    else:
        user = os.getenv('USER')
        if not user:
            user = os.getenv('LOGNAME')
        if not user:
            try:
                p = Popen(['whoami'], stdout=subprocess.PIPE)
                s = p.stdout.read()
                p.wait()
                user = s.strip()
                if type(user) is bytes:
                    user = decode(user)
            except:
                pass
        if not user:
            user = "user_%d" % os.getuid()
    return user


class Parameters(object):
    """Structure to hold standard part of command line parameters"""
    # we don't need to define any methods
    # pylint: disable=R0903
    def __init__(self):
        self.grass_gui = None
        self.create_new = None
        self.exit_grass = None
        self.force_gislock_removal = None
        self.mapset = None
        self.geofile = None
        self.tmp_location = False


def parse_cmdline(argv, default_gui):
    """Parse the standard part of command line parameters"""
    params = Parameters()
    args = []
    for i in argv:
        # Check if the user asked for the version
        if i in ["-v", "--version"]:
            message("GRASS GIS %s" % GRASS_VERSION)
            message('\n' + readfile(gpath("etc", "license")))
            sys.exit()
        # Check if the user asked for help
        elif i in ["help", "-h", "-help", "--help", "--h"]:
            help_message(default_gui=default_gui)
            sys.exit()
        # Check if the --text flag was given
        elif i in ["-text", "--text"]:
            params.grass_gui = 'text'
        # Check if the --gtext flag was given
        elif i in ["-gtext", "--gtext"]:
            params.grass_gui = 'gtext'
        # Check if the --gui flag was given
        elif i in ["-gui", "--gui"]:
            params.grass_gui = default_gui
        # Check if the -wxpython flag was given
        elif i in ["-wxpython", "-wx", "--wxpython", "--wx"]:
            params.grass_gui = 'wxpython'
        # Check if the user wants to create a new mapset
        elif i == "-c":
            params.create_new = True
        elif i == "-e":
            params.exit_grass = True
        elif i == "-f":
            params.force_gislock_removal = True
        elif i == "--config":
            print_params()
            sys.exit()
        elif i == "--tmp-location":
            params.tmp_location = True
        else:
            args.append(i)
    if len(args) > 1:
        params.mapset = args[1]
        params.geofile = args[0]
    elif len(args) == 1:
        if params.tmp_location:
            params.geofile = args[0]
        else:
            params.mapset = args[0]
    else:
        params.mapset = None
    return params


def validate_cmdline(params):
    """ Validate the cmdline params and exit if necessary. """
    if params.exit_grass and not params.create_new:
        fatal(_("Flag -e requires also flag -c"))
    if params.tmp_location and not params.geofile:
        fatal(
            _(
                "Coordinate reference system argument (e.g. EPSG)"
                " is needed for --tmp-location"
            )
        )
    if params.tmp_location and params.mapset:
        fatal(
            _(
                "Only one argument (e.g. EPSG) is needed for"
                " --tmp-location, mapset name <{}> provided"
            ).format(params.mapset)
        )


def main():
    """The main function which does the whole setup and run procedure

    Only few things are set on the module level.
    """
    # Set language
    # This has to be called before any _() function call!
    # Subsequent functions are using _() calls and
    # thus must be called only after Language has been set.
    grass_config_dir = get_grass_config_dir()
    set_language(grass_config_dir)

    # Set default GUI
    default_gui = "wxpython"

    # explain what is happening in debug mode (visible only in debug mode)
    debug("GRASS_DEBUG environmental variable is set. It is meant to be"
          " an internal variable for debugging only this script.\n"
          " Use 'g.gisenv set=\"DEBUG=[0-5]\"'"
          " to turn GRASS GIS debug mode on if you wish to do so.")

    # Set GRASS version number for R interface etc
    # (must be an env var for MS Windows)
    os.environ['GRASS_VERSION'] = GRASS_VERSION

    # Set the GIS_LOCK variable to current process id
    gis_lock = str(os.getpid())
    os.environ['GIS_LOCK'] = gis_lock

    batch_job = get_batch_job_from_env_variable()

    # Parse the command-line options and set several global variables
    batch_exec_param = '--exec'
    try:
        # raises ValueError when not found
        index = sys.argv.index(batch_exec_param)
        batch_job = sys.argv[index + 1:]
        clean_argv = sys.argv[1:index]
        params = parse_cmdline(clean_argv, default_gui=default_gui)
    except ValueError:
        params = parse_cmdline(sys.argv[1:], default_gui=default_gui)
    validate_cmdline(params)
    # For now, we allow, but not advertise/document, --tmp-location
    # without --exec (usefulness to be evaluated).

    grass_gui = params.grass_gui  # put it to variable, it is used a lot

    # TODO: with --tmp-location there is no point in loading settings
    # i.e. rc file from home dir, but the code is too spread out
    # to disable it at this point
    gisrcrc = get_gisrc_from_config_dir(grass_config_dir, batch_job)

    # Set the username
    user = get_username()


    # Set shell (needs to be called before load_env())
    sh, shellname = get_shell()
    grass_env_file = get_grass_env_file(sh, grass_config_dir)

    # Load environmental variables from the file (needs to be called
    # before create_tmp())
    load_env(grass_env_file)

    # Create the temporary directory and session grassrc file
    tmpdir = create_tmp(user, gis_lock)

    # Remove the tmpdir
    # The removal will be executed when the python process terminates.
    atexit.register(lambda: shutil.rmtree(tmpdir, ignore_errors=True))

    # Create the session grassrc file
    gisrc = create_gisrc(tmpdir, gisrcrc)

    ensure_home()
    # Set PATH, PYTHONPATH, ...
    set_paths(grass_config_dir=grass_config_dir)
    # Set GRASS_PAGER, GRASS_PYTHON, GRASS_GNUPLOT, GRASS_PROJSHARE
    set_defaults()
    set_display_defaults()
    # Set GRASS_HTML_BROWSER
    set_browser()

    # First time user - GISRC is defined in the GRASS script
    if not os.access(gisrc, os.F_OK):
        if grass_gui == 'text' and not params.mapset:
            fatal(_("Unable to start GRASS GIS. You have the choice to:\n"
                    " - Launch the graphical user interface with"
                    " the '--gui' switch\n"
                    "     {cmd_name} --gui\n"
                    " - Launch with path to "
                    "the location/mapset as an argument\n"
                    "     {cmd_name} /path/to/location/mapset`\n"
                    " - Create a location with '-c' and launch in its"
                    " PERMANENT mapset\n"
                    "     {cmd_name} -c EPSG:number /path/to/location\n"
                    "     {cmd_name} -c geofile /path/to/location\n"
                    " - Create manually the GISRC file ({gisrcrc})\n"
                    " - Use '--help' for further options\n"
                    "     {cmd_name} --help\n"
                    "See also: https://grass.osgeo.org/{cmd_name}/manuals/helptext.html").format(
                        cmd_name=CMD_NAME, gisrcrc=gisrcrc))
        create_initial_gisrc(gisrc)

    message(_("Starting GRASS GIS..."))

    # Ensure GUI is set
    if batch_job or params.exit_grass:
        grass_gui = 'text'
    else:
        if not grass_gui:
            # if GUI was not set previously (e.g. command line),
            # get it from rc file or env variable
            grass_gui = read_gui(gisrc, default_gui)
        # check that the GUI works but only if not doing a batch job
        grass_gui = check_gui(expected_gui=grass_gui)
        # save GUI only if we are not doibg batch job
        save_gui(gisrc, grass_gui)

    # Parsing argument to get LOCATION
    if not params.mapset and not params.tmp_location:
        # Try interactive startup
        # User selects LOCATION and MAPSET if not set
        if not set_mapset_interactive(grass_gui):
            # No GUI available, update gisrc file
            fatal(_("<{0}> requested, but not available. Run GRASS in text "
                    "mode (--text) or install missing package (usually "
                    "'grass-gui').").format(grass_gui))
    else:
        # Try non-interactive start up
        if params.tmp_location:
            # tmp loc requires other things to be set as well
            set_mapset(gisrc=gisrc, geofile=params.geofile,
                       create_new=True,
                       tmp_location=params.tmp_location, tmpdir=tmpdir)
        elif params.create_new and params.geofile:
            set_mapset(gisrc=gisrc, arg=params.mapset,
                       geofile=params.geofile, create_new=True)
        else:
            set_mapset(gisrc=gisrc, arg=params.mapset,
                       create_new=params.create_new)

    # Set GISDBASE, LOCATION_NAME, MAPSET, LOCATION from $GISRC
    # e.g. wxGUI startup screen writes to the gisrc file,
    # so loading it is the only universal way to obtain the values
    # this suppose that both programs share the right path to gisrc file
    # TODO: perhaps gisrcrc should be removed from here
    # alternatively, we can check validity here with all the info we have
    # about what was used to create gisrc
    mapset_settings = load_gisrc(gisrc, gisrcrc=gisrcrc)

    location = mapset_settings.full_mapset

    # check and create .gislock file
    lock_mapset(mapset_settings.full_mapset, user=user,
                force_gislock_removal=params.force_gislock_removal,
                grass_gui=grass_gui)
    # unlock the mapset which is current at the time of turning off
    # in case mapset was changed
    atexit.register(lambda: unlock_gisrc_mapset(gisrc, gisrcrc))
    # We now own the mapset (set and lock), so we can clean temporary
    # files which previous session may have left behind. We do it even
    # for first time user because the cost is low and first time user
    # doesn't necessarily mean that the mapset is used for the first time.
    clean_temp()

    # build user fontcap if specified but not present
    make_fontcap()

    # TODO: is this really needed? Modules should call this when/if required.
    ensure_db_connected(location)

    # Display the version and license info
    # only non-error, interactive version continues from here
    if batch_job:
        returncode = run_batch_job(batch_job)
        clean_all()
        sys.exit(returncode)
    elif params.exit_grass:
        # clean always at exit, cleans whatever is current mapset based on
        # the GISRC env variable
        clean_all()
        sys.exit(0)
    else:
        show_banner()
        say_hello()
        show_info(shellname=shellname,
                  grass_gui=grass_gui, default_gui=default_gui)
        if grass_gui == "wxpython":
            message(_("Launching <%s> GUI in the background, please wait...")
                    % grass_gui)
        if sh in ['csh', 'tcsh']:
            shell_process = csh_startup(mapset_settings.full_mapset,
                                        mapset_settings.location,
                                        mapset_settings.mapset,
                                        grass_env_file)
        elif sh in ['bash', 'msh', 'cygwin']:
            shell_process = bash_startup(mapset_settings.full_mapset,
                                         mapset_settings.location,
                                         grass_env_file)
        else:
            shell_process = default_startup(mapset_settings.full_mapset,
                                            mapset_settings.location)

        # start GUI and register shell PID in rc file
        start_gui(grass_gui)
        kv = read_gisrc(gisrc)
        kv['PID'] = str(shell_process.pid)
        write_gisrc(kv, gisrc)
        exit_val = shell_process.wait()
        if exit_val != 0:
            warning(_("Failed to start shell '%s'") % os.getenv('SHELL'))

        # close GUI if running
        close_gui()

        # here we are at the end of grass session
        clean_all()
        if not params.tmp_location:
            writefile(gisrcrc, readfile(gisrc))
        # After this point no more grass modules may be called
        # done message at last: no atexit.register()
        # or register done_message()
        done_message()

if __name__ == '__main__':
    main()
