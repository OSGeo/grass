import os

import grass.script as gs
from grass.tools import Tools


def test_crs_xy(tmp_path):
    """Check general XY project"""
    project = tmp_path / "test"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        assert tools.g_region(flags="p", format="json")["projection"] == 0
        assert tools.g_region(flags="p", format="json")["zone"] == 0


def test_crs_utm_north(tmp_path):
    """Check UTM 17N project"""
    project = tmp_path / "test"
    gs.create_project(project, epsg="6346")
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        assert tools.g_region(flags="p", format="json")["projection"] == 1
        assert tools.g_region(flags="p", format="json")["zone"] == 17


def test_crs_utm_south(tmp_path):
    """Check UTM 17S project"""
    project = tmp_path / "test"
    gs.create_project(project, epsg="32717")
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        assert tools.g_region(flags="p", format="json")["projection"] == 1
        assert tools.g_region(flags="p", format="json")["zone"] == -17


def test_crs_ll(tmp_path):
    """Check latitude/longitude (WGS84) project"""
    project = tmp_path / "test"
    gs.create_project(project, epsg="4326")
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        assert tools.g_region(flags="p", format="json")["projection"] == 3
        assert tools.g_region(flags="p", format="json")["zone"] == 0


def test_crs_other(tmp_path):
    """Check other CRS (NC SPM) project

    Note: In the past, projection code 2 was used for US state plane CRS in feet,
    but we are not supporting or testing this.
    """
    project = tmp_path / "test"
    gs.create_project(project, epsg="3358")
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        assert tools.g_region(flags="p", format="json")["projection"] == 99
        assert tools.g_region(flags="p", format="json")["zone"] == 0
