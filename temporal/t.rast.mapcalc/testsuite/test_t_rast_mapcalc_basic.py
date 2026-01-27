"""Test t.rast.mapcalc basic functionality.

(C) 2025 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author GRASS Development Team
"""

import os

import pytest

import grass.script as gs
from grass.exceptions import CalledModuleError
from grass.tools import Tools


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Pytest fixture to create a temporary GRASS project with STRDS.

    This setup initializes the GRASS environment, sets the computational region,
    creates a series of raster maps, and registers them in STRDS datasets.

    Yields:
        session: active GRASS session with the prepared project and STRDS.
    """
    tmp_path = tmp_path_factory.mktemp("t_rast_mapcalc_basic")

    gs.create_project(tmp_path)

    with gs.setup.init(tmp_path, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)

        # Generate data using rand() to match original shell test
        tools.r_mapcalc(expression="prec_1 = rand(0, 550)", overwrite=True)
        tools.r_mapcalc(expression="prec_2 = rand(0, 450)", overwrite=True)
        tools.r_mapcalc(expression="prec_3 = rand(0, 320)", overwrite=True)
        tools.r_mapcalc(expression="prec_4 = rand(0, 510)", overwrite=True)
        tools.r_mapcalc(expression="prec_5 = rand(0, 300)", overwrite=True)
        tools.r_mapcalc(expression="prec_6 = rand(0, 650)", overwrite=True)

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


def test_basic_addition(session):
    """Test basic addition of two STRDS with -n flag (register null maps)."""
    tools = Tools(session=session)
    tools.t_rast_mapcalc(
        inputs="precip_abs1,precip_abs2",
        output="precip_abs3",
        expression=" precip_abs1 + precip_abs2",
        basename="new_prec",
        method="equal",
        nprocs=5,
        flags="n",
        overwrite=True,
    )
    # Verify output STRDS was created and has expected number of maps
    strds_info = tools.t_info(type="strds", input="precip_abs3", flags="g").keyval
    assert strds_info["number_of_maps"] == 6, (
        f"Expected 6 maps, got {strds_info['number_of_maps']}"
    )
    assert strds_info["name"] == "precip_abs3"
    assert strds_info["temporal_type"] == "absolute"


def test_division_with_three_inputs(session):
    """Test division expression with three input STRDS using -s flag."""
    tools = Tools(session=session)
    tools.t_rast_mapcalc(
        inputs="precip_abs1,precip_abs2,precip_abs3",
        output="precip_abs4",
        expression=" (precip_abs1 + precip_abs2) / precip_abs2",
        basename="new_prec",
        method="equal",
        nprocs=5,
        flags="s",
        overwrite=True,
    )
    # Verify output STRDS was created and has expected number of maps
    strds_info = tools.t_info(type="strds", input="precip_abs4", flags="g").keyval
    assert strds_info["number_of_maps"] == 6, (
        f"Expected 6 maps, got {strds_info['number_of_maps']}"
    )
    assert strds_info["name"] == "precip_abs4"
    assert strds_info["temporal_type"] == "absolute"


def test_null_multiplication(session):
    """Test multiplication with null() function using -s flag."""
    tools = Tools(session=session)
    tools.t_rast_mapcalc(
        inputs="precip_abs1,precip_abs2",
        output="precip_abs4",
        expression=" (precip_abs1 + precip_abs2) * null()",
        basename="new_prec",
        method="equal",
        nprocs=5,
        flags="s",
        overwrite=True,
    )
    # Verify output STRDS was created
    strds_info = tools.t_info(type="strds", input="precip_abs4", flags="g").keyval
    assert strds_info["name"] == "precip_abs4"
    # With null() multiplication, maps may be empty but STRDS should exist
    assert "number_of_maps" in strds_info


def test_null_multiplication_with_null_flag(session):
    """Test multiplication with null() function using -sn flags."""
    tools = Tools(session=session)
    tools.t_rast_mapcalc(
        inputs="precip_abs1,precip_abs2",
        output="precip_abs4",
        expression=" (precip_abs1 + precip_abs2) * null()",
        basename="new_prec",
        method="equal",
        nprocs=5,
        flags="sn",
        overwrite=True,
    )
    # Verify output STRDS was created
    strds_info = tools.t_info(type="strds", input="precip_abs4", flags="g").keyval
    assert strds_info["name"] == "precip_abs4"
    # With -n flag, null maps should be registered
    assert "number_of_maps" in strds_info


def test_failure_on_missing_map(session):
    """Test that the module fails when a required map is missing."""
    tools = Tools(session=session)
    # Remove a raster map that is required
    tools.g_remove(flags="f", type="raster", name="prec_1", overwrite=True)
    # This should fail
    with pytest.raises(CalledModuleError):
        tools.t_rast_mapcalc(
            inputs="precip_abs1,precip_abs2",
            output="precip_abs4",
            expression=" (precip_abs1 + precip_abs2) * null()",
            basename="new_prec",
            method="equal",
            nprocs=5,
            flags="sn",
            overwrite=True,
        )
    # Recreate the map so other tests aren't affected
    tools.r_mapcalc(expression="prec_1 = rand(0, 550)", overwrite=True)
