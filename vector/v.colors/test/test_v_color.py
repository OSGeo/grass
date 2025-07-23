import os
import re
import pytest
import grass.script as gs
import grass.exceptions


@pytest.fixture
def simple_vector_map(tmp_path):
    """Fixture to create a simple vector map with 3 points and a numeric attribute column 'val'."""
    project = tmp_path / "v_colors_project"
    gs.create_project(project)

    with gs.setup.init(project, env=os.environ.copy()) as session:
        # Set the computational region
        gs.run_command("g.region", n=10, s=0, e=10, w=0, res=1, env=session.env)

        # Create a small ASCII file with coordinates
        coords = "1|1\n2|2\n3|3"
        path = tmp_path / "coords.csv"
        path.write_text(coords)

        # Import the points into a vector map
        gs.run_command(
            "v.in.ascii",
            input=str(path),
            output="test_points",
            separator="|",
            x=1,
            y=2,
            overwrite=True,
            env=session.env,
        )

        # Add a new attribute table and column to hold numeric values
        gs.run_command("v.db.addtable", map="test_points", env=session.env)
        gs.run_command(
            "v.db.addcolumn",
            map="test_points",
            columns="val DOUBLE PRECISION",
            env=session.env,
        )

        # Update the 'val' column for each feature
        for i, val in enumerate([10, 20, 30], start=1):
            gs.run_command(
                "v.db.update",
                map="test_points",
                column="val",
                value=str(val),
                where=f"cat = {i}",
                env=session.env,
            )

        yield "test_points", session


def test_color_by_category(simple_vector_map):
    """Test color assignment based on category (cat) values."""
    mapname, session = simple_vector_map
    gs.run_command("v.colors", map=mapname, use="cat", color="blues", env=session.env)
    output = gs.read_command("v.colors.out", map=mapname, env=session.env)
    lines = output.strip().splitlines()
    actual_lines = [line.split()[0] for line in lines]
    numeric_cats = [line for line in actual_lines if line.isdigit()]

    assert any(line.startswith("1 ") for line in lines)
    assert any(":" in line for line in lines)
    assert any("default" in line for line in lines)
    assert numeric_cats
    assert "default" in actual_lines
    assert "nv" in actual_lines


def test_color_by_attr_column(simple_vector_map):
    """Test color assignment based on attribute column 'val'."""
    mapname, session = simple_vector_map
    gs.run_command(
        "v.colors", map=mapname, use="attr", column="val", color="ryg", env=session.env
    )
    output = gs.read_command("v.colors.out", map=mapname, env=session.env)

    lines = [
        line
        for line in output.strip().splitlines()
        if line and not line.startswith(("default", "nv"))
    ]
    assert len(lines) >= 3
    colors = [line.split()[1] for line in lines]
    assert len(set(colors)) == len(colors)  # Ensure colors are distinct


def test_rgb_column_assignment(simple_vector_map):
    """Test whether RGB values are correctly assigned to GRASSRGB column."""
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
    lines = out.strip().splitlines()[1:]
    assert lines
    assert all(":" in line and len(line.split(":")) == 3 for line in lines)


def test_remove_color_table(simple_vector_map):
    """Test removing color table from the vector map."""
    mapname, session = simple_vector_map
    gs.run_command("v.colors", map=mapname, use="cat", color="blues", env=session.env)
    gs.run_command("v.colors", map=mapname, flags="r", env=session.env)
    output = gs.read_command("v.colors.out", map=mapname, env=session.env)
    assert output.strip() == ""


def test_constant_attribute_value(simple_vector_map):
    """Test color assignment when all attribute values are the same."""
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
        lines = lines[1:]
    assert len(set(lines)) == 1  # All colors must be the same


def test_invalid_column_error(simple_vector_map):
    """Test error is raised when using a non-existent column."""
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
    """Test color assignment using a custom color rules file."""
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
        "v.db.select", map=mapname, columns="GRASSRGB", env=session.env
    )
    assert "255:0:0" in out
    assert "0:255:0" in out
    assert "0:0:255" in out


def test_overwrite_rgb_column(simple_vector_map):
    """Test overwriting an existing RGB column with a new color scheme."""
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

    assert out1 != out2  # Color values must change


def test_use_cat_column(simple_vector_map):
    """Test RGB assignment based on category values (cat)."""
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

    for line in lines[1:]:
        rgb = line.strip()
        assert rgb_pattern.match(rgb), f"Invalid RGB format: {rgb}"

    data_lines = [line for line in lines if rgb_pattern.match(line.strip())]
    assert len(data_lines) > 0


def test_colors_out_from_attr(simple_vector_map):
    """Test `v.colors.out` output when using attribute-based coloring."""
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
    """Test application of logarithmic color scale using `-g` flag."""
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
    out = gs.read_command(
        "v.db.select", map=mapname, columns="GRASSRGB", env=session.env
    )
    lines = out.strip().splitlines()[1:]
    assert lines

    rgb_pattern = re.compile(r"^\d{1,3}:\d{1,3}:\d{1,3}$")
    for rgb in lines:
        rgb = rgb.strip()
        assert rgb_pattern.match(rgb), f"Invalid RGB format: {rgb}"
        values = list(map(int, rgb.split(":")))
        assert all(0 <= v <= 255 for v in values), f"RGB value out of range: {rgb}"
