"""
Create objects in a GRASS project

(C) 2020-2025 by the GRASS Development Team
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


def create_xy_project(path: Path | str) -> None:
    """Create a project with with general cartesian coordinate system (XY)

    :raises: OSError on error related to the file system
    """
    _create_project_dir(path)
    # create DEFAULT_WIND and WIND files
    _add_computation_region_to_project_dir(path, crs_type_code=0)


def _create_project_dir(path: Path | str) -> Path:
    """Create an empty project directory including directory for the default mapset

    :returns: Path to the default mapset
    """
    project_dir = Path(path)
    permanent_dir = project_dir / "PERMANENT"
    project_dir.mkdir()
    permanent_dir.mkdir()
    return permanent_dir


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
    path: Path | str, *, crs_type_code: int | None = None, zone: int | None = None
):
    """Add computational region to project directory

    Creates both the default and the current compuational regions.
    If the file already exists, it is overwritten.
    """
    permanent_dir = Path(path) / "PERMANENT"
    default_wind_path = permanent_dir / "DEFAULT_WIND"
    wind_path = permanent_dir / "WIND"
    size: float = 1
    regioninfo = [
        f"proj:       {crs_type_code if crs_type_code is not None else 0}",
        f"zone:       {zone if zone is not None else 0}",
        f"north:      {size}",
        "south:      0",
        f"east:       {size}",
        "west:       0",
        f"cols:       {size}",
        f"rows:       {size}",
        "e-w resol:  1",
        "n-s resol:  1",
        f"top:        {size}",
        "bottom:     0",
        f"cols3:      {size}",
        f"rows3:      {size}",
        f"depths:     {size}",
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


def create_mapset(database, location=None, mapset=None):
    """Creates a mapset in a specified location"""
    path = resolve_mapset_path(
        database,
        location,
        mapset,
    )
    # create an empty directory
    Path(path).mkdir()
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
    path: Path | str, filename: str, description: str | None = None
):
    """Create project from a GRASS native raster file (aka pack file)"""
    _create_project_dir(path)
    set_project_crs_from_pack(path, filename)
    if description:
        _set_project_description(path, description)


def set_project_crs_from_pack(path: Path | str, filename: Path | str):
    """Set project CRS from a GRASS native raster file.

    Overwrites the values of the current and the default computational region (if any).
    """
    with tarfile.open(os.fspath(filename)) as tar:
        data_names = [
            tarinfo.name for tarinfo in tar.getmembers() if "/" not in tarinfo.name
        ]
        try:
            tar_info = tar.getmember(data_names[0] + "/computational_region_crs.json")
        except KeyError as error:
            msg = "Missing complete CRS info: Use pack from version 8.5 (2025) or above"
            raise ValueError(msg) from error
        data = json.loads(tar.extractfile(tar_info).read().decode("utf-8"))
        _add_computation_region_to_project_dir(
            path,
            crs_type_code=data["type_code"],
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
            (Path(path) / "PERMANENT" / name).write_bytes(
                tar.extractfile(tar_info).read()
            )
