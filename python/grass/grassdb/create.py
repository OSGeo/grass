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
import tempfile

from grass.grassdb.checks import (
    mapset_exists,
    is_mapset_valid,
    get_mapset_invalid_reason,
)
from grass.grassdb.manage import delete_mapset, resolve_mapset_path, MapsetPath


def require_create_ensure_mapset(
    path, location=None, mapset=None, *, create=False, overwrite=False, ensure=False
):
    """Checks that mapsets exists or creates it in a specified location

    By default, it checks that the mapset exists and raises a ValueError otherwise.
    If *create* is True and the mapset does not exists, it creates it.
    If it exists and *overwrite* is True, it deletes the existing mapset
    (with all the data in it). If *ensure* is True, existing mapset is used
    as is and when there is none, a new mapset is created.

    Where the mapset is specified by a full path or by location name and path
    to the directory where the location is.

    The path argument is positional-only. Location and mapset are recommend to be used
    as positional.
    """
    path = resolve_mapset_path(
        path,
        location,
        mapset,
    )
    exists = mapset_exists(path)
    if create and exists:
        if overwrite:
            delete_mapset(path.directory, path.location, path.mapset)
        else:
            raise ValueError(
                f"Mapset '{path.mapset}' already exists, "
                "use a different name, overwrite, or ensure"
            )
    if create or (ensure and not exists):
        create_mapset(path.directory, path.location, path.mapset)
    elif not exists or not is_mapset_valid(path):
        reason = get_mapset_invalid_reason(path.directory, path.location, path.mapset)
        raise ValueError(f"Mapset {path.mapset} is not valid: {reason}")


def create_temporary_mapset(path, location=None) -> MapsetPath:
    import pathlib

    path = pathlib.Path(path)
    if location:
        path /= location
    tmp_dir = tempfile.mkdtemp(dir=path)
    new_path = resolve_mapset_path(tmp_dir)
    _directory_to_mapset(new_path)
    return new_path


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
