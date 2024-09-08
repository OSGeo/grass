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
import sys
from pathlib import Path


import grass.grassdb.config


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


class MapsetPath:
    """This is a representation of a path to mapset.

    Individual components are accessible through read-only properties
    and objects have an os.PathLike interface.

    Paths are currently stored as is (not resolved, not expanded),
    but that may change in the future.
    """

    def __init__(self, path, directory, location, mapset):
        # Path as an attribute. Inheriting from Path would be something to consider
        # here, however the Path inheritance is somewhat complex at this point.
        self._path = Path(path)
        self._directory = str(directory)
        self._location = location
        self._mapset = mapset

    def __repr__(self):
        return (
            f"{self.__class__.__name__}("
            f"{self._path!r}, "
            f"{self._directory!r}, {self._location!r}, {self._mapset!r})"
        )

    def __str__(self):
        return str(self._path)

    def __fspath__(self):
        return os.fspath(self._path)

    @property
    def path(self):
        """Full path to the mapset as a pathlib.Path object"""
        return self._path

    @property
    def directory(self):
        """Location name"""
        return self._directory

    @property
    def location(self):
        """Location name"""
        return self._location

    @property
    def mapset(self):
        """Mapset name"""
        return self._mapset


def split_mapset_path(mapset_path):
    """Split mapset path to three parts - grassdb, location, mapset"""
    mapset_path = Path(mapset_path)
    if len(mapset_path.parts) < 3:
        raise ValueError(
            _("Mapset path '{}' needs at least three components").format(mapset_path)
        )
    mapset = mapset_path.name
    location_path = mapset_path.parent
    location = location_path.name
    grassdb = location_path.parent
    return os.fspath(grassdb), location, mapset


def resolve_mapset_path(path, location=None, mapset=None) -> MapsetPath:
    """Resolve full path to mapset from given combination of parameters.

    Full or relative path to mapset can be provided as *path*. If the *path*
    points to a valid location instead of a valid mapset, the mapset defaults
    to PERMANENT.

    Alternatively, location and mapset can be provided separately. In that case,
    location and mapset are added to the path. If location is provided and mapset
    is not, mapset defaults to PERMANENT.

    Home represented by ``~`` (tilde) and relative paths are resolved
    and the result contains absolute paths.

    The function does not enforce the existence of the directory or that it
    is a mapset. It only manipulates the paths except for internal checks
    which help to determine the result in some cases. On Windows, if the path
    does not exist and ``..`` is present in the path, it will be not be resolved
    due to path resolution limitation in the Python pathlib package.

    Returns a MapsetPath object.
    """
    # We reduce the top-level imports because this is initialization code.
    # pylint: disable=import-outside-toplevel

    path = Path(path).expanduser()
    if not sys.platform.startswith("win") or path.exists():
        # The resolve function works just fine on Windows when the path exists
        # and everywhere even if it does not.
        # This also resolves symlinks which may or may not be desired.
        path = path.resolve()
    else:
        # On Windows when the path does not exist, resolve does not work.
        # This does not resolve `..` which is not desired.
        path = Path.cwd() / path
    default_mapset = grass.grassdb.config.permanent_mapset
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
            path /= default_mapset
        parts = path.parts
        if len(parts) < 3:
            raise ValueError(
                _(
                    "Parameter path needs to be 'path/to/location/mapset' "
                    "or location and mapset need to be set"
                )
            )
        directory, location, mapset = split_mapset_path(path)
    return MapsetPath(path=path, directory=directory, location=location, mapset=mapset)
