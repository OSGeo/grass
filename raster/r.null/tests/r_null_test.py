import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture
def session_with_maps(tmp_path):
    """A GRASS session with small rasters for r.null.

    map_basic: integer categories 1, 2, 3 by row
    map_fill_nulls: NULL, 2, 3 by row
    map_float: floating point values 1, 2.5, 3 by row
    """
    project = tmp_path / "r_null_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=3, s=0, e=3, w=0, res=1)
        tools.r_mapcalc(expression="map_basic = row()")
        tools.r_mapcalc(
            expression="map_fill_nulls = if(row() == 1, null(), if(row() == 2, 2, 3))"
        )
        tools.r_mapcalc(
            expression="map_float = if(row() == 1, 1, if(row() == 2, 2.5, 3))"
        )
        yield session


def test_setnull_integer(session_with_maps):
    """Cells matching setnull become NULL in an integer map (-i)."""
    tools = Tools(session=session_with_maps)

    tools.r_null(map="map_basic", setnull=1, flags="i")

    described = tools.r_describe(map="map_basic", format="json", flags="1").json
    assert described == {"has_nulls": True, "values": [2, 3]}


def test_setnull_float(session_with_maps):
    """Cells matching setnull become NULL in a floating point map (-f)."""
    tools = Tools(session=session_with_maps)

    tools.r_null(map="map_float", setnull=1, flags="f")

    described = tools.r_describe(map="map_float", format="json", flags="r").json
    assert described == {"has_nulls": True, "ranges": [{"min": 2.5, "max": 3}]}


def test_fill_nulls(session_with_maps):
    """NULL cells are replaced by the null= value."""
    tools = Tools(session=session_with_maps)

    tools.r_null(map="map_fill_nulls", null=1)

    described = tools.r_describe(map="map_fill_nulls", format="json", flags="1").json
    assert described == {"has_nulls": False, "values": [1, 2, 3]}
