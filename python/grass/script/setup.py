R"""Setup, initialization, and clean-up functions

Functions can be used in Python scripts to setup a GRASS environment
and session without using grassXY.

Usage::

    import os
    import sys
    import subprocess

    # define GRASS Database
    # add your path to grassdata (GRASS GIS database) directory
    gisdb = "~/grassdata"
    # the following path is the default path on MS Windows
    # gisdb = "~/Documents/grassdata"

    # specify (existing) Location and Mapset
    location = "nc_spm_08"
    mapset = "user1"

    # path to the GRASS GIS launch script
    # we assume that the GRASS GIS start script is available and on PATH
    # query GRASS itself for its GISBASE
    # (with fixes for specific platforms)
    # needs to be edited by the user
    executable = "grass"
    if sys.platform.startswith("win"):
        # MS Windows
        executable = r"C:\OSGeo4W\bin\grass.bat"
        # uncomment when using standalone WinGRASS installer
        # executable = r'C:\Program Files (x86)\GRASS GIS <version>\grass.bat'
        # this can be skipped if GRASS executable is added to PATH
    elif sys.platform == "darwin":
        # Mac OS X
        version = "@GRASS_VERSION_MAJOR@.@GRASS_VERSION_MINOR@"
        executable = f"/Applications/GRASS-{version}.app/Contents/Resources/bin/grass"

    # query GRASS GIS itself for its Python package path
    grass_cmd = [executable, "--config", "python_path"]
    process = subprocess.run(grass_cmd, check=True, text=True, stdout=subprocess.PIPE)

    # define GRASS-Python environment
    sys.path.append(process.stdout.strip())

    # import (some) GRASS Python bindings
    import grass.script as gs

    # launch session
    session = gs.setup.init(gisdb, location, mapset)

    # example calls
    gs.message("Current GRASS GIS 8 environment:")
    print(gs.gisenv())

    gs.message("Available raster maps:")
    for rast in gs.list_strings(type="raster"):
        print(rast)

    gs.message("Available vector maps:")
    for vect in gs.list_strings(type="vector"):
        print(vect)

    # clean up at the end
    session.finish()


(C) 2010-2024 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com>
@author Markus Metz
"""

# TODO: this should share code from lib/init/grass.py
# perhaps grass.py can import without much trouble once GISBASE
# is known, this would allow moving things from there, here
# then this could even do locking

from pathlib import Path
import datetime
import os
import shutil
import subprocess
import sys
import tempfile as tmpfile

WINDOWS = sys.platform.startswith("win")
MACOS = sys.platform.startswith("darwin")

VERSION_MAJOR = "@GRASS_VERSION_MAJOR@"
VERSION_MINOR = "@GRASS_VERSION_MINOR@"


def write_gisrc(dbase, location, mapset):
    """Write the ``gisrc`` file and return its path."""
    with tmpfile.NamedTemporaryFile(mode="w", delete=False) as rc:
        gisrc = rc.name
        rc.write("GISDBASE: %s\n" % dbase)
        rc.write("LOCATION_NAME: %s\n" % location)
        rc.write("MAPSET: %s\n" % mapset)
    return gisrc


def set_gui_path():
    """Insert wxPython GRASS path to sys.path."""
    gui_path = os.path.join(os.environ["GISBASE"], "gui", "wxpython")
    if gui_path and gui_path not in sys.path:
        sys.path.insert(0, gui_path)


