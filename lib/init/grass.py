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
# COPYRIGHT:    (C) 2000-2025 by the GRASS Development Team
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

from __future__ import annotations

import argparse
import atexit
import datetime
import errno
import gettext
import json
import locale
import os
import platform
import re
import shutil
import signal
import string
import subprocess
import sys
import tempfile
import unicodedata
import uuid

from pathlib import Path
from typing import TYPE_CHECKING, Literal, NoReturn

if TYPE_CHECKING:
    from _typeshed import StrPath

# mechanism meant for debugging this script (only)
# private global to store if we are debugging
_DEBUG = None

# for wxpath
_WXPYTHON_BASE = None

try:
    # Python >= 3.11
    ENCODING = locale.getencoding()
except AttributeError:
    ENCODING = locale.getdefaultlocale()[1]
if ENCODING is None:
    ENCODING = "UTF-8"
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
# - we get GISBASE from os.environ if it is defined (doesn't this mean that we are
#   already inside a GRASS session? If we are, why do we need to run this script
#   again???).
# - GISBASE exists as an ENV variable
#
# pmav99: Ugly as hell, but that's what the code before the refactoring was doing.
if "GISBASE" in os.environ and len(os.getenv("GISBASE")) > 0:
    GISBASE = os.path.normpath(os.environ["GISBASE"])
else:
    GISBASE = os.path.normpath("@GISBASE_INSTALL_PATH@")
    os.environ["GISBASE"] = GISBASE
CMD_NAME = "@START_UP@"
GRASS_VERSION = "@GRASS_VERSION_NUMBER@"
GRASS_VERSION_MAJOR = "@GRASS_VERSION_MAJOR@"
GRASS_VERSION_MINOR = "@GRASS_VERSION_MINOR@"
LD_LIBRARY_PATH_VAR = "@LD_LIBRARY_PATH_VAR@"
CONFIG_PROJSHARE = os.environ.get("GRASS_PROJSHARE", "@CONFIG_PROJSHARE@")

# Get the system name
WINDOWS = sys.platform.startswith("win")
CYGWIN = sys.platform.startswith("cygwin")
MACOS = sys.platform.startswith("darwin")


def try_remove(path: StrPath) -> None:
    try:
        os.remove(path)
    except:  # noqa: E722
        pass


def clean_env() -> None:
    gisrc = os.environ["GISRC"]
    env_curr = read_gisrc(gisrc)
    env_new = {}
    for k, v in env_curr.items():
        if k.endswith("PID") or k.startswith("MONITOR"):
            continue
        env_new[k] = v
    write_gisrc(env_new, gisrc)


def is_debug() -> bool:
    """Returns True if we are in debug mode

    For debug messages use ``debug()``.
    """
    global _DEBUG
    if _DEBUG is not None:
        return _DEBUG
    # translate to bool (no or empty variable means false)
    return bool(os.getenv("GRASS_DEBUG"))


def debug(msg: str) -> None:
    """Print a debug message if in debug mode

    Do not use translatable strings for debug messages.
    """
    if is_debug():
        sys.stderr.write("DEBUG: %s\n" % msg)
        sys.stderr.flush()


def message(msg: str) -> None:
    sys.stderr.write(msg + "\n")
    sys.stderr.flush()


def warning(text: str) -> None:
    sys.stderr.write(_("WARNING") + ": " + text + os.linesep)


def fatal(msg: str) -> NoReturn:
    sys.stderr.write("%s: " % _("ERROR") + msg + os.linesep)
    sys.exit(_("Exiting..."))


def readfile(path: StrPath) -> str:
    debug("Reading %s" % path)
    return Path(path).read_text()


def writefile(path: StrPath, s: str) -> None:
    debug("Writing %s" % path)
    Path(path).write_text(s)


def call(cmd, **kwargs):
    """Wrapper for subprocess.call to deal with platform-specific issues"""
    if WINDOWS:
        kwargs["shell"] = True
    return subprocess.call(cmd, **kwargs)


def Popen(cmd, **kwargs):  # pylint: disable=C0103
    """Wrapper for subprocess.Popen to deal with platform-specific issues"""
    if WINDOWS:
        kwargs["shell"] = True
    return subprocess.Popen(cmd, **kwargs)


def gpath(*args) -> str:
    """Construct path to file or directory in GRASS GIS installation

    Can be called only after GISBASE was set.
    """
    return os.path.join(GISBASE, *args)


def wxpath(*args) -> str:
    """Construct path to file or directory in GRASS wxGUI

    Can be called only after GISBASE was set.

    This function does not check if the directories exist or if GUI works
    this must be done by the caller if needed.
    """
    global _WXPYTHON_BASE
    if not _WXPYTHON_BASE:
        # this can be called only after GISBASE was set
        _WXPYTHON_BASE = gpath("gui", "wxpython")
    return os.path.join(_WXPYTHON_BASE, *args)


def count_wide_chars(s: str) -> int:
    """Returns the number of wide CJK characters in a string.

    :param str s: string
    """
    return sum(unicodedata.east_asian_width(c) in "WF" for c in s)


def fw(fmt: str, *args) -> str:
    """Adjusts fixed-width string specifiers for wide CJK characters and
    returns a formatted string. Does not support named arguments yet.

    :param fmt: format string
    :param *args: arguments for the format string
    """
    matches = []
    # https://docs.python.org/3/library/stdtypes.html#old-string-formatting
    for m in re.finditer(
        r"%([#0 +-]*)([0-9]*)(\.[0-9]*)?([hlL]?[diouxXeEfFgGcrsa%])", fmt
    ):
        matches.append(m)

    if len(matches) != len(args):
        msg = "The numbers of format specifiers and arguments do not match"
        raise Exception(msg)

    i = len(args) - 1
    for m in reversed(matches):
        f = m.group(1)
        w = m.group(2)
        p = m.group(3) or ""
        c = m.group(4)
        if c == "s" and w:
            w = str(int(w) - count_wide_chars(args[i]))
            fmt = "".join((fmt[: m.start()], "%", f, w, p, c, fmt[m.end() :]))
        i -= 1
    return fmt % args


# using format for most but leaving usage of template for the dynamic ones
# two different methods are easy way to implement two phase construction
HELP_TEXT = r"""GRASS GIS $VERSION_NUMBER
Geographic Resources Analysis Support System (GRASS GIS).

{usage}:
  $CMD_NAME [-h | --help] [-v | --version]
          [-c | -c geofile | -c EPSG:code[:datum_trans] | -c XY]
          [-e] [-f] [--text | --gtext | --gui] [--config param]
          [[[GISDBASE/]PROJECT/]MAPSET]
  $CMD_NAME [FLAG]... GISDBASE/PROJECT/MAPSET --exec EXECUTABLE [EPARAM]...
  $CMD_NAME --tmp-project [geofile | EPSG | XY] --exec EXECUTABLE [EPARAM]...
  $CMD_NAME --tmp-mapset GISDBASE/PROJECT/ --exec EXECUTABLE [EPARAM]...

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
  --tmp-project                  {tmp_project}
                                   {tmp_project_detail}
  --tmp-location                 {tmp_location}
                                   {tmp_location_detail}
  --tmp-mapset                   {tmp_mapset}
                                   {tmp_mapset_detail}

{params}:
  GISDBASE                       {gisdbase}
                                   {gisdbase_detail}
  PROJECT                        {project}
                                   {project_detail}
  MAPSET                         {mapset}

  GISDBASE/PROJECT/MAPSET        {full_mapset}

  EXECUTABLE                     {executable}
  EPARAM                         {executable_params}
  FLAG                           {standard_flags}

{env_vars}:
  GRASS_CONFIG_DIR               {config_dir_var}
  GRASS_GUI                      {gui_var}
  GRASS_HTML_BROWSER             {html_var}
  GRASS_ADDON_PATH               {addon_path_var}
  GRASS_ADDON_BASE               {addon_base_var}
  GRASS_PYTHON                   {python_var}
"""


def help_message(default_gui) -> None:
    t = string.Template(
        HELP_TEXT.format(
            usage=_("Usage"),
            flags=_("Flags"),
            help_flag=_("print this help message"),
            version_flag=_("show version information and exit"),
            create=_("create given database, project or mapset if it doesn't exist"),
            exit_after=_("exit after creation of project or mapset. Only with -c flag"),
            force_removal=_(
                "force removal of .gislock if exists (use with care!)."
                " Only with --text flag"
            ),
            text=_("use text based interface (skip graphical welcome screen)"),
            text_detail=_("and set as default"),
            gtext=_("use text based interface (show graphical welcome screen)"),
            gtext_detail=_("and set as default"),
            gui=_("use $DEFAULT_GUI graphical user interface"),
            gui_detail=_("and set as default"),
            config=_("print GRASS configuration parameters"),
            config_detail=_(
                "options: arch,build,compiler,date,path,python_path,revision,"
                "svn_revision,version"
            ),
            params=_("Parameters"),
            gisdbase=_("initial GRASS database directory"),
            gisdbase_detail=_("directory containing GRASS projects"),
            project=_("initial GRASS project (location)"),
            project_detail=_(
                "directory containing mapsets with one common"
                " coordinate reference system"
            ),
            mapset=_("initial GRASS mapset"),
            full_mapset=_("fully qualified initial mapset directory"),
            env_vars=_("Environment variables relevant for startup"),
            config_dir_var=_("set root path for configuration directory"),
            gui_var=_("select GUI (text, gui, gtext)"),
            html_var=_("set html web browser for help pages"),
            addon_path_var=_(
                "set additional path(s) to local GRASS modules or user scripts"
            ),
            addon_base_var=_(
                "set additional GISBASE for locally installed GRASS Addons"
            ),
            python_var=_("set Python interpreter name to override 'python'"),
            exec_=_("execute GRASS module or script"),
            exec_detail=_("provided executable will be executed in GRASS session"),
            executable=_("GRASS module, script or any other executable"),
            executable_params=_("parameters of the executable"),
            standard_flags=_("standard flags"),
            tmp_location=_("deprecated, use --tmp-project instead"),
            tmp_location_detail=_("location was renamed to project"),
            tmp_project=_("create temporary project (use with the --exec flag)"),
            tmp_project_detail=_(
                "created in a temporary directory and deleted at exit"
            ),
            tmp_mapset=_("create temporary mapset (use with the --exec flag)"),
            tmp_mapset_detail=_("created in the specified project and deleted at exit"),
        )
    )
    s = t.substitute(
        CMD_NAME=CMD_NAME, DEFAULT_GUI=default_gui, VERSION_NUMBER=GRASS_VERSION
    )
    sys.stderr.write(s)


