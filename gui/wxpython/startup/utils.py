"""
@package startup.utils

@brief General GUI-independent utilities for GUI startup of GRASS GIS

(C) 2017-2018 by Vaclav Petras the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail com>

This file should not use (import) anything from GUI code (wx or wxGUI).
This can potentially be part of the Python library (i.e. it needs to
solve the errors etc. in a general manner).
"""


import os
import shutil
import tempfile
import getpass
import sys


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


def get_lockfile_if_present(database, location, mapset):
    """Return path to lock if present, None otherwise

    Returns the path as a string or None if nothing was found, so the
    return value can be used to test if the lock is present.
    """
    lock_name = ".gislock"
    lockfile = os.path.join(database, location, mapset, lock_name)
    if os.path.isfile(lockfile):
        return lockfile
    else:
        return None


def create_mapset(database, location, mapset):
    """Creates a mapset in a specified location"""
    location_path = os.path.join(database, location)
    mapset_path = os.path.join(location_path, mapset)
    # create an empty directory
    os.mkdir(mapset_path)
    # copy DEFAULT_WIND file and its permissions from PERMANENT
    # to WIND in the new mapset
    region_path1 = os.path.join(location_path, "PERMANENT", "DEFAULT_WIND")
    region_path2 = os.path.join(location_path, mapset, "WIND")
    shutil.copy(region_path1, region_path2)
    # set permissions to u+rw,go+r (disabled; why?)
    # os.chmod(os.path.join(database,location,mapset,'WIND'), 0644)


def delete_mapset(database, location, mapset):
    """Deletes a specified mapset"""
    if mapset == "PERMANENT":
        # TODO: translatable or not?
        raise ValueError(
            "Mapset PERMANENT cannot be deleted" " (whole location can be)"
        )
    shutil.rmtree(os.path.join(database, location, mapset))


def delete_location(database, location):
    """Deletes a specified location"""
    shutil.rmtree(os.path.join(database, location))


def rename_mapset(database, location, old_name, new_name):
    """Rename mapset from *old_name* to *new_name*"""
    location_path = os.path.join(database, location)
    os.rename(
        os.path.join(location_path, old_name), os.path.join(location_path, new_name)
    )


def rename_location(database, old_name, new_name):
    """Rename location from *old_name* to *new_name*"""
    os.rename(os.path.join(database, old_name), os.path.join(database, new_name))


def mapset_exists(database, location, mapset):
    """Returns True whether mapset path exists."""
    location_path = os.path.join(database, location)
    mapset_path = os.path.join(location_path, mapset)
    if os.path.exists(mapset_path):
        return True
    return False


def location_exists(database, location):
    """Returns True whether location path exists."""
    location_path = os.path.join(database, location)
    if os.path.exists(location_path):
        return True
    return False


def get_default_mapset_name():
    """Returns default name for mapset."""
    try:
        defaultName = getpass.getuser()
        defaultName.encode("ascii")
    except UnicodeEncodeError:
        # raise error if not ascii (not valid mapset name)
        defaultName = "user"

    return defaultName
