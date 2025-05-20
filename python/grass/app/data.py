"""Provides functions for the main GRASS GIS executable

(C) 2020-2025 by Vaclav Petras and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Vaclav Petras <wenzeslaus gmail com>
.. sectionauthor:: Linda Kladivova <l.kladivova seznam cz>

This is not a stable part of the API. Use at your own risk.
"""

import os
import tempfile
import getpass
import subprocess
import sys
import time
from shutil import copytree, ignore_patterns
from pathlib import Path

import grass.script as gs
import grass.grassdb.config as cfg

from grass.grassdb.checks import is_location_valid


def get_possible_database_path():
    """Looks for directory 'grassdata' (case-insensitive) in standard
    locations to detect existing GRASS Database.

    Returns the path as a string or None if nothing was found.
    """
    home = os.path.expanduser("~")

    # try some common directories for grassdata
    candidates = [
        home,
        os.path.join(home, "Documents"),
    ]

    # find possible database path
    for candidate in candidates:
        if os.path.exists(candidate):
            for subdir in next(os.walk(candidate))[1]:
                if "grassdata" in subdir.lower():
                    return os.path.join(candidate, subdir)
    return None


def create_database_directory():
    """Creates the standard GRASS GIS directory.
    Creates database directory named grassdata in the standard location
    according to the platform.

    Returns the new path as a string or None if nothing was found or created.
    """
    home = os.path.expanduser("~")

    # Determine the standard path according to the platform
    if sys.platform == "win32":
        path = os.path.join(home, "Documents", "grassdata")
    else:
        path = os.path.join(home, "grassdata")

    # Create "grassdata" directory
    try:
        os.mkdir(path)
        return path
    except OSError:
        pass

    # Create a temporary "grassdata" directory if GRASS is running
    # in some special environment and the standard directories
    # cannot be created which might be the case in some "try out GRASS"
    # use cases.
    path = os.path.join(tempfile.gettempdir(), "grassdata_{}".format(getpass.getuser()))

    # The created tmp is not cleaned by GRASS, so we are relying on
    # the system to do it at some point. The positive outcome is that
    # another GRASS instance will find the data created by the first
    # one which is desired in the "try out GRASS" use case we are
    # aiming towards."
    if os.path.exists(path):
        return path
    try:
        os.mkdir(path)
        return path
    except OSError:
        pass

    return None


def _get_startup_location_in_distribution():
    """Check for startup location directory in distribution.

    Returns startup location if found or None if nothing was found.
    """
    gisbase = os.getenv("GISBASE")
    startup_location = os.path.join(gisbase, "demolocation")

    # Find out if startup location exists
    if os.path.exists(startup_location):
        return startup_location
    return None


def _copy_startup_location(startup_location, location_in_grassdb):
    """Copy the simple startup_location with some data to GRASS database.

    Returns True if successfully copied or False
    when an error was encountered.
    """
    # Copy source startup location into GRASS database
    try:
        copytree(
            startup_location,
            location_in_grassdb,
            ignore=ignore_patterns("*.tmpl", "Makefile*"),
        )
        return True
    except OSError:
        pass
    return False


def create_startup_location_in_grassdb(grassdatabase, startup_location_name) -> bool:
    """Create a new startup location in the given GRASS database.

    Returns True if a new startup location successfully created
    in the given GRASS database.
    Returns False if there is no location to copy in the installation
    or copying failed.
    """
    # Find out if startup location exists
    startup_location = _get_startup_location_in_distribution()
    if not startup_location:
        return False

    # Copy the simple startup_location with some data to GRASS database
    location_in_grassdb = os.path.join(grassdatabase, startup_location_name)
    return bool(_copy_startup_location(startup_location, location_in_grassdb))


def ensure_default_data_hierarchy():
    """Ensure that default gisdbase, location and mapset exist.
    Creates database directory based on the default path determined
    according to OS if needed. Creates location if needed.

    Returns the db, loc, mapset, mapset_path"""

    gisdbase = get_possible_database_path()
    location = cfg.default_location
    mapset = cfg.permanent_mapset

    # If nothing found, try to create GRASS directory
    if not gisdbase:
        gisdbase = create_database_directory()

    if not is_location_valid(gisdbase, location):
        # If not valid, copy startup loc
        create_startup_location_in_grassdb(gisdbase, location)

    mapset_path = os.path.join(gisdbase, location, mapset)

    return gisdbase, location, mapset, mapset_path


class MapsetLockingException(Exception):
    pass


