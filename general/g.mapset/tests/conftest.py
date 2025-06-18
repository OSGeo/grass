from types import SimpleNamespace

import grass.script as gs
import pytest

TEST_MAPSETS = ["PERMANENT", "test1", "test2", "test3"]


@pytest.fixture(scope="module")
def simple_dataset(tmp_path_factory):
    """Set up a GRASS session for the tests."""
    tmp_path = tmp_path_factory.mktemp("simple_dataset")
    project = "test"

    # Create a test project
    gs.create_project(tmp_path, project)

    # Initialize the GRASS session
    with gs.setup.init(tmp_path / project):
        # Create Mock Mapsets
        for mapset in TEST_MAPSETS:
            gs.run_command("g.mapset", project=project, mapset=mapset, flags="c")

        # Set current mapset to test1
        gs.run_command("g.mapset", project=project, mapset="test1")

        yield SimpleNamespace(
            mapsets=TEST_MAPSETS,
            project=project,
            current_mapset="test1",
        )
