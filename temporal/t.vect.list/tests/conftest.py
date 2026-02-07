"""Fixture for t.vect.list test"""

import os
from datetime import datetime
from types import SimpleNamespace

import pytest

import grass.script as gs


@pytest.fixture(scope="module")
def space_time_vector_dataset(tmp_path_factory):
    """Start a session and create a vector time series

    Returns object with attributes about the dataset.
    """
    tmp_path = tmp_path_factory.mktemp("vector_time_series")
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command(
            "g.region",
            s=0,
            n=80,
            w=0,
            e=120,
            res=10,
            env=session.env,
        )
        names = [f"precipitation_{i}" for i in range(1, 7)]
        coords = [f"{i * 10}|{i * 10}" for i in range(1, 7)]
        for name, coord in zip(names, coords, strict=False):
            gs.write_command(
                "v.in.ascii",
                input="-",
                stdin=coord,
                output=name,
                format="point",
                env=session.env,
            )
        dataset_name = "precipitation"
        gs.run_command(
            "t.create",
            type="stvds",
            temporaltype="absolute",
            output=dataset_name,
            title="Precipitation",
            description="Random series generated for tests",
            env=session.env,
        )
        gs.run_command(
            "t.register",
            type="vector",
            input=dataset_name,
            maps=",".join(names),
            start="2001-01-01",
            increment="1 month",
            env=session.env,
        )
        times = [datetime(2001, i, 1) for i in range(1, len(names) + 1)]
        full_names = [f"{name}@PERMANENT" for name in names]
        yield SimpleNamespace(
            session=session,
            name=dataset_name,
            vector_names=names,
            full_vector_names=full_names,
            start_times=times,
            env=session,
        )
