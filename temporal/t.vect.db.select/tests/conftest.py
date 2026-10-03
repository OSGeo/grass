"""Fixture for t.vect.db.select tests"""

import os
from types import SimpleNamespace

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def space_time_vector_dataset(tmp_path_factory):
    """Start a temporary session and create space time vector datasets.

    Returns an object containing metadata attributes about the generated datasets.
    """
    tmp_path = tmp_path_factory.mktemp("vector_time_series")
    project = tmp_path / "test_project_vector"
    gs.create_project(project)

    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)

        tools.g_region(s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)

        for i in range(1, 4):
            tools.r_mapcalc(expression=f"prec_int_{i} = {i}00.0", overwrite=True)
            tools.r_mapcalc(expression=f"prec_inst_{i} = {i}00.0", overwrite=True)

        tools.v_random(output="prec_points", n=3, seed=1, flags="z", overwrite=True)

        interval_strds = "precip_abs1"
        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output=interval_strds,
            title="A test",
            description="A test",
        )
        tools.t_register(
            type="raster",
            flags="i",
            input=interval_strds,
            maps=",".join([f"prec_int_{i}" for i in range(1, 4)]),
            start="2001-03-01 00:00:00",
            increment="1 months",
        )

        interval_stvds = "prec_observer_interval"
        tools.t_vect_observe_strds(
            input="prec_points",
            strds=interval_strds,
            output=interval_stvds,
            vector=interval_stvds,
            column="observation",
        )

        instance_strds = "precip_abs2"
        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output=instance_strds,
            title="B test",
            description="B test",
        )
        tools.t_register(
            type="raster",
            input=instance_strds,
            maps=",".join([f"prec_inst_{i}" for i in range(1, 4)]),
            start="2004-01-01 00:00:00",
            increment="3 months",
        )

        instance_stvds = "prec_observer_instance"
        tools.t_vect_observe_strds(
            input="prec_points",
            strds=instance_strds,
            output=instance_stvds,
            vector=instance_stvds,
            column="observation",
        )

        yield SimpleNamespace(
            session=session,
            interval_name=interval_stvds,
            instance_name=instance_stvds,
        )
