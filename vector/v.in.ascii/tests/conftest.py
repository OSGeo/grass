import os

import pytest

import grass.script as gs


@pytest.fixture
def xy_session(tmp_path):
    """Active session in an XY project"""
    project = tmp_path / "xy_test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield session
