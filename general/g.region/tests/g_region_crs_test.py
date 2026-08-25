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
        data = tools.g_region(flags="p", format="json")
        assert data["crs"]["type_code"] == 0
        assert data["crs"]["type"] == "xy"
        # We test the value we are returning (but the value is not part of the API).
        assert data["crs"]["name"] == "x,y"
        assert data["crs"]["zone"] is None


def test_crs_utm_north(tmp_path):
    """Check UTM 17N project"""
    project = tmp_path / "test"
    gs.create_project(project, epsg="6346")
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        data = tools.g_region(flags="p", format="json")
        assert data["crs"]["type_code"] == 1
        assert data["crs"]["type"] == "utm"
        # We simply test the value we are returning now, but more detailed name would be nice.
        assert data["crs"]["name"] == "UTM"
        assert data["crs"]["zone"] == 17


def test_crs_utm_south(tmp_path):
    """Check UTM 17S project"""
    project = tmp_path / "test"
    gs.create_project(project, epsg="32717")
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        data = tools.g_region(flags="p", format="json")
        assert data["crs"]["type_code"] == 1
        assert data["crs"]["type"] == "utm"
        # We simply test the value we are returning now, but more detailed name would be nice.
        assert data["crs"]["name"] == "UTM"
        assert data["crs"]["zone"] == -17


def test_crs_ll(tmp_path):
    """Check latitude/longitude (WGS84) project"""
    project = tmp_path / "test"
    gs.create_project(project, epsg="4326")
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        data = tools.g_region(flags="p", format="json")
        assert data["crs"]["type_code"] == 3
        assert data["crs"]["type"] == "ll"
        # We test the value we are returning (but the value is not part of the API).
        assert data["crs"]["name"] == "Latitude-Longitude"
        assert data["crs"]["zone"] is None


def test_crs_other_epsg(tmp_path):
    """Check other CRS (NC SPM) project from EPSG

    Note: In the past, projection code 2 was used for US state plane CRS in feet,
    but we are not supporting or testing this.
    """
    project = tmp_path / "test"
    gs.create_project(project, epsg="3358")
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        data = tools.g_region(flags="p", format="json")
        assert data["crs"]["type_code"] == 99
        assert data["crs"]["type"] == "other"
        # We loosely test for the name associated with the EPSG code.
        # It may as well be, e.g., Lambert Conformal Conic.
        # (But we should not get 'other or unknown projection' type of text.)
        name = data["crs"]["name"]
        assert "North Carolina".lower() in name.lower()
        assert "NAD83".lower() in name.lower()
        assert data["crs"]["zone"] is None


def test_crs_other_custom(tmp_path):
    """Check a custom CRS project"""
    project = tmp_path / "test"
    gs.create_project(project, proj4="+proj=merc +lat_ts=56.5 +ellps=GRS80")
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        data = tools.g_region(flags="p", format="json")
        assert data["crs"]["type_code"] == 99
        assert data["crs"]["type"] == "other"
        assert data["crs"]["name"] is None
        assert data["crs"]["zone"] is None
