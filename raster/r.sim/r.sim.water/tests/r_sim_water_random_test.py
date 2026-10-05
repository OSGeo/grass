# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of the random numbers of r.sim.water.

Each walker draws from a random number state of its own, set by the seed and
the walker's number, so its random numbers do not depend on the number of
threads. The water depth still does at nprocs > 1, because walkers in the same
cell add to the depth grid without synchronization, so the tests which compare
numbers of threads compare the walker positions instead. With a large hmax and
no infiltration, a walker's path depends only on the slope and its random
numbers, not on the depth. The elevation is a bowl, so that the walkers stay
in the region until the end of the simulation.
"""

import os
import re
from io import StringIO

import numpy as np
import pytest

import grass.script as gs
import grass.script.array as garray
from grass.exceptions import CalledModuleError
from grass.tools import Tools

# Larger than any depth the simulation reaches, so the diffusion does not
# depend on the depth.
NO_DEPTH_FEEDBACK = {"hmax": 1e6}


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Session in an XY project with a bowl as elevation."""
    project = tmp_path_factory.mktemp("r_sim_water_random") / "xy_test"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.g_region(s=0, n=40, w=0, e=40, res=1)
        tools.r_mapcalc(
            expression="elevation = 0.01 * ((col() - 20.5)^2 + (row() - 20.5)^2)"
        )
        yield session


def simulate_depth(session, **kwargs):
    """Run the simulation and return the water depth as an array."""
    tools = Tools(session=session)
    return tools.r_sim_water(
        elevation="elevation", depth=np.array, duration=5, **kwargs
    )


def simulate_walkers(session, name, **kwargs):
    """Run the simulation and return the final walker coordinates as an array."""
    tools = Tools(session=session)
    tools.r_sim_water(elevation="elevation", walkers_output=name, duration=5, **kwargs)
    text = tools.v_out_ascii(input=name, format="point", precision=15).text
    return np.loadtxt(StringIO(text), delimiter="|")


def test_same_seed_gives_same_depth(session):
    """Two runs with the same seed give the same water depth."""
    first = simulate_depth(session, random_seed=1)
    second = simulate_depth(session, random_seed=1)
    assert np.array_equal(first, second)


def test_different_seeds_give_different_depth(session):
    """Runs with different seeds give different water depths."""
    first = simulate_depth(session, random_seed=1)
    second = simulate_depth(session, random_seed=2)
    assert not np.array_equal(first, second)


def test_generated_seed_is_the_seed_option(session):
    """Without a seed, GRASS_RANDOM_SEED gives what the seed option gives."""
    env = session.env.copy()
    env["GRASS_RANDOM_SEED"] = "3"
    generated = Tools(env=env).r_sim_water(
        elevation="elevation", depth=np.array, duration=5
    )
    given = simulate_depth(session, random_seed=3)
    assert np.array_equal(generated, given)


def test_generated_seed_is_recorded(session):
    """A run without a seed records the seed it used, and the seed repeats it."""
    tools = Tools(session=session)
    tools.r_sim_water(elevation="elevation", depth="generated", duration=5)
    history = tools.r_info(map="generated", flags="h").text
    seed = int(re.search(r"random_seed=(-?\d+)", history).group(1))
    repeated = simulate_depth(session, random_seed=seed)
    generated = garray.array("generated", env=session.env)
    assert np.array_equal(repeated, generated)


def test_deprecated_flag_generates_a_seed(session):
    """The deprecated -s flag still gives what GRASS_RANDOM_SEED says."""
    env = session.env.copy()
    env["GRASS_RANDOM_SEED"] = "3"
    generated = Tools(env=env).r_sim_water(
        elevation="elevation", depth=np.array, duration=5, flags="s"
    )
    given = simulate_depth(session, random_seed=3)
    assert np.array_equal(generated, given)


def test_seed_and_flag_are_exclusive(session):
    """The seed option and the flag to generate a seed cannot be combined."""
    with pytest.raises(CalledModuleError, match="mutually exclusive"):
        simulate_depth(session, random_seed=1, flags="s")


@pytest.mark.parametrize("nprocs", [2, 4])
def test_walkers_do_not_depend_on_nprocs(session, nprocs):
    """Walkers end at the same positions with any number of threads."""
    serial = simulate_walkers(
        session, f"walkers_{nprocs}_serial", random_seed=5, **NO_DEPTH_FEEDBACK
    )
    parallel = simulate_walkers(
        session,
        f"walkers_{nprocs}_parallel",
        random_seed=5,
        nprocs=nprocs,
        **NO_DEPTH_FEEDBACK,
    )
    assert serial.size
    assert np.array_equal(parallel, serial)


def test_walkers_depend_on_seed(session):
    """The walker comparison above can detect a difference."""
    first = simulate_walkers(
        session, "walkers_seed_5", random_seed=5, **NO_DEPTH_FEEDBACK
    )
    second = simulate_walkers(
        session, "walkers_seed_6", random_seed=6, **NO_DEPTH_FEEDBACK
    )
    assert not np.array_equal(first, second)


@pytest.mark.parametrize("seed", [-(2**31) - 1, 2**32])
def test_seed_outside_range_is_an_error(session, seed):
    """A seed the generator cannot use is refused, not silently wrapped."""
    with pytest.raises(CalledModuleError, match="outside the range"):
        simulate_depth(session, random_seed=seed)


@pytest.mark.parametrize("seed", ["12abc", "1.5", "-", "99999999999999999999"])
def test_seed_which_is_not_an_integer_is_an_error(session, seed):
    """A seed which the parser lets through but which is not an integer is refused."""
    with pytest.raises(CalledModuleError, match="Invalid random seed"):
        simulate_depth(session, random_seed=seed)
