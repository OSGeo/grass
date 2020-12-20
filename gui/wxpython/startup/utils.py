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


def get_possible_database_path():
    """Looks for directory 'grassdata' (case-insensitive) in standard
    projects to detect existing GRASS Database.

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
    Creates database directory named grassdata in the standard project
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


def _get_startup_project_in_distribution():
    """Check for startup project directory in distribution.

    Returns startup project if found or None if nothing was found.
    """
    gisbase = os.getenv("GISBASE")
    startup_project = os.path.join(gisbase, "demoproject")

    # Find out if startup project exists
    if os.path.exists(startup_project):
        return startup_project
    return None


def _copy_startup_project(startup_project, project_in_grassdb):
    """Copy the simple startup_project with some data to GRASS database.

    Returns True if successfully copied or False
    when an error was encountered.
    """
    # Copy source startup project into GRASS database
    try:
        copytree(startup_project, project_in_grassdb,
                 ignore=ignore_patterns('*.tmpl', 'Makefile*'))
        return True
    except (IOError, OSError):
        pass
    return False


def create_startup_project_in_grassdb(grassdatabase, startup_project_name):
    """Create a new startup project in the given GRASS database.

    Returns True if a new startup project successfully created
    in the given GRASS database.
    Returns False if there is no project to copy in the installation
    or copying failed.
    """

    # Find out if startup project exists
    startup_project = _get_startup_project_in_distribution()
    if not startup_project:
        return False

    # Copy the simple startup_project with some data to GRASS database
    project_in_grassdb = os.path.join(grassdatabase, startup_project_name)
    if _copy_startup_project(startup_project, project_in_grassdb):
        return True
    return False
