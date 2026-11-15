import os
import re
import io

import pytest

import grass.script as gs
import grass.exceptions
from grass.tools import Tools


@pytest.fixture
def simple_vector_map(tmp_path_factory):
    """
    Fixture to create a basic GRASS GIS environment with a simple vector map containing attributes.

    Sets up a temporary GRASS project and region. Imports three points with attribute
    values into a vector map named 'test_points'.

    Yields:
        tuple: (map name, GRASS session handle)
    """
    tmp_path = tmp_path_factory.mktemp("v_colors_project")
    project = tmp_path / "grassdata"
    gs.create_project(project)

    with gs.setup.init(project, env=os.environ.copy()) as session:
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
    Test assigning colors based on vector category (cat) values using 'v.colors'.

    Verifies:
    - At least one numeric category is present in the color rules.
    - 'default' and 'nv' categories exist in the color rules.
    - Each color is a valid 6-digit hexadecimal RGB code.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_colors(map=mapname, use="cat", color="blues")
    rules = tools.v_colors_out(map=mapname, format="json")

    assert len(rules["table"])
    assert "default" in rules
    assert "nv" in rules

    for c in [rule["color"] for rule in rules["table"]]:
        assert re.match(r"^#[0-9A-Fa-f]{6}$", c), (
            f"Invalid hex RGB color format detected: {c}"
        )
    assert re.match(r"^#[0-9A-Fa-f]{6}$", rules["nv"]), (
        f"Invalid hex RGB color format detected: {rules['nv']}"
    )
    assert re.match(r"^#[0-9A-Fa-f]{6}$", rules["default"]), (
        f"Invalid hex RGB color format detected: {rules['default']}"
    )


def test_color_by_attr_column(simple_vector_map):
    """
    Test assigning colors based on an attribute column ('val').

    Checks:
    - At least 3 distinct color rules are generated.
    - All assigned colors are unique.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_colors(map=mapname, use="attr", column="val", color="ryg")
    rules = tools.v_colors_out(map=mapname, format="json")

    assert len(rules["table"]) >= 3
    colors = [r["color"] for r in rules["table"]]
    assert len(set(colors)) == len(colors), (
        "Duplicate color values found in color rules."
    )


def test_rgb_column_assignment(simple_vector_map):
    """
    Test if the RGB column (GRASSRGB) is correctly populated with RGB values after coloring.

    Checks:
    - The GRASSRGB column contains RGB values.
    - RGB values conform to the 'R:G:B' format where R, G, B are 0-255 integers.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_colors(
        map=mapname, use="attr", column="val", color="reds", rgb_column="GRASSRGB"
    )
    data = tools.v_db_select(map=mapname, columns="GRASSRGB", format="json")
    rgbs = [r["GRASSRGB"] for r in data["records"]]

    assert rgbs, "No RGB values found in the GRASSRGB column after applying colors."
    for rgb in rgbs:
        assert re.match(r"^\d{1,3}:\d{1,3}:\d{1,3}$", rgb), (
            f"Invalid RGB format found: '{rgb}'"
        )


def test_remove_color_table(simple_vector_map):
    """
    Test removal of the color table from a vector map using the -r flag of v.colors.

    Ensures:
    - No color rules remain after removal.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session, consistent_return_value=True)

    tools.v_colors(map=mapname, use="cat", color="blues")
    tools.v_colors(map=mapname, flags="r")
    result = tools.v_colors_out(map=mapname)
    assert result.stdout.strip() == "", "Color table was not successfully removed."


def test_constant_attribute_value(simple_vector_map):
    """
    Test color assignment when all attribute values in 'val' column are identical.

    Ensures:
    - All resulting RGB values in GRASSRGB column are identical.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_db_update(map=mapname, column="val", value="42")
    tools.v_colors(
        map=mapname, use="attr", column="val", color="greens", rgb_column="GRASSRGB"
    )
    data = tools.v_db_select(map=mapname, columns="GRASSRGB", format="json")

    rgbs = [r["GRASSRGB"] for r in data["records"]]
    unique_rgbs = set(rgbs)
    assert len(unique_rgbs) == 1, (
        f"Expected all RGB values to be identical but found multiple distinct values: {unique_rgbs}"
    )


def test_invalid_column_error(simple_vector_map):
    """
    Test that using a non-existent attribute column with v.colors raises a CalledModuleError.

    The error message should mention the missing column.
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
    Test applying colors from a custom rules file supplied as a file-like object.

    Validates:
    - The RGB column matches expected RGB values from the rules.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    rule = "1 red\n2 green\n3 blue\n"

    tools.v_colors(map=mapname, rules=io.StringIO(rule), rgb_column="GRASSRGB")
    data = tools.v_db_select(map=mapname, columns="GRASSRGB", format="json")

    actual_colors = [r["GRASSRGB"] for r in data["records"]]
    expected = ["255:0:0", "0:255:0", "0:0:255"]
    assert actual_colors == expected, (
        f"Expected RGB colors {expected}, but got {actual_colors}"
    )


def test_overwrite_rgb_column(simple_vector_map):
    """
    Test that the RGB column is updated after applying a new color scheme.

    Validates:
    - RGB values before and after are different, indicating an update.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_colors(
        map=mapname, use="attr", column="val", color="reds", rgb_column="GRASSRGB"
    )
    result1 = tools.v_db_select(map=mapname, columns="GRASSRGB", format="json")
    rgbs1 = [r["GRASSRGB"] for r in result1["records"]]

    tools.v_colors(
        map=mapname, use="attr", column="val", color="blues", rgb_column="GRASSRGB"
    )
    result2 = tools.v_db_select(map=mapname, columns="GRASSRGB", format="json")
    rgbs2 = [r["GRASSRGB"] for r in result2["records"]]

    assert rgbs1 != rgbs2, (
        "RGB column values did not change after applying a new color scheme."
    )


