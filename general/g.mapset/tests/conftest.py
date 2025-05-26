from types import SimpleNamespace

import os
import grass.script as gs
import pytest

TEST_MAPSETS = ["PERMANENT", "test1", "test2", "test3"]


@pytest.fixture(scope="module")
def simple_dataset(tmp_path_factory):
    """Set up a GRASS session for the tests."""
    tmp_path = tmp_path_factory.mktemp("simple_dataset")
    location = "test"
    gs.core._create_location_xy(tmp_path, location)
    with gs.setup.init(tmp_path / location, env=os.environ.copy()) as session:
        # Create Mock Mapsets
        for mapset in TEST_MAPSETS:
            gs.run_command(
                "g.mapset", project=location, mapset=mapset, flags="c", env=session.env
            )

        # Set current mapset to test1
        gs.run_command("g.mapset", project=location, mapset="test1", env=session.env)

        yield SimpleNamespace(
            session=session,
            mapsets=TEST_MAPSETS,
            project=location,
            current_mapset="test1",
        )
