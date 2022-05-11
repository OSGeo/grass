"""Fixtures for grass.script"""

import pytest

import grass.script as gs
import grass.script.setup as grass_setup


@pytest.fixture
def xy_session(tmp_path):
    """Active session in an XY location"""
    location = "xy_test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    with grass_setup.init(tmp_path / location) as session:
        yield session
