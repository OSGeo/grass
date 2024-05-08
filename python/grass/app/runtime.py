"""Provides functions for the main GRASS GIS executable

(C) 2020 by Vaclav Petras and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Vaclav Petras <wenzeslaus gmail com>
.. sectionauthor:: Linda Kladivova <l.kladivova seznam cz>

This is not a stable part of the API. Use at your own risk.
"""

import os
import subprocess
import sys

from .utils import to_text_string

# Get the system name
WINDOWS = sys.platform.startswith("win")
CYGWIN = sys.platform.startswith("cygwin")
MACOS = sys.platform.startswith("darwin")

GISBASE = None


def Popen(cmd, **kwargs):  # pylint: disable=C0103
    """Wrapper for subprocess.Popen to deal with platform-specific issues"""
    if WINDOWS:
        kwargs["shell"] = True
    return subprocess.Popen(cmd, **kwargs)


def set_gisbase(path, /):
    global GISBASE
    GISBASE = path


def gpath(*args):
    """Construct path to file or directory in GRASS GIS installation

    Can be called only after GISBASE was set.
    """
    return os.path.join(GISBASE, *args)


def wxpath(*args):
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


def path_prepend(directory, var):
    path = os.getenv(var)
    if path:
        path = directory + os.pathsep + path
    else:
        path = directory
    os.environ[var] = path


def set_paths(
    grass_config_dir, major_version, minor_version, ld_library_path_variable_name
):
    # addons (path)
    addon_path = os.getenv("GRASS_ADDON_PATH")
    if addon_path:
        for path in addon_path.split(os.pathsep):
            path_prepend(addon_path, "PATH")

    # addons (base)
    addon_base = os.getenv("GRASS_ADDON_BASE")
    if not addon_base:
        if MACOS:
            version = f"{major_version}.{minor_version}"
            addon_base = os.path.join(
                os.getenv("HOME"), "Library", "GRASS", version, "Addons"
            )
        else:
            addon_base = os.path.join(grass_config_dir, "addons")
        os.environ["GRASS_ADDON_BASE"] = addon_base
    if not WINDOWS:
        path_prepend(os.path.join(addon_base, "scripts"), "PATH")
    path_prepend(os.path.join(addon_base, "bin"), "PATH")

    # standard installation
    if not WINDOWS:
        path_prepend(gpath("scripts"), "PATH")
    path_prepend(gpath("bin"), "PATH")

    # set path for the GRASS man pages
    grass_man_path = gpath("docs", "man")
    addons_man_path = os.path.join(addon_base, "docs", "man")
    man_path = os.getenv("MANPATH")
    sys_man_path = None
    if man_path:
        path_prepend(addons_man_path, "MANPATH")
        path_prepend(grass_man_path, "MANPATH")
    else:
        try:
            # TODO: use higher level API
            nul = open(os.devnull, "w")
            p = Popen(["manpath"], stdout=subprocess.PIPE, stderr=nul)
            nul.close()
            s = p.stdout.read()
            p.wait()
            sys_man_path = s.strip()
        except FileNotFoundError:
            pass

        if sys_man_path:
            os.environ["MANPATH"] = to_text_string(sys_man_path)
            path_prepend(addons_man_path, "MANPATH")
            path_prepend(grass_man_path, "MANPATH")
        else:
            os.environ["MANPATH"] = to_text_string(addons_man_path)
            path_prepend(grass_man_path, "MANPATH")

    # Set LD_LIBRARY_PATH (etc) to find GRASS shared libraries
    # this works for subprocesses but won't affect the current process
    if ld_library_path_variable_name:
        path_prepend(gpath("lib"), ld_library_path_variable_name)


def find_exe(pgm):
    for directory in os.getenv("PATH").split(os.pathsep):
        path = os.path.join(directory, pgm)
        if os.access(path, os.X_OK):
            return path
    return None


def set_defaults(config_projshare_path):
    # GRASS_PAGER
    if not os.getenv("GRASS_PAGER"):
        if find_exe("more"):
            pager = "more"
        elif find_exe("less"):
            pager = "less"
        elif WINDOWS:
            pager = "more"
        else:
            pager = "cat"
        os.environ["GRASS_PAGER"] = pager

    # GRASS_PYTHON
    if not os.getenv("GRASS_PYTHON"):
        if WINDOWS:
            os.environ["GRASS_PYTHON"] = "python3.exe"
        else:
            os.environ["GRASS_PYTHON"] = "python3"

    # GRASS_GNUPLOT
    if not os.getenv("GRASS_GNUPLOT"):
        os.environ["GRASS_GNUPLOT"] = "gnuplot -persist"

    # GRASS_PROJSHARE
    if not os.getenv("GRASS_PROJSHARE") and config_projshare_path:
        os.environ["GRASS_PROJSHARE"] = config_projshare_path


def set_display_defaults():
    """Predefine monitor size for certain architectures"""
    if os.getenv("HOSTTYPE") == "arm":
        # small monitor on ARM (iPAQ, zaurus... etc)
        os.environ["GRASS_RENDER_HEIGHT"] = "320"
        os.environ["GRASS_RENDER_WIDTH"] = "240"


def set_browser():
    # GRASS_HTML_BROWSER
    browser = os.getenv("GRASS_HTML_BROWSER")
    if not browser:
        if MACOS:
            # OSX doesn't execute browsers from the shell PATH - route through a
            # script
            browser = gpath("etc", "html_browser_mac.sh")
            os.environ["GRASS_HTML_BROWSER_MACOSX"] = "-b com.apple.helpviewer"

        if WINDOWS:
            browser = "start"
        elif CYGWIN:
            browser = "explorer"
        else:
            # the usual suspects
            browsers = [
                "xdg-open",
                "x-www-browser",
                "htmlview",
                "konqueror",
                "mozilla",
                "mozilla-firefox",
                "firefox",
                "iceweasel",
                "opera",
                "google-chrome",
                "chromium",
                "netscape",
                "dillo",
                "lynx",
                "links",
                "w3c",
            ]
            for b in browsers:
                if find_exe(b):
                    browser = b
                    break

    elif MACOS:
        # OSX doesn't execute browsers from the shell PATH - route through a
        # script
        os.environ["GRASS_HTML_BROWSER_MACOSX"] = "-b %s" % browser
        browser = gpath("etc", "html_browser_mac.sh")

    if not browser:
        # even so we set to 'xdg-open' as a generic fallback
        browser = "xdg-open"

    os.environ["GRASS_HTML_BROWSER"] = browser


def ensure_home():
    """Set HOME if not set on MS Windows"""
    if WINDOWS and not os.getenv("HOME"):
        os.environ["HOME"] = os.path.join(os.getenv("HOMEDRIVE"), os.getenv("HOMEPATH"))
