"""
@package startup.utils

@brief General GUI-independent utilities for GUI startup of GRASS GIS

(C) 2017-2018 by Vaclav Petras the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail com>
@author Linda Kladivova <l.kladivova@seznam.cz>

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

    Returns path to startup location if successfully copied or None
    when an error was encountered.
    """
    src = startup_location
    dst = os.path.join(grassdatabase, "world_latlong_wgs84")

    # Copy source startup location into GRASS database
    try:
        copytree(src, dst, ignore=ignore_patterns('*.tmpl','Makefile*'))
        return dst
    except (IOError, OSError):
        pass
    return None


def _create_startup_mapset(location_path):
    """Create the new empty startup mapset named after user.

    Returns True if successfully created or False when an error was encountered.
    """
    name = get_default_mapset_name()
    mapset_path = os.path.join(location_path, name)
    counter = 2

    while os.path.exists(mapset_path):
        mapset_name = _("{name}_{counter}").format(name=name,
                                                   counter=str(counter))
        mapset_path = os.path.join(location_path, mapset_name)
        counter += 1

    # Create new startup mapset
    try:
        grassdatabase, location = os.path.split(location_path)
        create_mapset(grassdatabase, location, mapset_name)
        return grassdatabase, location, mapset_name
    except (IOError, OSError):
        pass
    return None


def get_startup_location(grassdatabase):
    """Wrapping function for managing startup location.

    Returns the copied location on success and None otherwise.
    Internally resolves all the cases (no location to copy, copying failed
    and an existing user's copy of the location)."""

    # Find out if startup location exists
    location = _get_startup_location_in_distribution()
    if location:
        location_path =_copy_startup_location(grassdatabase, location)
        if location_path:
            grassdatabase, location, mapset_name = _create_startup_mapset(location_path)
            return grassdatabase, location, mapset_name
    return None
