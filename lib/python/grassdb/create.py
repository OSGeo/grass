"""
Create objects in GRASS GIS Spatial Database

(C) 2020 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Vaclav Petras <wenzeslaus gmail com>
"""


import os
import shutil
import getpass


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
