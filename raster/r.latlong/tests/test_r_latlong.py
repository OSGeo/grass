import os
import pytest
import grass.script as gs


def _init_session(tmp_path, epsg_code):
    """
    Create a GRASS LOCATION and return its context manager.
    """
    project = tmp_path / f"r_latlong_{epsg_code}_project"
    gs.create_project(project)
    return gs.setup.init(project, env=os.environ.copy())


@pytest.fixture
def setup_maps(tmp_path):
    """Set up a GRASS session and create test raster maps in EPSG:4326."""
    with _init_session(tmp_path, "4326") as session:
        gs.run_command(
            "g.proj",
            flags="c",
            epsg="4326",
            env=session.env,
        )

        # Define a 3×3 region in degrees
        gs.run_command(
            "g.region",
            n=3,
            s=0,
            e=3,
            w=0,
            res=1,
            env=session.env,
        )

        # Dummy raster map (values are irrelevant to r.latlong)
        gs.mapcalc(
            "custom_map = 1",
            overwrite=True,
            env=session.env,
        )

        yield session


def clean_output(output):
    """Normalize output by stripping whitespace from each line."""
    return "\n".join([line.strip() for line in output.strip().splitlines()])


def test_latlong_lat(setup_maps):
    """Test default r.latlong output (latitude)."""
    session = setup_maps

    gs.run_command(
        "r.latlong",
        input="custom_map",
        output="latlong_output",
        env=session.env,
    )

    ascii_output = gs.read_command(
        "r.out.ascii",
        input="latlong_output",
        env=session.env,
    )
    ascii_clean = clean_output(ascii_output)

    expected_output = """north: 3N
south: 0
east: 3E
west: 0
rows: 3
cols: 3
2.5 2.5 2.5
1.5 1.5 1.5
0.5 0.5 0.5"""

    assert ascii_clean == expected_output


def test_l_flag(setup_maps):
    """Test -l (longitude) flag output."""
    session = setup_maps

    gs.run_command(
        "r.latlong",
        input="custom_map",
        output="latlong_output",
        flags="l",
        env=session.env,
    )

    ascii_output = gs.read_command(
        "r.out.ascii",
        input="latlong_output",
        env=session.env,
    )
    ascii_clean = clean_output(ascii_output)

    expected_output = """north: 3N
south: 0
east: 3E
west: 0
rows: 3
cols: 3
0.5 1.5 2.5
0.5 1.5 2.5
0.5 1.5 2.5"""

    assert ascii_clean == expected_output


def test_latlong_projected_epsg_3358(tmp_path):
    """
    test r.latlong in a projected CRS (EPSG:3358),
    verify lat/long outputs stay within valid degree ranges.
    """
    with _init_session(tmp_path, "3358") as session:
        gs.run_command(
            "g.proj",
            flags="c",
            epsg="3358",
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
            "custom_map = 1",
            overwrite=True,
            env=session.env,
        )

        # Test latitude output
        gs.run_command(
            "r.latlong",
            input="custom_map",
            output="latlong_proj",
            env=session.env,
        )
        info = gs.parse_command(
            "r.info",
            flags="r",  # range: to get min/max
            map="latlong_proj",
            env=session.env,
        )
        lat_min, lat_max = float(info["min"]), float(info["max"])
        assert -90.0 <= lat_min < lat_max <= 90.0

        # Test longitude output
        gs.run_command(
            "r.latlong",
            input="custom_map",
            output="lon_proj",
            flags="l",
            env=session.env,
        )
        info_lon = gs.parse_command(
            "r.info",
            flags="r",
            map="lon_proj",
            env=session.env,
        )
        lon_min, lon_max = float(info_lon["min"]), float(info_lon["max"])
        assert -180.0 <= lon_min < lon_max <= 180.0
