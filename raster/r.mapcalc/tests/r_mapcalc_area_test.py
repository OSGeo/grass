# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""r.mapcalc tests of the area() function in a latitude-longitude project"""

import os

import numpy as np
import pytest

import grass.script as gs
from grass.script import array as garray
from grass.tools import Tools

NORTH = 40
SOUTH = 39
ROWS = 120
WEST = -80
EAST = -79
COLS = 10


@pytest.fixture(scope="module")
def latlon_session(tmp_path_factory):
    """Active session in a latitude-longitude project (WGS84)"""
    project = tmp_path_factory.mktemp("r_mapcalc_area") / "latlon"
    gs.create_project(project, epsg=4326)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.g_region(n=NORTH, s=SOUTH, w=WEST, e=EAST, rows=ROWS, cols=COLS)
        yield session


def zone_area_to_equator(latitude, longitude_span):
    """Area on the WGS84 ellipsoid between the equator and a latitude

    The area is limited to the given span of longitude. Both arguments are in
    degrees and the result is in square meters.
    """
    semi_major_axis = 6378137.0
    flattening = 1 / 298.257223563
    e2 = flattening * (2 - flattening)
    e = np.sqrt(e2)
    sine = np.sin(np.radians(latitude))
    return (
        np.radians(longitude_span)
        * semi_major_axis**2
        * (1 - e2)
        / 2
        * (sine / (1 - e2 * sine**2) + np.arctanh(e * sine) / e)
    )


def expected_cell_areas():
    """Cell area for each row of the region computed independently of GRASS"""
    edges = np.linspace(NORTH, SOUTH, ROWS + 1)
    longitude_span = (EAST - WEST) / COLS
    return zone_area_to_equator(edges[:-1], longitude_span) - zone_area_to_equator(
        edges[1:], longitude_span
    )


@pytest.mark.parametrize("nprocs", [0, 1, 4])
def test_area_latlon(latlon_session, nprocs):
    """Check that each row has the area of its own latitude with any nprocs"""
    tools = Tools(session=latlon_session, overwrite=True)
    tools.r_mapcalc(expression="area = area()", nprocs=nprocs)
    area = garray.array("area", env=latlon_session.env)
    assert area.shape == (ROWS, COLS)
    expected = np.repeat(expected_cell_areas()[:, np.newaxis], COLS, axis=1)
    # The tolerance allows for rounding differences between GRASS and the
    # computation here (about 1e-11), but fails if a row has the area of
    # another row (the neighboring row differs by about 1e-4).
    np.testing.assert_allclose(area, expected, rtol=1e-8)
