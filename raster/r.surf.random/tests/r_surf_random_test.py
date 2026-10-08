# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Test the random values of r.surf.random for a given seed"""

import os

import numpy as np
import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture
def session(tmp_path):
    """A session in an XY project with a region of 2 rows and 3 columns"""
    project = tmp_path / "xy_test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        Tools(session=session).g_region(n=2, s=0, e=3, w=0, res=1)
        yield session


def test_float_values_for_seed(session):
    """Seed 42 gives the pinned values"""
    tools = Tools(session=session)
    result = tools.r_surf_random(output=np.array, seed=42)
    expected = np.array(
        [
            [74.45250000610066, 34.2701478718908, 11.10852824441615],
            [42.2338957988309, 8.111117117831057, 85.6440708026625],
        ]
    )
    np.testing.assert_array_equal(result, expected)


def test_integer_values_for_seed(session):
    """Seed 42 gives the pinned integers"""
    tools = Tools(session=session)
    result = tools.r_surf_random(output=np.array, seed=42, min=-20, max=7, flags="i")
    np.testing.assert_array_equal(result, [[-10, -1, -17], [-16, -5, 3]])
