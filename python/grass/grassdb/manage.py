"""
Managing existing objects in a GRASS GIS Spatial Database

(C) 2020 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Vaclav Petras <wenzeslaus gmail com>
"""


import collections
import os
import shutil
from pathlib import Path


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


def split_mapset_path(mapset_path):
    """Split mapset path to three parts - grassdb, location, mapset"""
    path, mapset = os.path.split(mapset_path.rstrip(os.sep))
    grassdb, location = os.path.split(path)
    return grassdb, location, mapset


class MapsetPath:
    def __init__(self, path, directory, location, mapset):
        # Path as an attribute. Inheriting from Path would be something to consider
        # here, however the Path inheritance is somewhat complex, but may be possible
        # in the future.
        self.path=Path(path)
        self.directory=str(directory)
        self.location=location
        self.mapset=mapset

    def __repr__(self):
       return (f'{self.__class__.__name__}('
               f'{self.path!r}, '
               f'{self.directory!r}, {self.location!r}, {self.mapset!r})')

    def __str__(self):
        return str(self.path)

    def __fspath__(self):
        return os.fspath(self.path)


def resolve_mapset_path(path, location=None, mapset=None):
    """Resolve full path to mapset from given combination of parameters.

    Full or relative path to mapset can be provided as *path*. If the *path*
    points to a valid location instead of a valid mapset, the mapset defaults
    to PERMANENT.

    Alternatively, location and mapset can be provided separately. In that case,
    location and mapset are added to the path. If location is provided and mapset
    is not, mapset defaults to PERMANENT.

    The function does not enforce the existence of the directory or that it
    it a mapset. It only manipulates the paths except for internal checks
    to determine default values in some cases.

    Home represented by ``~`` (tilde) and relative paths are resolved
    and the result contains absolute paths.

    Returns object with attributes path, db, location, and mapset.
    The path attribute is a libpath.Path object representing full path
    to the mapset. The db attribute is full path to the directory where
    the location directory is. The attributes location and mapset are names
    of location and mapset, respectively.
    """
    # This also resolves symlinks which may or may not be desired.
    path = Path(path).expanduser().resolve()
    default_mapset = "PERMANENT"
    if location and mapset:
        directory = str(path)
        path = path / location / mapset
    elif location:
        mapset = default_mapset
        directory = str(path)
        path = path / location / mapset
    elif mapset:
        # mapset, but not location
        raise ValueError(
            _(
                "Provide only path, or path and location, "
                "or path, location, and mapset, but not mapset without location"
            )
        )
    else:
        from grass.grassdb.checks import is_mapset_valid

        if not is_mapset_valid(path) and is_mapset_valid(path / default_mapset):
            path = path / default_mapset
        parts = path.parts
        if len(parts) < 3:
            raise ValueError(
                _(
                    "Parameter path needs to be 'path/to/location/mapset' "
                    "or location and mapset need to be set"
                )
            )
        mapset = parts[-1]
        location = parts[-2]
        directory = Path(*parts[:-2])
    return MapsetPath(path=path, directory=directory, location=location, mapset=mapset)
