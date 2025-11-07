"""Fixtures for grass.script"""

import os

import pytest

import grass.script as gs


@pytest.fixture
def mock_no_session(monkeypatch):
    """Set the environment variables as if there would be no background session.

    Use with usefixtures (not as a parameter) to avoid warnings about an unused
    parameter::

        @pytest.mark.usefixtures("mock_no_session")
        def test_session_handling():
            pass

    There may or may not be a session in the background (we don't check either way).
    """
    # Session
    monkeypatch.delenv("GISRC", raising=False)
    monkeypatch.delenv("GIS_LOCK", raising=False)
    # Runtime
    monkeypatch.delenv("GISBASE", raising=False)
    monkeypatch.delenv("GRASS_PREFIX", raising=False)


@pytest.fixture
def empty_session(tmp_path):
    """Set up a GRASS session for the tests."""
    project = tmp_path / "test_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield session


@pytest.fixture
def session_2x2(tmp_path):
    """Set up a GRASS session for the tests."""
    project = tmp_path / "test_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command("g.region", rows=2, cols=2, env=session.env)
        gs.mapcalc("ones = 1", env=session.env)
        gs.mapcalc(
            "nulls_and_one_1_1 = if(row() == 1 && col() == 1, 1, null())",
            env=session.env,
        )
        yield session


@pytest.fixture(scope="module")
def pack_raster_file4x5_rows(tmp_path_factory):
    """Native raster in pack format in EPSG:3358"""
    tmp_path = tmp_path_factory.mktemp("pack_raster")
    project = tmp_path / "xy_test4x5"
    gs.create_project(project, epsg="3358")
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command("g.region", rows=4, cols=5, env=session.env)
        gs.mapcalc("rows = row()", env=session.env)
        output_file = tmp_path / "rows4x5.grass_raster"
        gs.run_command(
            "r.pack",
            input="rows",
            output=output_file,
            flags="c",
            superquiet=True,
            env=session.env,
        )
    return output_file
