"""Fixtures for Jupyter tests

Fixture for grass.jupyter.TimeSeries test

Fixture for ReprojectionRenderer test with simple GRASS location, raster, vector.
"""

from types import SimpleNamespace

import grass.script as gs
import pytest

TEST_MAPSETS = ["PERMANENT", "test1", "test2", "test3"]
ACCESSIBLE_MAPSETS = ["test3", "PERMANENT"]


@pytest.fixture(scope="module")
def simple_dataset(tmp_path_factory):
    """Start a session and create a test mapsets
    Returns object with attributes about the dataset.
    """
    tmp_path = tmp_path_factory.mktemp("simple_dataset")
    location = "test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    with gs.setup.init(tmp_path / location):
        gs.run_command("g.proj", flags="c", epsg=26917)
        gs.run_command("g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)
        # Create Mock Mapsets
        for mapset in TEST_MAPSETS:
            gs.run_command("g.mapset", project=location, mapset=mapset, flags="c")

        yield SimpleNamespace(
            mapsets=TEST_MAPSETS, accessible_mapsets=ACCESSIBLE_MAPSETS
        )
