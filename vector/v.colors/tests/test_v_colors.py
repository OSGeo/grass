import os
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
    Fixture to create a vector map with attributes in one step (faster setup).

    Uses the Tools interface to create a project and initialize the region,
    then imports point data with associated attribute values directly from
    an in-memory string (avoiding temporary file creation).

    Yields:
        tuple: (map_name, session) for use in tests
    """
    tmp_path = tmp_path_factory.mktemp("v_colors_project")
    project = tmp_path / "grassdata"
    gs.create_project(str(project))

    with gs.setup.init(str(project), env=os.environ.copy()) as session:
        # Define geographic region extent and resolution for tests
        gs.run_command("g.region", n=10, s=0, e=10, w=0, res=1, env=session.env)

        # Prepare simple coordinates and attribute values as pipe-separated text
        coords = "1|1|10\n2|2|20\n3|3|30"
        tools = Tools(session=session)

        # Import points into vector map 'test_points' from string input
        tools.v_in_ascii(
            input=io.StringIO(coords),
            output="test_points",
            separator="|",
            x=1,
            y=2,
            columns="x DOUBLE PRECISION, y DOUBLE PRECISION, val DOUBLE PRECISION",
            overwrite=True,
            env=session.env,
        )

        yield "test_points", session


def test_color_by_category(simple_vector_map):
    """
    Verify color assignment based on category (cat) values.

    Runs 'v.colors' with use=cat and validates that:
    - Numeric categories exist
    - Special categories 'default' and 'nv' are present
    - Colors are valid hex RGB strings
    """
    mapname, session = simple_vector_map
    gs.run_command("v.colors", map=mapname, use="cat", color="blues", env=session.env)
    output = gs.read_command(
        "v.colors.out", map=mapname, format="json", env=session.env
    )
    data = json.loads(output)

    values = [str(rule["value"]) for rule in data]

    numeric_cats = [v for v in values if v.isdigit()]

    assert numeric_cats, "No numeric categories found"
    assert "default" in values, "'default' not found in values"
    assert "nv" in values, "'nv' not found in values"

    colors = [rule["color"] for rule in data]
    for c in colors:
        assert re.match(r"^#[0-9A-Fa-f]{6}$", c), f"Invalid color format: {c}"


def test_color_by_attr_column(simple_vector_map):
    """
    Verify color assignment based on an attribute column ('val').

    Checks that v.colors creates distinct colors for attribute values.
    """
    mapname, session = simple_vector_map
    gs.run_command(
        "v.colors", map=mapname, use="attr", column="val", color="ryg", env=session.env
    )
    output = gs.read_command(
        "v.colors.out", map=mapname, format="json", env=session.env
    )

    data = json.loads(output)

    # Exclude special categories and verify expected number of rules
    rules = [rule for rule in data if str(rule["value"]) not in ("nv", "default")]
    assert len(rules) >= 3

    colors = [rule["color"] for rule in rules]
    assert len(set(colors)) == len(colors)  # Ensure all colors are distinct


def test_rgb_column_assignment(simple_vector_map):
    """
    Verify that RGB values are correctly assigned to the GRASSRGB column in the attribute table.
    """
    mapname, session = simple_vector_map
    gs.run_command(
        "v.colors",
        map=mapname,
        use="attr",
        column="val",
        color="reds",
        rgb_column="GRASSRGB",
        env=session.env,
    )
    out = gs.read_command(
        "v.db.select", map=mapname, columns="GRASSRGB", env=session.env
    )
    lines = out.strip().splitlines()[1:]  # Skip header line

    assert lines, "No RGB data found"
    assert all(":" in line and len(line.split(":")) == 3 for line in lines), (
        "Invalid RGB format"
    )


def test_remove_color_table(simple_vector_map):
    """
    Verify that the color table can be removed successfully from the vector map.
    """
    mapname, session = simple_vector_map
    gs.run_command("v.colors", map=mapname, use="cat", color="blues", env=session.env)
    gs.run_command("v.colors", map=mapname, flags="r", env=session.env)
    output = gs.read_command("v.colors.out", map=mapname, env=session.env)

    assert output.strip() == ""


def test_constant_attribute_value(simple_vector_map):
    """
    Test color assignment behavior when all attribute values are identical.

    Expects all RGB values to be the same in the attribute column.
    """
    mapname, session = simple_vector_map
    gs.run_command(
        "v.db.update", map=mapname, column="val", value="42", env=session.env
    )
    gs.run_command(
        "v.colors",
        map=mapname,
        use="attr",
        column="val",
        color="greens",
        rgb_column="GRASSRGB",
        env=session.env,
    )
    out = gs.read_command(
        "v.db.select", map=mapname, columns="GRASSRGB", env=session.env
    )
    lines = list(filter(None, out.strip().splitlines()))
    if lines and lines[0] == "GRASSRGB":
        lines = lines[1:]  # Skip header if present
    assert len(set(lines)) == 1, "RGB values are not identical as expected"


def test_invalid_column_error(simple_vector_map):
    """
    Ensure that an error is raised when attempting to color by a non-existent attribute column.
    """
    mapname, session = simple_vector_map
    with pytest.raises(grass.exceptions.CalledModuleError):
        gs.run_command(
            "v.colors",
            map=mapname,
            use="attr",
            column="nonexistent",
            color="reds",
            rgb_column="GRASSRGB",
            env=session.env,
        )


def test_custom_rules_file(tmp_path, simple_vector_map):
    """
    Verify color assignment using a custom color rules file.

    The rules file maps categories to colors explicitly.
    Checks that the RGB values assigned in the database match the expected colors.
    """
    mapname, session = simple_vector_map
    rules_file = tmp_path / "rules.txt"
    rules_file.write_text("1 red\n2 green\n3 blue\n")
    gs.run_command(
        "v.colors",
        map=mapname,
        rules=str(rules_file),
        rgb_column="GRASSRGB",
        env=session.env,
    )
    out = gs.read_command(
        "v.db.select", map=mapname, columns="GRASSRGB", format="json", env=session.env
    )
    data = json.loads(out)

    actual_colors = [record["GRASSRGB"] for record in data["records"]]
    expected_colors = ["255:0:0", "0:255:0", "0:0:255"]

    assert actual_colors == expected_colors, "RGB colors do not match expected values"


def test_overwrite_rgb_column(simple_vector_map):
    """
    Test that overwriting the RGB color column with a new color scheme changes the values.
    """
    mapname, session = simple_vector_map
    gs.run_command(
        "v.colors",
        map=mapname,
        use="attr",
        column="val",
        color="reds",
        rgb_column="GRASSRGB",
        env=session.env,
    )
    out1 = gs.read_command(
        "v.db.select", map=mapname, columns="GRASSRGB", env=session.env
    )

    gs.run_command(
        "v.colors",
        map=mapname,
        use="attr",
        column="val",
        color="blues",
        rgb_column="GRASSRGB",
        env=session.env,
    )
    out2 = gs.read_command(
        "v.db.select", map=mapname, columns="GRASSRGB", env=session.env
    )

    assert out1 != out2, "RGB column values did not change after overwrite"


def test_use_cat_column(simple_vector_map):
    """
    Test RGB assignment based on category (cat) values.

    Checks that assigned RGB values conform to expected format.
    """
    mapname, session = simple_vector_map
    gs.run_command(
        "v.colors",
        map=mapname,
        use="cat",
        color="reds",
        rgb_column="GRASSRGB",
        env=session.env,
    )
    out = gs.read_command(
        "v.db.select", map=mapname, columns="GRASSRGB", env=session.env
    )
    lines = out.strip().splitlines()
    rgb_pattern = re.compile(r"^\d{1,3}:\d{1,3}:\d{1,3}$")

    for line in lines[1:]:  # Skip header
        rgb = line.strip()
        assert rgb_pattern.match(rgb), f"Invalid RGB format: {rgb}"

    data_lines = [line for line in lines if rgb_pattern.match(line.strip())]
    assert len(data_lines) > 0


def test_colors_out_from_attr(simple_vector_map):
    """
    Test output of `v.colors.out` when using attribute-based coloring.

    Verifies that the output contains expected values.
    """
    mapname, session = simple_vector_map
    gs.run_command(
        "v.colors",
        map=mapname,
        use="attr",
        column="val",
        color="greens",
        env=session.env,
    )
    out = gs.read_command(
        "v.colors.out",
        map=mapname,
        column="val",
        format="plain",
        color_format="rgb",
        env=session.env,
    )
    assert "1" in out


def test_logarithmic_color_scale(simple_vector_map):
    """
    Test application of logarithmic color scale using the `-g` flag.

    Uses Tools.v_db_select with JSON format to retrieve RGB values,
    parses the JSON output, and asserts valid RGB formatting and value ranges.
    """
    mapname, session = simple_vector_map
    gs.run_command(
        "v.colors",
        map=mapname,
        use="attr",
        column="val",
        color="reds",
        rgb_column="GRASSRGB",
        flags="g",
        env=session.env,
    )
    tools = Tools(session=session)
    data = tools.v_db_select(map=mapname, columns="GRASSRGB", format="json")

    # Extract raw JSON string from ToolResult's 'stdout' or 'output' attribute
    raw_json = getattr(data, "stdout", None) or getattr(data, "output", None)
    assert raw_json is not None, "No JSON output found in ToolResult"

    parsed = json.loads(raw_json)

    rgb_pattern = re.compile(r"^\d{1,3}:\d{1,3}:\d{1,3}$")
    for rec in parsed.get("records", []):
        rgb = rec.get("GRASSRGB", "").strip()
        assert rgb_pattern.match(rgb), f"Invalid RGB format: {rgb}"
        values = list(map(int, rgb.split(":")))
        assert all(0 <= v <= 255 for v in values), f"RGB value out of range: {rgb}"