def create_grass_config_dir() -> str:
    """Create configuration directory

    Determines path of GRASS GIS user configuration directory and creates
    it if it does not exist.

    Configuration directory is for example used for grass env file
    (the one which caries mapset settings from session to session).
    """
    from grass.app.runtime import get_grass_config_dir

    try:
        directory = get_grass_config_dir(
            GRASS_VERSION_MAJOR, GRASS_VERSION_MINOR, os.environ
        )
    except (RuntimeError, NotADirectoryError) as e:
        fatal(f"{e}")

    if not os.path.isdir(directory):
        try:
            os.makedirs(directory)
        except OSError as e:
            # Can happen as a race condition
            if e.errno != errno.EEXIST or not os.path.isdir(directory):
                fatal(
                    _(
                        "Failed to create configuration directory '{}' with error: {}"
                    ).format(directory, e.strerror)
                )
    return directory


def create_tmp(user, gis_lock) -> str:
    """Create temporary directory

    :param user: user name to be used in the directory name
    :param gis_lock: session lock filename to be used in the directory name
    """
    # use $TMPDIR if it exists, then $TEMP, otherwise /tmp
    tmp = os.getenv("TMPDIR")
    if not tmp:
        tmp = os.getenv("TEMP")
    if not tmp:
        tmp = os.getenv("TMP")
    if not tmp:
        tmp = tempfile.gettempdir()

    tmpdir_name = f"grass{GRASS_VERSION_MAJOR}-{user}-{gis_lock}"
    if tmp:
        tmpdir = os.path.join(tmp, tmpdir_name)
        try:
            os.mkdir(tmpdir, 0o700)
        except:  # noqa: E722
            tmp = None

    if not tmp:
        for ttmp in ("/tmp", "/var/tmp", "/usr/tmp"):
            tmp = ttmp
            tmpdir = os.path.join(tmp, tmpdir_name)
            try:
                os.mkdir(tmpdir, 0o700)
            except:  # noqa: E722
                tmp = None
            if tmp:
                break

    if not tmp:
        fatal(
            _("Unable to create temporary directory <{tmpdir_name}>! Exiting.").format(
                tmpdir_name=tmpdir_name
            )
        )

    # promoting the variable even if it was not defined before
    os.environ["TMPDIR"] = tmpdir

    debug(
        "Tmp directory '{tmpdir}' created for user '{user}'".format(
            tmpdir=tmpdir, user=user
        )
    )
    return tmpdir


def get_gisrc_from_config_dir(grass_config_dir, batch_job) -> str:
    """Set the global grassrc file (aka grassrcrc)"""
    if batch_job:
        # TODO: This is probably not needed since batch job does write to config.
        # use individual GISRCRC files when in batch mode (r33174)
        filename = os.path.join(grass_config_dir, "rc.%s" % platform.node())
        if os.access(filename, os.R_OK):
            return filename
    # use standard file if in normal mode or the special file is not available
    return os.path.join(grass_config_dir, "rc")


def create_gisrc(tmpdir: StrPath, gisrcrc: StrPath) -> str:
    # Set the session grassrc file
    gisrc = os.path.join(tmpdir, "gisrc")
    os.environ["GISRC"] = gisrc

    # remove invalid GISRC file to avoid disturbing error messages:
    try:
        s = readfile(gisrcrc)
        if "UNKNOWN" in s:
            try_remove(gisrcrc)
            s = None
    except Exception:
        s = None

    # Copy the global grassrc file to the session grassrc file
    if s:
        writefile(gisrc, s)
    return gisrc


def read_gisrc(filename: StrPath):
    kv = {}
    try:
        with open(filename) as f:
            for line in f:
                try:
                    k, v = line.split(":", 1)
                except ValueError as e:
                    warning(
                        _(
                            "Invalid line in RC file ({file}): '{line}' ({error})\n"
                        ).format(line=line, error=e, file=filename)
                    )
                    continue
                kv[k.strip()] = v.strip()
            if not kv:
                warning(_("Empty RC file ({file})").format(file=filename))

    except OSError:
        return kv

    return kv


def write_gisrcrc(gisrcrc: StrPath, gisrc: StrPath, skip_variable=None) -> None:
    """Reads gisrc file and write to gisrcrc"""
    debug("Reading %s" % gisrc)
    number = 0
    with open(gisrc) as f:
        lines = f.readlines()
        for line in lines:
            if skip_variable in line:
                del lines[number]
            number += 1
    with open(gisrcrc, "w") as f:
        f.writelines(lines)


def read_env_file(path: StrPath) -> dict[str, str]:
    kv: dict[str, str] = {}
    with open(path) as f:
        for line in f:
            k, v = line.split(":", 1)
            kv[k.strip()] = v.strip()
    return kv


def write_gisrc(kv, filename: StrPath, append=False) -> None:
    # use append=True to avoid a race condition between write_gisrc() and
    # grass_prompt() on startup (PR #548)
    with open(filename, "a" if append else "w") as f:
        f.writelines("%s: %s\n" % (k, v) for k, v in kv.items())


def add_mapset_to_gisrc(gisrc: StrPath, grassdb, location, mapset) -> None:
    kv = read_gisrc(gisrc) if os.access(gisrc, os.R_OK) else {}
    kv["GISDBASE"] = grassdb
    kv["LOCATION_NAME"] = location
    kv["MAPSET"] = mapset
    write_gisrc(kv, gisrc)


def add_last_mapset_to_gisrc(gisrc: StrPath, last_mapset_path) -> None:
    kv = read_gisrc(gisrc) if os.access(gisrc, os.R_OK) else {}
    kv["LAST_MAPSET_PATH"] = last_mapset_path
    write_gisrc(kv, gisrc)


def create_fallback_session(gisrc: StrPath, tmpdir) -> None:
    """Creates fallback temporary session"""
    # Create temporary location
    set_mapset(
        gisrc=gisrc, geofile="XY", create_new=True, tmp_location=True, tmpdir=tmpdir
    )


def read_gui(gisrc: StrPath, default_gui):
    grass_gui = None
    # At this point the GRASS user interface variable has been set from the
    # command line, been set from an external environment variable,
    # or is not set. So we check if it is not set
    # Check for a reference to the GRASS user interface in the grassrc file
    if os.access(gisrc, os.R_OK):
        kv = read_gisrc(gisrc)
        if "GRASS_GUI" in os.environ:
            grass_gui = os.environ["GRASS_GUI"]
        elif "GUI" in kv:
            grass_gui = kv["GUI"]
        elif "GRASS_GUI" in kv:
            # For backward compatibility (GRASS_GUI renamed to GUI)
            grass_gui = kv["GRASS_GUI"]
        else:
            # Set the GRASS user interface to the default if needed
            grass_gui = default_gui

    if not grass_gui:
        grass_gui = default_gui

    if grass_gui == "gui":
        grass_gui = default_gui

    # FIXME oldtcltk, gis.m, d.m no longer exist (remove this around 7.2)
    if grass_gui in {"d.m", "gis.m", "oldtcltk", "tcltk"}:
        warning(_("GUI <%s> not supported in this version") % grass_gui)
        grass_gui = default_gui

    return grass_gui


def create_initial_gisrc(filename: StrPath) -> None:
    # for convenience, define GISDBASE as pwd:
    s = r"""GISDBASE: %s
LOCATION_NAME: <UNKNOWN>
MAPSET: <UNKNOWN>
""" % Path.cwd()
    writefile(filename, s)


def check_gui(expected_gui):
    grass_gui = expected_gui
    # Check if we are running X windows by checking the DISPLAY variable
    if os.getenv("DISPLAY") or WINDOWS or MACOS:
        # Check if python is working properly
        if expected_gui in {"wxpython", "gtext"}:
            nul = open(os.devnull, "w")
            p = Popen(
                [os.environ["GRASS_PYTHON"]],
                stdin=subprocess.PIPE,
                stdout=nul,
                stderr=nul,
            )
            nul.close()
            p.stdin.write("variable=True".encode(ENCODING))
            p.stdin.close()
            p.wait()
            msg = None
            if p.returncode != 0:
                # Python was not found - switch to text interface mode
                msg = _(
                    "The python command does not work as expected.\n"
                    "Please check your installation or set the GRASS_PYTHON"
                    " environment variable."
                )
            if not os.path.exists(wxpath("wxgui.py")):
                msg = _("GRASS GUI not found. Please check your installation.")
            if msg:
                warning(_("{}\nSwitching to text based interface mode.").format(msg))
                grass_gui = "text"
    # Display a message if a graphical interface was expected
    elif expected_gui != "text":
        # Set the interface mode to text
        warning(
            _(
                "It appears that the X Windows system is not active.\n"
                "A graphical based user interface is not supported.\n"
                "(DISPLAY variable is not set.)\n"
                "Switching to text based interface mode."
            )
        )
        grass_gui = "text"
    return grass_gui


