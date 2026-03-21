"""Provides functions for the main GRASS executable

(C) 2024-2025 by Vaclav Petras and the GRASS Development Team

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
from pathlib import Path

from . import resource_paths

# Get the system name
WINDOWS = sys.platform.startswith("win")
CYGWIN = sys.platform.startswith("cygwin")
MACOS = sys.platform.startswith("darwin")


class RuntimePaths:
    """Get runtime paths to resources and basic GRASS build properties

    The resource paths are accessible as attributes (e.g., `.gisbase`, `.etc_dir`)
    and can optionally be exported to environment variables.

    Example:

    >>> paths = RuntimePaths(set_env_variables=True)
    >>> paths.etc_dir
    '/usr/lib/grass/etc'
    >>> os.environ["GRASS_ETCDIR"]
    '/usr/lib/grass/etc'
    """

    # Mapping of attribute names to environment variable name except the prefix.
    _env_vars = {
        "gisbase": "GISBASE",
    }

    def __init__(self, *, env=None, set_env_variables=False, prefix=None):
        if env is None:
            env = os.environ
        self.env = env
        if prefix:
            self._custom_prefix = os.path.normpath(prefix)
            self._custom_prefix = self._custom_prefix.removesuffix(
                resource_paths.GISBASE
            )
        else:
            self._custom_prefix = None
        if set_env_variables:
            self.set_env_variables()

    def set_env_variables(self):
        """Populate all GRASS-related environment variables."""
        self.env["GRASS_PREFIX"] = self.prefix
        for env_var in self._env_vars.values():
            self.env[env_var] = self.__get_dir(env_var, use_env_values=False)

    @property
    def version(self):
        return resource_paths.GRASS_VERSION

    @property
    def version_major(self):
        return resource_paths.GRASS_VERSION_MAJOR

    @property
    def version_minor(self):
        return resource_paths.GRASS_VERSION_MINOR

    @property
    def ld_library_path_var(self):
        return resource_paths.LD_LIBRARY_PATH_VAR

    @property
    def grass_exe_name(self):
        return resource_paths.GRASS_EXE_NAME

    @property
    def grass_version_git(self):
        return resource_paths.GRASS_VERSION_GIT

    @property
    def config_projshare(self):
        return self.env.get("GRASS_PROJSHARE", resource_paths.CONFIG_PROJSHARE)

    @property
    def grass_cmake_config_dir(self):
        return (
            str(Path(self.prefix, resource_paths.GRASS_CMAKE_CONFIG))
            if self.is_cmake_build
            else ""
        )

    @property
    def grass_cmake_module_dir(self):
        return (
            str(Path(self.prefix, resource_paths.GRASS_CMAKE_MODULES))
            if self.is_cmake_build
            else ""
        )

    @property
    def grass_cmake_prefix_path(self):
        return resource_paths.GRASS_CMAKE_PREFIX_PATH

    @property
    def grass_cmake_c_compiler(self):
        return (
            str(Path(resource_paths.GRASS_CMAKE_C_COMPILER))
            if Path(resource_paths.GRASS_CMAKE_C_COMPILER).exists()
            and self.is_cmake_build
            else ""
        )

    @property
    def grass_cmake_cxx_compiler(self):
        return (
            str(Path(resource_paths.GRASS_CMAKE_CXX_COMPILER))
            if Path(resource_paths.GRASS_CMAKE_CXX_COMPILER).exists()
            and self.is_cmake_build
            else ""
        )

    @property
    def is_cmake_build(self):
        return resource_paths.GRASS_CMAKE_MODULES != ""

    @property
    def prefix(self):
        if self._custom_prefix:
            return self._custom_prefix
        return os.path.normpath(resource_paths.GRASS_PREFIX)

    def __getattr__(self, name):
        """Access paths by attributes."""
        if name in self._env_vars:
            env_var = self._env_vars[name]
            return self.__get_dir(env_var)
        msg = f"{type(self).__name__!r} has no attribute {name!r}"
        raise AttributeError(msg)

    def __dir__(self):
        """List both static and dynamic attributes."""
        base_dir = set(super().__dir__())
        dynamic_dir = set(self._env_vars.keys())
        return sorted(base_dir | dynamic_dir)

    def __get_dir(self, env_var, *, use_env_values=True):
        """Get the directory stored in the environmental variable 'env_var'

        If the environmental variable not yet set, it is retrieved and
        set from resource_paths."""
        if use_env_values and env_var in self.env and self.env[env_var]:
            return os.path.normpath(self.env[env_var])
        # Default to path from the installation
        path = getattr(resource_paths, env_var)
        return os.path.normpath(os.path.join(self.prefix, path))


def get_grass_config_dir(*, env):
    """Get configuration directory path

    Determines path of GRASS user configuration directory for the current platform
    using the build-time version information.
    """
    paths = RuntimePaths(env=env)
    return get_grass_config_dir_for_version(
        paths.version_major, paths.version_minor, env=env
    )


def get_grass_config_dir_for_version(major_version, minor_version, *, env):
    """Get configuration directory path for specific version

    Determines path of GRASS user configuration directory for the current platform
    and for the provided version.
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

    if not Path(config_dir).is_dir():
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
        if Path(path).exists():
            paths.appendleft(path)
    else:
        # Without FHS, scripts are separated like in the source code.
        path = os.path.join(install_path, "scripts")
        if Path(path).exists():
            paths.appendleft(path)


def append_left_addon_paths(paths, config_dir, env):
    """Add addons to path"""
    # addons (base)
    addon_base = env.get("GRASS_ADDON_BASE")
    if not addon_base:
        name = "addons" if not MACOS else "Addons"
        addon_base = os.path.join(config_dir, name)
        env["GRASS_ADDON_BASE"] = addon_base

    # Adding the paths is platform-dependent, but we add them regardless of their
    # existence, because they might be created later after the setup is done
    # when installing addons.
    if not WINDOWS:
        script_path = os.path.join(addon_base, "scripts")
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


def set_paths(install_path, grass_config_dir):
    """Set variables with executable paths, library paths, and other paths"""
    set_executable_paths(
        install_path=install_path, grass_config_dir=grass_config_dir, env=os.environ
    )
    # Set LD_LIBRARY_PATH (etc) to find GRASS shared libraries.
    # This works for subprocesses, but won't affect the current process.
    set_dynamic_library_path(
        variable_name=resource_paths.LD_LIBRARY_PATH_VAR,
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
