"""Test environment and GIS environment functions"""

import pytest

import grass.script as gs
import grass.script.setup as grass_setup
import grass.pygrass.utils as pygrass_utils
import grass.lib.gis as libgis


@pytest.mark.parametrize("location_name", ["test1", "test2", "abc"])
def test_reading_respects_change_of_session(tmp_path, location_name):
    """Check new session file path is retrieved and the file is read"""
    # pylint: disable=protected-access
    gs.core._create_location_xy(tmp_path, location_name)
    with grass_setup.init(tmp_path / location_name):
        libgis.G__read_gisrc_path()
        libgis.G__read_gisrc_env()
        assert pygrass_utils.getenv("LOCATION_NAME") == location_name