def save_gui(gisrc: StrPath, grass_gui) -> None:
    """Save the user interface variable in the grassrc file"""
    if os.access(gisrc, os.F_OK):
        kv = read_gisrc(gisrc)
        kv["GUI"] = grass_gui
        write_gisrc(kv, gisrc)


def create_location(gisdbase, location, geostring) -> None:
    """Create GRASS Location using georeferenced file or EPSG

    EPSG code format is ``EPSG:code`` or ``EPSG:code:datum_trans``.

    :param gisdbase: Path to GRASS GIS database directory
    :param location: name of new Location
    :param geostring: path to a georeferenced file or EPSG code
    """
    if gpath("etc", "python") not in sys.path:
        sys.path.append(gpath("etc", "python"))
    from grass.script import core as gcore  # pylint: disable=E0611

    try:
        if geostring and geostring.upper().find("EPSG:") > -1:
            # create location using EPSG code
            epsg = geostring.split(":", 1)[1]
            if ":" in epsg:
                epsg, datum_trans = epsg.split(":", 1)
            else:
                datum_trans = None
            gcore.create_location(
                gisdbase, location, epsg=epsg, datum_trans=datum_trans
            )
        elif geostring == "XY":
            # create an XY location
            gcore.create_location(gisdbase, location)
        else:
            # create location using georeferenced file
            gcore.create_location(gisdbase, location, filename=geostring)
    except gcore.ScriptError as err:
        fatal(err.value.strip('"').strip("'").replace("\\n", os.linesep))


def can_create_location(gisdbase: StrPath, location) -> bool:
    """Checks if location can be created"""
    path = os.path.join(gisdbase, location)
    return not os.path.exists(path)


def cannot_create_location_reason(gisdbase: StrPath, location: str) -> str:
    """Returns a message describing why location cannot be created

    The goal is to provide the most suitable error message
    (rather than to do a quick check).

    :param gisdbase: Path to GRASS GIS database directory
    :param location: name of a Location
    :returns: translated message
    """
    from grass.grassdb.checks import is_location_valid

    path = os.path.join(gisdbase, location)
    if is_location_valid(gisdbase, location):
        return _(
            "Unable to create new project because"
            " the project <{location}>"
            " already exists."
        ).format(**locals())
    if os.path.isfile(path):
        return _(
            "Unable to create new project <{location}> because <{path}> is a file."
        ).format(**locals())
    if os.path.isdir(path):
        return _(
            "Unable to create new project <{location}> because"
            " the directory <{path}>"
            " already exists."
        ).format(**locals())
    return _(
        "Unable to create new project in the directory <{path}> for an unknown reason."
    ).format(**locals())


def set_mapset(
    gisrc: StrPath,
    arg=None,
    geofile=None,
    create_new=False,
    tmp_location=False,
    tmp_mapset=False,
    tmpdir: StrPath | None = None,
):
    """Selected Location and Mapset are checked and created if requested

    The gisrc (GRASS environment file) is written at the end
    (nothing is returned).

    tmp_location requires tmpdir (which is used as gisdbase)
    """
    from grass.grassdb.checks import (
        get_location_invalid_reason,
        get_location_invalid_suggestion,
        get_mapset_invalid_reason,
        is_location_valid,
        is_mapset_valid,
        mapset_exists,
    )

    # TODO: arg param seems to be always the mapset parameter (or a dash
    # in a distant past), refactor
    if arg:
        # TODO: the block below could be just one line: os.path.abspath(l)
        # abspath both resolves relative paths and normalizes the path
        # so that trailing / is stripped away and split then always returns
        # non-empty element as the last element (which is good for both mapset
        # and location split)
        if arg == ".":
            arg = str(Path.cwd())
        elif not os.path.isabs(arg):
            arg = os.path.abspath(arg)
        if arg.endswith(os.path.sep):
            arg = arg.rstrip(os.path.sep)
            # now we can get the last element by split on the first go
            # and it works for the last element being mapset or location

        if tmp_mapset:
            # We generate a random name and then create the mapset as usual.
            mapset = "tmp_" + uuid.uuid4().hex
            create_new = True
        else:
            arg, mapset = os.path.split(arg)
        arg, location_name = os.path.split(arg)
        gisdbase = arg

    # all was None for tmp loc so that case goes here quickly
    # TODO: but the above code needs review anyway
    if tmp_location:
        # set gisdbase to temporary directory
        gisdbase = tmpdir
        # we are already in a unique directory, so we can use fixed name
        location_name = "tmpproject"
        # we need only one mapset
        mapset = "PERMANENT"
        debug(
            "Using temporary project <{gdb}{sep}{lc}>".format(
                gdb=gisdbase, lc=location_name, sep=os.path.sep
            )
        )

    if gisdbase and location_name and mapset:
        path = os.path.join(gisdbase, location_name, mapset)
        # check if 'path' is a valid GRASS location/mapset
        path_is_valid_mapset = is_mapset_valid(path)

        if path_is_valid_mapset and tmp_mapset:
            # If we would be creating the mapset directory at the same time as
            # generating the name, we could just try another name in case of
            # conflict. Conflict is unlikely, but it would be worth considering
            # it during refactoring of this code.
            fatal(
                _(
                    "Mapset <{}> already exists."
                    " Unable to create a new temporary mapset of that name."
                ).format(path)
            )
        elif path_is_valid_mapset and create_new:
            fatal(_("Mapset <{}> already exists.").format(path))

        if not path_is_valid_mapset:
            if not create_new:
                # 'path' is not a valid mapset and user does not
                # want to create anything new
                reason = get_mapset_invalid_reason(gisdbase, location_name, mapset)
                if not mapset_exists(gisdbase, location_name, mapset):
                    suggestion = _("A new mapset can be created using '-c' flag.")
                else:
                    suggestion = _("Maybe you meant a different directory.")
                fatal("{reason}\n{suggestion}".format(**locals()))
            # 'path' is not valid and the user wants to create
            # mapset on the fly
            # check if 'location_name' is a valid GRASS location
            elif not is_location_valid(gisdbase, location_name):
                if not (tmp_location or tmp_mapset):
                    # 'location_name' is not a valid GRASS location
                    # and user requested its creation, so we parsed
                    # the path wrong and need to move one level
                    # and use 'PERMANENT' mapset
                    # (we already got that right in case of tmploc)
                    gisdbase = os.path.join(gisdbase, location_name)
                    location_name = mapset
                    mapset = "PERMANENT"
                if tmp_mapset:
                    suggestion = get_location_invalid_suggestion(
                        gisdbase, location_name
                    )
                    reason = get_location_invalid_reason(gisdbase, location_name)
                    if suggestion:
                        fatal("{reason}\n{suggestion}".format(**locals()))
                    else:
                        fatal(reason)
                if not can_create_location(gisdbase, location_name):
                    fatal(cannot_create_location_reason(gisdbase, location_name))
                # create new location based on the provided EPSG/...
                if not geofile:
                    fatal(_("Provide CRS to create a project"))
                if not tmp_location:
                    # Report report only when new location is not temporary.
                    message(
                        _("Creating new GRASS GIS project <{}>...").format(
                            location_name
                        )
                    )
                create_location(gisdbase, location_name, geofile)
            else:
                # 'location_name' is a valid GRASS location,
                # create new mapset
                if os.path.isfile(path):
                    # not a valid mapset, but dir exists, assuming
                    # broken/incomplete mapset
                    fatal(
                        _(
                            "Unable to create new mapset <{mapset}>"
                            " because <{path}> is a file."
                        ).format(mapset=mapset, path=path)
                    )
                elif os.path.isdir(path):
                    # not a valid mapset, but dir exists, assuming
                    # broken/incomplete mapset
                    warning(
                        _(
                            "The mapset <{}> is missing the WIND file"
                            " (computational region). It will be"
                            " fixed now. Note that this warning"
                            " may become an error in future versions."
                        ).format(mapset)
                    )
                else:
                    if geofile:
                        fatal(
                            _(
                                "No CRS is needed for creating mapset <{mapset}>, "
                                "but <{geofile}> was provided as CRS."
                                " Did you mean to create a new project?"
                            ).format(mapset=mapset, geofile=geofile)
                        )
                    if not tmp_mapset:
                        message(
                            _("Creating new GRASS GIS mapset <{}>...").format(mapset)
                        )
                    # create mapset directory
                    os.mkdir(path)
                    if tmp_mapset:
                        # The tmp location is handled by (re-)using the
                        # tmpdir, but we need to take care of the tmp
                        # mapset which is only a subtree in an existing
                        # location. We simply remove the tree at exit.
                        # All mapset cleaning functions should succeed
                        # because they are called before exit or registered
                        # only later (and thus called before this one).
                        # (Theoretically, they could be disabled if that's
                        # just cleaning a files in the mapset directory.)
                        atexit.register(lambda: shutil.rmtree(path, ignore_errors=True))
                # make directory a mapset, add the region
                # copy PERMANENT/DEFAULT_WIND to <mapset>/WIND
                s = readfile(
                    os.path.join(gisdbase, location_name, "PERMANENT", "DEFAULT_WIND")
                )
                writefile(os.path.join(path, "WIND"), s)
        add_mapset_to_gisrc(gisrc, gisdbase, location_name, mapset)
    else:
        fatal(
            _(
                "GRASS GIS database directory, project and mapset"
                " not set properly."
                " Use GUI or command line to set them."
            )
        )


