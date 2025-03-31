"""Likely going into grass.grassdb.create"""

import pathlib
import tempfile

from grass.grassdb.checks import (
    mapset_exists,
    is_mapset_valid,
    get_mapset_invalid_reason,
)
from grass.grassdb.create import (
    create_mapset,
    _directory_to_mapset,
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
        if not overwrite:
            msg = (
                f"Mapset '{path.mapset}' already exists, "
                "use a different name, overwrite, or ensure"
            )
            raise ValueError(msg)
        delete_mapset(path.directory, path.location, path.mapset)
    if create or (ensure and not exists):
        create_mapset(path.directory, path.location, path.mapset)
    elif not exists or not is_mapset_valid(path):
        reason = get_mapset_invalid_reason(path.directory, path.location, path.mapset)
        msg = f"Mapset {path.mapset} is not valid: {reason}"
        raise ValueError(msg)


def create_temporary_mapset(path, location=None) -> MapsetPath:
    """Create temporary mapset

    The user of this function is responsible for deleting the contents of the
    temporary directory and the directory itself when done with it.
    """
    path = pathlib.Path(path)
    if location:
        path /= location
    tmp_dir = tempfile.mkdtemp(dir=path)
    new_path = resolve_mapset_path(tmp_dir)
    _directory_to_mapset(new_path)
    return new_path
