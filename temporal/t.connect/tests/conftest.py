"""
Fixtures for t.connect pytest suite.
"""

import os
from types import SimpleNamespace

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def t_connect_session(tmp_path_factory):
    """Start an isolated session for t.connect tests.

    Returns a SimpleNamespace containing the session.
    """
    tmp_path = tmp_path_factory.mktemp("t_connect_test")
    project = tmp_path / "test_project"
    gs.create_project(project)

    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)

        db_path = "$GISDBASE/$LOCATION_NAME/$MAPSET/tgis/sqlite.db"
        tools.t_connect(driver="sqlite", database=db_path)

        yield SimpleNamespace(session=session)


@pytest.fixture(scope="module")
def empty_session(tmp_path_factory):
    """Start an isolated session with no connection."""
    tmp_path = tmp_path_factory.mktemp("t_connect_empty")
    project = tmp_path / "test_project_empty"
    gs.create_project(project)

    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield SimpleNamespace(session=session)
