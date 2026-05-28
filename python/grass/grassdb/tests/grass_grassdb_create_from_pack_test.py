import os
from pathlib import Path

import pytest

import grass.script as gs
from grass.tools import Tools
from grass.grassdb.create import create_project_from_pack, set_project_crs_from_pack


@pytest.mark.parametrize("project_path_type", [str, Path])
@pytest.mark.parametrize("file_path_type", [str, Path])
def test_path_types(
    tmp_path, project_path_type, file_path_type, pack_raster_file4x5_rows
):
    """Check that different path types are supported"""
    path = tmp_path / "project"
    create_project_from_pack(
        project_path_type(path), file_path_type(pack_raster_file4x5_rows)
    )
    path.exists()
    with (
        gs.setup.init(path, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        result_dict = tools.g_proj(flags="p", format="projjson")
        assert result_dict["id"]["authority"] == "EPSG"
        assert result_dict["id"]["code"] == 3358


def test_set_crs_in_xy(tmp_path, pack_raster_file4x5_rows):
    """Check that set CRS replaces the original CRS (and region)"""
    project = tmp_path / "project"
    gs.create_project(project, epsg=4326)
    project.exists()
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        region = tools.g_region(rows=42, cols=50, flags="p", format="json")
        assert region["crs"]["type"] == "ll"
        assert region["rows"] == 42
        assert region["cols"] == 50
    set_project_crs_from_pack(project, pack_raster_file4x5_rows)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        result_dict = tools.g_proj(flags="p", format="projjson")
        assert result_dict["id"]["authority"] == "EPSG"
        assert result_dict["id"]["code"] == 3358
        region = tools.g_region(flags="p", format="json")
        assert region["crs"]["type"] == "other"
        assert region["crs"]["type_code"] == 99
        assert region["crs"]["zone"] is None
        assert region["rows"] == 1
        assert region["cols"] == 1
