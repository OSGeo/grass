"""
Check when timestamps are written -- especially when write_timestamp_to_grass() is called.

:authors: Anirban Das
"""

import grass.script as gs
import grass.temporal as tgis
from grass_session import Session
import pytest
from unittest.mock import patch

import tempfile


# setting up a fixture to create a temporary GRASS environment
@pytest.fixture(scope="module")
def grass_env():
    # just ensuring that the test runs on a new GRASS environment every time and does not interfere with any existing GRASS session. This also ensures that the test is self-contained and does not rely on any external state.
    with (
        tempfile.TemporaryDirectory() as temp_dir,
        Session(
            gisdb=temp_dir,
            location="testloc",
            mapset="PERMANENT",
            create_opts="EPSG:4326",
        ),
    ):
        # initialize temporal GRASS
        tgis.init()

        gs.run_command("g.mapset", flags="c", mapset="NEW")
        gs.run_command("g.region", n=50, s=0, e=50, w=0, res=1)
        gs.run_command("r.mapcalc", expression="new_map = 1")

        # come back to PERMANENT mapset
        gs.run_command("g.mapset", mapset="PERMANENT")
        # make the NEW mapset visible
        gs.run_command("g.mapsets", mapset="NEW", operation="add")

        gs.run_command(
            "t.create",
            type="strds",
            output="test_dataset",
            temporaltype="absolute",
            title="test_dataset",
            description="testing outputs",
        )

        # next we have to register this map with timestamp
        gs.run_command(
            "t.register", input="test_dataset", maps="new_map", start="2020-01-01"
        )
        yield


def test_update_absolute_time(grass_env):
    ds = tgis.open_old_stds("test_dataset", "strds")
    maps = ds.get_registered_maps_as_objects()
    m = maps[0]
    with patch.object(type(m), "write_timestamp_to_grass") as mock_func:
        with pytest.raises(SystemExit):
            m.update_absolute_time("2020-01-01")

        mock_func.assert_not_called()


def test_update_relative_time(grass_env):
    ds = tgis.open_old_stds("test_dataset", "strds")
    maps = ds.get_registered_maps_as_objects()
    m = maps[0]
    with patch.object(type(m), "write_timestamp_to_grass") as mock_func:
        with pytest.raises(SystemExit):
            m.update_relative_time(0, None, "days")

        mock_func.assert_not_called()


# short explanation of the tests:
# 1. write_timestamp_to_grass() is defined in abstract_map_dataset.py as an @abstractmethod. but the real implementation is in space_time_datasets.py. so who is actually calling it when you run m.update_absolute_time() or m.update_relative_time() ? is it the abstract_map_dataset.py or space_time_datasets.py ? we want to check that. so we patch the method in the abstract_map_dataset.py and check if it is called when we run the update time methods. if it is called, then we know that the update time methods are calling the write_timestamp_to_grass() method as expected.
#
# 2. the solution is to patch where the method is looked up, not where it is defined.
#
# 3. python resolves it via the instance (m), so it calls the method from the actual class of 'm', not the abstract base.
#
# 4. patching it via (m) allows this test to run even when GRASS changes internals.
