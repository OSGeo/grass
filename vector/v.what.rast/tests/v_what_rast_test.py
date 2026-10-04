import os
from io import StringIO

import pytest

import grass.script as gs
from grass.tools import Tools

# Points sampled in the tests, as cat: (easting, northing).
# The region is 10 by 10 cells with n=10, s=0, e=10, w=0 and res=1.
POINTS = {
    1: (0.5, 9.5),  # interior, first row and column
    2: (9.5, 0.5),  # interior, last row and column
    3: (5.5, 0),  # southern edge
    4: (10, 5.5),  # eastern edge
    5: (10, 0),  # south-eastern corner
    6: (0, 9.5),  # western edge
    7: (5.5, 10),  # northern edge
}

# Values of grid = (row() - 1) * 10 + col() at those points, as reported by
# r.what, which brings the southern and eastern edges into the region.
EXPECTED = {1: 1, 2: 100, 3: 96, 4: 50, 5: 100, 6: 1, 7: 6}


@pytest.fixture
def session_with_points(tmp_path):
    """A GRASS session with a grid raster and points on the region edges.

    The raster encodes its own cell position, so the value tells which cell
    was sampled.
    """
    project = tmp_path / "v_what_rast_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=10, s=0, e=10, w=0, res=1)
        tools.r_mapcalc(expression="grid = (row() - 1) * 10 + col()")
        text = "".join(f"{cat}|{x}|{y}\n" for cat, (x, y) in POINTS.items())
        tools.v_in_ascii(
            input=StringIO(text),
            output="points",
            separator="|",
            x=2,
            y=3,
            cat=1,
            columns="cat integer, x double precision, y double precision",
        )
        tools.v_db_addtable(map="points", columns="value integer")
        yield session


def sampled_values(tools):
    """Run v.what.rast and return the sampled value per category."""
    tools.v_what_rast(map="points", raster="grid", column="value")
    records = tools.v_db_select(map="points", format="json").json["records"]
    return {record["cat"]: record["value"] for record in records}


@pytest.mark.parametrize("cat", [3, 4, 5], ids=["south", "east", "corner"])
def test_points_on_south_and_east_edge_are_sampled(session_with_points, cat):
    """Points on the southern or eastern edge are inside the region.

    Those edges belong to the first cell beyond the region, so they have to be
    brought into the last row or column instead of being dropped.
    """
    tools = Tools(session=session_with_points)

    assert sampled_values(tools)[cat] == EXPECTED[cat]


@pytest.mark.parametrize("cat", [1, 2, 6, 7], ids=["nw", "se", "west", "north"])
def test_points_inside_region_are_sampled(session_with_points, cat):
    """Interior points and the northern and western edges are sampled."""
    tools = Tools(session=session_with_points)

    assert sampled_values(tools)[cat] == EXPECTED[cat]


def test_values_agree_with_r_what(session_with_points):
    """v.what.rast and r.what report the same value for the same coordinates."""
    tools = Tools(session=session_with_points)
    values = sampled_values(tools)

    for cat, coordinates in POINTS.items():
        queried = tools.r_what(map="grid", coordinates=coordinates, format="json").json
        assert values[cat] == queried[0]["grid"]["value"], f"category {cat}"


def test_points_outside_region_are_not_sampled(session_with_points):
    """Points beyond the region are still reported as outside and skipped."""
    tools = Tools(session=session_with_points)
    tools.v_in_ascii(
        input=StringIO("8|20|20\n"),
        output="outside",
        separator="|",
        x=2,
        y=3,
        cat=1,
        columns="cat integer, x double precision, y double precision",
    )
    tools.v_db_addtable(map="outside", columns="value integer")

    tools.v_what_rast(map="outside", raster="grid", column="value")

    records = tools.v_db_select(map="outside", format="json").json["records"]
    assert records[0]["value"] is None
