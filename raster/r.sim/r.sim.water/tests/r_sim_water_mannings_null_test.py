# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of r.sim.water with null cells in the Manning's n and infiltration maps."""

import os

import numpy as np
import pytest

import grass.script as gs
import grass.script.array as garray
from grass.tools import Tools

ROWS = 20
COLS = 30
MANNINGS_N = 0.05
# Below the rain of 50 mm/hr, so that water remains to flow.
INFILTRATION = 10
row, col = np.mgrid[0:ROWS, 0:COLS]
# A plane descending to the east and, more gently, to the north. The partial
# derivatives have the signs of the r.slope.aspect output, positive where the
# elevation decreases to the east and to the north.
ELEVATION = 0.2 * (COLS - col) + 0.05 * row
DX = np.full((ROWS, COLS), 0.2)
DY = np.full((ROWS, COLS), 0.05)
# In the middle of the region, so that water from upslope flows into it.
PATCH = (row >= 6) & (row < 12) & (col >= 15) & (col < 20)


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    project = tmp_path_factory.mktemp("mannings_null") / "project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        Tools(session=session).g_region(w=0, e=COLS, s=0, n=ROWS, res=1)
        yield session


def simulate(session, elevation, man, **kwargs):
    """Run r.sim.water and return depth, discharge and the run summary.

    Nulls in the outputs are returned as NaN. The output maps are overwritten
    by the next run, so that the map names in the summaries are the same.
    """
    # The partial derivatives are given so that they are the same in both
    # runs compared. When they are computed from the elevation, null
    # elevation is left out of the derivatives in the neighboring cells, while
    # null Manning's n does not change the derivatives.
    tools = Tools(session=session)
    result = tools.r_sim_water(
        elevation=elevation,
        dx=DX,
        dy=DY,
        man=man,
        depth="depth",
        discharge="discharge",
        rain_value=50,
        nwalkers=2000,
        duration=2,
        random_seed=1,
        nprocs=1,
        flags="p",
        format="json",
        overwrite=True,
        **kwargs,
    )
    depth = garray.array("depth", null="nan", env=session.env)
    discharge = garray.array("discharge", null="nan", env=session.env)
    return np.asarray(depth), np.asarray(discharge), result.json


def test_null_mannings_n_is_like_null_elevation(session):
    """Null Manning's n removes cells from the domain as null elevation does.

    The outputs are null in these cells, the discharge is nowhere negative,
    the mean Manning's n in the summary comes only from the other cells, and
    the outputs and the summary are the same as with null elevation.
    """
    null_man = simulate(
        session,
        elevation=ELEVATION,
        man=np.where(PATCH, np.nan, MANNINGS_N),
        infil_value=0,
    )
    null_elevation = simulate(
        session,
        elevation=np.where(PATCH, np.nan, ELEVATION),
        man=np.full((ROWS, COLS), MANNINGS_N),
        infil_value=0,
    )
    depth, discharge, summary = null_man
    assert np.isnan(depth[PATCH]).all()
    assert np.isnan(discharge[PATCH]).all()
    assert np.nanmin(discharge) >= 0
    assert summary["mean_mannings_n"] == pytest.approx(MANNINGS_N)
    np.testing.assert_array_equal(depth, null_elevation[0])
    np.testing.assert_array_equal(discharge, null_elevation[1])
    assert summary == null_elevation[2]


def test_null_infiltration_is_no_infiltration(session):
    """Null infiltration gives the same run as zero infiltration.

    The outputs and the summary, including the mean infiltration, are the
    same as with zero infiltration in the same cells.
    """
    man = np.full((ROWS, COLS), MANNINGS_N)
    null_infil = simulate(
        session,
        elevation=ELEVATION,
        man=man,
        infil=np.where(PATCH, np.nan, INFILTRATION),
    )
    zero_infil = simulate(
        session,
        elevation=ELEVATION,
        man=man,
        infil=np.where(PATCH, 0, INFILTRATION),
    )
    depth, discharge, summary = null_infil
    assert not np.isnan(depth).any()
    np.testing.assert_array_equal(depth, zero_infil[0])
    np.testing.assert_array_equal(discharge, zero_infil[1])
    assert summary == zero_infil[2]
