# SPDX-FileCopyrightText: 2026 Vaclav Petras
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of v.surf.rst with more than one thread"""

import numpy as np
import pytest

from grass.tools import Tools


@pytest.fixture
def tools(xy_dataset_session):
    """Tools in a session with random points with values in column z"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(s=0, n=20, w=0, e=20, res=1)
    tools.v_random(
        output="points",
        npoints=100,
        seed=1,
        column="z",
        column_type="double",
        zmin=0,
        zmax=10,
    )
    return tools


def elevation(tools, **kwargs):
    """Return the interpolated surface as an array"""
    # Small segments, so that each thread gets some
    return tools.v_surf_rst(
        input="points",
        zcolumn="z",
        segmax=10,
        npmin=30,
        elevation=np.array,
        **kwargs,
    )


def test_mask_with_threads(tools):
    """With a raster mask, more threads give the result of one thread"""
    tools.r_mapcalc(expression="ones = 1")
    tools.r_mask(raster="ones")
    np.testing.assert_array_equal(
        elevation(tools, nprocs=2), elevation(tools, nprocs=1)
    )
