import os
import pytest
import grass.script as gs


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Set up a GRASS session for the tests."""
    tmp_path = tmp_path_factory.mktemp("grass_session")
    location = "test_location"

    # Create a test location
    gs.core._create_location_xy(tmp_path, location)

    # Initialize the GRASS session
    with gs.setup.init(tmp_path / location, env=os.environ.copy()) as session:
        yield session
