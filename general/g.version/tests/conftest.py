import os
from types import SimpleNamespace
import pytest
import grass.script as gs


@pytest.fixture(scope="module")
def grass_session(tmp_path_factory):
    """Set up a GRASS session for the tests."""
    tmp_path = tmp_path_factory.mktemp("grass_session")
    location = "test_location"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access

    with gs.setup.init(tmp_path / location, env=os.environ.copy()) as session:
        yield SimpleNamespace(session=session)
