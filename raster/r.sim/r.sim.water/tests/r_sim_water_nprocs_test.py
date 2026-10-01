"""Tests of r.sim.water results with more than one thread."""

import os

import numpy as np
import pytest

import grass.script as gs
from grass.tools import Tools

SIZE = 21
# Walkers start in the middle of a bowl, far enough from the edges that
# diffusion does not take any of them out of the region. The total of the
# water accumulated over the simulation then depends only on the rainfall
# excess and the time, not on the paths of the walkers or the random numbers.
row, col = np.mgrid[0:SIZE, 0:SIZE]
BOWL = ((row - SIZE // 2) ** 2 + (col - SIZE // 2) ** 2) / 10.0
RAIN = np.where(
    (abs(row - SIZE // 2) <= SIZE // 4) & (abs(col - SIZE // 2) <= SIZE // 4), 50, 0
)


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    project = tmp_path_factory.mktemp("nprocs") / "project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        Tools(session=session).g_region(w=0, e=SIZE, s=0, n=SIZE, res=1)
        yield session


def depth(session, nprocs, rain=RAIN, **kwargs):
    return Tools(session=session).r_sim_water(
        elevation=BOWL,
        depth=np.array,
        rain=rain,
        man_value=0.05,
        nwalkers=400,
        duration=2,
        random_seed=1,
        nprocs=nprocs,
        **kwargs,
    )


def total_water(depth):
    """Return the sum over cells of the accumulated walker weights.

    The depth output is the accumulated weight to the power of 3/5.
    """
    return np.sum(depth.astype(np.float64) ** (5 / 3))


def test_total_does_not_depend_on_nprocs(session):
    """No water is lost when walkers are processed by several threads.

    Without diffusion, all walkers end up in the few cells at the bottom of
    the bowl, so unsynchronized updates of the same cell by several threads
    are likely.
    """
    options = {"infil_value": 0, "diffusion_coeff": 0}
    single = total_water(depth(session, nprocs=1, **options))
    multiple = total_water(depth(session, nprocs=4, **options))
    assert multiple == pytest.approx(single, rel=1e-6)


def test_all_walkers_move_with_nprocs(session):
    """Every walker is simulated whatever the number of threads.

    With rain in all cells, the walkers requested are created as one per
    cell, 441 in total, which is not a multiple of the 4 threads. The last
    walker is in a corner of the bowl, so it has to be moved to add its
    water. Without diffusion, no walker leaves the region.
    """
    options = {"rain": np.full((SIZE, SIZE), 50), "diffusion_coeff": 0}
    single = total_water(depth(session, nprocs=1, **options))
    multiple = total_water(depth(session, nprocs=4, **options))
    assert multiple == pytest.approx(single, rel=1e-6)
