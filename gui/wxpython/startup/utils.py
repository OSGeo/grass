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
    # copy WIND file and its permissions from PERMANENT
    # this uses PERMANENT's current region instead of default (?)
    region_file = 'WIND'
    region_path = os.path.join(location_path, 'PERMANENT', region_file)
    shutil.copy(region_path, mapset_path)
    # set permissions to u+rw,go+r (disabled)
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


# TODO: similar to (but not the same as) read_gisrc function in grass.py
def read_gisrc(self):
    """Read variables from a current GISRC file

    Returns a dictionary representation of the file content.
    """
    grassrc = {}

    gisrc = os.getenv("GISRC")

    if gisrc and os.path.isfile(gisrc):
        try:
            rc = open(gisrc, "r")
            for line in rc.readlines():
                try:
                    key, val = line.split(":", 1)
                except ValueError as e:
                    sys.stderr.write(
                        _('Invalid line in GISRC file (%s):%s\n' % (e, line)))
                grassrc[key.strip()] = DecodeString(val.strip())
        finally:
            rc.close()

    return grassrc
