"""Fixtures for Jupyter tests

Fixture for grass.jupyter.TimeSeries test

Fixture for ReprojectionRenderer test with simple GRASS location, raster, vector.
"""

import os
from datetime import datetime
from types import SimpleNamespace

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def space_time_raster_dataset(tmp_path_factory):
    """Start a session and create a raster time series
    Returns object with attributes about the dataset.
    """
    tmp_path = tmp_path_factory.mktemp("raster_time_series")
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project):
        gs.run_command("g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)
        names = [f"precipitation_{i}" for i in range(1, 7)]
        max_values = [550, 450, 320, 510, 300, 650]
        for name, value in zip(names, max_values, strict=False):
            gs.mapcalc(f"{name} = rand(0, {value})", seed=1)
        dataset_name = "precipitation"
        gs.run_command(
            "t.create",
            type="strds",
            temporaltype="absolute",
            output=dataset_name,
            title="Precipitation",
            description="Random series generated for tests",
        )
        dataset_file = tmp_path / "names.txt"
        dataset_file.write_text("\n".join(names))
        gs.run_command(
            "t.register",
            type="raster",
            flags="i",
            input=dataset_name,
            file=dataset_file,
            start="2001-01-01",
            increment="1 month",
        )
        # unregister a map so that we can test fill_gaps
        gs.run_command(
            "t.unregister",
            type="raster",
            input=dataset_name,
            maps=names[1],
        )
        times = [datetime(2001, i, 1) for i in range(1, len(names) + 1)]
        times.pop(1)
        full_names = [f"{name}@PERMANENT" for name in names]
        full_names.pop(1)
        names.pop(1)
        yield SimpleNamespace(
            name=dataset_name,
            raster_names=names,
            full_raster_names=full_names,
            start_times=times,
        )


@pytest.fixture(scope="module")
def simple_dataset(tmp_path_factory):
    """Start a session and create a raster time series
    Returns object with attributes about the dataset.
    """
    tmp_path = tmp_path_factory.mktemp("simple_dataset")
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project):
        gs.run_command("g.proj", flags="c", epsg=26917)
        gs.run_command("g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)
        # Create Vector
        vector_name = "point"
        gs.write_command("v.in.ascii", input="-", stdin="50|50", output=vector_name)
        # Create Random Raster
        raster_name = "precipitation"
        max_value = 100
        gs.mapcalc(f"{raster_name} = rand(0, {max_value})", seed=1)
        yield SimpleNamespace(
            raster_name=raster_name,
            full_raster_name=f"{raster_name}@PERMANENT",
            vector_name=vector_name,
            full_vector_name=f"{vector_name}@PERMANENT",
        )


@pytest.fixture
def session(tmp_path):
    project = tmp_path / "xy_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield session


@pytest.fixture
def session_with_data(tmp_path):
    project = tmp_path / "xy_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(s=0, n=5, w=0, e=2, res=1)
        tools.r_mapcalc(expression="data = row() * col()")
        yield session
