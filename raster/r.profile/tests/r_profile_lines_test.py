# SPDX-FileCopyrightText: 2026 Anna Petrasova
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of r.profile with profile lines from a vector map"""

from io import StringIO

import pytest

from grass.tools import ToolError

# Two lines in a region of 6 columns and 5 rows with resolution 1.
# Line 1 runs north through cell centers of column 1, line 2 runs east
# along the top row and the raster value is the row number (1 at the top).
# As with coordinates, the end point of the last segment is not sampled.
LINES = """\
L 2 1
 0.5 0.5
 0.5 4.5
 1 1
L 2 1
 1.5 4.5
 5.5 4.5
 1 2
"""


@pytest.fixture
def lines(session_tools):
    """Vector map with the two lines from LINES, each with a table record"""
    session_tools.v_in_ascii(
        input=StringIO(LINES), output="lines", format="standard", flags="n"
    )
    session_tools.v_db_addtable(map="lines")
    return "lines"


def test_each_line_is_separate_profile(session_tools, lines):
    """Each line gets its category and its distance starts at zero"""
    result = session_tools.r_profile(input="rows_raster", lines=lines, format="json")
    by_cat = {}
    for point in result:
        by_cat.setdefault(point["cat"], []).append(point)
    assert set(by_cat) == {1, 2}
    assert [p["distance"] for p in by_cat[1]] == pytest.approx([0, 1, 2, 3])
    assert [p["value"] for p in by_cat[1]] == [5, 4, 3, 2]
    assert [p["distance"] for p in by_cat[2]] == pytest.approx([0, 1, 2, 3])
    assert [p["value"] for p in by_cat[2]] == [1, 1, 1, 1]


def test_vertices_are_turning_points(session_tools):
    """Each vertex is sampled and the distance keeps running across vertices

    The vertices of the first three segments are closer than the
    resolution, so only their start points are sampled, and the repeated
    vertex is sampled once.
    """
    session_tools.v_in_ascii(
        input=StringIO(
            "L 6 1\n 0.5 0.5\n 0.5 0.8\n 0.5 1.1\n 0.5 1.4\n 0.5 1.4\n 0.5 4.5\n 1 1\n"
        ),
        output="vertices",
        format="standard",
        flags="n",
    )
    result = session_tools.r_profile(
        input="rows_raster", lines="vertices", format="json"
    )
    assert [p["distance"] for p in result] == pytest.approx(
        [0, 0.3, 0.6, 0.9, 1.9, 2.9, 3.9]
    )
    assert [p["value"] for p in result] == [5, 5, 4, 4, 3, 2, 1]


def test_cats_selects_lines(session_tools, lines):
    result = session_tools.r_profile(
        input="rows_raster", lines=lines, cats=2, format="json"
    )
    assert {point["cat"] for point in result} == {2}


def test_invalid_cats_selects_nothing(session_tools, lines):
    result = session_tools.r_profile(
        input="rows_raster", lines=lines, cats="abc", format="json"
    )
    assert result.json == []


def test_where_selects_lines(session_tools, lines):
    result = session_tools.r_profile(
        input="rows_raster", lines=lines, where="cat = 1", format="json"
    )
    assert {point["cat"] for point in result} == {1}


def test_coordinates_flag_adds_coordinates(session_tools, lines):
    result = session_tools.r_profile(
        input="rows_raster", lines=lines, cats=1, format="json", flags="g"
    )
    assert [p["easting"] for p in result] == pytest.approx([0.5, 0.5, 0.5, 0.5])
    assert [p["northing"] for p in result] == pytest.approx([0.5, 1.5, 2.5, 3.5])


def test_csv_and_plain_have_cat_column(session_tools, lines):
    csv = session_tools.r_profile(
        input="rows_raster", lines=lines, cats=2, format="csv"
    ).text.splitlines()
    assert csv[0] == "cat,distance,value"
    assert csv[1] == "2,0.000000,1"
    plain = session_tools.r_profile(
        input="rows_raster", lines=lines, cats=2, format="plain"
    ).text.splitlines()
    assert plain[0] == "2 0.000000 1"
    plain = session_tools.r_profile(
        input="rows_raster", lines=lines, cats=2, format="plain", flags="g"
    ).text.splitlines()
    assert plain[0] == "2 1.500000 4.500000 0.000000 1"


def test_line_without_category(session_tools):
    """A line without category is profiled with a null category"""
    session_tools.v_in_ascii(
        input=StringIO("L 2\n 0.5 0.5\n 0.5 2.5\n"),
        output="no_cat",
        format="standard",
        flags="n",
    )
    result = session_tools.r_profile(input="rows_raster", lines="no_cat", format="json")
    assert [p["cat"] for p in result] == [None, None]
    csv = session_tools.r_profile(
        input="rows_raster", lines="no_cat", format="csv"
    ).text.splitlines()
    assert csv[1] == "*,0.000000,5"


def test_lines_exclusive_with_coordinates(session_tools, lines):
    with pytest.raises(ToolError):
        session_tools.r_profile(
            input="rows_raster", lines=lines, coordinates=(0.5, 0.5, 0.5, 4.5)
        )


def test_selection_requires_valid_layer(session_tools, lines):
    with pytest.raises(ToolError):
        session_tools.r_profile(
            input="rows_raster", lines=lines, layer=-1, cats=1, format="json"
        )


def test_cats_requires_lines(session_tools):
    with pytest.raises(ToolError):
        session_tools.r_profile(
            input="rows_raster", coordinates=(0.5, 0.5, 0.5, 4.5), cats=1
        )
