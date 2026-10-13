import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Active session in an XY project with a seeded CSR point pattern.

    The pattern are 500 uniformly random points in a 1000x1000 window,
    giving an intensity of 5e-4 points per unit area.
    """
    project = tmp_path_factory.mktemp("v_ppa") / "xy_test"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.g_region(s=0, n=1000, w=0, e=1000, res=1)
        tools.v_random(output="points_csr", npoints=500, seed=42)
        yield session


@pytest.fixture
def tools(session):
    return Tools(session=session)