# we don't follow the LOCATION_NAME legacy naming here but we have to still
# translate to it, so always double check
class MapsetSettings:
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
            self._full_mapset = os.path.join(self.gisdbase, self.location, self.mapset)
        return self._full_mapset

    # TODO: perhaps conversion to bool would be nicer
    def is_valid(self):
        return self.gisdbase and self.location and self.mapset


def get_mapset_settings(gisrc: StrPath) -> MapsetSettings | None:
    """Get the settings of Location and Mapset from the gisrc file"""
    mapset_settings = MapsetSettings()
    kv = read_gisrc(gisrc)
    mapset_settings.gisdbase = kv.get("GISDBASE")
    mapset_settings.location = kv.get("LOCATION_NAME")
    mapset_settings.mapset = kv.get("MAPSET")
    if not mapset_settings.is_valid():
        return None
    return mapset_settings


# TODO: does it really makes sense to tell user about gisrcrc?
# anything could have happened in between loading from gisrcrc and now
# (we do e.g. GUI or creating loctation)
def load_gisrc(gisrc: StrPath, gisrcrc: StrPath) -> MapsetSettings:
    """Get the settings of Location and Mapset from the gisrc file

    :returns: MapsetSettings object
    """
    mapset_settings = get_mapset_settings(gisrc)
    if not mapset_settings:
        fatal(
            _(
                "Error reading data path information from g.gisenv.\n"
                "GISDBASE={gisdbase}\n"
                "LOCATION_NAME={location}\n"
                "MAPSET={mapset}\n\n"
                "Check the <{file}> file."
            ).format(
                gisdbase=mapset_settings.gisdbase,
                location=mapset_settings.location,
                mapset=mapset_settings.mapset,
                file=gisrcrc,
            )
        )
    return mapset_settings


# load environmental variables from grass_env_file
def load_env(grass_env_file: StrPath) -> None:
    if not os.access(grass_env_file, os.R_OK):
        return

    # Regular expression for lines starting with "export var=val" (^export
    # lines below). Environment variables should start with a-zA-Z or _.
    # \1 and \2 are a variable name and its value, respectively.
    export_re = re.compile(r"^export[ \t]+([a-zA-Z_]+[a-zA-Z0-9_]*)=(.*?)[ \t]*$")

    for line in readfile(grass_env_file).splitlines():
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
        elif v.startswith(("'", '"')) or v.endswith(("'", '"')):
            # here, let's try to ignore unmatching single/double quotes, which
            # might be a multi-line variable or just a user error
            debug("Ignoring multi-line environmental variable {0}".format(k))
            continue
        if expand:
            # finally, expand $var's within a non-single quoted value
            v = os.path.expanduser(
                os.path.expandvars(v.replace("\\$", "\0")).replace("\0", "$")
            )
        debug("Environmental variable set {0}={1}".format(k, v))
        # create a new environment variable
        os.environ[k] = v


def install_notranslation() -> None:
    # If locale is not supported, _ function might be missing
    # This function just installs _ as a pass-through function
    # See trac #3875 for details
    import builtins

    builtins.__dict__["_"] = lambda x: x


def set_language(grass_config_dir: StrPath) -> None:
    # This function is used to override system default language and locale
    # Such override can be requested only from wxGUI
    # An override if user has provided correct environmental variables as
    # LC_MESSAGES or LANG is not necessary.
    # Unfortunately currently a working solution for Windows is lacking
    # thus it always on Vista and XP will print an error.
    # See discussion for Windows not following its own documentation and
    # not accepting ISO codes as valid locale identifiers
    # http://bugs.python.org/issue10466
    # As this code relies heavily on various locale calls, it is necessary
    # to track related python changes:
    # https://bugs.python.org/issue43557
    encoding = None

    # Override value is stored in wxGUI preferences file.
    try:
        with open(os.path.join(grass_config_dir, "wx.json")) as json_file:
            try:
                language = json.load(json_file)["language"]["locale"]["lc_all"]
            except KeyError:
                language = None
    except FileNotFoundError:
        language = None

    # Backwards compatibility with old wx preferences files
    if language == "C":
        language = "en"

    if not language:
        # Language override is disabled (system language specified)
        # As by default program runs with C locale, but users expect to
        # have their default locale, we'll just set default locale
        try:
            locale.setlocale(locale.LC_ALL, "")
        except locale.Error as e:
            # If we get here, system locale settings are terribly wrong
            # There is no point to continue as GRASS/Python will fail
            # in some other unpredictable way.
            sys.stderr.write(
                "System locale is not usable (LC_ALL variable not defined)."
                " Most likely it indicates misconfigured environment.\n"
            )
            sys.stderr.write("Reported error message: %s\n" % e)
            # it would be too drastic to exit
            # sys.exit("Fix system locale settings and then try again.")
            locale.setlocale(locale.LC_ALL, "C")
            sys.stderr.write(
                "Default locale settings are missing. GRASS running with C locale.\n"
            )

        try:
            # Python >= 3.11
            language, encoding = locale.getlocale()
        except AttributeError:
            language, encoding = locale.getdefaultlocale()
        if not language:
            sys.stderr.write(
                "Default locale settings are missing. GRASS running with C locale.\n"
            )
            install_notranslation()
            return

    else:
        debug(
            "A language override has been requested."
            " Trying to switch GRASS into '%s'..." % language
        )
        try:
            locale.setlocale(locale.LC_ALL, language)
        except locale.Error:
            try:
                # Locale lang.encoding might be missing. Let's try
                # UTF-8 encoding before giving up as on Linux systems
                # lang.UTF-8 locales are more common than legacy
                # ISO-8859 ones.
                encoding = "UTF-8"
                normalized = locale.normalize("%s.%s" % (language, encoding))
                locale.setlocale(locale.LC_ALL, normalized)
            except locale.Error:
                if language == "en":
                    # A workaround for Python Issue30755
                    # https://bugs.python.org/issue30755
                    if locale.normalize("C.UTF-8") == "en_US.UTF-8":
                        locale.setlocale(locale.LC_ALL, "C")
                        os.environ["LANGUAGE"] = "C"
                        os.environ["LANG"] = "C"
                        os.environ["LC_MESSAGES"] = "C"
                        os.environ["LC_NUMERIC"] = "C"
                        os.environ["LC_TIME"] = "C"
                        sys.stderr.write(
                            "To avoid Unicode errors in GUI, install"
                            " en_US.UTF-8 locale and restart GRASS.\n"
                            "Also consider upgrading your Python version"
                            " to one containing fix for Python Issue 30755.\n"
                        )
                        install_notranslation()
                        return
                    # en_US locale might be missing, still all messages in
                    # GRASS are already in en_US language.
                    # Using plain C as locale forces encodings to ascii
                    # thus lets try our luck with C.UTF-8 first.
                    # See bugs #3441 and #3423
                    try:
                        locale.setlocale(locale.LC_ALL, "C.UTF-8")
                    except locale.Error:
                        # All lost. Setting to C as much as possible.
                        # We can not call locale.normalize on C as it
                        # will transform it to en_US and we already know
                        # it doesn't work.
                        locale.setlocale(locale.LC_ALL, "C")
                        os.environ["LANGUAGE"] = "C"
                        os.environ["LANG"] = "C"
                        os.environ["LC_MESSAGES"] = "C"
                        os.environ["LC_NUMERIC"] = "C"
                        os.environ["LC_TIME"] = "C"
                        gettext.install("grasslibs", gpath("locale"))
                        sys.stderr.write(
                            "All attempts to enable English language have"
                            " failed. GRASS running with C locale.\n"
                            "If you observe UnicodeError in Python,"
                            " install en_US.UTF-8"
                            " locale and restart GRASS.\n"
                        )
                        install_notranslation()
                        return
                else:
                    # The last attempt...
                    try:
                        encoding = locale.getpreferredencoding()
                        normalized = locale.normalize("%s.%s" % (language, encoding))
                        locale.setlocale(locale.LC_ALL, normalized)
                    except locale.Error as e:
                        # If we got so far, attempts to set up language and locale have
                        # failed on this system.
                        sys.stderr.write(
                            "Failed to enforce user specified language "
                            f"'{language}' with error: '{e}'\n"
                        )
                        sys.stderr.write(
                            "A LANGUAGE environmental variable has been set.\n"
                            "Part of messages will be displayed in"
                            " the requested language.\n"
                        )
                        # Even if setting locale will fail, let's set LANG in a hope,
                        # that UI will use it GRASS texts will be in selected language,
                        # system messages (i.e. OK, Cancel etc.) - in system default
                        # language
                        os.environ["LANGUAGE"] = language
                        os.environ["LANG"] = language
                        install_notranslation()
                        return

    # Set up environment for subprocesses
    os.environ["LANGUAGE"] = language
    os.environ["LANG"] = language

    if WINDOWS and (language == "ko" or (language == "ko_KR" and encoding == "cp949")):
        # The default encoding for the Korean language in Windows is cp949,
        # Microsoft's proprietary extension to euc-kr, but gettext prints no
        # translated messages at all in the Command Prompt window if LC_CTYPE
        # is set to ko_KR.cp949. Here, force LC_CTYPE to be euc-kr.
        normalized = "euc-kr"
        encoding = None

        # XXX: In UN*X, LC_CTYPE needs to be set to *any* value before GRASS
        # starts when the language setting is overridden by the user. For
        # example, 'LC_CTYPE= grass' will break the welcome message.
        # Interestingly, modules' help messages look fine.
    elif encoding:
        normalized = locale.normalize("%s.%s" % (language, encoding))
    else:
        normalized = language

    for lc in (
        "LC_CTYPE",
        "LC_MESSAGES",
        "LC_TIME",
        "LC_COLLATE",
        "LC_MONETARY",
        "LC_PAPER",
        "LC_NAME",
        "LC_ADDRESS",
        "LC_TELEPHONE",
        "LC_MEASUREMENT",
        "LC_IDENTIFICATION",
    ):
        os.environ[lc] = normalized

    # Some code in GRASS might not like other decimal separators than .
    # Other potential sources for problems are: LC_TIME LC_CTYPE
    locale.setlocale(locale.LC_NUMERIC, "C")
    os.environ["LC_NUMERIC"] = "C"
    if os.getenv("LC_ALL"):
        del os.environ["LC_ALL"]  # Remove LC_ALL to not override LC_NUMERIC

    # From now on enforce the new language
    gettext.install("grasslibs", gpath("locale"))


