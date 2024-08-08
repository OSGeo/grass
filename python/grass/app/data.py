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
import tempfile
import getpass
import subprocess
import sys
from shutil import copytree, ignore_patterns
from pathlib import Path
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


def lock_mapset(install_path, mapset_path, force_gislock_removal, user):
    """Lock the mapset and return name of the lock file

    Behavior on error must be changed somehow; now it fatals but GUI case is
    unresolved.
    """
    if not os.path.exists(mapset_path):
        raise MapsetLockingException(_("Path '%s' doesn't exist") % mapset_path)
    if not os.access(mapset_path, os.W_OK):
        error = _("Path '%s' not accessible.") % mapset_path
        stat_info = os.stat(mapset_path)
        mapset_uid = stat_info.st_uid
        if mapset_uid != os.getuid():
            error = "%s\n%s" % (
                error,
                _("You are not the owner of '%s'.") % mapset_path,
            )
        raise MapsetLockingException(error)
    # Check for concurrent use
    lockfile = os.path.join(mapset_path, ".gislock")
    install_path = Path(install_path)
    ret = subprocess.run(
        [install_path / "etc" / "lock", lockfile, "%d" % os.getpid()], check=False
    ).returncode
    msg = None
    if ret == 2:
        if not force_gislock_removal:
            lockfile = Path(lockfile)
            msg = _(
                "{user} is currently running GRASS in selected mapset"
                " (file {file} found). Concurrent use not allowed.\n"
                "You can force launching GRASS using -f flag"
                " (note that you need permission for this operation)."
                " Have another look in the processor "
                "manager just to be sure...".format(
                    user=lockfile.owner(), file=lockfile
                )
            )
        else:
            try_remove(lockfile)
            message(
                _(
                    "%(user)s is currently running GRASS in selected mapset"
                    " (file %(file)s found). Forcing to launch GRASS..."
                    % {"user": user, "file": lockfile}
                )
            )
    elif ret != 0:
        msg = (
            _("Unable to properly access '%s'.\nPlease notify system personnel.")
            % lockfile
        )

    if msg:
        raise MapsetLockingException(msg)
    return lockfile
