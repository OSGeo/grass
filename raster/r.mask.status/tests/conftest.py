"""Fixtures for simple sessions"""

import os
import pytest
import grass.script as gs


@pytest.fixture
def session_no_data(tmp_path):
    """Set up a GRASS session for the tests."""
    project = "test_project"
    gs.create_project(tmp_path, project)
    with gs.setup.init(tmp_path / project, env=os.environ.copy()) as session:
        yield session


@pytest.fixture
def session_with_data(tmp_path):
    """Set up a GRASS session for the tests."""
    project = tmp_path / "test_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command("g.region", rows=2, cols=2, env=session.env)
        gs.mapcalc("a = 1", env=session.env)
        yield session
