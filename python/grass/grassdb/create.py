"""
Create objects in GRASS Spatial Database

(C) 2020 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Vaclav Petras <wenzeslaus gmail com>
"""

import os
import shutil
import getpass

from grass.grassdb.manage import resolve_mapset_path, MapsetPath


def _directory_to_mapset(path: MapsetPath):
    """Turn an existing directory into a mapset"""
    # copy DEFAULT_WIND file and its permissions from PERMANENT
    # to WIND in the new mapset
    region_path1 = path.path.parent / "PERMANENT" / "DEFAULT_WIND"
    region_path2 = path.path / "WIND"
    shutil.copy(region_path1, region_path2)


def create_mapset(database, location, mapset):
    """Creates a mapset in a specified location"""
    path = resolve_mapset_path(
        database,
        location,
        mapset,
    )
    # create an empty directory
    os.mkdir(path)
    _directory_to_mapset(path)
    # set permissions to u+rw,go+r (disabled; why?)
    # os.chmod(os.path.join(database,location,mapset,'WIND'), 0644)


def get_default_mapset_name():
    """Returns default name for mapset."""
    try:
        result = getpass.getuser()
        # Raise error if not ascii (not valid mapset name).
        result.encode("ascii")
    except UnicodeEncodeError:
        # Fall back to fixed name.
        result = "user"

    return result
