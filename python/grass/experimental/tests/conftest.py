import pytest

import grass.script as gs


@pytest.fixture
def xy_dataset_session(tmp_path):
    """Creates a session with a mapset which has vector with a float column"""
    gs.core._create_location_xy(tmp_path, "test")  # pylint: disable=protected-access
    with gs.setup.init(tmp_path / "test") as session:
        yield session
