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
        # Deliberately off the cell boundaries: with a 1 unit resolution
        # starting at 0, a whole-numbered coordinate falls on a cell edge,
        # where rounding to the nearest cell and truncating to the containing
        # cell agree. These do not, so they detect a half cell shift.
        points.write_text("10.6|10.7|1\n20.2|20.3|2\n30.9|15.1|3\n")
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
