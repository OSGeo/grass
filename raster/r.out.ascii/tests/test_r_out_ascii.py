import os
import pytest
import grass.script as gs


@pytest.fixture
def setup_maps(tmp_path):
    """Set up a GRASS session and create test raster maps."""

    project = tmp_path / "r_out_ascii_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        # Set the region
        gs.run_command(
            "g.region",
            n=3,
            s=0,
            e=3,
            w=0,
            res=1,
            env=session.env,
        )

        gs.mapcalc(
            "custom_map = row()",
            overwrite=True,
            env=session.env,
        )

        gs.mapcalc(
            "custom_map2 = if(row() == 1, null(), if(row() == 2, 2.555, if(row() == 3, 3, null())))",
            overwrite=True,
            env=session.env,
        )

        yield session  # Pass the session to tests


def clean_output(output):
    """Normalize output by stripping whitespace from each line."""
    return "\n".join([line.strip() for line in output.strip().splitlines()])


def test_ascii_basic(setup_maps):
    """Test default ascii output."""
    session = setup_maps

    output = gs.read_command("r.out.ascii", input="custom_map", env=session.env)

    expected_output = """north: 3
south: 0
east: 3
west: 0
rows: 3
cols: 3
1 1 1
2 2 2
3 3 3"""

    output_lines = clean_output(output)
    expected_lines = clean_output(expected_output)

    assert output_lines == expected_lines


def test_ascii_surfer(setup_maps):
    """Test SURFER (Golden Software) ASCII grid output with width flag."""
    session = setup_maps

    output = gs.read_command(
        "r.out.ascii", input="custom_map", flags="s", width="2", env=session.env
    )

    expected_output = """DSAA
3 3
0.5 2.5
0.5 2.5
1.000000 3.000000
3 3
3

2 2
2

1 1
1"""

    output_lines = clean_output(output)
    expected_lines = clean_output(expected_output)

    assert output_lines == expected_lines


def test_ascii_modflow(setup_maps):
    """Test MODFLOW (USGS) ASCII array output."""
    session = setup_maps

    output = gs.read_command(
        "r.out.ascii", input="custom_map", flags="m", env=session.env
    )

    expected_output = """INTERNAL  1  (FREE)  -1
 1 1 1
 2 2 2
 3 3 3"""

    output_lines = clean_output(output)
    expected_lines = clean_output(expected_output)

    assert output_lines == expected_lines


def test_file_output(setup_maps, tmp_path):
    """Test file output"""
    session = setup_maps

    output_file = tmp_path / "output_r_out_ascii.txt"
    gs.read_command(
        "r.out.ascii",
        input="custom_map",
        output=output_file,
        overwrite=True,
        env=session.env,
    )

    file_content = output_file.read_text().strip()

    expected_output = """north: 3
south: 0
east: 3
west: 0
rows: 3
cols: 3
1 1 1
2 2 2
3 3 3"""

    assert clean_output(file_content) == expected_output


def test_precision_null(setup_maps):
    """Test precision and null value flags."""
    session = setup_maps

    output = gs.read_command(
        "r.out.ascii",
        input="custom_map2",
        precision="2",
        null_value="-",
        env=session.env,
    )

    expected_output = """north: 3
south: 0
east: 3
west: 0
rows: 3
cols: 3
- - -
2.56 2.56 2.56
3 3 3"""

    output_lines = clean_output(output)
    expected_lines = clean_output(expected_output)

    assert output_lines == expected_lines


def test_h_i_flag(setup_maps):
    """Test -h (suppress header information) and -i (force integer values) flags."""
    session = setup_maps

    output = gs.read_command(
        "r.out.ascii", input="custom_map2", flags="hi", env=session.env
    )

    expected_output = """* * *
3 3 3
3 3 3"""

    output_lines = clean_output(output)
    expected_lines = clean_output(expected_output)

    assert output_lines == expected_lines
