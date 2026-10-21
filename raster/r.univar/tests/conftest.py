"""Fixtures for the r.univar tests."""

import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def session_with_maps(tmp_path_factory):
    """A GRASS session with an all-null map and a map with an all-null zone."""
    project = tmp_path_factory.mktemp("r_univar") / "project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=10, s=0, e=10, w=0, res=1)
        tools.r_mapcalc(expression="all_null = null()")
        tools.r_mapcalc(expression="zones = if(col() <= 5, 1, 2)")
        tools.r_mapcalc(expression="data = if(col() <= 5, row(), null())")
        yield session
