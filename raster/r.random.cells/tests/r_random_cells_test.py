# SPDX-FileCopyrightText: 2015 Vaclav Petras
# SPDX-FileCopyrightText: 2026 Rajveer Bishnoi
# SPDX-License-Identifier: GPL-2.0-or-later
import os

import numpy as np
import pytest

import grass.script as gs
from grass.tools import Tools

# (20-10) / 2 * (60-40) / 2 = 50 cells
N_CELLS = 50


@pytest.fixture
def session_with_region(tmp_path):
    """An isolated GRASS session with the region r.random.cells fills."""
    project = tmp_path / "r_random_cells_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=20, s=10, e=60, w=40, res=2)
        yield session


def test_fill_all(session_with_region):
    """A near-zero distance leaves no cell without a unique category."""
    tools = Tools(session=session_with_region)
    tools.r_random_cells(output="all", distance=0.01, seed=100)

    univar = tools.r_univar(map="all", format="json")
    assert univar["cells"] == N_CELLS
    assert univar["n"] == N_CELLS
    assert univar["null_cells"] == 0
    assert univar["min"] == 1
    assert univar["max"] == N_CELLS


def test_fill_some(session_with_region):
    """A distance just over one cell width leaves most cells unfilled."""
    tools = Tools(session=session_with_region)
    tools.r_random_cells(output="some", distance=2.00001, seed=100)

    univar = tools.r_univar(map="some", format="json")
    assert univar["cells"] == N_CELLS
    assert univar["min"] == 1
    # The exact count depends on the random sequence, so this only pins an
    # upper bound: the minimum distance between filled cells rules out more
    # than half of them being filled.
    assert univar["max"] <= N_CELLS // 2


def test_fill_count(session_with_region):
    """ncells= limits the number of filled cells to an explicit count."""
    tools = Tools(session=session_with_region)
    count = 12
    tools.r_random_cells(output="count", distance=2, seed=100, ncells=count)

    univar = tools.r_univar(map="count", format="json")
    assert univar["cells"] == N_CELLS
    assert univar["n"] == count
    assert univar["null_cells"] == N_CELLS - count
    assert univar["min"] == 1
    assert univar["max"] == count


@pytest.fixture
def session_small_region(tmp_path):
    """A session with a region of 4 rows and 5 columns."""
    project = tmp_path / "r_random_cells_small"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        Tools(session=session).g_region(n=4, s=0, e=5, w=0, res=1)
        yield session


# The maps are integer, so unfilled cells come back from the array conversion
# as 0 rather than as a null. Categories start at 1, so the two do not overlap.
def test_cells_of_a_seed(session_small_region):
    """The cells a seed fills are the pinned ones."""
    tools = Tools(session=session_small_region)
    result = tools.r_random_cells(output=np.array, distance=2, seed=100)
    expected = [
        [4, 0, 2, 0, 0],
        [0, 0, 0, 0, 5],
        [0, 0, 0, 0, 0],
        [3, 0, 0, 1, 0],
    ]
    np.testing.assert_array_equal(result, expected)


def test_cells_of_a_seed_with_count(session_small_region):
    """ncells= keeps the first cells of the same seed."""
    tools = Tools(session=session_small_region)
    result = tools.r_random_cells(output=np.array, distance=2, seed=100, ncells=3)
    expected = [
        [0, 0, 2, 0, 0],
        [0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0],
        [3, 0, 0, 1, 0],
    ]
    np.testing.assert_array_equal(result, expected)