def get_install_path(path=None):
    """Get path to GRASS installation usable for setup of environmental variables.

    The function tries to determine path tp GRASS GIS installation so that the
    returned path can be used for setup of environmental variable for GRASS runtime.
    If the search fails, None is returned.

    By default, the resulting path is derived relatively from the location of the
    Python package (specifically this module) in the file system. This derived path
    is returned only if it has subdirectories called ``bin`` and ``lib``.
    If the parameter or certain environmental variables are set, the following
    attempts are made to find the path.

    If *path* is provided and it is an existing executable, the executable is queried
    for the path. Otherwise, provided *path* is returned as is.

    If *path* is not provided, the GISBASE environmental variable is used as the path
    if it exists. If GRASSBIN environmental variable exists and it is an existing
    executable, the executable is queried for the path.

    If *path* is not provided and no relevant environmental variables are set, the
    default relative path search is performed.
    If that fails and executable called ``grass`` exists, it is queried for the path.
    None is returned if all the attempts failed.

    If an existing executable is called as a subprocess is called during the search
    and it fails, the CalledProcessError exception is propagated from the subprocess
    call.
    """

    def ask_executable(arg):
        """Query the GRASS executable for the path"""
        return subprocess.run(
            [arg, "--config", "path"], text=True, check=True, capture_output=True
        ).stdout.strip()

    # Executable was provided as parameter.
    if path and shutil.which(path):
        # The path was provided by the user and it is an executable
        # (on path or provided with full path), so raise exception on failure.
        return ask_executable(path)

    # Presumably directory was provided.
    if path:
        return path

    # GISBASE is already set.
    env_gisbase = os.environ.get("GISBASE")
    if env_gisbase:
        return env_gisbase

    # Executable provided in environment (name is from grass-session).
    # The variable is supported (here), documented, but not widely promoted
    # at this point (to be re-evaluated).
    grass_bin = os.environ.get("GRASSBIN")
    if grass_bin and shutil.which(grass_bin):
        return ask_executable(grass_bin)

    # Derive the path from path to this file (Python module).
    # This is the standard way when there is no user-provided settings.
    # Uses relative path to find the right parent and then tests presence of lib
    # and bin. Removing 5 parts from the path works for
    # .../grass_install_prefix/etc/python/grass and also .../python3/dist-packages/.
    install_path = Path(*Path(__file__).parts[:-5])
    bin_path = install_path / "bin"
    lib_path = install_path / "lib"
    if bin_path.is_dir() and lib_path.is_dir():
        return install_path

    # As a last resort, try running grass command if it exists.
    # This is less likely give the right result than the relative path on systems
    # with multiple installations (where an explicit setup is likely required).
    # However, it allows for non-standard installations with standard command.
    grass_bin = "grass"
    if grass_bin and shutil.which(grass_bin):
        return ask_executable(grass_bin)

    return None


def setup_runtime_env(gisbase=None, *, env=None):
    """Setup the runtime environment.

    Modifies environment so that GRASS modules can run. It does not setup a session,
    but only the system environment to execute commands.

    Modifies the environment provided with _env_. If _env_ is not
    provided, modifies the global environment (os.environ). Pass a copy of the
    environment if you don't want the source environment modified.

    If _gisbase_ is not provided, a heuristic is used to find the path to GRASS
    installation (see the :func:`get_install_path` function for details).
    """
    if not gisbase:
        gisbase = get_install_path()

    # Accept Path objects.
    gisbase = os.fspath(gisbase)

    # If environment is not provided, use the global one.
    if not env:
        env = os.environ

    # Set GISBASE
    env["GISBASE"] = gisbase

    # define PATH
    path_addition = os.pathsep + os.path.join(gisbase, "bin")
    path_addition += os.pathsep + os.path.join(gisbase, "scripts")
    if WINDOWS:
        path_addition += os.pathsep + os.path.join(gisbase, "extrabin")

    # add addons to the PATH, use GRASS_ADDON_BASE if set
    # copied and simplified from lib/init/grass.py
    addon_base = env.get("GRASS_ADDON_BASE")
    if not addon_base:
        if WINDOWS:
            config_dirname = f"GRASS{VERSION_MAJOR}"
            addon_base = os.path.join(env.get("APPDATA"), config_dirname, "addons")
        elif MACOS:
            version = f"{VERSION_MAJOR}.{VERSION_MINOR}"
            addon_base = os.path.join(
                env.get("HOME"), "Library", "GRASS", version, "Addons"
            )
        else:
            config_dirname = f".grass{VERSION_MAJOR}"
            addon_base = os.path.join(env.get("HOME"), config_dirname, "addons")
        env["GRASS_ADDON_BASE"] = addon_base

    if not WINDOWS:
        path_addition += os.pathsep + os.path.join(addon_base, "scripts")
    path_addition += os.pathsep + os.path.join(addon_base, "bin")

    env["PATH"] = path_addition + os.pathsep + env.get("PATH")

    # define LD_LIBRARY_PATH
    if "@LD_LIBRARY_PATH_VAR@" not in env:
        env["@LD_LIBRARY_PATH_VAR@"] = ""
    env["@LD_LIBRARY_PATH_VAR@"] += os.pathsep + os.path.join(gisbase, "lib")

    # Set GRASS_PYTHON and PYTHONPATH to find GRASS Python modules
    if not env.get("GRASS_PYTHON"):
        if WINDOWS:
            env["GRASS_PYTHON"] = "python3.exe"
        else:
            env["GRASS_PYTHON"] = "python3"

    path = env.get("PYTHONPATH")
    etcpy = os.path.join(gisbase, "etc", "python")
    if path:
        path = etcpy + os.pathsep + path
    else:
        path = etcpy
    env["PYTHONPATH"] = path


