"""Fixture for t.rast.list test"""

import os

from datetime import datetime
from types import SimpleNamespace

import pytest

import grass.script as gs


@pytest.fixture(scope="module")
def space_time_raster_dataset(tmp_path_factory):
    """Start a session and create a raster time series

    Returns object with attributes about the dataset.
    """
    tmp_path = tmp_path_factory.mktemp("raster_time_series")
    location = "test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    with gs.setup.init(tmp_path / location, env=os.environ.copy()) as session:
        gs.run_command(
            "g.region",
            s=0,
            n=80,
            w=0,
            e=120,
            b=0,
            t=50,
            res=10,
            res3=10,
            env=session.env,
        )
        names = [f"precipitation_{i}" for i in range(1, 7)]
        max_values = [550, 450, 320, 510, 300, 650]
        for name, value in zip(names, max_values):
            gs.mapcalc(f"{name} = rand(0, {value})", seed=1, env=session.env)
        dataset_name = "precipitation"
        gs.run_command(
            "t.create",
            type="strds",
            temporaltype="absolute",
            output=dataset_name,
            title="Precipitation",
            description="Random series generated for tests",
            env=session.env,
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
            env=session.env,
        )
        times = [datetime(2001, i, 1) for i in range(1, len(names) + 1)]
        full_names = [f"{name}@PERMANENT" for name in names]
        yield SimpleNamespace(
            session=session,
            name=dataset_name,
            raster_names=names,
            full_raster_names=full_names,
            start_times=times,
            env=session,
        )
