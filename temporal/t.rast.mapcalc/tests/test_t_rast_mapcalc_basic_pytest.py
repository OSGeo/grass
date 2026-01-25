"""Test t.rast.mapcalc.

(C) 2025 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author GRASS Development Team
"""

import os
import shutil

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Create a temporary GRASS project with STRDS."""
    tmp_path = tmp_path_factory.mktemp("raster_mapcalc_basic")

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
            mapname = f"prec_{i}"
            tools.r_mapcalc(expression=f"{mapname} = {i}00", overwrite=True)

        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output="precip_abs1",
            title="A test",
            description="A test",
            overwrite=True,
        )
        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output="precip_abs2",
            title="A test",
            description="A test",
            overwrite=True,
        )
        tools.t_register(
            type="raster",
            input="precip_abs1",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            start="2001-01-01",
            increment="3 months",
            flags="i",
            overwrite=True,
        )
        tools.t_register(
            type="raster",
            input="precip_abs2",
            maps="prec_1,prec_2,prec_3,prec_4,prec_5,prec_6",
            overwrite=True,
        )

        yield session


def test_multiplication_with_null(session):
    """Test multiplication with null() using -s flag."""
    tools = Tools(session=session)
    tools.t_rast_mapcalc(
        inputs="precip_abs1,precip_abs2",
        output="precip_abs4",
        expression="(precip_abs1 + precip_abs2) * null()",
        basename="new_prec",
        method="equal",
        nprocs=5,
        overwrite=True,
    )
    strds_info = tools.t_info(input="precip_abs4", type="strds", flags="g").keyval
    assert "number_of_maps" in strds_info
