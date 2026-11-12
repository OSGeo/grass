# SPDX-FileCopyrightText: 2026 Rajveer Bishnoi
# SPDX-License-Identifier: GPL-2.0-or-later
import os

import numpy as np
import pytest

import grass.script as gs
from grass.script import array as garray
from grass.tools import ToolError, Tools


@pytest.fixture
def session(tmp_path):
    """An isolated GRASS session with a small region."""
    project = tmp_path / "r_random_surface_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        Tools(session=session).g_region(n=20, s=0, e=20, w=0, res=1)
        yield session


def read(session, name):
    return garray.array(name, env=session.env)


def test_negative_seed_is_reproducible(session):
    """A negative seed is a seed, so two runs give the same surface."""
    tools = Tools(session=session)
    first = tools.r_random_surface(output=np.array, seed=-5)
    second = tools.r_random_surface(output=np.array, seed=-5)
    np.testing.assert_array_equal(first, second)


@pytest.mark.parametrize("seed", ["5000000000", "-2147483649", "12abc"])
def test_invalid_seed_refused(session, seed):
    """A seed out of range or not wholly an integer is an error."""
    tools = Tools(session=session)
    with pytest.raises(ToolError):
        tools.r_random_surface(output="surface", seed=seed)
    assert not tools.g_list(type="raster", pattern="surface", format="json")


def test_one_seed_for_several_maps(session):
    """With one seed, the maps differ and the first is the single map."""
    tools = Tools(session=session)
    tools.r_random_surface(output="first,second,third", seed=5)
    maps = [read(session, name) for name in ("first", "second", "third")]
    assert not np.array_equal(maps[0], maps[1])
    assert not np.array_equal(maps[0], maps[2])
    assert not np.array_equal(maps[1], maps[2])
    np.testing.assert_array_equal(
        maps[0], tools.r_random_surface(output=np.array, seed=5)
    )


def test_one_seed_per_map(session):
    """With a seed for each map, each map is the single map of its seed."""
    tools = Tools(session=session)
    tools.r_random_surface(output="first,second", seed="4,9")
    np.testing.assert_array_equal(
        read(session, "first"), tools.r_random_surface(output=np.array, seed=4)
    )
    np.testing.assert_array_equal(
        read(session, "second"), tools.r_random_surface(output=np.array, seed=9)
    )


def test_seed_count_must_match(session):
    """Several seeds which do not match the number of maps are an error."""
    tools = Tools(session=session)
    with pytest.raises(ToolError, match="seeds given"):
        tools.r_random_surface(output="first,second,third", seed="1,2")
    assert not tools.g_list(type="raster", pattern="first", format="json")


def test_generated_seed_is_recorded(session):
    """Without a seed, the maps share a generated seed saved in their titles."""
    env = session.env.copy()
    env["GRASS_RANDOM_SEED"] = "99"
    tools = Tools(session=session, env=env)
    tools.r_random_surface(output="first,second")
    assert not np.array_equal(read(session, "first"), read(session, "second"))
    for name in ("first", "second"):
        title = tools.r_info(map=name, format="json")["title"]
        assert title.endswith("seed=99")
    np.testing.assert_array_equal(
        read(session, "first"), tools.r_random_surface(output=np.array, seed=99)
    )
