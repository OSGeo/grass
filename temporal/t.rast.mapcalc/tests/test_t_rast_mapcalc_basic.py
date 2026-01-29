"""Test t.rast.mapcalc basic functionality.

(C) 2025 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author GRASS Development Team
"""

import pytest

import grass.script as gs
from grass.exceptions import CalledModuleError


def _assert_tinfo_key_value(env, strds_name, reference_string, sep="="):
    """Parse t.info output and assert key-value pairs match reference string."""
    output = gs.read_command(
        "t.info", type="strds", input=strds_name, flags="g", env=env
    )
    actual = gs.parse_key_val(output, sep=sep)
    reference = dict(
        line.strip().split(sep, 1)
        for line in reference_string.strip().split("\n")
        if sep in line
    )
    for key, value in reference.items():
        assert key in actual, f"Missing key: {key}"
        assert actual[key] == value, f"{key}: expected {value!r}, got {actual[key]!r}"


def test_basic_addition(mapcalc_session_basic):
    """Test basic addition of two STRDS with -n flag (register null maps)."""
    env = mapcalc_session_basic.env
    gs.run_command(
        "t.rast.mapcalc",
        flags="n",
        inputs="precip_abs1,precip_abs2",
        output="precip_abs3",
        expression=" precip_abs1 + precip_abs2",
        basename="new_prec",
        method="equal",
        nprocs=5,
        overwrite=True,
        env=env,
    )
    tinfo_string = """number_of_maps=6
temporal_type=absolute
name=precip_abs3"""
    _assert_tinfo_key_value(env, "precip_abs3", tinfo_string)


def test_division_with_three_inputs(mapcalc_session_basic):
    """Test division expression with three input STRDS using -s flag."""
    env = mapcalc_session_basic.env
    gs.run_command(
        "t.rast.mapcalc",
        flags="n",
        inputs="precip_abs1,precip_abs2",
        output="precip_abs3",
        expression=" precip_abs1 + precip_abs2",
        basename="new_prec",
        method="equal",
        nprocs=5,
        overwrite=True,
        env=env,
    )
    gs.run_command(
        "t.rast.mapcalc",
        flags="s",
        inputs="precip_abs1,precip_abs2,precip_abs3",
        output="precip_abs4",
        expression=" (precip_abs1 + precip_abs2) / precip_abs2",
        basename="new_prec",
        method="equal",
        nprocs=5,
        overwrite=True,
        env=env,
    )
    tinfo_string = """number_of_maps=6
temporal_type=absolute
name=precip_abs4"""
    _assert_tinfo_key_value(env, "precip_abs4", tinfo_string)


def test_null_multiplication(mapcalc_session_basic):
    """Test multiplication with null() function using -s flag."""
    env = mapcalc_session_basic.env
    gs.run_command(
        "t.rast.mapcalc",
        flags="s",
        inputs="precip_abs1,precip_abs2",
        output="precip_abs4",
        expression=" (precip_abs1 + precip_abs2) * null()",
        basename="new_prec",
        method="equal",
        nprocs=5,
        overwrite=True,
        env=env,
    )
    output = gs.read_command(
        "t.info", type="strds", input="precip_abs4", flags="g", env=env
    )
    strds_info = gs.parse_key_val(output, sep="=")
    assert strds_info["name"] == "precip_abs4"
    assert int(strds_info["number_of_maps"]) == 0


def test_null_multiplication_with_null_flag(mapcalc_session_basic):
    """Test multiplication with null() function using -sn flags."""
    env = mapcalc_session_basic.env
    gs.run_command(
        "t.rast.mapcalc",
        flags="sn",
        inputs="precip_abs1,precip_abs2",
        output="precip_abs4",
        expression=" (precip_abs1 + precip_abs2) * null()",
        basename="new_prec",
        method="equal",
        nprocs=5,
        overwrite=True,
        env=env,
    )
    output = gs.read_command(
        "t.info", type="strds", input="precip_abs4", flags="g", env=env
    )
    strds_info = gs.parse_key_val(output, sep="=")
    assert strds_info["name"] == "precip_abs4"
    assert int(strds_info["number_of_maps"]) == 6


def test_failure_on_missing_map(mapcalc_session_basic):
    """Test that the module fails when a required map is missing."""
    env = mapcalc_session_basic.env
    gs.run_command(
        "g.remove", flags="f", type="raster", name="prec_1", overwrite=True, env=env
    )
    with pytest.raises(CalledModuleError):
        gs.run_command(
            "t.rast.mapcalc",
            flags="sn",
            inputs="precip_abs1,precip_abs2",
            output="precip_abs4",
            expression=" (precip_abs1 + precip_abs2) * null()",
            basename="new_prec",
            method="equal",
            nprocs=5,
            overwrite=True,
            env=env,
        )
    gs.run_command(
        "r.mapcalc", expression="prec_1 = rand(0, 550)", overwrite=True, env=env
    )
