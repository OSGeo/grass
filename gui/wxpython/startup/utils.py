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


def get_possible_database_path():
    """Finds a path to what is possibly a GRASS Database. 

    Looks for directory named grassdata in the usual locations.

    Returns the path as a string or None if nothing was found, so the
    return value can be used to test if the directory was found.
    """
    home = os.path.expanduser('~')
    # try some common directories for grassdata
    # grassdata (lowercase) in home for Linux (first choice)
    # Documents and My Documents for Windows
    # potential translations (old Windows and some Linux)
    # but ~ and ~/Documents should cover most of the cases
    # ordered by preference and then likelihood
    candidates = [
        os.path.join(home, "grassdata"),
        os.path.join(home, "Documents", "grassdata"),
        os.path.join(home, "My Documents", "grassdata"),
    ]
    try:
        # here goes everything which has potential unicode issues
        candidates.append(os.path.join(home, _("Documents"), "grassdata"))
        candidates.append(os.path.join(home, _("My Documents"), "grassdata"))
    except UnicodeDecodeError:
        # just ignore the errors if it doesn't work
        pass
    path = None
    for candidate in candidates:
        if os.path.exists(candidate):
            path = candidate
            break  # get the first match
    return path


def get_lockfile_if_present(database, location, mapset):
    """Return path to lock if present, None otherwise

    Returns the path as a string or None if nothing was found, so the
    return value can be used to test if the lock is present.
    """
    lock_name = '.gislock'
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
    region_path1 = os.path.join(location_path, 'PERMANENT', 'DEFAULT_WIND')
    region_path2 = os.path.join(location_path, mapset, 'WIND')
    shutil.copy(region_path1, region_path2)
    # set permissions to u+rw,go+r (disabled; why?)
    # os.chmod(os.path.join(database,location,mapset,'WIND'), 0644)


def delete_mapset(database, location, mapset):
    """Deletes a specified mapset"""
    if mapset == 'PERMANENT':
        # TODO: translatable or not?
        raise ValueError("Mapset PERMANENT cannot be deleted"
                         " (whole location can be)")
    shutil.rmtree(os.path.join(database, location, mapset))


def delete_location(database, location):
    """Deletes a specified location"""
    shutil.rmtree(os.path.join(database, location))



def rename_mapset(database, location, old_name, new_name):
    """Rename mapset from *old_name* to *new_name*"""
    location_path = os.path.join(database, location)
    os.rename(os.path.join(location_path, old_name),
              os.path.join(location_path, new_name))


def rename_location(database, old_name, new_name):
    """Rename location from *old_name* to *new_name*"""
    os.rename(os.path.join(database, old_name),
              os.path.join(database, new_name))
