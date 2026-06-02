"""
Configuration and fixtures for t.list pytest suite.
"""

import os
from types import SimpleNamespace

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def space_time_dataset(tmp_path_factory):
    """Start an isolated session and create a raster time series.

    Returns an object with attributes about the dataset.
    """
    tmp_path = tmp_path_factory.mktemp("raster_time_series")
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
        )

        yield SimpleNamespace(
            session=session,
            name=dataset_name,
            map_names=names,
        )
