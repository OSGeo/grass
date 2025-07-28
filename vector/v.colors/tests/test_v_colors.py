import re
import pytest
import json
import io

import grass.script as gs
import grass.exceptions
from grass.tools import Tools


@pytest.fixture(scope="module")
def simple_vector_map(tmp_path_factory):
    """
    Creates a basic GRASS GIS environment with a simple vector map containing attributes.

    This fixture initializes a temporary GRASS project and region using `Tools`.
    It imports three points with associated attribute values into a vector map
    named 'test_points'.
    """
    tmp_path = tmp_path_factory.mktemp("v_colors_project")
    project = tmp_path / "grassdata"
    gs.create_project(project)

    with gs.setup.init(project) as session:
        tools = Tools(session=session)
        tools.g_region(n=10, s=0, e=10, w=0, res=1)

        coords = "1|1|10\n2|2|20\n3|3|30"
        tools.v_in_ascii(
            input=io.StringIO(coords),
            output="test_points",
            separator="|",
            x=1,
            y=2,
            columns="x DOUBLE PRECISION, y DOUBLE PRECISION, val DOUBLE PRECISION",
        )

        yield "test_points", session


def test_color_by_category(simple_vector_map):
    """
    Test assigning colors based on category (cat) values using 'v.colors'.

    Verifies:
    - At least one numeric category is present
    - 'default' and 'nv' categories are present
    - Colors are valid 6-digit hex codes
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_colors(map=mapname, use="cat", color="blues")
    result = tools.v_colors_out(map=mapname, format="json")
    rules = json.loads(result.stdout)

    values = [str(rule["value"]) for rule in rules]
    assert any(v.isdigit() for v in values), (
        "Expected at least one numeric category, found none."
    )
    assert "default" in values, "'default' category not found in color rules."
    assert "nv" in values, "'nv' (null value) category not found in color rules."

    for c in [rule["color"] for rule in rules]:
        assert re.match(r"^#[0-9A-Fa-f]{6}$", c), f"Invalid hex RGB format: {c}"


def test_color_by_attr_column(simple_vector_map):
    """
    Test assigning colors based on attribute column 'val'.

    Ensures:
    - Expected number of rules
    - All colors are distinct
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_colors(map=mapname, use="attr", column="val", color="ryg")
    result = tools.v_colors_out(map=mapname, format="json")
    rules = json.loads(result.stdout)
    filtered = [r for r in rules if str(r["value"]) not in ("nv", "default")]

    assert len(filtered) >= 3, (
        f"Expected at least 3 color rules, found {len(filtered)}."
    )
    colors = [r["color"] for r in filtered]
    assert len(set(colors)) == len(colors), "Color values are not unique."


def test_rgb_column_assignment(simple_vector_map):
    """
    Test whether the GRASSRGB column is correctly filled with RGB values.

    Validates:
    - RGB values are assigned
    - Format matches 'R:G:B' format
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_colors(
        map=mapname, use="attr", column="val", color="reds", rgb_column="GRASSRGB"
    )
    result = tools.v_db_select(map=mapname, columns="GRASSRGB", format="json")
    data = json.loads(result.stdout)
    rgbs = [r["GRASSRGB"] for r in data["records"]]

    assert rgbs, "No RGB values found in GRASSRGB column."
    for rgb in rgbs:
        assert re.match(r"^\d{1,3}:\d{1,3}:\d{1,3}$", rgb), f"Invalid RGB format: {rgb}"


def test_remove_color_table(simple_vector_map):
    """
    Test removal of color table using v.colors -r flag.

    Ensures no color rules remain afterward.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_colors(map=mapname, use="cat", color="blues")
    tools.v_colors(map=mapname, flags="r")
    result = tools.v_colors_out(map=mapname)
    assert result.stdout.strip() == "", "Color table was not removed as expected."


