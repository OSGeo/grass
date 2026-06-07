"""Tests of r.sim.water"""

import pytest


@pytest.mark.parametrize("nprocs", [1, 2])
def test_water_is_physical_and_flows_downhill(session_tools, nprocs):
    """Water is physical and accumulates downhill for serial and parallel runs.

    Uses a small synthetic surface that slopes down toward the south with a
    gentle valley. The checks are properties that hold regardless of parallel
    walker scheduling (which makes exact per-cell values nondeterministic):
    depth is never negative, every cell has a value, water is present, and more
    water accumulates in the lower (south) third than in the upper (north)
    third.
    """
    tools = session_tools
    tools.g_region(s=0, n=100, w=0, e=100, res=1)
    tools.r_mapcalc(expression="elev = 0.05 * y() + 0.0006 * (x() - 50.0)^2")
    tools.r_sim_water(elevation="elev", depth="depth", random_seed=1, nprocs=nprocs)

    depth = tools.r_univar(map="depth", format="json").json
    assert depth["min"] >= 0
    assert depth["null_cells"] == 0
    assert depth["sum"] > 0

    tools.r_mapcalc(expression="south = if(y() < 33, depth, null())")
    tools.r_mapcalc(expression="north = if(y() > 67, depth, null())")
    south = tools.r_univar(map="south", format="json").json
    north = tools.r_univar(map="north", format="json").json
    assert south["mean"] > north["mean"]
