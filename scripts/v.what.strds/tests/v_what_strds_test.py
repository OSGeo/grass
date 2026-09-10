# SPDX-License-Identifier: GPL-2.0-or-later
import os

import pytest

import grass.script as gs
from grass.tools import Tools

EXPECTED_VALUES = "100|200|300|400"


@pytest.fixture
def session(tmp_path):
    """A GRASS session with a small space time raster dataset and three
    points sharing the same location, so every point samples the same
    known values from each of the four registered rasters."""
    project = tmp_path / "v_what_strds_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)
        for i, value in enumerate([100, 200, 300, 400], start=1):
            tools.r_mapcalc(expression=f"a_{i} = {value}")
        tools.v_random(output="points", npoints=3, seed=1)

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
    """Each point's attribute table row holds the value of every raster
    registered in the strds, one column per map."""
    tools = Tools(session=session)
    tools.v_what_strds(input="points", strds="A", output="what_strds")

    result = tools.v_db_select(map="what_strds")
    lines = result.text.splitlines()
    assert lines[0] == "cat|A_2001_01_01|A_2001_04_01|A_2001_07_01|A_2001_10_01"
    assert len(lines) == 4
    for line in lines[1:]:
        assert line.split("|", 1)[1] == EXPECTED_VALUES