# TODO: the gisrcrc here does not make sense, remove it from load_gisrc
def unlock_gisrc_mapset(gisrc: StrPath, gisrcrc: StrPath) -> None:
    """Unlock mapset from the gisrc file"""
    settings: MapsetSettings = load_gisrc(gisrc, gisrcrc)
    lockfile = os.path.join(settings.full_mapset, ".gislock")
    # this fails silently, perhaps a warning would be helpful to
    # catch cases when removal was not possible due to e.g. another
    # session force-removing the file (unlocking the mapset)
    try_remove(lockfile)


def make_fontcap() -> None:
    # TODO: is GRASS_FONT_CAP ever defined? It seems it must be defined in system
    fc = os.getenv("GRASS_FONT_CAP")
    if fc and not os.access(fc, os.R_OK):
        message(_("Building user fontcap..."))
        call(["g.mkfontcap"])


def ensure_db_connected(mapset: StrPath) -> None:
    """Predefine default driver if DB connection not defined

    :param mapset: full path to the mapset
    """
    if not os.access(os.path.join(mapset, "VAR"), os.F_OK):
        call(["db.connect", "-c", "--quiet"])


def get_shell():
    # set SHELL on ms windowns
    # this was at the very beginning of the script but it can be anywhere
    if WINDOWS:
        if os.getenv("GRASS_SH"):
            os.environ["SHELL"] = os.getenv("GRASS_SH")
        if not os.getenv("SHELL"):
            os.environ["SHELL"] = os.getenv("COMSPEC", "cmd.exe")

    # cygwin has many problems with the shell setup
    # below, so i hardcoded everything here.
    if CYGWIN:
        sh = "CYGWIN"
        shellname = "GNU Bash (Cygwin)"
        os.environ["SHELL"] = "/usr/bin/bash.exe"
        os.environ["OSTYPE"] = "cygwin"
    else:
        # In a Docker container the 'SHELL' variable may not be set
        # unless 'ENV SHELL /bin/bash' is set in Dockerfile. However, often Bash
        # is available, so we try Bash first and fall back to sh if needed.
        sh = os.getenv("SHELL")
        if sh:
            sh = os.path.basename(sh)
        else:
            # If SHELL is not set, see if there is Bash and use it.
            # Fallback to sh if there is no Bash on path.
            sh = "bash" if shutil.which("bash") else "sh"
            # Ensure the variable is set.
            os.environ["SHELL"] = sh

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
    if not os.getenv("SHELL"):
        fatal(_("The SHELL variable is not set"))
    return sh, shellname


def get_grass_env_file(sh, grass_config_dir: StrPath) -> str:
    """Get name of the shell-specific GRASS environment (rc) file"""
    if sh in {"csh", "tcsh"}:
        grass_env_file = os.path.join(grass_config_dir, "cshrc")
    elif sh in {"bash", "msh", "cygwin", "sh"}:
        grass_env_file = os.path.join(grass_config_dir, "bashrc")
    elif sh == "zsh":
        grass_env_file = os.path.join(grass_config_dir, "zshrc")
    elif sh in {"cmd", "powershell"}:
        grass_env_file = os.path.join(grass_config_dir, "env.bat")
    else:
        grass_env_file = os.path.join(grass_config_dir, "bashrc")
        warning(
            _("Unsupported shell <{sh}>: {env_file}").format(
                sh=sh, env_file=grass_env_file
            )
        )
    return grass_env_file


# No reason to use list over Sequence except that
# importing of Sequence changed in Python 3.9.
# (Replace list by Sequence in the future.)
def run_batch_job(batch_job: list):
    """Runs script, module or any command

    :param batch_job: executable and parameters as a list
    """
    # Lazy-import to avoid dependency during standard import time.
    # pylint: disable=import-outside-toplevel
    import grass.script as gs

    batch_job_string = " ".join(batch_job)  # for messages only
    gs.verbose(_("Executing <%s> ...") % batch_job_string)

    def script_path(batch_job):
        """Adjust script path

        :param batch_job list: index 0, script path

        :return str or None: script path or None
        """
        script_in_addon_path = None
        if "GRASS_ADDON_BASE" in os.environ:
            script_in_addon_path = os.path.join(
                os.environ["GRASS_ADDON_BASE"], "scripts", batch_job[0]
            )
        if script_in_addon_path and os.path.exists(script_in_addon_path):
            batch_job[0] = script_in_addon_path
            return script_in_addon_path
        if os.path.exists(batch_job[0]):
            return batch_job[0]

    try:
        script = script_path(batch_job)
        proc = Popen(batch_job, shell=False, env=os.environ)
    except OSError as error:
        error_message = _("Execution of <{cmd}> failed:\n{error}").format(
            cmd=batch_job_string, error=error
        )
        # No such file or directory
        if error.errno == errno.ENOENT:
            if script and os.access(batch_job[0], os.X_OK):
                # Allow run py script with CRLF line terminators
                proc = Popen([sys.executable] + batch_job, shell=False)
            else:
                fatal(error_message)
        else:
            fatal(error_message)
    returncode = proc.wait()
    gs.verbose(_("Execution of <%s> finished.") % batch_job_string)
    return returncode


def start_gui(grass_gui: Literal["wxpython"]):
    """Start specified GUI

    :param grass_gui: GUI name (supported values: 'wxpython')

    Returns the process for a supported GUI, None otherwise.
    """
    # Start the chosen GUI but ignore text
    debug("GRASS GUI should be <%s>" % grass_gui)
    # Check for gui interface
    if grass_gui == "wxpython":
        # Lazy-import to avoid dependency during standard import time.
        # pylint: disable=import-outside-toplevel
        import grass.script as gs

        if is_debug() or "WX_DEBUG" in gs.gisenv():
            stderr_redirect = None
        else:
            stderr_redirect = subprocess.DEVNULL
        # TODO: report start failures?
        return Popen(
            [os.getenv("GRASS_PYTHON"), wxpath("wxgui.py")], stderr=stderr_redirect
        )
    return None


def close_gui() -> None:
    """Close GUI if running"""
    if gpath("etc", "python") not in sys.path:
        sys.path.append(gpath("etc", "python"))
    from grass.script import core as gcore  # pylint: disable=E0611

    env = gcore.gisenv()
    if "GUI_PID" not in env:
        return
    for pid in env["GUI_PID"].split(","):
        debug("Exiting GUI with pid={0}".format(pid))
        try:
            os.kill(int(pid), signal.SIGTERM)
        except OSError as e:
            message(_("Unable to close GUI. {0}").format(e))


def show_banner() -> None:
    """Write GRASS GIS ASCII name to stderr"""
    sys.stderr.write(
        r"""
          __________  ___   __________    _______________
         / ____/ __ \/   | / ___/ ___/   / ____/  _/ ___/
        / / __/ /_/ / /| | \__ \\_  \   / / __ / / \__ \
       / /_/ / _, _/ ___ |___/ /__/ /  / /_/ // / ___/ /
       \____/_/ |_/_/  |_/____/____/   \____/___//____/

"""
    )


def say_hello() -> None:
    """Write welcome to stderr including code revision if in git copy"""
    sys.stderr.write(_("Welcome to GRASS GIS %s") % GRASS_VERSION)
    if GRASS_VERSION.endswith("dev"):
        try:
            with open(gpath("etc", "VERSIONNUMBER")) as filerev:
                linerev = filerev.readline().rstrip("\n")

            revision = linerev.split(" ")[1]
            sys.stderr.write(" (" + revision + ")")
        except Exception:
            pass


INFO_TEXT = r"""
%-41shttps://grass.osgeo.org
%-41s%s (%s)
%-41sg.manual -i
%-41sg.version -c
%-41sg.version -x
"""


def show_info(shellname, grass_gui, default_gui) -> None:
    """Write basic info about GRASS GIS and GRASS session to stderr"""
    sys.stderr.write(
        fw(
            INFO_TEXT,
            _("GRASS GIS homepage:"),
            # GTC Running through: SHELL NAME
            _("This version running through:"),
            shellname,
            os.getenv("SHELL"),
            _("Help is available with the command:"),
            _("See the licence terms with:"),
            _("See citation options with:"),
        )
    )

    if grass_gui == "wxpython":
        message(fw("%-41sg.gui wxpython", _("If required, restart the GUI with:")))
    else:
        message(fw("%-41sg.gui %s", _("Start the GUI with:"), default_gui))

    message(fw("%-41sexit", _("When ready to quit enter:")))
    message("")


def start_shell():
    if WINDOWS:
        # "$ETC/run" doesn't work at all???
        process = subprocess.Popen([os.getenv("SHELL")])
    else:
        process = Popen([gpath("etc", "run"), os.getenv("SHELL")])
    return process


