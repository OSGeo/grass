"""Fixture for t.vect.univar tests"""

import io
import os
from types import SimpleNamespace

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def space_time_vector_dataset(tmp_path_factory):
    """Start a session and create a vector time series with known attribute values.

    Two point maps with deterministic values in the column "height":
    points_1 holds 10, 20, 30 and points_2 holds 40, 50, 60, so statistics
    like min, max, and mean can be asserted exactly.
    """
    tmp_path = tmp_path_factory.mktemp("t_vect_univar")
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(s=0, n=80, w=0, e=120, res=10)
        names = ["points_1", "points_2"]
        values = {"points_1": [10.0, 20.0, 30.0], "points_2": [40.0, 50.0, 60.0]}
        for name in names:
            rows = "\n".join(
                f"{(i + 1) * 10}|{(i + 1) * 10}|{value}"
                for i, value in enumerate(values[name])
            )
            tools.v_in_ascii(
                input=io.StringIO(rows),
                output=name,
                separator="pipe",
                columns="x double precision, y double precision, height double precision",
                x=1,
                y=2,
                cat=0,
            )
        dataset_name = "points"
        tools.t_create(
            type="stvds",
            temporaltype="absolute",
            output=dataset_name,
            title="Points",
            description="Point series with known values for tests",
        )
        tools.t_register(
            type="vector",
            flags="i",
            input=dataset_name,
            maps=",".join(names),
            start="2001-01-01",
            increment="1 month",
        )
        yield SimpleNamespace(
            session=session,
            name=dataset_name,
            vector_names=names,
            column="height",
            values=values,
        )
