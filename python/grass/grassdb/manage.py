"""
Managing existing objects in a GRASS GIS Spatial Database

(C) 2020 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Vaclav Petras <wenzeslaus gmail com>
"""


import os
import shutil


def delete_mapset(database, location, mapset):
    """Deletes a specified mapset"""
    if mapset == "PERMANENT":
        raise ValueError(
            _("Mapset PERMANENT cannot be deleted (a whole location can be)")
        )
    shutil.rmtree(os.path.join(database, location, mapset))


def delete_location(database, location):
    """Deletes a specified location"""
    shutil.rmtree(os.path.join(database, location))


def delete_grassdb(database):
    """Deletes a specified GRASS database"""
    shutil.rmtree(database)


def rename_mapset(database, location, old_name, new_name):
    """Rename mapset from *old_name* to *new_name*"""
    if old_name == "PERMANENT":
        raise ValueError(_("Mapset PERMANENT cannot be renamed"))
    location_path = os.path.join(database, location)
    os.rename(
        os.path.join(location_path, old_name), os.path.join(location_path, new_name)
    )


def rename_location(database, old_name, new_name):
    """Rename location from *old_name* to *new_name*"""
    os.rename(os.path.join(database, old_name), os.path.join(database, new_name))
