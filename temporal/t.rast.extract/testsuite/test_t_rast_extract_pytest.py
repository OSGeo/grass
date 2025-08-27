"""Test t.rast.extract.

(C) 2025 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Stefan Blumentrath
"""

import os
import shutil

import grass.script as gs
import pytest
from grass.tools import Tools


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Pytest fixture to create a temporary GRASS project with a vector map
    containing sample points and associated attribute data.

    This setup initializes the GRASS environment, sets the computational region,
    imports point data from ASCII format, creates an attribute column 'value',
    and populates it with numeric values for testing classification.

    Yields:
        session: active GRASS session with the prepared project and vector map.

    """
    # Create a single temporary project directory once per module
    tmp_path = tmp_path_factory.mktemp("raster_time_series")

    # Remove if exists (defensive cleanup)
    if tmp_path.exists():
        shutil.rmtree(tmp_path)

    gs.create_project(tmp_path)

    with gs.setup.init(
        tmp_path.parent,
        location=tmp_path.name,
        env=os.environ.copy(),
    ) as session:
        tools = Tools(session=session)
        tools.g_mapset(mapset="perc", flags="c")
        tools.g_gisenv(set="TGIS_USE_CURRENT_MAPSET=1")
        tools.g_region(s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)
        for i in range(1, 7):
            tools.r_mapcalc(expression=f"prec_{i} = {i}00", overwrite=True)
            tools.r_semantic_label(
                map=f"prec_{i}",
                semantic_label=f"S2_{i}",
                overwrite=True,
            )

        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output="prec",
            title="A test",
            description="A test",
            overwrite=True,
            env=session.env,
        )
        tools.t_register(
            flags="i",
            type="raster",
            input="prec",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            start="2001-01-01",
            increment="3 months",
            overwrite=True,
            env=session.env,
        )
        yield session


def test_selection(session):
    """Perform a simple selection by datetime."""
    tools = Tools(session=session)
    gisenv = gs.gisenv(env=session.env)
    tools.t_rast_extract(
        input="prec",
        output="prec_1",
        basename="prec_1",
        where="start_time > '2001-06-01'",
        verbose=True,
        overwrite=True,
        env=session.env,
    )
    assert gisenv["MAPSET"] == "perc"


def test_selection_and_expression(session):
    """Perform a selection and a r.mapcalc expression with full name."""
    tools = Tools(session=session)
    gisenv = gs.gisenv(env=session.env)
    tools.t_rast_extract(
        input="prec",
        output="prec_2",
        where="start_time > '2001-06-01'",
        expression=" if(prec@perc>400,prec@perc,null())",
        basename="prec_2",
        nprocs=2,
        verbose=True,
        overwrite=True,
    )
    strds = tools.t_list().text
    assert "prec_2" in strds
    assert gisenv["MAPSET"] == "perc"


def test_selection_and_expression_simple_name(session):
    """Perform a selection and a r.mapcalc expression with simple name."""
    tools = Tools(session=session)
    gisenv = gs.gisenv(env=session.env)
    tools.t_rast_extract(
        input="prec",
        output="prec_3",
        where="start_time > '2001-06-01'",
        expression=" if(prec> 400, prec, null())",
        basename="prec_3",
        nprocs=2,
        verbose=True,
        overwrite=True,
    )
    strds = tools.t_list().text
    assert "prec_3" in strds
    assert gisenv["MAPSET"] == "perc"
