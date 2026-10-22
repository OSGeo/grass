"""
Configuration and fixtures for t.list pytest suite.
"""

import os
from io import StringIO
from types import SimpleNamespace

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def space_time_dataset(tmp_path_factory):
    """Start an isolated session and create raster and vector time series.

    Returns an object with attributes about the datasets.
    """
    tmp_path = tmp_path_factory.mktemp("time_series")
    project = tmp_path / "test_project"
    gs.create_project(project)

    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)

        tools.g_region(s=0, n=80, w=0, e=120, res=10)

        names = [f"temp_{i}" for i in range(1, 4)]
        for i, name in enumerate(names, start=1):
            tools.r_mapcalc(expression=f"{name} = {i}", overwrite=True)

        dataset_name = "temperature_dataset"

        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output=dataset_name,
            title="Temperature",
            description="Random series generated for tests",
        )

        dataset_file = tmp_path / "names.txt"
        dataset_file.write_text("\n".join(names))
        tools.t_register(
            type="raster",
            input=dataset_name,
            file=dataset_file,
            start="2026-01-01",
            increment="1 month",
            flags="i",
        )

        vector_names = [f"vect_{i}" for i in range(1, 3)]
        coords = [f"{i * 10}|{i * 10}" for i in range(1, 3)]
        for name, coord in zip(vector_names, coords, strict=True):
            tools.v_in_ascii(
                input=StringIO(coord),
                output=name,
                format="point",
            )
        stvds_name = "vector_dataset"
        tools.t_create(
            type="stvds",
            temporaltype="absolute",
            output=stvds_name,
            title="Vector dataset",
            description="Vector series generated for tests",
        )
        vector_file = tmp_path / "vector_names.txt"
        vector_file.write_text("\n".join(vector_names))
        tools.t_register(
            type="vector",
            input=stvds_name,
            file=vector_file,
            start="2026-01-01",
            increment="1 month",
            flags="i",
        )

        gs.run_command("g.mapset", mapset="user1", flags="c", env=session.env)

        raster_dataset_name = "precipitation_dataset"
        tools.r_mapcalc(expression="precip_1 = 2", overwrite=True)
        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output=raster_dataset_name,
            title="Precip",
            description="user1 Dataset",
        )
        tools.t_register(
            type="raster",
            input=raster_dataset_name,
            maps="precip_1",
            start="2026-01-01",
            increment="1 month",
            flags="i",
        )

        gs.run_command("g.mapset", mapset="PERMANENT", env=session.env)

        yield SimpleNamespace(
            session=session,
            name=dataset_name,
            raster_dataset1=dataset_name,
            raster_dataset2=raster_dataset_name,
            map_names=names,
            raster_dataset=raster_dataset_name,
            stvds_name=stvds_name,
            vector_names=vector_names,
        )


@pytest.fixture(scope="module")
def empty_session(tmp_path_factory):
    """Start an isolated session with no temporal database."""
    tmp_path = tmp_path_factory.mktemp("empty_time_series")
    project = tmp_path / "empty_project"
    gs.create_project(project)

    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield SimpleNamespace(session=session)