def acquire_mapset_lock(
    mapset_path,
    *,
    process_id=None,
    timeout=30,
    initial_sleep=1,
    message_callback=None,
    env=None,
):
    """
    Lock a mapset and return lock process return code and name of new lock file

    Acquires a lock for a mapset by calling the lock program. Returns lock process
    return code and name of new lock file.

    When locking fails, it will re-attempt locking again until it succeeds
    or it times out. The initial sleep time is used the first time and then
    it is doubled after each failed attempt. However, the sleep time is limited
    to thousand times the initial sleep time, so for longer timeouts, the attempts
    will start happening in equal intervals.

    A *timeout* is the maximum time to wait for the lock to be released in seconds.

    A *process_id* is the process ID of the process locking the mapset. If not
    given, the current process ID is used.

    :param mapset_path: full path to the mapset
    :param process_id: process id to use for locking
    :param timeout: give up in *timeout* in seconds
    :param initial_sleep: initial sleep time in seconds
    :param message_callback: callback to show messages when locked
    :param env: system environment variables

    The function assumes the `GISBASE` variable is in the environment. The variable is
    used to find the lock program. If *env* is not provided, `os.environ` is used.
    """
    if process_id is None:
        process_id = os.getpid()
    if not env:
        env = os.environ
    lock_file = os.path.join(mapset_path, ".gislock")
    locker_path = os.path.join(env["GISBASE"], "etc", "lock")
    total_sleep = 0
    try_number = 0
    initial_sleep = min(initial_sleep, timeout)
    sleep_time = initial_sleep
    start_time = time.time()
    while True:
        return_code = subprocess.run(
            [locker_path, lock_file, f"{process_id}"], check=False
        ).returncode
        elapsed_time = time.time() - start_time
        if return_code == 0 or elapsed_time >= timeout or total_sleep >= timeout:
            # If we successfully acquired the lock or we did our last attempt,
            # stop the loop and report whatever the result is.
            break
        try_number += 1
        if message_callback:
            # Show project and mapset name, but not the whole path.
            display_mapset = Path("...").joinpath(*(Path(mapset_path).parts[-2:]))
            message_callback(
                _(
                    "Mapset <{mapset}> locked "
                    "(attempt {try_number}), "
                    "but will retry in {sleep_time:.2f} seconds..."
                ).format(
                    mapset=display_mapset,
                    try_number=try_number,
                    sleep_time=sleep_time,
                )
            )
        time.sleep(sleep_time)
        total_sleep += sleep_time
        # New sleep time as double of the old one, but limited by the initial one.
        # Don't sleep longer than the timeout allows.
        sleep_time = min(
            2 * sleep_time,
            1000 * initial_sleep,
            timeout - elapsed_time,
            timeout - total_sleep,
        )
    return return_code, lock_file


def lock_mapset(
    mapset_path,
    *,
    force_lock_removal,
    timeout,
    message_callback,
    process_id=None,
    env=None,
):
    """Acquire a lock for a mapset and return name of new lock file

    Raises MapsetLockingException when it is not possible to acquire a lock for the
    given mapset either because of existing lock or due to insufficient permissions.
    A corresponding localized message is given in the exception.

    The *timeout*, *process_id*, and *env* parameters are the same as for the
    :func:`acquire_mapset_lock` function. *force_lock_removal* implies zero *timeout*.

    A *message_callback* is a function which will be called to report messages about
    certain states. Specifically, the function is called when forcibly unlocking the
    mapset.

    Assumes that the runtime is set up (specifically that GISBASE is in
    the environment). Environment can be provided as *env*.
    """
    if process_id is None:
        process_id = os.getpid()
    if not env:
        env = os.environ
    if not os.path.exists(mapset_path):
        raise MapsetLockingException(_("Path '{}' doesn't exist").format(mapset_path))
    if not os.access(mapset_path, os.W_OK):
        error = _("Path '{}' not accessible.").format(mapset_path)
        stat_info = Path(mapset_path).stat()
        mapset_uid = stat_info.st_uid
        if mapset_uid != os.getuid():
            error = "{error}\n{detail}".format(
                error=error,
                detail=_("You are not the owner of '{}'.").format(mapset_path),
            )
        raise MapsetLockingException(error)
    if force_lock_removal:
        # Do not wait when removing the lock anyway.
        timeout = 0
    ret, lockfile = acquire_mapset_lock(
        mapset_path,
        timeout=timeout,
        message_callback=message_callback,
        process_id=process_id,
        env=env,
    )
    msg = None
    if ret == 2:
        lock_info = _("File {file} owned by {user} found.").format(
            user=Path(lockfile).owner(), file=lockfile
        )
        if not force_lock_removal:
            cli_solution = _(
                "On the command line, you can force GRASS to start by using the -f flag."
            )
            python_solution = _(
                "In Python, you can start a session with force_unlock=True."
            )
            msg = _(
                "Selected mapset is currently being used by another GRASS session. "
                "{lock_info} "
                "Concurrent access to a mapset is not allowed. However, you can either "
                "use another mapset within the same project or remove the lock if you "
                "are sure that no other session is active.\n"
                "{cli_solution}\n"
                "{python_solution}\n"
                "Make sure you have sufficient access permissions to remove the lock "
                "file. You may want to use a process manager "
                "to check that no other process is using the mapset."
            ).format(
                lock_info=lock_info,
                cli_solution=cli_solution,
                python_solution=python_solution,
            )
        else:
            message_callback(
                _("Removing lock in the selected mapset: {lock_info}").format(
                    lock_info=lock_info
                )
            )
            gs.try_remove(lockfile)
    elif ret != 0:
        msg = _(
            "Unable to properly access lock file '{name}' or run the lock program.\n"
            "Please resolve this with your system administrator. "
            "(Lock program return code: {returncode})"
        ).format(name=lockfile, returncode=ret)

    if msg:
        raise MapsetLockingException(msg)
    return lockfile


def unlock_mapset(mapset_path):
    """Unlock a mapset"""
    lockfile = os.path.join(mapset_path, ".gislock")
    gs.try_remove(lockfile)
