import os
import pytest
import grass.script as gs


@pytest.fixture
def setup_maps(tmp_path):
    """Set up a GRASS session and create test raster maps."""

    # Initialize GRASS project
    project = tmp_path / "r_clump_project"
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
    """Test SURFER (Golden Software) ASCII grid output."""
    session = setup_maps

    output = gs.read_command(
        "r.out.ascii", input="custom_map", flags="s", env=session.env
    )

    expected_output = """DSAA
3 3
0.5 2.5
0.5 2.5
1.000000 3.000000
3 3 3

2 2 2

1 1 1"""

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