def csh_startup(location, grass_env_file: StrPath):
    userhome = os.getenv("HOME")  # save original home
    home = location
    os.environ["HOME"] = home

    cshrc = os.path.join(home, ".cshrc")
    tcshrc = os.path.join(home, ".tcshrc")
    try_remove(cshrc)
    try_remove(tcshrc)

    f = open(cshrc, "w")
    f.write("set home = %s\n" % userhome)
    f.write("set history = 10000000 savehist = (10000000 merge) noclobber ignoreeof\n")
    f.write("set histfile = %s\n" % os.path.join(os.getenv("HOME"), ".history"))

    f.write("alias _location g.gisenv get=LOCATION_NAME\n")
    f.write("alias _mapset g.gisenv get=MAPSET\n")
    f.write("alias precmd 'echo \"Mapset <`_mapset`> in project <`_location`>\"'\n")
    f.write('set prompt="GRASS > "\n')

    # csh shell rc file left for backward compatibility
    path = os.path.join(userhome, ".grass.cshrc")
    if os.access(path, os.R_OK):
        f.write(readfile(path) + "\n")
    if os.access(grass_env_file, os.R_OK):
        f.write(readfile(grass_env_file) + "\n")

    mail_re = re.compile(r"^ *set  *mail *= *")

    for filename in [".cshrc", ".tcshrc", ".login"]:
        path = os.path.join(userhome, filename)
        if os.access(path, os.R_OK):
            s = readfile(path)
            lines = s.splitlines()
            for line in lines:
                if mail_re.match(line):
                    f.write(line)

    path = os.getenv("PATH").split(":")
    f.write("set path = ( %s ) \n" % " ".join(path))
    f.close()
    writefile(tcshrc, readfile(cshrc))

    process = start_shell()
    os.environ["HOME"] = userhome
    return process


def sh_like_startup(location, location_name, grass_env_file, sh):
    """Start Bash or Z shell (but not sh (Bourne Shell))"""
    if sh == "bash":
        # set bash history to record an unlimited command history
        sh_history_limit = ""  # unlimited
        os.environ["HISTSIZE"] = sh_history_limit
        os.environ["HISTFILESIZE"] = sh_history_limit
        sh_history = ".bash_history"
        shrc = ".bashrc"
        grass_shrc = ".grass.bashrc"
    elif sh == "zsh":
        # zsh does not have an unlimited history setting, so 1e8 is set as a proxy
        sh_history_limit = "100000000"  # proxy for unlimited
        os.environ["SAVEHIST"] = sh_history_limit
        os.environ["HISTSIZE"] = sh_history_limit
        sh_history = ".zsh_history"
        shrc = ".zshrc"
        grass_shrc = ".grass.zshrc"
    else:
        msg = "Only bash-like and zsh shells are supported by sh_like_startup()"
        raise ValueError(msg)

    # save command history in mapset dir and remember more
    # bash history file handled in specific_addition
    if sh != "bash":
        os.environ["HISTFILE"] = os.path.join(location, sh_history)

    # instead of changing $HOME, start bash with:
    #   --rcfile "$LOCATION/.bashrc" ?
    #   if so, must care be taken to explicitly call .grass.bashrc et al
    #   for non-interactive bash batch jobs?
    userhome = os.getenv("HOME")  # save original home
    home = location  # save .bashrc in $LOCATION
    os.environ["HOME"] = home

    shell_rc_file = os.path.join(home, shrc)
    try_remove(shell_rc_file)

    f = open(shell_rc_file, "w")

    if sh == "zsh":
        f.write("test -r {home}/.alias && source {home}/.alias\n".format(home=userhome))
    else:
        f.write("test -r ~/.alias && . ~/.alias\n")

    # GRASS GIS and ISIS blend
    grass_name = "GRASS" if not os.getenv("ISISROOT") else "ISIS-GRASS"

    if sh == "zsh":
        f.write("setopt PROMPT_SUBST\n")
        f.write("PS1='{name} : %1~ > '\n".format(name=grass_name))
    else:
        f.write(
            "PS1='{name} {db_place}:\\W > '\n".format(
                name=grass_name, db_place="$_GRASS_DB_PLACE"
            )
        )

    mask2d_test = "r.mask.status -t"
    # TODO: have a function and/or module to test this
    mask3d_test = 'test -d "$MAPSET_PATH/grid3/RASTER3D_MASK"'

    specific_addition = ""
    if sh == "zsh":
        specific_addition = """
    local z_lo=`g.gisenv get=LOCATION_NAME`
    local z_ms=`g.gisenv get=MAPSET`
    ZLOC="Mapset <$z_ms> in <$z_lo>"
    if [ "$_grass_old_mapset" != "$MAPSET_PATH" ] ; then
        fc -A -I
        HISTFILE="$MAPSET_PATH/{sh_history}"
        fc -R
        _grass_old_mapset="$MAPSET_PATH"
    fi
    """.format(sh_history=sh_history)
    elif sh == "bash":
        # Append existing history to file ("flush").
        # Clear the (in-memory) history.
        # Change the file.
        # Read history from that file.
        specific_addition = """
    if [ "$_grass_old_mapset" != "$MAPSET_PATH" ] ; then
        history -a
        history -c
        HISTFILE="$MAPSET_PATH/{sh_history}"
        history -r
        _grass_old_mapset="$MAPSET_PATH"
    fi
    """.format(sh_history=sh_history)
        # Ubuntu sudo creates a file .sudo_as_admin_successful and bash checks
        # for this file in the home directory from /etc/bash.bashrc and prints a
        # message if it's not detected. This can be suppressed with either
        # creating the file ~/.sudo_as_admin_successful (it's always empty) or
        # by creating a file ~/.hushlogin (also an empty file)
        # Here we create the file in the Mapset directory if it exists in the
        # user's home directory.
        sudo_success_file = ".sudo_as_admin_successful"
        if os.path.exists(os.path.join(userhome, sudo_success_file)):
            try:
                # Open with append so that if the file already exists there
                # isn't any error.
                fh = open(os.path.join(home, sudo_success_file), "a")
            finally:
                fh.close()

    # double curly brackets means single one for format function
    # setting LOCATION for backwards compatibility
    f.write(
        """grass_prompt() {{
    MAPSET_PATH="`g.gisenv get=GISDBASE,LOCATION_NAME,MAPSET separator='/'`"
    _GRASS_DB_PLACE="`g.gisenv get=LOCATION_NAME,MAPSET separator='/'`"
    {specific_addition}
    if {mask2d_test} && {mask3d_test} ; then
        echo "[{both_masks}]"
    elif {mask2d_test} ; then
        echo "[{mask2d}]"
    elif {mask3d_test} ; then
        echo "[{mask3d}]"
    fi
}}
PROMPT_COMMAND=grass_prompt\n""".format(
            both_masks=_("2D and 3D raster masks present"),
            mask2d=_("Raster mask present"),
            mask3d=_("3D raster mask present"),
            mask2d_test=mask2d_test,
            mask3d_test=mask3d_test,
            specific_addition=specific_addition,
        )
    )

    if sh == "zsh":
        f.write('precmd() { eval "$PROMPT_COMMAND" }\n')
        f.write("RPROMPT='${ZLOC}'\n")

    # this line was moved here from below .grass.bashrc to allow ~ and $HOME in
    # .grass.bashrc
    f.write('export HOME="%s"\n' % userhome)  # restore user home path

    # read other settings (aliases, ...) since environmental variables
    # have been already set by load_env(), see #3462
    for env_file in [os.path.join(userhome, grass_shrc), grass_env_file]:
        if not os.access(env_file, os.R_OK):
            continue
        for line in readfile(env_file).splitlines():
            # Bug related to OS X "SIP", see
            # https://trac.osgeo.org/grass/ticket/3462#comment:13
            if MACOS or not line.startswith("export"):
                f.write(line + "\n")

    f.write('export PATH="%s"\n' % os.getenv("PATH"))
    # fix trac issue #3009 https://trac.osgeo.org/grass/ticket/3009
    # re: failure to "Quit GRASS" from GUI
    f.write('trap "exit" TERM\n')
    f.close()

    process = start_shell()
    os.environ["HOME"] = userhome
    return process


def default_startup(location, location_name):
    """Start shell making no assumptions about what is supported in PS1"""
    os.environ["PS1"] = "GRASS > "
    return start_shell()


def done_message() -> None:
    # here was something for batch job but it was never called
    message(_("Done."))
    message("")
    message(_("Goodbye from GRASS GIS"))
    message("")


def clean_temp() -> None:
    """Clean mapset temporary directory

    Simple wrapper around the library function avoiding the need to solve imports at
    the top level. This can hopefully be avoided in the future.
    """
    from grass.script import setup as gsetup

    gsetup.clean_temp()


def clean_all(*, start_time) -> None:
    from grass.script import setup as gsetup

    # clean default sqlite db
    gsetup.clean_default_db(modified_after=start_time)
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


def io_is_interactive() -> bool:
    """Return True if running in an interactive terminal (TTY), False otherwise"""
    return sys.stdin.isatty() and sys.stdout.isatty()


