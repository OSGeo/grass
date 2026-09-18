# SPDX-License-Identifier: GPL-2.0-or-later
import os

import pytest

import grass.script as gs
from grass.tools import Tools

# (20-10) / 2 * (60-40) / 2 = 50 cells
N_CELLS = 50


@pytest.fixture
def session(tmp_path):
    """An isolated GRASS session with the region r.random.cells fills."""
    project = tmp_path / "r_random_cells_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=20, s=10, e=60, w=40, res=2)
        yield session


def test_fill_all(session):
    """A near-zero distance leaves no cell without a unique category."""
    tools = Tools(session=session)
    tools.r_random_cells(output="all", distance=0.01, seed=100)

    univar = tools.r_univar(map="all", format="json").json
    assert univar["cells"] == N_CELLS
    assert univar["n"] == N_CELLS
    assert univar["null_cells"] == 0
    assert univar["min"] == 1
    assert univar["max"] == N_CELLS


def test_fill_some(session):
    """A distance just over one cell width leaves most cells unfilled."""
    tools = Tools(session=session)
    tools.r_random_cells(output="some", distance=2.00001, seed=100)

    univar = tools.r_univar(map="some", format="json").json
    assert univar["cells"] == N_CELLS
    assert univar["min"] == 1
    # The exact count depends on the random sequence, so this only pins an
    # upper bound: the minimum distance between filled cells rules out more
    # than half of them being filled.
    assert univar["max"] <= N_CELLS // 2


def test_fill_count(session):
    """ncells= limits the number of filled cells to an explicit count."""
    tools = Tools(session=session)
    count = 12
    tools.r_random_cells(output="count", distance=2, seed=100, ncells=count)

    univar = tools.r_univar(map="count", format="json").json
    assert univar["cells"] == N_CELLS
    assert univar["n"] == count
    assert univar["null_cells"] == N_CELLS - count
    assert univar["min"] == 1
    assert univar["max"] == count
