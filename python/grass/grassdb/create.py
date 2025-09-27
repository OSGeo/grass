"""
Create objects in GRASS Spatial Database

(C) 2020 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Vaclav Petras <wenzeslaus gmail com>
"""

from __future__ import annotations

import json
import tarfile
import codecs
import os
import shutil
import getpass
from pathlib import Path

from grass.grassdb.manage import resolve_mapset_path, MapsetPath


def create_xy_project(path: Path | str):
    _create_project_dir(path)
    # create DEFAULT_WIND and WIND files
    _add_computation_region_to_project_dir(path, projection=0, zone=0)


def _create_project_dir(path: Path | str):
    project_dir = Path(path)
    permanent_dir = project_dir / "PERMANENT"
    project_dir.mkdir()
    permanent_dir.mkdir()


def _set_project_description(path: Path | str, text: str):
    """Set description (aka title aka MYNAME) for a project"""
    with codecs.open(
        os.path.join(path, "PERMANENT", "MYNAME"),
        encoding="utf-8",
        mode="w",
    ) as fd:
        if text:
            fd.write(text + os.linesep)
        else:
            fd.write(os.linesep)


def _add_computation_region_to_project_dir(
    path: Path | str, projection: int | None = None, zone: int | None = None
):
    """Add computational region to project directory

    Creates both the default and the current compuational regions.
    If the file already exists, it is overwritten.
    """
    project_dir = Path(path)
    permanent_dir = project_dir / "PERMANENT"
    default_wind_path = permanent_dir / "DEFAULT_WIND"
    wind_path = permanent_dir / "WIND"
    regioninfo = [
        f"proj:      {projection if projection is not None else 0}",
        f"zone:      {zone if zone is not None else 0}",
        "north:      1",
        "south:      0",
        "east:       1",
        "west:       0",
        "cols:       1",
        "rows:       1",
        "e-w resol:  1",
        "n-s resol:  1",
        "top:        1",
        "bottom:     0",
        "cols3:      1",
        "rows3:      1",
        "depths:     1",
        "e-w resol3: 1",
        "n-s resol3: 1",
        "t-b resol:  1",
    ]
    # create the files
    default_wind_path.write_text("\n".join(regioninfo))
    shutil.copy(default_wind_path, wind_path)


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


def create_project_from_pack(
    mapset_path: MapsetPath, filename: str, description: str | None = None
):
    _create_project_dir(Path(mapset_path.directory) / mapset_path.location)
    set_project_crs_from_pack(mapset_path, filename)
    if description:
        # A little inconsistency with create_project which always creates the description file.
        _set_project_description(
            Path(mapset_path.directory) / mapset_path.location, description
        )


def set_project_crs_from_pack(mapset_path: MapsetPath, filename: str):
    """Overwrites current region (if any)"""
    with tarfile.open(filename) as tar:
        data_names = [
            tarinfo.name for tarinfo in tar.getmembers() if "/" not in tarinfo.name
        ]
        tar_info = tar.getmember(data_names[0] + "/" + "computational_region_crs.json")
        data = json.loads(tar.extractfile(tar_info).read().decode("utf-8"))
        _add_computation_region_to_project_dir(
            Path(mapset_path.directory) / mapset_path.location,
            projection=data["projection"],
            zone=data["zone"],
        )

        crs_files = [
            "PROJ_UNITS",
            "PROJ_INFO",
            "PROJ_EPSG",
            "PROJ_SRID",
            "PROJ_WKT",
        ]
        for name in crs_files:
            try:
                tar_info = tar.getmember(data_names[0] + "/" + name)
            except KeyError:
                continue
            Path(Path(mapset_path) / name).write_bytes(tar.extractfile(tar_info).read())