def print_params(params) -> None:
    """Write compile flags and other configuration to stdout"""
    if not params:
        params = [
            "arch",
            "build",
            "compiler",
            "path",
            "python_path",
            "revision",
            "version",
            "date",
        ]

    # check if we are dealing with parameters which require dev files
    dev_params = ["arch", "compiler", "build", "date"]
    if any(param in dev_params for param in params):
        plat = gpath("include", "Make", "Platform.make")
        if not os.path.exists(plat):
            fatal(_("Please install the GRASS GIS development package"))
        with open(plat) as fileplat:
            # this is in fact require only for some, but prepare it anyway
            linesplat = fileplat.readlines()

    for arg in params:
        if arg == "path":
            sys.stdout.write("%s\n" % GISBASE)
        elif arg in {"python_path", "python-path"}:
            sys.stdout.write("%s\n" % gpath("etc", "python"))
        elif arg == "arch":
            val = grep("ARCH", linesplat)
            sys.stdout.write("%s\n" % val[0].split("=")[1].strip())
        elif arg == "build":
            build = gpath("include", "grass", "confparms.h")
            with open(build) as filebuild:
                val = filebuild.readline()
            sys.stdout.write("%s\n" % val.strip().strip('"').strip())
        elif arg == "compiler":
            val = grep("CC", linesplat)
            sys.stdout.write("%s\n" % val[0].split("=")[1].strip())
        elif arg == "revision":
            sys.stdout.write("@GRASS_VERSION_GIT@\n")
        elif arg == "svn_revision":
            with open(gpath("etc", "VERSIONNUMBER")) as filerev:
                linerev = filerev.readline().rstrip("\n")
            try:
                revision = linerev.split(" ")[1]
                sys.stdout.write("%s\n" % revision[1:])
            except Exception:
                sys.stdout.write("No SVN revision defined\n")
        elif arg == "version":
            sys.stdout.write("%s\n" % GRASS_VERSION)
        elif arg == "date":
            date_str = "#define GRASS_HEADERS_DATE "
            gdate = gpath("include", "grass", "version.h")
            with open(gdate) as filegdate:
                for line in filegdate:
                    if line.startswith(date_str):
                        sys.stdout.write(
                            "{}\n".format(
                                line.replace(date_str, "").lstrip()[1:-2]
                            )  # remove quotes
                        )
                        break
        else:
            message(_("Parameter <%s> not supported") % arg)


def get_username() -> str:
    """Get name of the current user"""
    if WINDOWS:
        user = os.getenv("USERNAME")
        if not user:
            user = "user_name"
    else:
        user = os.getenv("USER")
        if not user:
            user = os.getenv("LOGNAME")
        if not user and (whoami_executable := shutil.which("whoami")):
            try:
                user = subprocess.run(
                    [whoami_executable], stdout=subprocess.PIPE, text=True, check=True
                ).stdout.strip()
            except (OSError, subprocess.SubprocessError):
                pass
        if not user:
            user = "user_%d" % os.getuid()
    return user


class Parameters:
    """Structure to hold standard part of command line parameters"""

    # we don't need to define any methods
    # pylint: disable=R0903

    def __init__(self) -> None:
        self.grass_gui: str | None = None
        self.create_new = None
        self.exit_grass = None
        self.force_gislock_removal: bool | None = None
        self.mapset = None
        self.geofile = None
        self.tmp_location = False
        self.tmp_mapset = False
        self.batch_job = None


def add_mapset_arguments(
    parser: argparse.ArgumentParser, mapset_as_option: bool
) -> None:
    if mapset_as_option:
        parser.add_argument(
            "-m", "--mapset", metavar="PATH", type=str, help=_("use mapset %(metavar)s")
        )
        parser.add_argument(
            "--tmp-mapset",
            metavar="PATH",
            type=str,
            help=_("use temporary mapset in project %(metavar)s"),
        )
    else:
        parser.add_argument(
            "mapset",
            metavar="PATH",
            type=str,
            nargs="?",
            help=_("path to mapset (or project if creating one)"),
        )
        parser.add_argument(
            "--tmp-mapset", action="store_true", help=_("use temporary mapset")
        )
    parser.add_argument(
        "--tmp-project",
        metavar="CRS",
        type=str,
        help=_(
            "use temporary project with %(metavar)s (EPSG, georeferenced file, ...)"
        ),
    )
    parser.add_argument(
        "--tmp-location",
        metavar="CRS",
        type=str,
        help=_("deprecated, use --tmp-project instead"),
    )
    parser.add_argument(
        "-f",
        "--force-remove-lock",
        action="store_true",
        help=_("remove lock if present"),
    )


def update_params_with_mapset_arguments(
    params: Parameters, args: argparse.Namespace
) -> None:
    """Update location and mapset related parameters"""
    if args.force_remove_lock:
        params.force_gislock_removal = True
    if args.tmp_location:
        params.tmp_location = True
        params.geofile = args.tmp_location
        sys.stdout.write(
            _("Option --tmp-location is deprecated, use --tmp-project instead\n")
        )
        sys.stdout.flush()
    if args.tmp_project:
        params.tmp_location = True
        params.geofile = args.tmp_project
    if args.tmp_mapset:
        params.tmp_mapset = True
    if args.mapset:
        params.mapset = args.mapset


def classic_parser(argv, default_gui) -> Parameters:
    """Parse CLI similar to v7 but with argparse

    --exec is handled before argparse is used.
    Help requests are also handled separately.

    Not only help but also version and config are handled in this function.
    """
    # Check if the user asked for help
    # Checking also the less standard -help and help.
    help_requests = ["help", "-h", "-help", "--help", "--h"]
    if len(argv) == 2 and argv[1] in help_requests:
        help_message(default_gui=default_gui)
        sys.exit()

    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--version", action="store_true")
    parser.add_argument("--text", action="store_true")
    parser.add_argument("--gtext", action="store_true")
    parser.add_argument("--gui", action="store_true")
    # -c works as plain True when only mapset is being created.
    # However, option with a value is hungry, so it eats the mapset path.
    mapset_tag = "__create_mapset_placeholder__"
    parser.add_argument(
        "-c", metavar="CRS", nargs="?", type=str, dest="create", const=mapset_tag
    )
    parser.add_argument("-e", action="store_true", dest="exit")
    parser.add_argument("--config", nargs="*")
    add_mapset_arguments(parser, mapset_as_option=False)
    parser.add_argument(
        "--exec",
        nargs=argparse.REMAINDER,
        help=_("execute module or script (followed by executable with arguments)"),
    )
    parsed_args = parser.parse_args(args=argv[1:])

    params = Parameters()
    # Check if the --text flag was given
    if parsed_args.text:
        params.grass_gui = "text"
    # Check if the --gtext flag was given
    if parsed_args.gtext:
        params.grass_gui = "gtext"
    # Check if the --gui flag was given
    if parsed_args.gui:
        params.grass_gui = default_gui
    # Check if the user wants to create a new mapset
    if parsed_args.create:
        params.create_new = True
        if parsed_args.create != mapset_tag:
            if parsed_args.mapset:
                # A value here is request to create a location.
                params.geofile = parsed_args.create
            else:
                # The loc/mapset path argument is optional, so it can get
                # eaten by the option. So here we got it mixed up and the
                # option value is actually the path, not CRS.
                params.mapset = parsed_args.create
    if parsed_args.exit:
        params.exit_grass = True
    if parsed_args.exec:
        params.batch_job = parsed_args.exec
    # Cases to execute immediately
    if parsed_args.version:
        sys.stdout.write("GRASS GIS %s" % GRASS_VERSION)
        sys.stdout.write("\n" + readfile(gpath("etc", "license")))
        sys.exit()
    if parsed_args.config is not None:
        # None if not provided, empty list if present without values.
        print_params(parsed_args.config)
        sys.exit()
    update_params_with_mapset_arguments(params, parsed_args)
    return params


def parse_cmdline(argv, default_gui) -> Parameters:
    """Parse command line parameters

    Returns Parameters object used throughout the script.
    """
    params: Parameters = classic_parser(argv, default_gui)
    validate_cmdline(params)
    return params


def validate_cmdline(params: Parameters) -> None:
    """Validate the cmdline params and exit if necessary."""
    if params.exit_grass and not params.create_new:
        fatal(_("Flag -e requires also flag -c"))
    if params.create_new and not params.mapset:
        fatal(_("Flag -c requires name of project or mapset"))
    if params.tmp_location and params.tmp_mapset:
        fatal(
            _("Either --tmp-project or --tmp-mapset can be used, not both").format(
                params.mapset
            )
        )
    if params.tmp_location and not params.geofile:
        fatal(
            _(
                "Coordinate reference system argument (e.g. EPSG)"
                " is needed for --tmp-project"
            )
        )
    if params.tmp_location and params.mapset:
        fatal(
            _(
                "Only one argument (e.g. EPSG) is needed for"
                " --tmp-project, mapset name <{}> provided"
            ).format(params.mapset)
        )
    # For now, we allow, but not advertise/document, --tmp-location
    # without --exec (usefulness to be evaluated).


def find_grass_python_package() -> None:
    """Find path to grass package and add it to path"""
    if os.path.exists(gpath("etc", "python")):
        path_to_package = gpath("etc", "python")
        sys.path.append(path_to_package)
        # now we can import stuff from grass package
    else:
        # Not translatable because we don't have translations loaded.
        msg = (
            "The grass Python package is missing. "
            "Is the installation of GRASS GIS complete?"
        )
        raise RuntimeError(msg)