def test_constant_attribute_value(simple_vector_map):
    """
    Test behavior when all attribute values are the same.

    Ensures:
    - All resulting RGB values are identical
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_db_update(map=mapname, column="val", value="42")
    tools.v_colors(
        map=mapname, use="attr", column="val", color="greens", rgb_column="GRASSRGB"
    )
    result = tools.v_db_select(map=mapname, columns="GRASSRGB", format="json")
    data = json.loads(result.stdout)

    rgbs = [r["GRASSRGB"] for r in data["records"]]
    assert len(set(rgbs)) == 1, (
        "Expected all RGB values to be identical, but found multiple."
    )


def test_invalid_column_error(simple_vector_map):
    """
    Test that using a non-existent column raises an error.

    Expects CalledModuleError from GRASS when column doesn't exist.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    with pytest.raises(grass.exceptions.CalledModuleError, match=r"Column.*not found"):
        tools.v_colors(
            map=mapname,
            use="attr",
            column="nonexistent",
            color="reds",
            rgb_column="GRASSRGB",
        )


def test_custom_rules_file(tmp_path, simple_vector_map):
    """
    Test use of custom color rules file.

    Ensures RGB values in GRASSRGB column match explicit definitions.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    rules_file = tmp_path / "rules.txt"
    rules_file.write_text("1 red\n2 green\n3 blue\n")

    tools.v_colors(map=mapname, rules=str(rules_file), rgb_column="GRASSRGB")
    result = tools.v_db_select(map=mapname, columns="GRASSRGB", format="json")
    data = json.loads(result.stdout)

    actual_colors = [r["GRASSRGB"] for r in data["records"]]
    expected = ["255:0:0", "0:255:0", "0:0:255"]
    assert actual_colors == expected, f"Expected {expected}, but got {actual_colors}"


def test_overwrite_rgb_column(simple_vector_map):
    """
    Test that RGB column is updated after applying a new color scheme.

    Ensures the RGB values differ after second application.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_colors(
        map=mapname, use="attr", column="val", color="reds", rgb_column="GRASSRGB"
    )
    result1 = tools.v_db_select(map=mapname, columns="GRASSRGB", format="json")
    rgbs1 = [r["GRASSRGB"] for r in json.loads(result1.stdout)["records"]]

    tools.v_colors(
        map=mapname, use="attr", column="val", color="blues", rgb_column="GRASSRGB"
    )
    result2 = tools.v_db_select(map=mapname, columns="GRASSRGB", format="json")
    rgbs2 = [r["GRASSRGB"] for r in json.loads(result2.stdout)["records"]]

    assert rgbs1 != rgbs2, (
        "RGB column values did not change after applying a new color scheme."
    )


def test_use_cat_column(simple_vector_map):
    """
    Test RGB assignment using category values and RGB column.

    Verifies valid RGB format and presence of values.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_colors(map=mapname, use="cat", color="reds", rgb_column="GRASSRGB")
    result = tools.v_db_select(map=mapname, columns="GRASSRGB", format="json")
    data = json.loads(result.stdout)

    pattern = re.compile(r"^\d{1,3}:\d{1,3}:\d{1,3}$")
    valid = [r["GRASSRGB"] for r in data["records"] if pattern.match(r["GRASSRGB"])]

    assert valid, "No valid RGB values found in GRASSRGB column."
    for rgb in valid:
        assert pattern.match(rgb), f"Invalid RGB format: {rgb}"


def test_colors_out_from_attr(simple_vector_map):
    """
    Test output of 'v.colors.out' for attribute-based rules.

    Ensures output contains a rule for the first category.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_colors(map=mapname, use="attr", column="val", color="greens")
    result = tools.v_colors_out(
        map=mapname, column="val", format="plain", color_format="rgb"
    )
    assert "1" in result.stdout, (
        "Expected to find category value '1' in v.colors.out output."
    )


def test_logarithmic_color_scale(simple_vector_map):
    """
    Test use of logarithmic color scaling (flag -g).

    Ensures RGB values remain in valid format and numeric range.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_colors(
        map=mapname,
        use="attr",
        column="val",
        color="reds",
        rgb_column="GRASSRGB",
        flags="g",
    )
    result = tools.v_db_select(map=mapname, columns="GRASSRGB", format="json")
    data = json.loads(result.stdout)

    rgb_pattern = re.compile(r"^\d{1,3}:\d{1,3}:\d{1,3}$")
    for rec in data.get("records", []):
        rgb = rec.get("GRASSRGB", "").strip()
        assert rgb_pattern.match(rgb), f"Invalid RGB format: {rgb}"
        values = list(map(int, rgb.split(":")))
        assert all(0 <= v <= 255 for v in values), f"RGB value out of range: {rgb}"
