"""Provides functions for the main GRASS GIS executable

(C) 2024 by Vaclav Petras and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Vaclav Petras <wenzeslaus gmail com>

This is not a stable part of the API. Use at your own risk.
"""

import collections
import os
import shutil
import subprocess
import sys

# Get the system name
WINDOWS = sys.platform.startswith("win")
CYGWIN = sys.platform.startswith("cygwin")
MACOS = sys.platform.startswith("darwin")


def get_grass_config_dir(major_version, minor_version, env):
    """Get configuration directory

    Determines path of GRASS GIS user configuration directory.
    """
    if env.get("GRASS_CONFIG_DIR"):
        # use GRASS_CONFIG_DIR environmental variable is defined
        env_dirname = "GRASS_CONFIG_DIR"
    else:
        env_dirname = "APPDATA" if WINDOWS else "HOME"

    config_dir = env.get(env_dirname)
    if config_dir is None:
        msg = (
            f"The {env_dirname} variable is not set, ask your operating system support"
        )
        raise RuntimeError(msg)

    if not os.path.isdir(config_dir):
        msg = (
            f"The {env_dirname} variable points to directory which does"
            " not exist, ask your operating system support"
        )
        raise NotADirectoryError(msg)

    if WINDOWS:
        config_dirname = f"GRASS{major_version}"
    elif MACOS:
        config_dirname = os.path.join(
            "Library", "GRASS", f"{major_version}.{minor_version}"
        )
    else:
        config_dirname = f".grass{major_version}"

    return os.path.join(config_dir, config_dirname)


def append_left_main_executable_paths(paths, install_path):
    """Add executables to PATH"""
    paths.appendleft(os.path.join(install_path, "bin"))
    if WINDOWS:
        # Standalone installer has dependencies which are on path in other cases.
        path = os.path.join(install_path, "extrabin")
        if os.path.exists(path):
            paths.appendleft(path)
    else:
        # Without FHS, scripts are separated like in the source code.
        path = os.path.join(install_path, "scripts")
        if os.path.exists(path):
            paths.appendleft(path)


def append_left_addon_paths(paths, config_dir, env):
    """Add addons to path"""
    # addons (base)
    addon_base = env.get("GRASS_ADDON_BASE")
    if not addon_base:
        name = "addons" if not MACOS else "Addons"
        addon_base = os.path.join(config_dir, name)
        env["GRASS_ADDON_BASE"] = addon_base

    if not WINDOWS:
        script_path = os.path.join(addon_base, "scripts")
        if os.path.exists(script_path):
            paths.appendleft(script_path)
    paths.appendleft(os.path.join(addon_base, "bin"))

    # addons (path)
    addon_path = env.get("GRASS_ADDON_PATH")
    if addon_path:
        for path in addon_path.split(os.pathsep):
            paths.appendleft(path)


def set_executable_paths(install_path, grass_config_dir, env):
    """Add paths with executables to PATH in _env_"""
    paths = collections.deque()
    # Addons
    append_left_addon_paths(paths, grass_config_dir, env=env)
    # Standard installation
    append_left_main_executable_paths(paths, install_path=install_path)

    paths.append(env.get("PATH"))
    env["PATH"] = os.pathsep.join(paths)


def set_paths(install_path, grass_config_dir, ld_library_path_variable_name):
    """Set variables with executable paths, library paths, and other paths"""
    set_executable_paths(
        install_path=install_path, grass_config_dir=grass_config_dir, env=os.environ
    )
    # Set LD_LIBRARY_PATH (etc) to find GRASS shared libraries
    # this works for subprocesses but won't affect the current process
    if ld_library_path_variable_name:
        set_dynamic_library_path(
            variable_name=ld_library_path_variable_name,
            install_path=install_path,
            env=os.environ,
        )
    set_python_path_variable(install_path=install_path, env=os.environ)

    # retrieving second time, but now it is always set
    addon_base = os.getenv("GRASS_ADDON_BASE")
    set_man_path(install_path=install_path, addon_base=addon_base, env=os.environ)
    set_isis()


