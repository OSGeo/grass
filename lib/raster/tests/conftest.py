import os

import pytest

import grass.script as gs
from grass.experimental import TemporaryMapsetSession
from grass.tools import Tools


@pytest.fixture(scope="module")
def session_for_module(tmp_path_factory):
    """Module-scoped session with a raster and a possible mask"""
    tmp_path = tmp_path_factory.mktemp("data") / "project"
    gs.create_project(tmp_path)
    with gs.setup.init(tmp_path, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=20, s=0, e=20, w=0, rows=20, cols=20)
        tools.r_mapcalc(expression="data = row() + col()")
        tools.r_mapcalc(expression="raster_mask = if(row() < 10, 1, null())")
        yield session


@pytest.fixture
def session(session_for_module):
    """A session in a temporary mapset"""
    with TemporaryMapsetSession(env=session_for_module.env) as session:
        yield session
