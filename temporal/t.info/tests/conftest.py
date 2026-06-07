"""Fixture for t.info test with STRDS"""

import os

import pytest

import grass.script as gs
from grass.grassdb.create import create_mapset
from grass.tools import Tools


@pytest.fixture(scope="module")
def space_time_raster_dataset(tmp_path_factory):
    """Start a session and create a space time raster dataset (STRDS)

    Returns an object with metadata attributes about the generated dataset.
    """
    mapset = "tinfo_mapset"
    tmp_path = tmp_path_factory.mktemp("temporal_tools")
    project = tmp_path / "test_project_strds"
    gs.create_project(project)
    create_mapset(project / mapset)

    with gs.setup.init(project / mapset, env=os.environ.copy()) as session:
        tools = Tools(session=session)

        tools.g_region(
            s=0,
            n=80,
            w=0,
            e=120,
            res=10,
            flags="p",
        )

        for i in range(1, 4):
            tools.r_mapcalc(expression=f"prec_{i} = {i}", overwrite=True)

        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output="precip_abs1",
            title="A test",
            description="A test",
        )

        tools.t_register(
            type="raster",
            flags="i",
            input="precip_abs1",
            maps="prec_1,prec_2,prec_3",
            start="2001-01-01",
            increment="1 months",
        )

        yield mapset, session


@pytest.fixture(scope="module")
def space_time_vector_dataset(tmp_path_factory):
    """Start a session and create a space time raster dataset (STRDS)

    Returns an object with metadata attributes about the generated dataset.
    """
    mapset = "tinfo_mapset"
    tmp_path = tmp_path_factory.mktemp("temporal_tools")
    project = tmp_path / "test_project_strds"
    gs.create_project(project)
    create_mapset(project / mapset)

    with gs.setup.init(project / mapset, env=os.environ.copy()) as session:
        tools = Tools(session=session)

        tools.g_region(
            s=0,
            n=80,
            w=0,
            e=120,
            res=10,
            flags="p",
        )

        for i in range(1, 4):
            tools.v_random(
                output=f"vect_{i}",
                npoints=20,
                seed=i,
                overwrite=True,
            )

        tools.t_create(
            type="stvds",
            temporaltype="absolute",
            output="vect_abs1",
            title="A test STVDS",
            description="Test vector time series",
            overwrite=True,
        )

        tools.t_register(
            type="vector",
            flags="i",
            input="vect_abs1",
            maps="vect_1,vect_2,vect_3",
            start="2001-01-01",
            increment="1 months",
            overwrite=True,
        )

        yield mapset, session
