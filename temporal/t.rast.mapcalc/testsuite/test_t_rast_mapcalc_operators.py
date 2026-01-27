"""Test t.rast.mapcalc temporal operators.

(C) 2025 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author GRASS Development Team
"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Pytest fixture to create a temporary GRASS project with STRDS.

    This setup initializes the GRASS environment, sets the computational region,
    creates a series of raster maps, and registers them in STRDS datasets.

    Yields:
        session: active GRASS session with the prepared project and STRDS.
    """
    tmp_path = tmp_path_factory.mktemp("t_rast_mapcalc_operators")

    gs.create_project(tmp_path)

    with gs.setup.init(tmp_path, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)

        # Generate data with -s flag (silent) to match original shell test
        tools.r_mapcalc(expression="prec_1 = rand(0, 550)", flags="s", overwrite=True)
        tools.r_mapcalc(expression="prec_2 = rand(0, 450)", flags="s", overwrite=True)
        tools.r_mapcalc(expression="prec_3 = rand(0, 320)", flags="s", overwrite=True)
        tools.r_mapcalc(expression="prec_4 = rand(0, 510)", flags="s", overwrite=True)
        tools.r_mapcalc(expression="prec_5 = rand(0, 300)", flags="s", overwrite=True)
        tools.r_mapcalc(expression="prec_6 = rand(0, 650)", flags="s", overwrite=True)

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


def test_start_time_end_time_operators(session):
    """Test start_time() and end_time() temporal operators."""
    tools = Tools(session=session)
    tools.t_rast_mapcalc(
        inputs="precip_abs1,precip_abs2",
        output="precip_abs3",
        expression="if(start_time() >= 0 && end_time() >= 0, (precip_abs1*td() + precip_abs2) / td(), null()) ",
        basename="new_prec",
        method="equal",
        nprocs=5,
        flags="sn",
        verbose=True,
        overwrite=True,
    )
    # Verify output STRDS was created
    strds_info = tools.t_info(type="strds", input="precip_abs3", flags="g").keyval
    assert strds_info["name"] == "precip_abs3"
    assert strds_info["number_of_maps"] == 6, (
        f"Expected 6 maps, got {strds_info['number_of_maps']}"
    )
    assert strds_info["temporal_type"] == "absolute"


def test_start_time_components(session):
    """Test start_year(), start_month(), start_day(), start_hour(), start_minute(), start_second() operators."""
    tools = Tools(session=session)
    tools.t_rast_mapcalc(
        inputs="precip_abs1,precip_abs2",
        output="precip_abs3",
        expression="if(start_year()>=0&&start_month()>=0&&start_day()>=0&&start_hour()>=0&&start_minute()>=0&&start_second()>=0, (precip_abs1*td() + precip_abs2) / td(), null()) ",
        basename="new_prec",
        method="equal",
        nprocs=5,
        flags="sn",
        verbose=True,
        overwrite=True,
    )
    # Verify output STRDS was created
    strds_info = tools.t_info(type="strds", input="precip_abs3", flags="g").keyval
    assert strds_info["name"] == "precip_abs3"
    assert strds_info["number_of_maps"] == 6, (
        f"Expected 6 maps, got {strds_info['number_of_maps']}"
    )


def test_end_time_components(session):
    """Test end_year(), end_month(), end_day(), end_hour(), end_minute(), end_second() operators."""
    tools = Tools(session=session)
    tools.t_rast_mapcalc(
        inputs="precip_abs1,precip_abs2",
        output="precip_abs3",
        expression="if(end_year()>=0&&end_month()>=0&&end_day()>=0&&end_hour()>=0&&end_minute()>=0&&end_second()>=0, (precip_abs1*td() + precip_abs2) / td(), null()) ",
        basename="new_prec",
        method="equal",
        nprocs=5,
        flags="sn",
        verbose=True,
        overwrite=True,
    )
    # Verify output STRDS was created
    strds_info = tools.t_info(type="strds", input="precip_abs3", flags="g").keyval
    assert strds_info["name"] == "precip_abs3"
    assert strds_info["number_of_maps"] == 6, (
        f"Expected 6 maps, got {strds_info['number_of_maps']}"
    )


def test_start_doy_dow_operators(session):
    """Test start_doy() and start_dow() temporal operators."""
    tools = Tools(session=session)
    tools.t_rast_mapcalc(
        inputs="precip_abs1,precip_abs2",
        output="precip_abs3",
        expression="if(start_doy() >= 0 && start_dow() >= 0, (precip_abs1*td() + precip_abs2) / td(), null()) ",
        basename="new_prec",
        method="equal",
        nprocs=5,
        flags="sn",
        verbose=True,
        overwrite=True,
    )
    # Verify output STRDS was created
    strds_info = tools.t_info(type="strds", input="precip_abs3", flags="g").keyval
    assert strds_info["name"] == "precip_abs3"
    assert strds_info["number_of_maps"] == 6, (
        f"Expected 6 maps, got {strds_info['number_of_maps']}"
    )


def test_end_doy_dow_operators(session):
    """Test end_doy() and end_dow() temporal operators."""
    tools = Tools(session=session)
    tools.t_rast_mapcalc(
        inputs="precip_abs1,precip_abs2",
        output="precip_abs3",
        expression="if(end_doy() >= 0 && end_dow() >= 0, (precip_abs1*td() + precip_abs2) / td(), null()) ",
        basename="new_prec",
        method="equal",
        nprocs=5,
        flags="sn",
        verbose=True,
        overwrite=True,
    )
    # Verify output STRDS was created
    strds_info = tools.t_info(type="strds", input="precip_abs3", flags="g").keyval
    assert strds_info["name"] == "precip_abs3"
    assert strds_info["number_of_maps"] == 6, (
        f"Expected 6 maps, got {strds_info['number_of_maps']}"
    )


def test_map_comparison_operator(session):
    """Test comparison operator with map names."""
    tools = Tools(session=session)
    tools.t_rast_mapcalc(
        inputs="precip_abs1",
        output="precip_abs3",
        expression="if(precip_abs1 == prec_1, prec_1, null())",
        basename="new_prec",
        flags="sn",
        verbose=True,
        overwrite=True,
    )
    # Verify output STRDS was created
    strds_info = tools.t_info(type="strds", input="precip_abs3", flags="g").keyval
    assert strds_info["name"] == "precip_abs3"
    assert "number_of_maps" in strds_info
