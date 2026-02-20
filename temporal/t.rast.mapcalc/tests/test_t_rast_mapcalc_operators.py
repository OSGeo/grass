"""Test t.rast.mapcalc temporal operators.

(C) 2025 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author GRASS Development Team
"""

import grass.script as gs
from grass.tools import Tools


def _assert_tinfo_key_value(tools, strds_name, reference):
    output = tools.t_info(type="strds", input=strds_name, flags="g").text
    actual = gs.parse_key_val(output, sep="=")
    for key, value in reference.items():
        assert key in actual, f"Missing key: {key}"
        assert actual[key] == value, f"{key}: expected {value!r}, got {actual[key]!r}"


def test_start_time_end_time_operators(mapcalc_session_operators):
    """Test start_time() and end_time() temporal operators."""
    tools = Tools(session=mapcalc_session_operators.session, overwrite=True)
    tools.t_rast_mapcalc(
        flags="sn",
        inputs="precip_abs1,precip_abs2",
        output="precip_abs3",
        expression="if(start_time() >= 0 && end_time() >= 0, (precip_abs1*td() + precip_abs2) / td(), null()) ",
        basename="new_prec",
        method="equal",
        nprocs=5,
        verbose=True,
    )
    reference = {
        "number_of_maps": "6",
        "temporal_type": "absolute",
        "name": "precip_abs3",
    }
    _assert_tinfo_key_value(tools, "precip_abs3", reference)


def test_start_time_components(mapcalc_session_operators):
    """Test start_year(), start_month(), start_day(), start_hour(), start_minute(), start_second() operators."""
    tools = Tools(session=mapcalc_session_operators.session, overwrite=True)
    tools.t_rast_mapcalc(
        flags="sn",
        inputs="precip_abs1,precip_abs2",
        output="precip_abs3",
        expression="if(start_year()>=0&&start_month()>=0&&start_day()>=0&&start_hour()>=0&&start_minute()>=0&&start_second()>=0, (precip_abs1*td() + precip_abs2) / td(), null()) ",
        basename="new_prec",
        method="equal",
        nprocs=5,
        verbose=True,
    )
    reference = {
        "number_of_maps": "6",
        "name": "precip_abs3",
    }
    _assert_tinfo_key_value(tools, "precip_abs3", reference)


def test_end_time_components(mapcalc_session_operators):
    """Test end_year(), end_month(), end_day(), end_hour(), end_minute(), end_second() operators."""
    tools = Tools(session=mapcalc_session_operators.session, overwrite=True)
    tools.t_rast_mapcalc(
        flags="sn",
        inputs="precip_abs1,precip_abs2",
        output="precip_abs3",
        expression="if(end_year()>=0&&end_month()>=0&&end_day()>=0&&end_hour()>=0&&end_minute()>=0&&end_second()>=0, (precip_abs1*td() + precip_abs2) / td(), null()) ",
        basename="new_prec",
        method="equal",
        nprocs=5,
        verbose=True,
    )
    reference = {
        "number_of_maps": "6",
        "name": "precip_abs3",
    }
    _assert_tinfo_key_value(tools, "precip_abs3", reference)


def test_start_doy_dow_operators(mapcalc_session_operators):
    """Test start_doy() and start_dow() temporal operators."""
    tools = Tools(session=mapcalc_session_operators.session, overwrite=True)
    tools.t_rast_mapcalc(
        flags="sn",
        inputs="precip_abs1,precip_abs2",
        output="precip_abs3",
        expression="if(start_doy() >= 0 && start_dow() >= 0, (precip_abs1*td() + precip_abs2) / td(), null()) ",
        basename="new_prec",
        method="equal",
        nprocs=5,
        verbose=True,
    )
    reference = {
        "number_of_maps": "6",
        "name": "precip_abs3",
    }
    _assert_tinfo_key_value(tools, "precip_abs3", reference)


def test_end_doy_dow_operators(mapcalc_session_operators):
    """Test end_doy() and end_dow() temporal operators."""
    tools = Tools(session=mapcalc_session_operators.session, overwrite=True)
    tools.t_rast_mapcalc(
        flags="sn",
        inputs="precip_abs1,precip_abs2",
        output="precip_abs3",
        expression="if(end_doy() >= 0 && end_dow() >= 0, (precip_abs1*td() + precip_abs2) / td(), null()) ",
        basename="new_prec",
        method="equal",
        nprocs=5,
        verbose=True,
    )
    reference = {
        "number_of_maps": "6",
        "name": "precip_abs3",
    }
    _assert_tinfo_key_value(tools, "precip_abs3", reference)


def test_map_comparison_operator(mapcalc_session_operators):
    """Test comparison operator with map names."""
    tools = Tools(session=mapcalc_session_operators.session, overwrite=True)
    tools.t_rast_mapcalc(
        flags="sn",
        inputs="precip_abs1",
        output="precip_abs3",
        expression="if(precip_abs1 == prec_1, prec_1, null())",
        basename="new_prec",
        verbose=True,
    )
    output = tools.t_info(type="strds", input="precip_abs3", flags="g").text
    strds_info = gs.parse_key_val(output, sep="=")
    assert strds_info["name"] == "precip_abs3"
    assert int(strds_info["number_of_maps"]) == 6
