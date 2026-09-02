"""Tests for column-windowed reading of GDAL-linked (r.external) raster maps

Rast_get_row() reads GDAL-linked maps through read_data_gdal(), which
restricts the GDAL read to the range of native columns that overlap the
current region instead of always reading the full native row width.

These tests generate a raster with a value unique to each cell, link it
back with r.external, and check that reading it through various regions
(fully inside, partially outside, and fully outside the file's extent)
returns the expected values.
"""

import os

import numpy as np
import pytest

import grass.script as gs
from grass.experimental import TemporaryMapsetSession
from grass.script import array as garray
from grass.tools import Tools

ROWS = 20
COLS = 30
NULL = -999999


@pytest.fixture(scope="module")
def linked_session(tmp_path_factory):
    """Module-scoped session with a GeoTIFF exported and linked as 'linked'

    The source raster has a value of (row - 1) * 1000 + (col - 1) at its
    1-based row/col, so a cell's expected value can be computed from its
    position without needing a second, independent read of the file.
    """
    project = tmp_path_factory.mktemp("gdal_link_window") / "project"
    gs.create_project(project, epsg=3358)
    tif_path = tmp_path_factory.mktemp("gdal_link_window_data") / "source.tif"
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=ROWS, s=0, w=0, e=COLS, res=1)
        tools.r_mapcalc(expression="source = (row() - 1) * 1000 + (col() - 1)")
        tools.r_out_gdal(
            input="source", output=str(tif_path), format="GTiff", type="Int32"
        )
        tools.r_external(input=str(tif_path), output="linked")
        yield session, tif_path


@pytest.fixture
def session(linked_session):
    """A session in its own temporary mapset, so each test has its own region"""
    session, _ = linked_session
    with TemporaryMapsetSession(env=session.env) as mapset_session:
        yield mapset_session


@pytest.fixture
def source_tif(linked_session):
    """Path to the GeoTIFF file linked as 'linked' in the session fixture"""
    _, tif_path = linked_session
    return tif_path


def expected_values(row_offset, col_offset, rows, cols):
    """Expected 'linked' values for a region shifted by row/col_offset cells"""
    row_values = (row_offset + np.arange(rows)) * 1000
    col_values = col_offset + np.arange(cols)
    return row_values[:, None] + col_values[None, :]


def test_region_fully_inside_source_extent(session):
    """A region fully inside the file reads the correct sub-window"""
    # Source extent is n=20, s=0, w=0, e=30. This region's origin is shifted
    # by (20 - 15) rows and (12 - 0) columns into the source raster.
    Tools(session=session).g_region(n=15, s=8, w=12, e=25, res=1)
    arr = np.array(garray.array("linked", null=NULL, env=session.env))
    assert np.array_equal(arr, expected_values(5, 12, *arr.shape))


def test_region_partially_outside_source_extent(session):
    """Columns outside the file's extent read as null, the rest as data"""
    # w=-5 puts the first 5 columns of the region outside the source
    # raster's extent (west=0); columns 5 and up still overlap it.
    Tools(session=session).g_region(n=10, s=5, w=-5, e=10, res=1)
    arr = np.array(garray.array("linked", null=NULL, env=session.env))
    assert np.all(arr[:, :5] == NULL)
    assert np.array_equal(arr[:, 5:], expected_values(10, 0, arr.shape[0], 10))


def test_region_fully_outside_source_extent(session):
    """A region with no overlap at all reads back as entirely null"""
    Tools(session=session).g_region(n=10, s=5, w=-50, e=-40, res=1)
    arr = np.array(garray.array("linked", null=NULL, env=session.env))
    assert np.all(arr == NULL)


def test_r_in_gdal_ignores_region(session, source_tif):
    """r.in.gdal imports the full file regardless of the current region"""
    tools = Tools(session=session)
    tools.g_region(n=15, s=8, w=12, e=25, res=1)
    tools.r_in_gdal(input=str(source_tif), output="imported")
    tools.g_region(raster="imported")
    arr = np.array(garray.array("imported", env=session.env))
    assert arr.shape == (ROWS, COLS)
    assert np.array_equal(arr, expected_values(0, 0, ROWS, COLS))
