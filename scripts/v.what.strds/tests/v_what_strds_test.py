# SPDX-License-Identifier: GPL-2.0-or-later
import os
from io import StringIO

import pytest

import grass.script as gs
from grass.tools import Tools

# Three points, one per line, as "east|north|cat". Each sits at the centre of
# a different cell, so the value sampled from a raster identifies both the
# cell and the point it belongs to.
POINTS = """5|75|1
55|45|2
115|5|3
"""

# Each raster is map_number * 1000 + row() * 10 + col(), so every cell holds a
# distinct value and a shifted cell or a reordered point changes the result.
EXPECTED_ROWS = [
    "1|1011|2011|3011|4011",
    "2|1046|2046|3046|4046",
    "3|1092|2092|3092|4092",
]


@pytest.fixture
def session(tmp_path):
    """A GRASS session with a four-raster strds and three points at known
    cell centres.

    Raster values encode position, so sampling the wrong cell or returning
    the points in the wrong order produces different numbers rather than the
    same one.
    """
    project = tmp_path / "v_what_strds_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)
        for i in range(1, 5):
            tools.r_mapcalc(expression=f"a_{i} = {i} * 1000 + row() * 10 + col()")
        tools.v_in_ascii(
            input=StringIO(POINTS),
            output="points",
            format="point",
            separator="|",
            x=1,
            y=2,
            cat=3,
        )

        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output="A",
            title="A test",
            description="A test",
        )
        tools.t_register(
            flags="i",
            type="raster",
            input="A",
            maps="a_1,a_2,a_3,a_4",
            start="2001-01-01",
            increment="3 months",
        )
        yield session


def test_output_map_is_created(session):
    """v.what.strds creates the requested output vector map."""
    tools = Tools(session=session)
    tools.v_what_strds(input="points", strds="A", output="what_strds")

    maps = gs.list_strings("vector", env=session.env)
    assert "what_strds@PERMANENT" in maps


def test_sampled_values_match_the_registered_rasters(session):
    """Each point's attribute table row holds the value its own cell has in
    every raster registered in the strds, one column per map."""
    tools = Tools(session=session)
    tools.v_what_strds(input="points", strds="A", output="what_strds")

    result = tools.v_db_select(map="what_strds")
    lines = result.text.splitlines()
    assert lines[0] == "cat|A_2001_01_01|A_2001_04_01|A_2001_07_01|A_2001_10_01"
    assert lines[1:] == EXPECTED_ROWS