def test_use_cat_column(simple_vector_map):
    """
    Test RGB assignment using category values with RGB column.

    Verifies:
    - Valid RGB format is present in the GRASSRGB column.
    - At least one valid RGB value exists.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_colors(map=mapname, use="cat", color="reds", rgb_column="GRASSRGB")
    data = tools.v_db_select(map=mapname, columns="GRASSRGB", format="json")

    pattern = re.compile(r"^\d{1,3}:\d{1,3}:\d{1,3}$")
    valid = [r["GRASSRGB"] for r in data["records"] if pattern.match(r["GRASSRGB"])]

    assert valid, "No valid RGB values found in GRASSRGB column."
    for rgb in valid:
        assert pattern.match(rgb), f"Invalid RGB format detected: {rgb}"


def test_colors_out_from_attr(simple_vector_map):
    """
    Test output of 'v.colors.out' for attribute-based color rules.

    Ensures:
    - The output contains a rule for the first category.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_colors(map=mapname, use="attr", column="val", color="greens")
    result = tools.v_colors_out(
        map=mapname, column="val", format="plain", color_format="rgb"
    )
    assert "1" in result.stdout, "Expected category value '1' in v.colors.out output."


def test_logarithmic_color_scale(simple_vector_map):
    """
    Test use of logarithmic color scaling flag (-g) with v.colors.

    Validates:
    - RGB values are correctly formatted.
    - RGB values are within the valid range 0-255.
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
    data = tools.v_db_select(map=mapname, columns="GRASSRGB", format="json")

    rgb_pattern = re.compile(r"^\d{1,3}:\d{1,3}:\d{1,3}$")
    for rec in data["records"]:
        rgb = rec["GRASSRGB"].strip()
        assert rgb_pattern.match(rgb), f"Invalid RGB format: {rgb}"
        values = list(map(int, rgb.split(":")))
        assert all(0 <= v <= 255 for v in values), (
            f"RGB value out of 0-255 range: {rgb}"
        )


@pytest.mark.parametrize("color_table", ["viridis", "plasma", "terrain"])
def test_v_colors_named_table(simple_vector_map, color_table):
    """
    Parametrized test for applying named color tables to a vector map.

    Checks:
    - GRASSRGB column is assigned values for each feature.
    - The number of records is as expected.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    tools.v_colors(map=mapname, column="val", color=color_table, rgb_column="GRASSRGB")

    data = tools.v_db_select(map=mapname, columns="GRASSRGB", format="json")

    assert len(data["records"]) == 3, f"Expected 3 records, got {len(data['records'])}."
    for rec in data["records"]:
        assert rec["GRASSRGB"], "GRASSRGB value is empty or null."


def test_v_colors_missing_rule_file(simple_vector_map):
    """
    Test error raised when a non-existent rules file is specified.

    Expects:
    - CalledModuleError with message indicating inability to load the rules file.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    missing_path = "/nonexistent/rules.txt"

    with pytest.raises(
        grass.exceptions.CalledModuleError, match=r"Unable to load rules file"
    ):
        tools.v_colors(map=mapname, rules=missing_path)


def test_v_colors_invalid_rule_format(simple_vector_map):
    """
    Test error raised when rules file content is invalid.

    Expects:
    - CalledModuleError with message indicating bad rule format.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    invalid_color_rule = "1 notacolor"

    with pytest.raises(grass.exceptions.CalledModuleError, match=r"bad rule"):
        tools.v_colors(map=mapname, column="val", rules=io.StringIO(invalid_color_rule))


def test_v_colors_reversed_order_rules(simple_vector_map, tmp_path):
    """
    Test applying custom color rules from a file-like object with reversed category order.

    Verifies:
    - The RGB values in GRASSRGB column match the expected explicit RGB assignments.
    """
    mapname, session = simple_vector_map
    tools = Tools(session=session)

    # Reversed order: category 3 blue, 2 green, 1 red
    rule_content = "3 0:0:255\n2 0:255:0\n1 255:0:0\n"

    tools.v_colors(
        map=mapname,
        rules=io.StringIO(rule_content),
        column="cat",
        rgb_column="GRASSRGB",
    )
    data = tools.v_db_select(map=mapname, columns="cat,GRASSRGB", format="json")

    expected = {
        1: "255:0:0",
        2: "0:255:0",
        3: "0:0:255",
    }

    for row in data["records"]:
        cat = row["cat"]
        rgb = row["GRASSRGB"]
        assert rgb == expected[cat], (
            f"For category {cat}, expected RGB {expected[cat]} but got {rgb}"
        )
