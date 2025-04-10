import os
import pytest
import grass.script as gs


@pytest.fixture
def setup_maps(tmp_path):
    """Set up a GRASS session and create test raster maps."""

    project = tmp_path / "r_latlong_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command(
            "g.proj",
            flags="c",
            proj4="+proj=longlat +datum=WGS84 +no_defs",
            env=session.env,
        )

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

        yield session


def clean_output(output):
    """Normalize output by stripping whitespace from each line."""
    return "\n".join([line.strip() for line in output.strip().splitlines()])


def test_latlong_lat(setup_maps):
    """Test default r.latlong output"""
    session = setup_maps

    gs.run_command(
        "r.latlong", input="custom_map", output="latlong_output", env=session.env
    )

    # Export latitude map to ASCII.
    ascii_output = gs.read_command(
        "r.out.ascii", input="latlong_output", env=session.env
    )
    ascii_clean = clean_output(ascii_output)

    expected_output = """north: 3N
south: 0
east: 3E
west: 0
rows: 3
cols: 3
3 3 3
2 2 2
1 1 1"""

    assert ascii_clean == expected_output


def test_l_flag(setup_maps):
    """Test -l (longitude) flag output"""
    session = setup_maps

    gs.run_command(
        "r.latlong",
        input="custom_map",
        output="latlong_output",
        flags="l",
        env=session.env,
    )

    # Export longitude map to ASCII.
    ascii_output = gs.read_command(
        "r.out.ascii", input="latlong_output", env=session.env
    )
    ascii_clean = clean_output(ascii_output)

    expected_output = """north: 3N
south: 0
east: 3E
west: 0
rows: 3
cols: 3
0 1 2
0 1 2
0 1 2"""

    assert ascii_clean == expected_output
