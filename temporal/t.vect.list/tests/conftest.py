"""Fixture for t.vect.list test"""

import io
import os
from datetime import datetime
from types import SimpleNamespace

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def space_time_vector_dataset(tmp_path_factory):
    """Start a session and create a vector time series

    Returns object with attributes about the dataset.
    """
    tmp_path = tmp_path_factory.mktemp("vector_time_series")
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(
            s=0,
            n=80,
            w=0,
            e=120,
            b=0,
            t=50,
            res=10,
            res3=10,
        )
        names = [f"precipitation_{i}" for i in range(1, 7)]
        coords = [f"{i * 10}|{i * 10}" for i in range(1, 7)]
        for name, coord in zip(names, coords, strict=False):
            tools.v_in_ascii(
                input=io.StringIO(coord),
                output=name,
                format="point",
            )
        dataset_name = "precipitation"
        tools.t_create(
            type="stvds",
            temporaltype="absolute",
            output=dataset_name,
            title="Precipitation",
            description="Random series generated for tests",
        )
        dataset_file = tmp_path / "names.txt"
        dataset_file.write_text("\n".join(names))
        tools.t_register(
            type="vector",
            flags="i",
            input=dataset_name,
            file=dataset_file,
            start="2001-01-01",
            increment="1 month",
        )
        times = [datetime(2001, i, 1) for i in range(1, len(names) + 1)]
        yield SimpleNamespace(
            session=session,
            name=dataset_name,
            vector_names=names,
            start_times=times,
        )
