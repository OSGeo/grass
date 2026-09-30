# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of the random numbers of r.sim.sediment.

Each walker draws from a random number state of its own, set by the seed and
the walker's number, so its random numbers do not depend on the number of
threads. The sediment flux still does at nprocs > 1, because walkers in the
same cell add to the concentration grid without synchronization, so the test
which compares numbers of threads compares the walker positions instead. A
walker's path depends only on the slope and its random numbers.
"""

import os
from io import StringIO

import numpy as np
import pytest

import grass.script as gs
from grass.exceptions import CalledModuleError
from grass.tools import Tools


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Session in an XY project with a bowl as elevation and constant inputs."""
    project = tmp_path_factory.mktemp("r_sim_sediment_random") / "xy_test"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.g_region(s=0, n=40, w=0, e=40, res=1)
        tools.r_mapcalc(
            expression="elevation = 0.01 * ((col() - 20.5)^2 + (row() - 20.5)^2)"
        )
        tools.r_mapcalc(expression="water_depth = 0.1")
        tools.r_mapcalc(expression="detachment = 0.001")
        tools.r_mapcalc(expression="transport = 0.001")
        tools.r_mapcalc(expression="shear_stress = 0")
        yield session


def simulate(session, walkers=None, env=None, **kwargs):
    """Run the simulation, return the flux and the walker coordinates."""
    tools = Tools(env=env) if env else Tools(session=session)
    if walkers:
        kwargs["walkers_output"] = walkers
    flux = tools.r_sim_sediment(
        elevation="elevation",
        water_depth="water_depth",
        detachment_coeff="detachment",
        transport_coeff="transport",
        shear_stress="shear_stress",
        sediment_flux=np.array,
        duration=2,
        **kwargs,
    )
    if not walkers:
        return flux, None
    text = tools.v_out_ascii(input=walkers, format="point", precision=15).text
    return flux, np.loadtxt(StringIO(text), delimiter="|")


def test_same_seed_gives_same_flux(session):
    """Two runs with the same seed give the same sediment flux."""
    first, _ = simulate(session, random_seed=1)
    second, _ = simulate(session, random_seed=1)
    assert np.array_equal(first, second)


def test_different_seeds_give_different_results(session):
    """Runs with different seeds give different fluxes and walker positions."""
    first_flux, first_walkers = simulate(session, "walkers_seed_1", random_seed=1)
    second_flux, second_walkers = simulate(session, "walkers_seed_2", random_seed=2)
    assert not np.array_equal(first_flux, second_flux)
    assert not np.array_equal(first_walkers, second_walkers)


def test_generated_seed_is_the_seed_option(session):
    """The -s flag with GRASS_RANDOM_SEED gives what the seed option gives."""
    env = session.env.copy()
    env["GRASS_RANDOM_SEED"] = "3"
    generated, _ = simulate(session, env=env, flags="s")
    given, _ = simulate(session, random_seed=3)
    assert np.array_equal(generated, given)


@pytest.mark.parametrize("nprocs", [2, 4])
def test_walkers_do_not_depend_on_nprocs(session, nprocs):
    """Walkers end at the same positions with any number of threads."""
    _, serial = simulate(session, f"walkers_{nprocs}_serial", random_seed=5)
    _, parallel = simulate(
        session, f"walkers_{nprocs}_parallel", random_seed=5, nprocs=nprocs
    )
    assert serial.size
    assert np.array_equal(parallel, serial)


@pytest.mark.parametrize("seed", [-(2**31) - 1, 2**32])
def test_seed_outside_range_is_an_error(session, seed):
    """A seed the generator cannot use is refused, not silently wrapped."""
    with pytest.raises(CalledModuleError, match="outside the range"):
        simulate(session, random_seed=seed)


@pytest.mark.parametrize("seed", ["12abc", "1.5", "-", "99999999999999999999"])
def test_seed_which_is_not_an_integer_is_an_error(session, seed):
    """A seed which the parser lets through but which is not an integer is refused."""
    with pytest.raises(CalledModuleError, match="Invalid random seed"):
        simulate(session, random_seed=seed)