def set_man_path(install_path, addon_base, env):
    """Set path for the GRASS man pages"""
    grass_man_path = os.path.join(install_path, "docs", "man")
    addons_man_path = os.path.join(addon_base, "docs", "man")
    man_path = env.get("MANPATH")
    paths = collections.deque()
    if man_path:
        paths.appendleft(man_path)
    else:
        system_path = None
        if manpath_executable := shutil.which("manpath"):
            try:
                system_path = subprocess.run(
                    [manpath_executable],
                    text=True,
                    check=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.DEVNULL,
                    timeout=2,
                ).stdout.strip()
            except (OSError, subprocess.SubprocessError):
                pass

        if system_path:
            # Variable does not exist, but the system has the information.
            paths.appendleft(system_path)
    paths.appendleft(addons_man_path)
    paths.appendleft(grass_man_path)
    os.environ["MANPATH"] = os.pathsep.join(paths)


def set_dynamic_library_path(variable_name, install_path, env):
    """Define path to dynamic libraries (LD_LIBRARY_PATH on Linux)"""
    if variable_name not in env:
        env[variable_name] = ""
    env[variable_name] += os.pathsep + os.path.join(install_path, "lib")


def set_python_path_variable(install_path, env):
    """Set PYTHONPATH to find GRASS Python package in subprocesses"""
    path = env.get("PYTHONPATH")
    etcpy = os.path.join(install_path, "etc", "python")
    path = etcpy + os.pathsep + path if path else etcpy
    env["PYTHONPATH"] = path


def set_path_to_python_executable(env):
    """Set GRASS_PYTHON environment variable"""
    if not env.get("GRASS_PYTHON"):
        env["GRASS_PYTHON"] = sys.executable


def set_defaults(config_projshare_path):
    """Set paths or commands for dependencies and auxiliary utilities"""
    # GRASS_PAGER
    if not os.getenv("GRASS_PAGER"):
        if shutil.which("more"):
            pager = "more"
        elif shutil.which("less"):
            pager = "less"
        elif WINDOWS:
            pager = "more"
        else:
            pager = "cat"
        os.environ["GRASS_PAGER"] = pager

    # GRASS_PYTHON
    set_path_to_python_executable(env=os.environ)

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


def set_browser(install_path):
    """Set path to HTML browser"""
    # GRASS_HTML_BROWSER
    browser = os.getenv("GRASS_HTML_BROWSER")
    if not browser:
        if MACOS:
            # OSX doesn't execute browsers from the shell PATH - route through a script
            browser = os.path.join(install_path, "etc", "html_browser_mac.sh")
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
            for candidate in browsers:
                if shutil.which(candidate):
                    browser = candidate
                    break

    elif MACOS:
        # OSX doesn't execute browsers from the shell PATH - route through a script
        os.environ["GRASS_HTML_BROWSER_MACOSX"] = "-b %s" % browser
        browser = os.path.join(install_path, "etc", "html_browser_mac.sh")

    if not browser:
        # even so we set to 'xdg-open' as a generic fallback
        browser = "xdg-open"

    os.environ["GRASS_HTML_BROWSER"] = browser


def set_isis():
    """Enable a mixed ISIS-GRASS environment

    ISIS is Integrated Software for Imagers and Spectrometers by USGS.
    """
    if os.getenv("ISISROOT"):
        isis = os.getenv("ISISROOT")
        os.environ["ISIS_LIB"] = isis + os.sep + "lib"
        os.environ["ISIS_3RDPARTY"] = isis + os.sep + "3rdParty" + os.sep + "lib"
        os.environ["QT_PLUGIN_PATH"] = isis + os.sep + "3rdParty" + os.sep + "plugins"
        # os.environ['ISIS3DATA'] = isis + "$ISIS3DATA"
        libpath = os.getenv("LD_LIBRARY_PATH", "")
        isislibpath = os.getenv("ISIS_LIB")
        isis3rdparty = os.getenv("ISIS_3RDPARTY")
        os.environ["LD_LIBRARY_PATH"] = (
            libpath + os.pathsep + isislibpath + os.pathsep + isis3rdparty
        )


def ensure_home():
    """Set HOME if not set on MS Windows"""
    if WINDOWS and not os.getenv("HOME"):
        os.environ["HOME"] = os.path.join(os.getenv("HOMEDRIVE"), os.getenv("HOMEPATH"))