def main() -> None:
    """The main function which does the whole setup and run procedure

    Only few things are set on the module level.
    """
    # Set language
    # This has to be called before any _() function call!
    # Subsequent functions are using _() calls and
    # thus must be called only after Language has been set.
    find_grass_python_package()
    grass_config_dir = create_grass_config_dir()
    set_language(grass_config_dir)

    # Set default GUI
    default_gui = "wxpython"

    # explain what is happening in debug mode (visible only in debug mode)
    debug(
        "GRASS_DEBUG environmental variable is set. It is meant to be"
        " an internal variable for debugging only this script.\n"
        " Use 'g.gisenv set=\"DEBUG=[0-5]\"'"
        " to turn GRASS GIS debug mode on if you wish to do so."
    )

    # Set GRASS version number for R interface etc
    # (must be an env var for MS Windows)
    os.environ["GRASS_VERSION"] = GRASS_VERSION

    # Set the GIS_LOCK variable to current process id
    gis_lock = str(os.getpid())
    os.environ["GIS_LOCK"] = gis_lock

    params: Parameters = parse_cmdline(sys.argv, default_gui=default_gui)

    grass_gui = params.grass_gui  # put it to variable, it is used a lot

    # A shell is activated when using TTY.
    # Explicit --[g]text in command line, forces a shell to be activated.
    force_shell = grass_gui in {"text", "gtext"}
    use_shell = io_is_interactive() or force_shell
    if not use_shell:
        # If no shell is used, always use actual GUI as GUI, even when "text" is set as
        # a GUI in the gisrcrc because otherwise there would be nothing for the user
        # unless running in the batch mode. (The gisrcrc file is loaded later on in case
        # nothing was provided in the command line).
        grass_gui = default_gui

    # TODO: with --tmp-location there is no point in loading settings
    # i.e. rc file from home dir, but the code is too spread out
    # to disable it at this point
    gisrcrc = get_gisrc_from_config_dir(grass_config_dir, params.batch_job)

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

    from grass.app.runtime import (
        ensure_home,
        set_browser,
        set_defaults,
        set_display_defaults,
        set_paths,
    )

    ensure_home()
    # Set PATH, PYTHONPATH, ...
    set_paths(
        install_path=GISBASE,
        grass_config_dir=grass_config_dir,
        ld_library_path_variable_name=LD_LIBRARY_PATH_VAR,
    )
    # Set GRASS_PAGER, GRASS_PYTHON, GRASS_GNUPLOT, GRASS_PROJSHARE
    set_defaults(config_projshare_path=CONFIG_PROJSHARE)
    # Set GRASS_HTML_BROWSER
    set_browser(install_path=GISBASE)
    set_display_defaults()

    # First time user - GISRC is defined in the GRASS script
    if not os.access(gisrc, os.F_OK):
        if grass_gui == "text" and not params.mapset:
            fatal(
                _(
                    "Unable to start GRASS GIS. You have the choice to:\n"
                    " - Launch the graphical user interface with"
                    " the '--gui' switch\n"
                    "     {cmd_name} --gui\n"
                    " - Launch with path to "
                    "the project/mapset as an argument\n"
                    "     {cmd_name} /path/to/project/mapset`\n"
                    " - Create a project with '-c' and launch in its"
                    " PERMANENT mapset\n"
                    "     {cmd_name} -c EPSG:number /path/to/project\n"
                    "     {cmd_name} -c geofile /path/to/project\n"
                    " - Create manually the GISRC file ({gisrcrc})\n"
                    " - Use '--help' for further options\n"
                    "     {cmd_name} --help\n"
                    "See also: https://grass.osgeo.org/{cmd_name}/manuals/helptext.html"
                ).format(cmd_name=CMD_NAME, gisrcrc=gisrcrc)
            )
        create_initial_gisrc(gisrc)

    if not params.batch_job and not params.exit_grass:
        # Only for interactive sessions, not for 'one operation' sessions.
        message(_("Starting GRASS GIS..."))

    # Ensure GUI is set
    if params.batch_job or params.exit_grass:
        grass_gui = "text"
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
    # Mapset is not specified in command line arguments
    if not params.mapset and not params.tmp_location:
        # Get mapset parameters from gisrc file
        mapset_settings = get_mapset_settings(gisrc)
        last_mapset_path = mapset_settings.full_mapset
        # Check if mapset from gisrc is usable
        from grass.grassdb.checks import can_start_in_mapset

        last_mapset_usable = can_start_in_mapset(
            mapset_path=last_mapset_path, ignore_lock=params.force_gislock_removal
        )
        debug(f"last_mapset_usable: {last_mapset_usable}")
        if not last_mapset_usable:
            from grass.app import ensure_default_data_hierarchy
            from grass.grassdb.checks import is_first_time_user

            fallback_session = False

            # Add last used mapset to gisrc
            add_last_mapset_to_gisrc(gisrc, last_mapset_path)

            if is_first_time_user():
                # Ensure default data hierarchy
                (
                    default_gisdbase,
                    default_location,
                    unused_default_mapset,
                    default_mapset_path,
                ) = ensure_default_data_hierarchy()

                if not default_gisdbase:
                    fatal(
                        _(
                            "Failed to start GRASS GIS, grassdata directory cannot"
                            " be found or created."
                        )
                    )
                elif not default_location:
                    fatal(
                        _(
                            "Failed to start GRASS GIS, no default project to copy in"
                            " the installation or copying failed."
                        )
                    )
                if can_start_in_mapset(
                    mapset_path=default_mapset_path, ignore_lock=False
                ):
                    # Use the default location/mapset.
                    set_mapset(gisrc=gisrc, arg=default_mapset_path)
                else:
                    fallback_session = True
                    add_last_mapset_to_gisrc(gisrc, default_mapset_path)
            else:
                fallback_session = True

            if fallback_session:
                if grass_gui == "text":
                    # Fallback in command line is just failing in a standard way.
                    set_mapset(gisrc=gisrc, arg=last_mapset_path)
                else:
                    # Create fallback temporary session
                    create_fallback_session(gisrc, tmpdir)
                    params.tmp_location = True
        else:
            # Use the last used mapset.
            set_mapset(gisrc=gisrc, arg=last_mapset_path)
    # Mapset was specified in command line parameters.
    elif params.tmp_location:
        # tmp loc requires other things to be set as well
        set_mapset(
            gisrc=gisrc,
            geofile=params.geofile,
            create_new=True,
            tmp_location=params.tmp_location,
            tmpdir=tmpdir,
        )
    elif params.create_new and params.geofile:
        set_mapset(
            gisrc=gisrc, arg=params.mapset, geofile=params.geofile, create_new=True
        )
    elif params.tmp_mapset:
        set_mapset(gisrc=gisrc, arg=params.mapset, tmp_mapset=params.tmp_mapset)
    else:
        set_mapset(gisrc=gisrc, arg=params.mapset, create_new=params.create_new)

    # Set GISDBASE, LOCATION_NAME, MAPSET, LOCATION from $GISRC
    # e.g. wxGUI startup screen writes to the gisrc file,
    # so loading it is the only universal way to obtain the values
    # this suppose that both programs share the right path to gisrc file
    # TODO: perhaps gisrcrc should be removed from here
    # alternatively, we can check validity here with all the info we have
    # about what was used to create gisrc
    mapset_settings = load_gisrc(gisrc, gisrcrc=gisrcrc)

    location = mapset_settings.full_mapset

    from grass.app.data import MapsetLockingException, lock_mapset

    try:
        # check and create .gislock file
        lock_mapset(
            mapset_path=mapset_settings.full_mapset,
            force_lock_removal=params.force_gislock_removal,
            message_callback=message,
        )
    except MapsetLockingException as e:
        fatal(e.args[0])
        sys.exit(_("Exiting..."))

    start_time = datetime.datetime.now(datetime.timezone.utc)

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
    if params.batch_job:
        returncode = run_batch_job(params.batch_job)
        clean_all(start_time=start_time)
        sys.exit(returncode)
    elif params.exit_grass:
        # clean always at exit, cleans whatever is current mapset based on
        # the GISRC env variable
        clean_all(start_time=start_time)
        sys.exit(0)
    else:
        if use_shell:
            show_banner()
            say_hello()
            show_info(shellname=shellname, grass_gui=grass_gui, default_gui=default_gui)
            if grass_gui == "wxpython":
                message(
                    _("Launching <%s> GUI in the background, please wait...")
                    % grass_gui
                )
            if sh in {"csh", "tcsh"}:
                shell_process = csh_startup(mapset_settings.full_mapset, grass_env_file)
            elif sh == "zsh":
                shell_process = sh_like_startup(
                    mapset_settings.full_mapset,
                    mapset_settings.location,
                    grass_env_file,
                    "zsh",
                )
            elif sh in {"bash", "msh", "cygwin"}:
                shell_process = sh_like_startup(
                    mapset_settings.full_mapset,
                    mapset_settings.location,
                    grass_env_file,
                    "bash",
                )
            else:
                shell_process = default_startup(
                    mapset_settings.full_mapset, mapset_settings.location
                )
        else:
            shell_process = None

        # start GUI and register shell PID in rc file
        gui_process = start_gui(grass_gui)

        if shell_process:
            kv = {}
            kv["PID"] = str(shell_process.pid)
            # grass_prompt() tries to read gisrc while write_gisrc() is adding PID
            # to this file, so don't rewrite it; just append PID to make it
            # available to grass_prompt() at all times (PR #548)
            write_gisrc(kv, gisrc, append=True)
            exit_val = shell_process.wait()
            if exit_val != 0:
                warning(_("Failed to start shell '%s'") % os.getenv("SHELL"))
        else:
            gui_process.wait()

        # close GUI if running
        close_gui()

        # here we are at the end of grass session
        clean_all(start_time=start_time)
        mapset_settings = load_gisrc(gisrc, gisrcrc=gisrcrc)
        if not params.tmp_location or (
            params.tmp_location and mapset_settings.gisdbase != os.environ["TMPDIR"]
        ):
            write_gisrcrc(gisrcrc, gisrc, skip_variable="LAST_MAPSET_PATH")
        # After this point no more grass modules may be called
        # done message at last: no atexit.register()
        # or register done_message()
        if use_shell:
            done_message()


if __name__ == "__main__":
    main()
