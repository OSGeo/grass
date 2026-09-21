# SPDX-License-Identifier: GPL-2.0-or-later
"""Fixture for t.remove tests"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def remove_session(tmp_path_factory):
    """Active session in a project with the region set for small test rasters.

    Module-scoped: only the project and region setup are shared. Each test
    creates its own uniquely named datasets and maps, so tests do not
    interfere with each other despite sharing the session.
    """
    tmp_path = tmp_path_factory.mktemp("t_remove")
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        Tools(session=session).g_region(s=0, n=80, w=0, e=120, res=10)
        yield session
