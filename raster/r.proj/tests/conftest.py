"""This is a source project with two small rasters and two destination sessions."""

import os

import pytest

import grass.script as gs

INPUT_EXPRESSION = "row() * 100 + col() + (row() * row() + col() * col()) % 13"

SRC_PROJECT = "src4326"
# Mid-latitude box for the 3857 tests.
INPUT_MID = "input_mid"
# High-latitude, full-longitude box so a north-polar frame has data to read.
INPUT_POLAR = "input_polar"


@pytest.fixture(scope="session")
def gisdbase_with_source(tmp_path_factory):
    """GISDBASE containing src4326 with the mid and polar input rasters."""
    gisdbase = tmp_path_factory.mktemp("rproj_parallel")
    gs.create_project(gisdbase / SRC_PROJECT, epsg="4326")
    with gs.setup.init(gisdbase / SRC_PROJECT, env=os.environ.copy()) as session:
        env = session.env
        gs.run_command("g.region", n=41, s=40, w=-100, e=-99, rows=50, cols=50, env=env)
        gs.run_command(
            "r.mapcalc", expression=f"{INPUT_MID} = {INPUT_EXPRESSION}", env=env
        )
        gs.run_command("g.region", n=89, s=50, w=-180, e=180, rows=50, cols=50, env=env)
        gs.run_command(
            "r.mapcalc", expression=f"{INPUT_POLAR} = {INPUT_EXPRESSION}", env=env
        )
    return gisdbase


@pytest.fixture(scope="session")
def session_3857(gisdbase_with_source):
    """Active session in an EPSG:3857 destination project."""
    gs.create_project(gisdbase_with_source / "dst3857", epsg="3857")
    with gs.setup.init(
        gisdbase_with_source / "dst3857", env=os.environ.copy()
    ) as session:
        yield session
