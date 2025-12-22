"""Fixtures for grass.tools"""

from __future__ import annotations
import os

import pytest

import grass.script as gs
from grass.tools import Tools
from grass.tools.support import ToolResult, ToolFunctionResolver
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from pathlib import Path


@pytest.fixture
def xy_dataset_session(tmp_path: Path):
    """Creates a session with XY project"""
    gs.create_project(tmp_path / "test")
    with gs.setup.init(tmp_path / "test", env=os.environ.copy()) as session:
        yield session


@pytest.fixture(scope="module")
def xy_dataset_session_for_module(
    tmp_path_factory: pytest.TempPathFactory,
):
    """Creates a session with XY project"""
    tmp_path = tmp_path_factory.mktemp("module_project")
    gs.create_project(tmp_path / "test")
    with gs.setup.init(tmp_path / "test", env=os.environ.copy()) as session:
        yield session


@pytest.fixture
def plain_tools(xy_dataset_session_for_module):
    with Tools(session=xy_dataset_session_for_module) as tools:
        yield tools


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


@pytest.fixture(scope="module")
def rows_raster_file3x2(tmp_path_factory: pytest.TempPathFactory) -> Path:
    """Native raster pack file

    Smallest possible file, but with rows and columns greater than one,
    and a different number of rows and columns.
    """
    tmp_path = tmp_path_factory.mktemp("rows_raster_file3x2")
    project = tmp_path / "xy_test3x2"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command("g.region", rows=3, cols=2, env=session.env)
        gs.mapcalc("rows = row()", env=session.env)
        output_file = tmp_path / "rows3x2.grass_raster"
        gs.run_command(
            "r.pack",
            input="rows",
            output=output_file,
            flags="c",
            superquiet=True,
            env=session.env,
        )
    return output_file


@pytest.fixture(scope="module")
def rows_raster_file4x5(tmp_path_factory: pytest.TempPathFactory) -> Path:
    """Native raster pack file

    Small file, but slightly larger than the smallest.
    """
    tmp_path = tmp_path_factory.mktemp("rows_raster_file4x5")
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


@pytest.fixture(scope="module")
def ones_raster_file_epsg3358(tmp_path_factory: pytest.TempPathFactory) -> Path:
    """Native raster pack with EPSG:3358"""
    tmp_path = tmp_path_factory.mktemp("ones_raster_file4x5")
    project = tmp_path / "xy_test4x5"
    gs.create_project(project, crs="EPSG:3358")
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command("g.region", rows=4, cols=5, env=session.env)
        gs.mapcalc("ones = 1", env=session.env)
        output_file = tmp_path / "ones4x5.grass_raster"
        gs.run_command(
            "r.pack",
            input="ones",
            output=output_file,
            flags="c",
            superquiet=True,
            env=session.env,
        )
    return output_file


@pytest.fixture(scope="module")
def ones_raster_file_epsg4326(tmp_path_factory: pytest.TempPathFactory) -> Path:
    """Native raster pack with EPSG:4326 (LL)"""
    tmp_path = tmp_path_factory.mktemp("ones_raster_file4x5")
    project = tmp_path / "xy_test4x5"
    gs.create_project(project, crs="EPSG:4326")
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command("g.region", rows=4, cols=5, env=session.env)
        gs.mapcalc("ones = 1", env=session.env)
        output_file = tmp_path / "ones4x5.grass_raster"
        gs.run_command(
            "r.pack",
            input="ones",
            output=output_file,
            flags="c",
            superquiet=True,
            env=session.env,
        )
    return output_file
