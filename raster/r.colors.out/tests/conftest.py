"""Fixture for r.colors.out and r3.colors.out test"""

from types import SimpleNamespace

import os
import pytest
import grass.script as gs


def setup_grass_location(tmp_path, location_name):
    """Initialize a new GRASS location."""
    gs.core._create_location_xy(tmp_path, location_name)
    return tmp_path / location_name


def configure_grass_region(session_env):
    """Configure the GRASS region for testing with defined bounds and resolution."""
    gs.run_command(
        "g.region", s=0, n=90, w=0, e=100, b=0, t=50, res=10, res3=10, env=session_env
    )


def create_test_rasters(session_env):
    """Generate raster layers with specific values for testing."""
    gs.run_command(
        "r.mapcalc",
        expression="test_elev_int_1 = int(rand(-15.0, 5.0))",
        seed=1,
        env=session_env,
    )
    gs.run_command(
        "r.mapcalc",
        expression="test_elev_int_2 = int(rand(0.0, 10.0))",
        seed=1,
        env=session_env,
    )
    gs.run_command(
        "r.mapcalc",
        expression="test_elev_int_3 = int(rand(5.0, 15.0))",
        seed=1,
        env=session_env,
    )

    return "test_elev_int_1,test_elev_int_2,test_elev_int_3"


def apply_random_color_to_rasters(raster_names, session_env):
    """Apply random colors to the specified rasters."""
    gs.run_command("r.colors", map=raster_names, color="random", env=session_env)


def create_test_rasters3(session_env):
    """Generate raster3 layers with specific values for testing."""
    gs.run_command(
        "r3.mapcalc",
        expression="volume_double = double(col() + row() + depth())",
        env=session_env,
    )
    gs.run_command(
        "r3.mapcalc",
        expression=(
            "volume_double_null = if("
            "row() == 1 || row() == 5, "
            "null(), volume_double)"
        ),
        env=session_env,
    )

    return "volume_double_null"


def apply_random_color_to_rasters3(raster3_names, session_env):
    """Apply elevation colors to the specified raster3."""
    gs.run_command("r3.colors", map=raster3_names, color="elevation", env=session_env)


@pytest.fixture
def raster_color_dataset(tmp_path_factory):
    """Set up a GRASS session and create test rasters with color rules."""

    tmp_path = tmp_path_factory.mktemp("raster_color_test")
    location_name = "test_location"
    location_path = setup_grass_location(tmp_path, location_name)

    with gs.setup.init(location_path, env=os.environ.copy()) as session:
        configure_grass_region(session.env)

        raster_names = create_test_rasters(session.env)
        apply_random_color_to_rasters(raster_names, session.env)

        yield SimpleNamespace(
            session=session,
            raster_names="test_elev_int_3",
            env=session.env,
        )


@pytest.fixture
def raster3_color_dataset(tmp_path_factory):
    """Set up a GRASS session and create test raster3 with color rules."""

    tmp_path = tmp_path_factory.mktemp("raster3_color_test")
    location_name = "test_location"
    location_path = setup_grass_location(tmp_path, location_name)

    with gs.setup.init(location_path, env=os.environ.copy()) as session:
        configure_grass_region(session.env)

        raster3_names = create_test_rasters3(session.env)
        apply_random_color_to_rasters3(raster3_names, session.env)

        yield SimpleNamespace(
            session=session,
            raster3_names=raster3_names,
            env=session.env,
        )
