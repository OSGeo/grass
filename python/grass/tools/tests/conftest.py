"""Fixtures for grass.tools"""

import os

import pytest

import grass.script as gs
from grass.experimental import MapsetSession
from grass.tools.support import ToolResult, ToolFunctionResolver


@pytest.fixture
def xy_dataset_session(tmp_path):
    """Creates a session with XY project"""
    gs.create_project(tmp_path / "test")
    with gs.setup.init(tmp_path / "test", env=os.environ.copy()) as session:
        yield session


@pytest.fixture
def xy_mapset_non_permament(xy_dataset_session):  # pylint: disable=redefined-outer-name
    """Active session in a mapset of an XY location

    Mapset scope is function, while the location scope is module.

    The mapset is not removed by this fixture and relies on the underlying cleanup
    procedures which means that it can be examined in the temporary directories
    pytest creates.
    """
    with MapsetSession("test1", create=True, env=xy_dataset_session.env) as session:
        yield session


@pytest.fixture
def empty_result():
    return ToolResult(
        name=None, command=None, kwargs=None, returncode=None, stdout=None, stderr=None
    )


@pytest.fixture
def empty_string_result():
    return ToolResult(
        name=None, command=None, kwargs=None, returncode=None, stdout="", stderr=""
    )


@pytest.fixture
def echoing_resolver():
    return ToolFunctionResolver(run_function=lambda x: x, env=os.environ.copy())


@pytest.fixture
def rows_raster_file3x3(tmp_path):
    project = tmp_path / "xy_test3x3"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command("g.region", rows=3, cols=3, env=session.env)
        gs.mapcalc("rows = row()", env=session.env)
        output_file = tmp_path / "rows3x3.grass_raster"
        gs.run_command(
            "r.pack",
            input="rows",
            output=output_file,
            flags="c",
            superquiet=True,
            env=session.env,
        )
    return output_file


@pytest.fixture
def rows_raster_file4x5(tmp_path):
    project = tmp_path / "xy_test4x5"
    gs.create_project(project)
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
