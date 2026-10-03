"""Fixture for t.rast3d.list test"""

import os
from types import SimpleNamespace

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def space_time_raster3d_dataset(tmp_path_factory):
    """Start a session and create a 3D raster time series

    Returns an object with metadata attributes about the generated dataset.
    """
    tmp_path = tmp_path_factory.mktemp("raster3d_time_series")
    project = tmp_path / "test_project3d"
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
            tbres=10,
        )

        names = [f"vol_{i}" for i in range(1, 7)]
        max_values = [550, 445, 320, 510, 300, 650]

        for name, value in zip(names, max_values, strict=True):
            tools.r3_mapcalc(
                expression=f"{name} = rand(0, {value})",
                seed=1,
            )

        std_dataset = "volume_abs1"
        tools.t_create(
            type="str3ds",
            temporaltype="absolute",
            output=std_dataset,
            title="A test with sequential files",
            description="A test with sequential files",
        )

        file_std = tmp_path / "names_std.txt"
        file_std.write_text("\n".join(names))
        tools.t_register(
            type="raster_3d",
            flags="i",
            input=std_dataset,
            file=file_std,
            start="2001-01-01",
            increment="1 months",
        )

        yield SimpleNamespace(
            session=session,
            name=std_dataset,
            raster_names=names,
        )
