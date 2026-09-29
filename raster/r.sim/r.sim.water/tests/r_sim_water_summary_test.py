"""Tests of the r.sim.water run summary (-p flag, format option)"""

import re

import pytest

# rows_raster from the shared fixture is a plane rising 1 m per row, 5 rows
# by 6 columns at 1 m resolution. Walkers move a quarter of a cell per
# iteration whatever the time step, so a large Manning's n (slow flow, long
# time step, few iterations per minute) and a small diffusion coefficient are
# needed to keep walkers in the domain for the whole run. The default
# Manning's n is used where an early stop is wanted.
ELEVATION = "rows_raster"
SLOW_FLOW = {
    "elevation": ELEVATION,
    "man_value": 8,
    "diffusion_coeff": 0.05,
    "random_seed": 1,
}

COMMON_KEYS = {
    "walkers_requested",
    "walkers_generated",
    "walkers_active",
    "duration",
    "simulated_time",
    "time_step",
    "time_coefficient",
    "iterations_planned",
    "iterations_completed",
    "iterations_per_output",
    "stopped_early",
    "elevation_min",
    "elevation_max",
    "mean_velocity",
    "mean_mannings_n",
    "mean_source_rate",
    "mean_infiltration",
    "threads",
    "outputs",
}
OUTPUT_KEYS = {
    "simulated_time",
    "timestamp",
    "walkers_active",
    "depth",
    "discharge",
    "error",
    "walkers",
}


def timestamp_for(simulated_time):
    """Timestamp text the tool writes for a simulated time in seconds"""
    return f"{int(simulated_time / 60 + 0.5)} minutes"


def history_value(history, key):
    """Value of a key=value pair in the history text as float"""
    return float(re.search(rf"{key}=([-+.\deE]+)", history).group(1))


def test_json_summary(session_tools):
    """JSON summary has the documented keys and values matching the inputs"""
    result = session_tools.r_sim_water(
        **SLOW_FLOW,
        depth="depth",
        discharge="discharge",
        nwalkers=100,
        duration=1,
        flags="p",
        format="json",
    )
    summary = result.json
    assert set(summary) == COMMON_KEYS
    assert summary["walkers_requested"] == 100
    assert summary["walkers_generated"] > 0
    assert summary["duration"] == 60
    # The coefficient is 4 whenever the water time step governs, which is
    # always the case without a sediment time step.
    assert summary["time_coefficient"] == 4
    assert summary["iterations_planned"] == int(
        summary["duration"] / (summary["time_step"] * summary["time_coefficient"])
    )
    assert summary["iterations_planned"] > 1
    assert summary["iterations_completed"] == summary["iterations_planned"]
    assert summary["simulated_time"] == int(
        summary["iterations_completed"]
        * summary["time_step"]
        * summary["time_coefficient"]
    )
    assert summary["stopped_early"] is False
    assert summary["walkers_active"] > 0
    assert summary["elevation_min"] == 1
    assert summary["elevation_max"] == 5
    assert summary["mean_velocity"] > 0
    # The harmonic mean of a constant is the constant.
    assert summary["mean_mannings_n"] == pytest.approx(8)
    assert summary["mean_source_rate"] > 0
    assert summary["mean_infiltration"] == 0
    assert summary["threads"] == 1

    assert len(summary["outputs"]) == 1
    output = summary["outputs"][0]
    assert set(output) == OUTPUT_KEYS
    assert output["depth"] == "depth"
    assert output["discharge"] == "discharge"
    assert output["error"] is None
    assert output["walkers"] is None
    assert output["simulated_time"] == summary["simulated_time"]
    assert output["walkers_active"] == summary["walkers_active"]
    assert output["timestamp"] == timestamp_for(output["simulated_time"])
    for name in ("depth", "discharge"):
        assert session_tools.r_info(map=name, format="json")["rows"] == 5


def test_json_summary_time_series(session_tools):
    """Time series run reports one output entry per written step"""
    summary = session_tools.r_sim_water(
        **SLOW_FLOW,
        depth="depth",
        duration=2,
        output_step=1,
        flags="tp",
        format="json",
    ).json
    outputs = summary["outputs"]
    assert [output["depth"] for output in outputs] == ["depth.01", "depth.02"]
    assert outputs[0]["simulated_time"] < outputs[1]["simulated_time"]
    assert summary["simulated_time"] >= outputs[1]["simulated_time"]
    assert summary["iterations_per_output"] == int(
        60 / (summary["time_step"] * summary["time_coefficient"])
    )
    for output in outputs:
        assert output["timestamp"] == timestamp_for(output["simulated_time"])
        assert output["discharge"] is None
        assert session_tools.r_info(map=output["depth"], format="json")["rows"] == 5


def test_json_summary_stopped_early(session_tools):
    """Fast flow empties the domain before the duration is reached"""
    summary = session_tools.r_sim_water(
        elevation=ELEVATION,
        depth="depth",
        duration=1,
        random_seed=1,
        flags="p",
        format="json",
    ).json
    assert summary["stopped_early"] is True
    assert summary["walkers_active"] == 0
    assert summary["iterations_completed"] < summary["iterations_planned"]


def test_json_summary_walkers_output(session_tools):
    """Walker vector output is reported by name"""
    summary = session_tools.r_sim_water(
        **SLOW_FLOW,
        depth="depth",
        walkers_output="walkers",
        duration=1,
        flags="p",
        format="json",
    ).json
    assert summary["outputs"][0]["walkers"] == "walkers"
    assert session_tools.v_info(map="walkers", format="json")["points"] > 0


def test_plain_summary(session_tools):
    """Plain summary prints one key per line with the same keys as JSON"""
    text = session_tools.r_sim_water(
        **SLOW_FLOW, depth="depth", nwalkers=100, duration=1, flags="p"
    ).text
    lines = text.splitlines()
    assert "walkers_requested: 100" in lines
    assert "duration: 60" in lines
    assert "stopped_early: false" in lines
    assert any(line.startswith("time_step: ") for line in lines)
    assert "  depth: depth" in lines


def test_no_output_without_print_flag(session_tools):
    """Standard output stays empty unless -p is given, whatever the format"""
    for kwargs in ({}, {"format": "json"}):
        result = session_tools.r_sim_water(
            **SLOW_FLOW, depth="depth", duration=1, overwrite=True, **kwargs
        )
        # Tools returns None when the tool produced no standard output.
        assert result is None


def test_history(session_tools):
    """Raster history carries the summary values under the same keys"""
    summary = session_tools.r_sim_water(
        **SLOW_FLOW,
        depth="depth",
        discharge="discharge",
        nwalkers=100,
        duration=1,
        flags="p",
        format="json",
    ).json
    for name in ("depth", "discharge"):
        history = session_tools.r_info(map=name, flags="h").text
        for key in (
            "walkers_requested",
            "walkers_generated",
            "walkers_active",
            "duration",
            "simulated_time",
        ):
            assert history_value(history, key) == summary[key]
        for key in ("time_step", "mean_velocity", "mean_mannings_n"):
            assert history_value(history, key) == pytest.approx(summary[key], abs=1e-6)
        assert history_value(history, "mean_infiltration") == 0
