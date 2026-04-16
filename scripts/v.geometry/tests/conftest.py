"""Fixtures for v.geometry tests."""

import io
import os
from types import SimpleNamespace

import pytest

import grass.script as gs
from grass.tools import Tools


LINE_ASCII = """\
ORGANIZATION: GRASS Test
DIGIT DATE: today
DIGIT NAME: test
MAP NAME: line
MAP DATE: today
MAP SCALE: 1
OTHER INFO:
ZONE: 0
MAP THRESH: 0.500000
VERTI:
L 2 1
 0 0
 100 100
 1 1
"""


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Session with a 2x2 rectangle grid, a set of points, and a straight line."""
    tmp_path = tmp_path_factory.mktemp("v_geometry")
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as grass_session:
        tools = Tools(session=grass_session)
        tools.g_region(s=0, n=100, w=0, e=100, res=1)

        # 2x2 grid of 50x50 rectangles over (0,0)-(100,100).
        tools.v_mkgrid(map="grid", grid=(2, 2))

        # Three points at known coordinates.
        points_ascii = "10|20\n30|40\n50|60\n"
        tools.v_in_ascii(
            input=io.StringIO(points_ascii),
            output="points",
            format="point",
            separator="pipe",
        )

        # Single straight line from (0,0) to (100,100) with category 1.
        tools.v_in_ascii(
            input=io.StringIO(LINE_ASCII),
            output="line",
            format="standard",
        )

        yield SimpleNamespace(session=grass_session)
