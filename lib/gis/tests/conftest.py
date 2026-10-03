import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def xy_session_for_module(tmp_path_factory):
    """Active session in an XY location (scope: module)

    The location is not removed by this fixture and relies on temporary directory
    handling allowing for inspection of the data as with other pytest temporary
    directories.
    """
    tmp_path = tmp_path_factory.mktemp("xy_session_for_module")
    project = tmp_path / "xy_test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield session


@pytest.fixture(scope="module")
def tools(xy_session_for_module):
    with Tools(session=xy_session_for_module) as tools:
        yield tools
