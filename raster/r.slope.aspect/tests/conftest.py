import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture
def xy_dataset_session(tmp_path):
    """Active session in an XY location (scope: function)"""
    project = tmp_path / "xy_test"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.g_region(s=0, n=5, w=0, e=6, res=1)
        tools.r_mapcalc(expression="rows_raster = row()")
        yield session
