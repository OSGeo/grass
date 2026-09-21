"""pytest test fixtures for g.gisenv"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture
def session(tmp_path):
    """Active session in a fresh, empty project (scope: function)"""
    project = tmp_path / "test_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield session


@pytest.fixture
def tools(session):
    """A Tools object ready to be used.

    See the underlying fixture for more info.
    """
    with Tools(session=session) as tools:
        yield tools