def init(path, location=None, mapset=None, *, grass_path=None, env=None):
    """Initialize system variables to run GRASS modules

    This function is for running GRASS GIS without starting it with the
    standard main executable grass. No GRASS modules shall be called before
    call of this function but any module or user script can be called
    afterwards because a GRASS session has been set up. GRASS Python
    libraries are usable as well in general but the ones using C
    libraries through ``ctypes`` are not (which is caused by library
    path not being updated for the current process which is a common
    operating system limitation).

    When the path or specified mapset does not exist, ValueError is raised.

    The :func:`get_install_path` function is used to determine where
    the rest of GRASS files is installed. The *grass_path* parameter is
    passed to it if provided. If the path cannot be determined,
    ValueError is raised. Exceptions from the underlying function are propagated.

    To create a GRASS session a session file (aka gisrc file) is created.
    The session object returned by this function will take care of deleting it
    as long as the object is used as a context manager or the *finish* method
    of the object is called explicitly. Using methods of the session object is
    preferred over calling the function :func:`finish`.

    Basic usage::

        # ... setup GISBASE and sys.path before import
        import grass.script as gs
        session = gs.setup.init(
            "~/grassdata/nc_spm_08/user1",
            grass_path="/usr/lib/grass",
        )
        # ... use GRASS modules here
        # end the session
        session.finish()

    The returned object is a context manager, so the ``with`` statement can be used to
    ensure that the session is finished (closed) at the end::

        # ... setup sys.path before import
        import grass.script as gs
        with gs.setup.init("~/grassdata/nc_spm_08/user1")
            # ... use GRASS modules here

    :param path: path to GRASS database
    :param location: location name
    :param mapset: mapset within given location (default: 'PERMANENT')
    :param grass_path: path to GRASS installation or executable

    :returns: reference to a session handle object which is a context manager
    """
    # The path heuristic always uses the global environment.
    grass_path = get_install_path(grass_path)
    if not grass_path:
        raise ValueError(
            _("Parameter grass_path or GISBASE environmental variable must be set")
        )
    # We reduce the top-level imports because this is initialization code.
    # pylint: disable=import-outside-toplevel
    from grass.grassdb.checks import get_mapset_invalid_reason, is_mapset_valid
    from grass.grassdb.manage import resolve_mapset_path

    # Support ~ in the path for user home directory.
    path = Path(path).expanduser()
    # A simple existence test. The directory, whatever it is, should exist.
    if not path.exists():
        raise ValueError(_("Path '{path}' does not exist").format(path=path))
    # A specific message when it exists, but it is a file.
    if path.is_file():
        raise ValueError(
            _("Path '{path}' is a file, but a directory is needed").format(path=path)
        )
    mapset_path = resolve_mapset_path(path=path, location=location, mapset=mapset)
    if not is_mapset_valid(mapset_path):
        raise ValueError(
            _("Mapset {path} is not valid: {reason}").format(
                path=mapset_path.path,
                reason=get_mapset_invalid_reason(
                    mapset_path.directory, mapset_path.location, mapset_path.mapset
                ),
            )
        )

    # If environment is not provided, use the global one.
    if not env:
        env = os.environ
    setup_runtime_env(grass_path, env=env)

    # TODO: lock the mapset?
    env["GIS_LOCK"] = str(os.getpid())

    env["GISRC"] = write_gisrc(
        mapset_path.directory, mapset_path.location, mapset_path.mapset
    )
    return SessionHandle(env=env)


