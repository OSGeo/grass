from types import SimpleNamespace

import grass.script as gs
import pytest

TEST_MAPSETS = ["PERMANENT", "test1", "test2", "test3"]


@pytest.fixture(scope="module")
def simple_dataset(tmp_path_factory):
    """Set up a GRASS session for the tests."""
    tmp_path = tmp_path_factory.mktemp("simple_dataset")
    location = "test"

    # Create a test location
    gs.create_project(tmp_path, location)

    # Initialize the GRASS session
    with gs.setup.init(tmp_path / location):
        # Create Mock Mapsets
        for mapset in TEST_MAPSETS:
            gs.run_command("g.mapset", project=location, mapset=mapset, flags="c")

        # Set current mapset to test1
        gs.run_command("g.mapset", project=location, mapset="test1")

        yield SimpleNamespace(
            mapsets=TEST_MAPSETS,
            project=location,
            current_mapset="test1",
        )
