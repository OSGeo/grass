# SPDX-License-Identifier: GPL-2.0-or-later
import os
import pathlib

import numpy as np
import pytest

import grass.script as gs
from grass.tools import Tools

DATA = pathlib.Path(__file__).parent / "data"
INPUT = DATA / "input_ascii.txt"
REFERENCE = {
    mode: DATA / f"output_{mode}.txt" for mode in ("wmean", "mean", "median", "mode")
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
def test_matches_reference(session, mode):
    """Each mode fills the gaps the way its reference raster does."""
    tools = Tools(session=session)

    actual = tools.r_fill_stats(
        input="ascii", output=np.array, distance=1, mode=mode, power=2, cells=2
    )
    reference = tools.r_in_ascii(
        input=str(REFERENCE[mode]), output=np.array, type="DCELL"
    )

    assert np.allclose(actual, reference, rtol=0, atol=1e-6, equal_nan=True)