class SessionHandle:
    """Object used to manage GRASS sessions.

    Do not create objects of this class directly. Use the *init* function
    to get a session object.

    Basic usage::

        # ... setup sys.path before import as needed

        import grass.script as gs

        session = gs.setup.init("~/grassdata/nc_spm_08/user1")

        # ... use GRASS modules here

        # end the session
        session.finish()

    Context manager usage::

        # ... setup sys.path before import as needed

        import grass.script as gs

        with gs.setup.init("~/grassdata/nc_spm_08/user1"):
            # ... use GRASS modules here
        # session ends automatically here

    The example above is modifying the global, process environment (`os.environ`).
    If you don't want to modify the global environment, use the _env_ parameter
    for the _init_ function to modify the provided environment instead.
    This environment is then available as an attribute of the session object.
    The attribute then needs to be passed to all calls of GRASS
    tools and functions that wrap them.
    Context manager usage with custom environment::

        # ... setup sys.path before import as needed

        import grass.script as gs

        with gs.setup.init("~/grassdata/nc_spm_08/user1", env=os.environ.copy()):
            # ... use GRASS modules here with env parameter
            gs.run_command("g.region", flags="p", env=session.env)
        # session ends automatically here, global environment was never modifed
    """

    def __init__(self, *, env, active=True):
        self._env = env
        self._active = active
        self._start_time = datetime.datetime.now(datetime.timezone.utc)

    @property
    def active(self):
        """True if session is active (not finished)"""
        return self._active

    @property
    def env(self):
        return self._env

    def __enter__(self):
        """Enter the context manager context.

        Notably, the session is activated using the *init* function.

        :returns: reference to the object (self)
        """
        if not self.active:
            raise ValueError(
                "Attempt to use inactive (finished) session as a context manager"
            )
        return self

    def __exit__(self, type, value, traceback):
        """Exit the context manager context.

        Finishes the existing session.
        """
        self.finish()

    def finish(self):
        """Finish the session.

        If not used as a context manager, call explicitly to clean and close the mapset
        and finish the session. No GRASS modules can be called afterwards.
        """
        if not self.active:
            raise ValueError("Attempt to finish an already finished session")
        self._active = False
        finish(env=self._env, start_time=self._start_time)


# clean-up functions when terminating a GRASS session
# these fns can only be called within a valid GRASS session


def clean_default_db(*, modified_after=None, env=None):
    """Clean (vacuum) the default db if it is SQLite

    When *modified_after* is set, database is cleaned only when it was modified
    since the *modified_after* time.
    """
    # Limiting usage of in other function by lazy-imports.
    # pylint: disable=import-outside-toplevel
    import grass.script as gs

    conn = gs.db_connection(env=env)
    if not conn or conn["driver"] != "sqlite":
        return
    # check if db exists
    gis_env = gs.gisenv(env=env)
    database = conn["database"]
    database = database.replace("$GISDBASE", gis_env["GISDBASE"])
    database = database.replace("$LOCATION_NAME", gis_env["LOCATION_NAME"])
    database = database.replace("$MAPSET", gis_env["MAPSET"])
    database = Path(database)
    if not database.is_file():
        return
    file_stat = database.stat()
    # Small size based on MEMORYMB (MiB) or its default.
    small_db_size = int(gis_env.get("MEMORYMB", 300)) * (1 << 20)
    if file_stat.st_size <= small_db_size:
        return
    if modified_after:
        modified_time = datetime.datetime.fromtimestamp(
            file_stat.st_mtime, tz=datetime.timezone.utc
        )
        if modified_after >= modified_time:
            return
    # Start the vacuum process, then show the message in parallel while
    # the vacuum is running. Finally, wait for the vacuum process to finish.
    # Error handling is the same as errors="ignore".
    process = gs.start_command("db.execute", sql="VACUUM", env=env)
    gs.verbose(_("Cleaning up SQLite attribute database..."), env=env)
    process.wait()


def call(cmd, **kwargs):
    """Wrapper for subprocess.call to deal with platform-specific issues"""
    if WINDOWS:
        kwargs["shell"] = True
    return subprocess.call(cmd, **kwargs)


def clean_temp(env=None):
    """Clean mapset temporary directory"""
    # Lazy-importing to reduce dependencies (this can be eventually removed).
    # pylint: disable=import-outside-toplevel
    import grass.script as gs

    if not env:
        env = os.environ

    gs.verbose(_("Cleaning up temporary files..."), env=env)
    gisbase = env["GISBASE"]
    call(
        [os.path.join(gisbase, "etc", "clean_temp")], stdout=subprocess.DEVNULL, env=env
    )


def finish(*, env=None, start_time=None):
    """Terminate the GRASS session and clean up

    GRASS commands can no longer be used after this function has been
    called

    Basic usage::
        import grass.script as gs

        gs.setup.finish()

    The function is not completely symmetrical with :func:`init` because it only
    closes the mapset, but doesn't undo the runtime environment setup.

    When *start_time* is set, it might be used to determine cleaning procedures.
    Currently, it is used to do SQLite database vacuum only when database was modified
    since the session started.
    """
    if not env:
        env = os.environ

    clean_default_db(modified_after=start_time, env=env)
    clean_temp(env=env)
    # TODO: unlock the mapset?
    # unset the GISRC and delete the file
    from grass.script import utils as gutils

    gutils.try_remove(env["GISRC"])
    del env["GISRC"]
    # remove gislock env var (not the gislock itself
    del env["GIS_LOCK"]
