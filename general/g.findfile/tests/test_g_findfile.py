import os
import pytest
from pathlib import Path

import grass.script as gs


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Set up a GRASS session for the tests."""
    tmp_path = tmp_path_factory.mktemp("grass_session")
    project = "test_project"

    # Create a test location
    gs.create_project(tmp_path, project)

    # Initialize the GRASS session
    with gs.setup.init(tmp_path / project, env=os.environ.copy()) as session:
        gs.run_command("g.region", rows=3, cols=3, env=session.env)
        gs.run_command("r.mapcalc", expression="test_raster = 1", env=session.env)
        gs.run_command("r3.mapcalc", expression="test_raster_3d = 1", env=session.env)
        gs.run_command("v.edit", map="test_vector", tool="create", env=session.env)
        gs.run_command("g.region", save="test_region", env=session.env)
        yield session


def test_find_raster(session):
    """Test that g.findfile found the raster"""
    gisenv = gs.gisenv(env=session.env)
    path = Path(gisenv["GISDBASE"]) / gisenv["LOCATION_NAME"] / gisenv["MAPSET"]
    found = gs.read_command(
        "g.findfile",
        element="raster",
        name="test_raster",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found, env=session.env)
    assert result["name"] == "test_raster"
    assert result["file"] == path / "cell" / "test_raster"

    found = gs.read_command(
        "g.findfile",
        element="cell",
        name="test_raster",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found, env=session.env)
    assert result["name"] == "test_raster"
    assert result["file"] == path / "cell" / "test_raster"

    found = gs.read_command(
        "g.findfile",
        element="cats",
        name="test_raster",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found, env=session.env)
    assert result["name"] == "test_raster"
    assert result["file"] == path / "cats" / "test_raster"

    found = gs.read_command(
        "g.findfile",
        element="cell",
        name="test_vector",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found, env=session.env)
    assert result["name"] == ""
    assert result["file"] == ""


def test_find_vector(session):
    """Test that g.findfile found the vector"""
    gisenv = gs.gisenv(env=session.env)
    path = Path(gisenv["GISDBASE"]) / gisenv["LOCATION_NAME"] / gisenv["MAPSET"]
    found = gs.read_command(
        "g.findfile",
        element="vector",
        name="test_vector",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found, env=session.env)
    assert result["name"] == "test_vector"
    assert result["file"] == path / "vector" / "test_vector"

    found = gs.read_command(
        "g.findfile",
        element="vector",
        name="test_raster",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found, env=session.env)
    assert result["name"] == ""
    assert result["file"] == ""


def test_find_raster_3d(session):
    """Test that g.findfile found the raster_3d"""
    gisenv = gs.gisenv(env=session.env)
    path = Path(gisenv["GISDBASE"]) / gisenv["LOCATION_NAME"] / gisenv["MAPSET"]
    found = gs.read_command(
        "g.findfile",
        element="raster3d",
        name="test_raster_3d",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found, env=session.env)
    assert result["name"] == "test_raster_3d"
    assert result["file"] == path / "grid3" / "test_raster_3d"

    found = gs.read_command(
        "g.findfile",
        element="grid3",
        name="test_raster_3d",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found, env=session.env)
    assert result["name"] == "test_raster_3d"
    assert result["file"] == path / "grid3" / "test_raster_3d"

    found = gs.read_command(
        "g.findfile",
        element="grid3",
        name="test_raster",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found, env=session.env)
    assert result["name"] == ""
    assert result["file"] == ""


def test_find_region(session):
    """Test that g.findfile found the region"""
    gisenv = gs.gisenv(env=session.env)
    path = Path(gisenv["GISDBASE"]) / gisenv["LOCATION_NAME"] / gisenv["MAPSET"]
    found = gs.read_command(
        "g.findfile",
        element="region",
        name="test_region",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found, env=session.env)
    assert result["name"] == "test_raster_3d"
    assert result["file"] == path / "windows" / "test_region"

    found = gs.read_command(
        "g.findfile",
        element="windows",
        name="test_region",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found, env=session.env)
    assert result["name"] == "test_region"
    assert result["file"] == path / "windows" / "test_region"

    found = gs.read_command(
        "g.findfile",
        element="grid3",
        name="test_raster",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found, env=session.env)
    assert result["name"] == ""
    assert result["file"] == ""
