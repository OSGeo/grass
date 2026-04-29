"""Test t.rast.mapcalc basic functionality.

(C) 2025 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

import pytest

import grass.exceptions
from grass.tools import Tools


def test_basic_addition(mapcalc_session, assert_tinfo):
    """Test basic addition of two STRDS with -n flag (register null maps)."""
    tools = Tools(session=mapcalc_session, overwrite=True)
    tools.t_rast_mapcalc(
        flags="n",
        inputs="precip_abs1,precip_abs2",
        output="precip_abs3",
        expression="precip_abs1 + precip_abs2",
        basename="new_prec",
        method="equal",
        nprocs=3,
    )
    assert_tinfo(
        tools,
        "precip_abs3",
        {
            "number_of_maps": 6,
            "temporal_type": "absolute",
            "name": "precip_abs3",
        },
    )


def test_division_with_three_inputs(mapcalc_session, assert_tinfo):
    """Test division expression across three input STRDS.

    First creates precip_abs3 = precip_abs1 + precip_abs2, then runs the
    test expression (precip_abs1 + precip_abs2) / precip_abs2 over all
    three STRDS with the -s (sampling) flag.
    """
    tools = Tools(session=mapcalc_session, overwrite=True)
    tools.t_rast_mapcalc(
        flags="n",
        inputs="precip_abs1,precip_abs2",
        output="precip_abs3",
        expression="precip_abs1 + precip_abs2",
        basename="new_prec",
        method="equal",
        nprocs=3,
    )
    tools.t_rast_mapcalc(
        flags="s",
        inputs="precip_abs1,precip_abs2,precip_abs3",
        output="precip_abs4",
        expression="(precip_abs1 + precip_abs2) / precip_abs2",
        basename="new_prec",
        method="equal",
        nprocs=3,
    )
    assert_tinfo(
        tools,
        "precip_abs4",
        {
            "number_of_maps": 6,
            "temporal_type": "absolute",
            "name": "precip_abs4",
        },
    )


def test_null_multiplication(mapcalc_session, assert_tinfo):
    """Test multiplication with null() function using -s flag."""
    tools = Tools(session=mapcalc_session, overwrite=True)
    tools.t_rast_mapcalc(
        flags="s",
        inputs="precip_abs1,precip_abs2",
        output="precip_abs4",
        expression="(precip_abs1 + precip_abs2) * null()",
        basename="new_prec",
        method="equal",
        nprocs=3,
    )
    assert_tinfo(
        tools,
        "precip_abs4",
        {"name": "precip_abs4", "number_of_maps": 0},
    )


def test_null_multiplication_with_null_flag(mapcalc_session, assert_tinfo):
    """Test multiplication with null() function using -sn flags."""
    tools = Tools(session=mapcalc_session, overwrite=True)
    tools.t_rast_mapcalc(
        flags="sn",
        inputs="precip_abs1,precip_abs2",
        output="precip_abs4",
        expression="(precip_abs1 + precip_abs2) * null()",
        basename="new_prec",
        method="equal",
        nprocs=3,
    )
    assert_tinfo(
        tools,
        "precip_abs4",
        {"name": "precip_abs4", "number_of_maps": 6},
    )


def test_failure_on_missing_map(mapcalc_session):
    """Test that the module fails when a registered raster is missing.

    Hides prec_1 by renaming it so that the STRDS still references the name
    but the raster does not resolve. The rename is restored in a finally
    block so subsequent tests in this module-scoped session see prec_1.
    """
    tools = Tools(session=mapcalc_session, overwrite=True)
    tools.g_rename(raster="prec_1,prec_1_hidden")
    try:
        with pytest.raises(grass.exceptions.CalledModuleError):
            tools.t_rast_mapcalc(
                flags="sn",
                inputs="precip_abs1,precip_abs2",
                output="precip_abs4",
                expression="(precip_abs1 + precip_abs2) * null()",
                basename="new_prec",
                method="equal",
                nprocs=3,
            )
    finally:
        tools.g_rename(raster="prec_1_hidden,prec_1")
