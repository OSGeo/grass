import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture
def session_with_maps(tmp_path):
    """A GRASS session with two small integer rasters for r.cross.

    map1: category by row -> 1, 2, 3
    map2: category by column -> 1, 2, NULL
    """
    project = tmp_path / "r_cross_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=3, s=0, e=3, w=0, res=1)
        tools.r_mapcalc(expression="map1 = if(row() == 1, 1, if(row() == 2, 2, 3))")
        tools.r_mapcalc(
            expression="map2 = if(col() == 1, 1, if(col() == 2, 2, null()))"
        )
        yield session


def test_cross_basic(session_with_maps):
    """Without -z, every combination (including NULL ones) gets a category."""
    session = session_with_maps
    tools = Tools(session=session)

    tools.r_cross(input="map1,map2", output="crossed_map")

    described = tools.r_describe(map="crossed_map", format="json").json
    assert described == {"has_nulls": False, "ranges": [{"min": 0, "max": 8}]}


def test_cross_with_z_flag(session_with_maps):
    """With -z, combinations containing NULL are dropped and the output has NULLs."""
    session = session_with_maps
    tools = Tools(session=session)

    tools.r_cross(input="map1,map2", output="crossed_map", flags="z")

    described = tools.r_describe(map="crossed_map", format="json").json
    assert described == {"has_nulls": True, "ranges": [{"min": 0, "max": 5}]}
