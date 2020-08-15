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
import tempfile
import getpass
import sys
from shutil import copytree, ignore_patterns
from grass.grassdb.create import create_mapset, get_default_mapset_name

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


def _copy_startup_location(grassdatabase, startup_location):
    """Copy the simple startup_location with some data to GRASS database.

    Returns True if successfully copied or False when an error was encountered.
    """
    src = startup_location
    dst = os.path.join(grassdatabase, "demolocation")

    # Copy source startup location into GRASS database
    try:
        copytree(src, dst, ignore=ignore_patterns('*.tmpl','Makefile*'))
        return True
    except (IOError, OSError):
        pass
    return False


def _create_startup_mapset(grassdatabase, startup_location):
    """Create the new empty startup mapset named after user.

    Returns True if successfully created or False when an error was encountered.
    """
    name = get_default_mapset_name()
    mapset_path = os.path.join(grassdatabase, startup_location, name)
    counter = 1

    while os.path.exists(mapset_path):
        name = name + "_" + str(counter)
        mapset_path = os.path.join(grassdatabase, startup_location, name)

    # Create new startup mapset
    try:
        create_mapset(grassdatabase, startup_location, name)
        return True
    except (IOError, OSError):
        pass
    return False


def get_startup_location(grassdatabase):
    """Wrapping function for managing startup location.

    Returns the location on success and None otherwise.
    Internally resolves all the cases (no location to copy, copying failed
    and an existing user's copy of the location)."""

    # Find out if startup location exists
    location = _get_startup_location_in_distribution()
    if location:
        if _copy_startup_location(grassdatabase, location):
            if _create_startup_mapset(grassdatabase, location):
                return location
    return None
