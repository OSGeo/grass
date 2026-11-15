"""Test functions in grass.script.setup"""

import shutil
import os

from pathlib import Path

import pytest

import grass.script as gs
from grass.tools import Tools


def test_raster_pack_pack_param(tmp_path, pack_raster_file4x5_rows):
    project = tmp_path / "test"
    gs.create_project(project, pack=pack_raster_file4x5_rows)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        assert tools.g_proj(flags="p", format="shell").keyval["srid"] == "EPSG:3358"


@pytest.mark.parametrize("suffix", [".rpack", ".grass_raster", ".grr"])
def test_raster_pack_crs_param_extensions(tmp_path, pack_raster_file4x5_rows, suffix):
    project = tmp_path / "test"
    test_file = tmp_path / f"test{suffix}"
    shutil.copy(pack_raster_file4x5_rows, test_file)
    gs.create_project(project, crs=pack_raster_file4x5_rows)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        assert tools.g_proj(flags="p", format="shell").keyval["srid"] == "EPSG:3358"


def test_raster_pack_file(tmp_path, pack_raster_file4x5_rows):
    project = tmp_path / "test"
    gs.create_project(project, pack=pack_raster_file4x5_rows)
    assert project.exists()
    for name in ["PROJ_INFO", "PROJ_UNITS", "PROJ_SRID"]:
        assert Path(project, "PERMANENT", name).exists()


@pytest.mark.parametrize("compression", ["c", None])
def test_xy_crs_and_compression(tmp_path, compression):
    project = tmp_path / "test"
    test_file = tmp_path / "result.grass_raster"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.r_mapcalc_simple(expression="row() * col()", output="result")
        tools.r_pack(input="result", output=test_file, flags=compression)

    project = tmp_path / "new_test"
    gs.create_project(project, pack=test_file)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        assert tools.g_region(flags="p", format="shell").keyval["projection"] == 0


@pytest.mark.parametrize("crs", [("4326", 3), ("3358", 99)])
def test_crs(tmp_path, crs):
    project = tmp_path / "test"
    test_file = tmp_path / "result.grass_raster"
    gs.create_project(project, epsg=crs[0])
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.r_mapcalc_simple(expression="row() * col()", output="result")
        tools.r_pack(input="result", output=test_file)

    project = tmp_path / "new_test"
    gs.create_project(project, pack=test_file)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        assert tools.g_region(flags="p", format="shell").keyval["projection"] == crs[1]
        assert (
            tools.g_proj(flags="p", format="shell").keyval["srid"] == f"EPSG:{crs[0]}"
        )
