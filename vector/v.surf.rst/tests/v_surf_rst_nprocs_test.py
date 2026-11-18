# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of the number of threads v.surf.rst uses"""

import xml.etree.ElementTree as ET

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


@pytest.mark.parametrize("nprocs", [0, -1])
def test_nprocs_gives_result_of_one_thread(tools, nprocs):
    """nprocs of 0 and below are accepted and do not change the result"""
    np.testing.assert_array_equal(
        elevation(tools, nprocs=nprocs), elevation(tools, nprocs=1)
    )


def test_mask_with_threads(tools):
    """With a raster mask, more threads give the result of one thread"""
    tools.r_mapcalc(expression="ones = 1")
    tools.r_mask(raster="ones")
    np.testing.assert_array_equal(
        elevation(tools, nprocs=2), elevation(tools, nprocs=1)
    )


def nprocs_default(tools):
    """Return the default of nprocs from the interface description"""
    xml = tools.run_cmd(["v.surf.rst", "--interface-description"]).text
    return ET.fromstring(xml).find("parameter[@name='nprocs']/default").text.strip()


def test_nprocs_from_gisenv(tools):
    """NPROCS set by g.gisenv replaces the default of one thread"""
    assert nprocs_default(tools) == "1"
    tools.g_gisenv(set="NPROCS=2")
    assert nprocs_default(tools) == "2"
