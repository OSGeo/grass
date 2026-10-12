# SPDX-License-Identifier: GPL-2.0-or-later
import os
import pathlib

import numpy as np
import pytest

import grass.script as gs
import grass.script.array as garray
from grass.tools import Tools

# The ASCII fixtures live next to this file, carried over unchanged from the
# gunittest version. They are read from disk rather than inlined so the bytes
# on disk are what the tool sees, independent of the platform the test runs on.
DATA = pathlib.Path(__file__).parent / "data"
INPUT = DATA / "input_ascii.txt"
REFERENCE = {
    mode: DATA / f"output_{mode}.txt" for mode in ("wmean", "mean", "median", "mode")
}

# The reference rasters have the northeast corner null in every mode, but the
# current tool fills it. The gunittest version compared the two rasters through
# their difference, and a difference involving a null is itself null, so a value
# on one side and a null on the other never showed up. See the corner assertions
# below, and issue #8057.
KNOWN_CORNER_VALUES = {
    "wmean": pytest.approx(3.926776695296637),
    "mean": pytest.approx(3.6666666666666665),
    "median": 4.0,
    "mode": 4,
}


@pytest.fixture
def session(tmp_path):
    """A GRASS session with the input raster used by all modes."""
    project = tmp_path / "r_fill_stats_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.r_in_ascii(input=str(INPUT), output="ascii", type="CELL")
        tools.g_region(raster="ascii")
        yield session


@pytest.mark.parametrize("mode", ["wmean", "mean", "median", "mode"])
def test_matches_reference_except_known_corner_cell(session, mode):
    """Each mode matches its reference raster cell by cell, except for the
    one corner cell described in the REFERENCE comment above."""
    tools = Tools(session=session)
    tools.r_fill_stats(
        input="ascii", output="stats", distance=1, mode=mode, power=2, cells=2
    )
    tools.r_in_ascii(input=str(REFERENCE[mode]), output="reference", type="DCELL")

    actual = garray.array(mapname="stats", env=session.env, null=np.nan)
    reference = garray.array(mapname="reference", env=session.env, null=np.nan)

    corner = (0, 6)
    assert actual[corner] == KNOWN_CORNER_VALUES[mode]
    assert np.isnan(reference[corner])

    actual_rest = np.delete(actual, np.ravel_multi_index(corner, actual.shape))
    reference_rest = np.delete(reference, np.ravel_multi_index(corner, reference.shape))
    assert np.allclose(actual_rest, reference_rest, rtol=0, atol=1e-6, equal_nan=True)
