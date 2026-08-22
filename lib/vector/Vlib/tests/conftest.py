import os

import pytest

import grass.script as gs


@pytest.fixture(scope="module")
def xy_session_env(tmp_path_factory):
    """Environment of a session in an XY project (scope: module)

    The environment is yielded for a subprocess to use rather than applied to
    this process, because the tests call the library through ctypes, where a
    failure ends the whole process instead of just the test.
    """
    tmp_path = tmp_path_factory.mktemp("xy_session")
    project = tmp_path / "xy"
    gs.create_project(project)
    env = os.environ.copy()
    with gs.setup.init(project, env=env):
        yield env
