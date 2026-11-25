"""Tests of grass.grassdb.data"""

import grass.script as gs
from grass.grassdb.data import map_exists


def test_map_exists(session_with_raster):
    """Existing and missing maps are reported"""
    env = session_with_raster.env
    assert map_exists("r1", "raster", mapset="PERMANENT", env=env)
    assert not map_exists("missing", "raster", mapset="PERMANENT", env=env)


def test_map_exists_nonexistent_mapset(session_with_raster):
    """A mapset that does not exist gives False instead of an exception"""
    gisenv = gs.gisenv(env=session_with_raster.env)
    gisrc, env = gs.create_environment(
        gisenv["GISDBASE"],
        gisenv["LOCATION_NAME"],
        "missing",
        env=session_with_raster.env,
    )
    try:
        assert not map_exists("r1", "raster", mapset="missing", env=env)
    finally:
        gs.try_remove(gisrc)
