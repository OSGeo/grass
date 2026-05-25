import os

import pytest

import grass.script as gs


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Set up a GRASS session for db.connect tests."""
    tmp_path = tmp_path_factory.mktemp("grass_session")
    project = tmp_path / "test_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield session
