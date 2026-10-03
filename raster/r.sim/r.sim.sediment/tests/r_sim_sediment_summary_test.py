# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of the r.sim.sediment run summary (-p flag, format option)"""

import re

import pytest

# rows_raster from the shared fixture is a plane rising 1 m per row, 5 rows
# by 6 columns at 1 m resolution. See the r.sim.water tests for why the flow
# is slowed down with Manning's n and a small diffusion coefficient.
ELEVATION = "rows_raster"

SEDIMENT_KEYS = {
    "walkers_requested",
    "walkers_generated",
    "walkers_remaining",
    "duration",
    "simulated_time",
    "time_step",
    "time_step_sediment",
    "iterations_planned",
    "iterations_completed",
    "stopped_early",
    "mean_velocity",
    "velocity_max",
    "sigma_max",
    "mean_mannings_n",
    "mean_source_rate",
    "threads",
    "transport_capacity",
    "tlimit_erosion_deposition",
    "outputs",
}
OUTPUT_KEYS = {
    "simulated_time",
    "timestamp",
    "walkers_remaining",
    "sediment_concentration",
    "sediment_flux",
    "erosion_deposition",
    "walkers",
}
MAP_KEYS = OUTPUT_KEYS - {"simulated_time", "timestamp", "walkers_remaining"}


def timestamp_for(simulated_time):
    """Timestamp text the tool writes for a simulated time in seconds"""
    return f"{int(simulated_time / 60 + 0.5)} minutes"


def history_value(history, key):
    """Value of a key=value pair in the history text as float"""
    return float(re.search(rf"{key}=([-+.\deE]+)", history).group(1))


@pytest.fixture
def sediment_inputs(session_tools):
    """Constant water depth and soil parameter maps for a run on rows_raster"""
    session_tools.r_mapcalc(expression="water_depth = 0.1")
    session_tools.r_mapcalc(expression="detachment = 0.001")
    session_tools.r_mapcalc(expression="transport = 0.001")
    session_tools.r_mapcalc(expression="shear_stress = 0.01")
    return {
        "elevation": ELEVATION,
        "water_depth": "water_depth",
        "detachment_coeff": "detachment",
        "transport_coeff": "transport",
        "shear_stress": "shear_stress",
        "man_value": 1,
        "diffusion_coeff": 0.05,
        "random_seed": 1,
    }


def test_json_summary(session_tools, sediment_inputs):
    """JSON summary has the sediment keys and names every written map"""
    summary = session_tools.r_sim_sediment(
        **sediment_inputs,
        transport_capacity="tc",
        tlimit_erosion_deposition="et",
        sediment_concentration="conc",
        sediment_flux="flux",
        erosion_deposition="erdep",
        nwalkers=100,
        duration=1,
        flags="p",
        format="json",
    ).json
    assert set(summary) == SEDIMENT_KEYS
    assert summary["walkers_requested"] == 100
    assert summary["walkers_generated"] > 0
    assert summary["duration"] == 60
    assert summary["time_step"] > 0
    assert summary["iterations_planned"] > 1
    assert summary["iterations_completed"] <= summary["iterations_planned"]
    assert summary["time_step_sediment"] > 0
    assert summary["velocity_max"] >= summary["mean_velocity"] > 0
    assert summary["sigma_max"] > 0
    assert summary["mean_mannings_n"] == pytest.approx(1)
    assert summary["mean_source_rate"] > 0
    assert summary["threads"] == 1
    assert summary["transport_capacity"] == "tc"
    assert summary["tlimit_erosion_deposition"] == "et"

    assert len(summary["outputs"]) == 1
    output = summary["outputs"][0]
    assert set(output) == OUTPUT_KEYS
    assert output["sediment_concentration"] == "conc"
    assert output["sediment_flux"] == "flux"
    assert output["erosion_deposition"] == "erdep"
    assert output["walkers"] is None
    assert output["simulated_time"] == summary["simulated_time"]
    assert output["timestamp"] == timestamp_for(output["simulated_time"])
    for name in ("tc", "et", "conc", "flux", "erdep"):
        assert session_tools.r_info(map=name, format="json")["rows"] == 5


def test_json_summary_optional_outputs(session_tools, sediment_inputs):
    """Maps which were not requested are reported as null"""
    summary = session_tools.r_sim_sediment(
        **sediment_inputs, sediment_flux="flux", duration=1, flags="p", format="json"
    ).json
    assert summary["transport_capacity"] is None
    assert summary["tlimit_erosion_deposition"] is None
    output = summary["outputs"][0]
    assert output["sediment_flux"] == "flux"
    for key in MAP_KEYS - {"sediment_flux"}:
        assert output[key] is None


def test_plain_summary(session_tools, sediment_inputs):
    """Plain summary prints one key per line"""
    lines = session_tools.r_sim_sediment(
        **sediment_inputs, sediment_flux="flux", nwalkers=100, duration=1, flags="p"
    ).text.splitlines()
    assert "walkers_requested: 100" in lines
    assert any(line.startswith("time_step_sediment: ") for line in lines)
    assert "  sediment_flux: flux" in lines


def test_no_output_without_print_flag(session_tools, sediment_inputs):
    """Standard output stays empty unless -p is given, whatever the format"""
    for kwargs in ({}, {"format": "json"}):
        result = session_tools.r_sim_sediment(
            **sediment_inputs,
            sediment_flux="flux",
            duration=1,
            overwrite=True,
            **kwargs,
        )
        # Tools returns None when the tool produced no standard output.
        assert result is None


def test_history(session_tools, sediment_inputs):
    """Sediment flux history carries the summary values under the same keys"""
    summary = session_tools.r_sim_sediment(
        **sediment_inputs,
        sediment_flux="flux",
        nwalkers=100,
        duration=1,
        flags="p",
        format="json",
    ).json
    history = session_tools.r_info(map="flux", flags="h").text
    for key in ("walkers_requested", "walkers_generated"):
        assert history_value(history, key) == summary[key]
    for key in ("simulated_time", "time_step", "mean_velocity", "mean_mannings_n"):
        assert history_value(history, key) == pytest.approx(summary[key], abs=1e-6)
    assert "mean_infiltration=" not in history
