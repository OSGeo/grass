import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture
def xy_points_session(tmp_path):
    """Active session in an XY project holding a small point map"""
    project = tmp_path / "xy_test"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        points = tmp_path / "points.txt"
        points.write_text("10|10|1\n20|20|2\n30|15|3\n")
        tools.g_region(s=0, n=50, w=0, e=50, res=1)
        tools.v_in_ascii(
            input=str(points),
            output="points",
            separator="pipe",
            x=1,
            y=2,
            cat=3,
        )
        yield session
