import os
from pathlib import Path

import grass.script as gs
import pytest


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
        file="test_raster",
        flags="n",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found)
    assert result["name"] == "test_raster"
    assert result["file"] == str(path / "cell" / "test_raster")

    found = gs.read_command(
        "g.findfile",
        element="cell",
        file="test_raster",
        flags="n",
        format="shell",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found)
    assert result["name"] == "test_raster"
    assert result["file"] == str(path / "cell" / "test_raster")

    found = gs.read_command(
        "g.findfile",
        element="cats",
        file="test_raster",
        flags="n",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found)
    assert result["name"] == "test_raster"
    assert result["file"] == str(path / "cats" / "test_raster")

    found = gs.read_command(
        "g.findfile",
        element="cell",
        file="test_vector",
        flags="n",
        format="shell",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found)
    assert result["name"] == ""
    assert result["file"] == ""


def test_find_vector(session):
    """Test that g.findfile found the vector"""
    gisenv = gs.gisenv(env=session.env)
    path = Path(gisenv["GISDBASE"]) / gisenv["LOCATION_NAME"] / gisenv["MAPSET"]
    found = gs.read_command(
        "g.findfile",
        element="vector",
        file="test_vector",
        flags="n",
        format="shell",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found)
    assert result["name"] == "test_vector"
    assert result["file"] == str(path / "vector" / "test_vector")

    found = gs.read_command(
        "g.findfile",
        element="vector",
        file="test_raster",
        flags="n",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found)
    assert result["name"] == ""
    assert result["file"] == ""


def test_find_raster_3d(session):
    """Test that g.findfile found the raster_3d"""
    gisenv = gs.gisenv(env=session.env)
    path = Path(gisenv["GISDBASE"]) / gisenv["LOCATION_NAME"] / gisenv["MAPSET"]
    found = gs.read_command(
        "g.findfile",
        element="raster_3d",
        file="test_raster_3d",
        flags="n",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found)
    assert result["name"] == "test_raster_3d"
    assert result["file"] == str(path / "grid3" / "test_raster_3d")

    found = gs.read_command(
        "g.findfile",
        element="grid3",
        file="test_raster_3d",
        flags="n",
        format="shell",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found)
    assert result["name"] == "test_raster_3d"
    assert result["file"] == str(path / "grid3" / "test_raster_3d")

    found = gs.read_command(
        "g.findfile",
        element="grid3",
        file="test_raster",
        flags="n",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found)
    assert result["name"] == ""
    assert result["file"] == ""


def test_find_region(session):
    """Test that g.findfile found the region"""
    gisenv = gs.gisenv(env=session.env)
    path = Path(gisenv["GISDBASE"]) / gisenv["LOCATION_NAME"] / gisenv["MAPSET"]
    found = gs.read_command(
        "g.findfile",
        element="region",
        file="test_region",
        flags="n",
        format="shell",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found)
    assert result["name"] == "test_region"
    assert result["file"] == str(path / "windows" / "test_region")

    found = gs.read_command(
        "g.findfile",
        element="windows",
        file="test_region",
        flags="n",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found)
    assert result["name"] == "test_region"
    assert result["file"] == str(path / "windows" / "test_region")

    found = gs.read_command(
        "g.findfile",
        element="windows",
        file="test_raster",
        flags="n",
        errors="ignore",
        env=session.env,
    )
    result = gs.parse_key_val(found)
    assert result["name"] == ""
    assert result["file"] == ""


def test_t_flag(session):
    found = gs.run_command(
        "g.findfile",
        element="region",
        file="test_region",
        flags="t",
        errors="status",
        env=session.env,
    )
    assert found == 0

    found = gs.run_command(
        "g.findfile",
        element="region",
        file="test_raster",
        flags="t",
        errors="status",
        env=session.env,
    )
    assert found == 1


def test_json_format(session):
    gisenv = gs.gisenv(env=session.env)
    path = Path(gisenv["GISDBASE"]) / gisenv["LOCATION_NAME"] / gisenv["MAPSET"]
    result = gs.parse_command(
        "g.findfile",
        element="raster",
        file="test_raster",
        format="json",
        env=session.env,
    )
    expected = {
        "file": str(path / "cell" / "test_raster"),
        "fullname": "test_raster@PERMANENT",
        "mapset": "PERMANENT",
        "name": "test_raster",
    }
    assert result == expected

    result = gs.parse_command(
        "g.findfile",
        element="cell",
        file="test_vector",
        format="json",
        env=session.env,
    )
    expected = {
        "file": None,
        "fullname": None,
        "mapset": None,
        "name": None,
    }
    assert result == expected
