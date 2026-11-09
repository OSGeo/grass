"""Fixtures for v.geometry tests."""

import io
import os

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


# Three non-overlapping areas (two sharing cat 2, one cat 1) plus a line
# with cat 10. Boundaries carry no categories, so v.to.db option=count
# emits spurious cat=-1 records for them. Used to check that aligning
# count to the other metric's cat set drops the line and the cat=-1
# records, and that counts for repeated cats aggregate correctly.
MIXED_ASCII = """\
ORGANIZATION: GRASS Test
DIGIT DATE: today
DIGIT NAME: test
MAP NAME: mixed
MAP DATE: today
MAP SCALE: 1
OTHER INFO:
ZONE: 0
MAP THRESH: 0.500000
VERTI:
B 5
 0 0
 50 0
 50 50
 0 50
 0 0
C 1 1
 25 25
 1 1
B 5
 60 0
 100 0
 100 50
 60 50
 60 0
C 1 1
 80 25
 1 2
B 5
 0 60
 50 60
 50 100
 0 100
 0 60
C 1 1
 25 80
 1 2
L 2 1
 70 70
 90 90
 1 10
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

        # Mixed map: a line (cat 1) and a 50x50 area (cat 2).
        tools.v_in_ascii(
            input=io.StringIO(MIXED_ASCII),
            output="mixed",
            format="standard",
        )

        yield grass_session
