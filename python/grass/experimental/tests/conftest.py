"""Fixtures for grass.script"""

import uuid
import os

import pytest

import grass.script as gs
import grass.experimental as experimental


@pytest.fixture
def xy_session(tmp_path):
    """Active session in an XY location (scope: function)"""
    location = "xy_test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    with gs.setup.init(tmp_path / location, env=os.environ.copy()) as session:
        yield session


@pytest.fixture(scope="module")
def xy_session_for_module(tmp_path_factory):
    """Active session in an XY location (scope: module)

    The location is not removed by this fixture and relies on temporary directory
    handling allowing for inspection of the data as with other pytest temporary
    directories.
    """
    tmp_path = tmp_path_factory.mktemp("xy_session_for_module")
    location = "xy_test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    with gs.setup.init(tmp_path / location, env=os.environ.copy()) as session:
        yield session


@pytest.fixture
def unique_id():
    """A unique alphanumeric identifier"""
    return uuid.uuid4().hex


@pytest.fixture
def xy_mapset_session(
    xy_session_for_module, unique_id
):  # pylint: disable=redefined-outer-name
    """Active session in a mapset of an XY location

    Mapset scope is function, while the location scope is module.

    The mapset is not removed by this fixture and relies on the underlying cleanup
    procedures which means that it can be examined in the temporary directories
    pytest creates.
    """
    with experimental.MapsetSession(
        f"test_{unique_id}", create=True, env=xy_session_for_module.env
    ) as session:
        yield session


@pytest.fixture
def xy_mapset_non_permament(xy_session):  # pylint: disable=redefined-outer-name
    """Active session in a mapset of an XY location

    Mapset scope is function, while the location scope is module.

    The mapset is not removed by this fixture and relies on the underlying cleanup
    procedures which means that it can be examined in the temporary directories
    pytest creates.
    """
    with experimental.MapsetSession(
        "test1", create=True, env=xy_session.env
    ) as session:
        yield session
