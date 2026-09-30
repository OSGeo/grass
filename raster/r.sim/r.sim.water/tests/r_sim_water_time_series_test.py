# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of when and under which names r.sim.water writes time series maps"""

import os

import pytest

import grass.script as gs
from grass.experimental.mapset import TemporaryMapsetSession
from grass.tools import Tools

# On this slope, the time step is about 10.2 s, so a 1-minute output step
# does not fall on an iteration and the old schedule, which wrote every
# (int)(60 / 10.2) = 5 iterations and named maps by the rounded simulated
# time, wrote dep.03 and dep.08 twice and no dep.10.
SLOPE = {
    "elevation": "elevation",
    "man_value": 0.1,
    "duration": 10,
    "random_seed": 1,
    "nprocs": 1,
}


@pytest.fixture(scope="module")
def slope_project(tmp_path_factory):
    """Project with a 500 m by 500 m plane at 10 m rising 0.1 m per row"""
    project = tmp_path_factory.mktemp("simwe_time_series") / "simwe"
    gs.create_project(project, epsg=3358)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(s=0, n=500, w=0, e=500, res=10, flags="s")
        tools.r_mapcalc(expression="elevation = row() * 0.1")
        yield session


@pytest.fixture
def slope_tools(slope_project):
    with (
        TemporaryMapsetSession(env=slope_project.env) as session,
        Tools(session=session, capture_stderr=True) as tools,
    ):
        yield tools


def minutes_of_timestamp(tools, name):
    """Minutes of a raster timestamp such as '1 minute' or '10 minutes'"""
    number, unit = tools.r_timestamp(map=name).text.split()
    assert unit in {"minute", "minutes"}
    return int(number)


def check_series(tools, summary, minutes):
    """Series maps are named and timestamped by the given minutes.

    Each map is written at the first iteration reaching its time, except the
    last one which is written at the end of the run.
    """
    outputs = summary["outputs"]
    names = [f"dep.{minute:02d}" for minute in minutes]
    assert [output["depth"] for output in outputs] == names
    listed = [item["name"] for item in tools.g_list(type="raster", format="json")]
    assert sorted(listed) == sorted([*names, "elevation"])
    for minute, output in zip(minutes[:-1], outputs[:-1], strict=True):
        assert output["timestamp"] == f"{minute} minutes"
        assert minutes_of_timestamp(tools, output["depth"]) == minute
        assert (
            minute * 60 <= output["simulated_time"] < minute * 60 + summary["time_step"]
        )
    assert outputs[-1]["timestamp"] == f"{minutes[-1]} minutes"
    assert minutes_of_timestamp(tools, names[-1]) == minutes[-1]
    assert outputs[-1]["simulated_time"] == summary["simulated_time"]


def test_every_output_step_written_once(slope_tools):
    """Each output step gets one map named and timestamped by its time"""
    summary = slope_tools.r_sim_water(
        **SLOPE,
        depth="dep",
        walkers_output="walkers",
        output_step=1,
        flags="tp",
        format="json",
    ).json
    check_series(slope_tools, summary, list(range(1, 11)))
    # Walker maps follow the same schedule with an underscore separator.
    walkers = [output["walkers"] for output in summary["outputs"]]
    assert walkers == [f"walkers_{minute:02d}" for minute in range(1, 11)]
    listed = slope_tools.g_list(type="vector", format="json")
    assert sorted(item["name"] for item in listed) == walkers


def test_final_step_named_by_duration(slope_tools):
    """Duration which is not a multiple of output_step still ends the series"""
    summary = slope_tools.r_sim_water(
        **SLOPE, depth="dep", output_step=3, flags="tp", format="json"
    ).json
    check_series(slope_tools, summary, [3, 6, 9, 10])


def test_time_step_longer_than_output_step(slope_tools):
    """An iteration passing several output steps writes the latest one

    A minimum time step of 17.5 s makes the simulated time per iteration
    70 s (the time step times the time coefficient 4), so the iteration
    ending at 420 s passes both 6 and 7 minutes. The old schedule wrote
    every (int)(60 / 70) = 0 iterations, that is, no maps at all.
    """
    result = slope_tools.r_sim_water(
        **SLOPE,
        depth="dep",
        output_step=1,
        mintimestep=17.5,
        flags="tp",
        format="json",
    )
    summary = result.json
    assert summary["time_step"] == 70
    check_series(slope_tools, summary, [1, 2, 3, 4, 5, 7, 8, 9, 10])
    assert "longer than output_step" in result.stderr


def test_final_step_after_early_stop(session_tools):
    """The series ends with the duration also when all walkers left early

    With the default Manning's n, all walkers leave the small domain from
    the shared fixture before the first output step.
    """
    summary = session_tools.r_sim_water(
        elevation="rows_raster",
        depth="depth",
        duration=2,
        output_step=1,
        random_seed=1,
        flags="tp",
        format="json",
    ).json
    assert summary["stopped_early"] is True
    assert summary["simulated_time"] < 60
    assert [output["depth"] for output in summary["outputs"]] == ["depth.02"]
    assert summary["outputs"][0]["timestamp"] == "2 minutes"
