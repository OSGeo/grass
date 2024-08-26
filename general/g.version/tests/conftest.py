import os
from types import SimpleNamespace
import pytest
import grass.script as gs


@pytest.fixture(scope="module")
def grass_session(tmp_path_factory):
    """Set up a GRASS session for the tests."""
    tmp_path = tmp_path_factory.mktemp("grass_session")
    project = "test_project"
    gs.create_project(tmp_path, project)

    with gs.setup.init(tmp_path / project, env=os.environ.copy()) as session:
        yield SimpleNamespace(session=session)
