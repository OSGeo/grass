# SPDX-License-Identifier: GPL-2.0-or-later
"""Regression test for the map metadata read through the C library interface

The computational region is deliberately not square, so that rows and
columns reported the other way round are detected.
"""

import pytest

import grass.script as gs
import grass.temporal as tgis
from grass.tools import Tools

ROWS = 8
COLS = 12


@pytest.fixture
def session_with_maps(tmp_path):
    """Start a session with a raster and a 3D raster map"""
    project = tmp_path / "project"
    gs.create_project(project)
    with gs.setup.init(project) as session:
        tools = Tools(session=session)
        tools.g_region(n=80, s=0, e=120, w=0, t=50, b=0, res=10, res3=10)
        tools.r_mapcalc(expression="test_raster = 1")
        tools.r3_mapcalc(expression="test_raster_3d = 1")
        yield session


def test_raster_rows_and_cols(session_with_maps):
    """Rows and columns of a raster map are the ones reported by r.info"""
    tools = Tools(session=session_with_maps)
    tgis.init()
    ciface = tgis.get_tgis_c_library_interface()

    kvp = ciface.read_raster_info("test_raster", tgis.get_current_mapset())
    info = tools.r_info(map="test_raster", format="json").json

    assert kvp["rows"] == ROWS
    assert kvp["cols"] == COLS
    assert kvp["rows"] == info["rows"]
    assert kvp["cols"] == info["cols"]


def test_raster3d_rows_and_cols(session_with_maps):
    """Rows and columns of a 3D raster map are the ones reported by r3.info"""
    tools = Tools(session=session_with_maps)
    tgis.init()
    ciface = tgis.get_tgis_c_library_interface()

    kvp = ciface.read_raster3d_info("test_raster_3d", tgis.get_current_mapset())
    info = tools.r3_info(map="test_raster_3d", format="json").json

    assert kvp["rows"] == ROWS
    assert kvp["cols"] == COLS
    assert kvp["rows"] == info["rows"]
    assert kvp["cols"] == info["cols"]
