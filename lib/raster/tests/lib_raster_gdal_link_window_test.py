"""Tests for column-windowed reading of GDAL-linked (r.external) raster maps.

Rast_get_row() reads GDAL-linked maps through read_data_gdal(), which
restricts the GDAL read to the range of native columns that overlap the
current region instead of always reading the full native row width.
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
# Geometry of the source file, matching the g.region call in linked_session.
FILE_NORTH = ROWS
FILE_WEST = 0
FILE_RES = 1


@pytest.fixture(scope="module")
def linked_session(tmp_path_factory):
    """Session with a GeoTIFF exported and linked as 'linked'.

    The source raster has cell values that can be computed from their
    row and column positions without needing a second, independent
    read of the file.
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
    """A session in its own temporary mapset, so each test has its own region."""
    session, _ = linked_session
    with TemporaryMapsetSession(env=session.env) as mapset_session:
        yield mapset_session


@pytest.fixture
def source_tif(linked_session):
    """Path to the GeoTIFF file linked as 'linked' in the session fixture."""
    _, tif_path = linked_session
    return tif_path


@pytest.fixture(scope="module")
def latlon_session(tmp_path_factory):
    """WGS84 session with a full-longitude GeoTIFF linked as 'latlon'.

    The region covers a -180 to 180 longitude range at 1 degree resolution,
    to test wrapping of lat/lon in Rast__create_window_mapping() (window_map.c).
    """
    project = tmp_path_factory.mktemp("gdal_link_window_ll") / "project"
    gs.create_project(project, epsg=4326)
    tif_path = tmp_path_factory.mktemp("gdal_link_window_ll_data") / "source_ll.tif"
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=5, s=-5, w=-180, e=180, res=1)
        tools.r_mapcalc(expression="source_ll = (row() - 1) * 1000 + (col() - 1)")
        tools.r_out_gdal(
            input="source_ll", output=str(tif_path), format="GTiff", type="Int32"
        )
        tools.r_external(input=str(tif_path), output="latlon")
        yield session


@pytest.fixture
def latlon_mapset(latlon_session):
    """A session in its own temporary mapset, in the WGS84 project"""
    with TemporaryMapsetSession(env=latlon_session.env) as mapset_session:
        yield mapset_session


def expected_values(row_offset, col_offset, rows, cols):
    """Expected 'linked' values for a region shifted by row/col_offset cells."""
    row_values = (row_offset + np.arange(rows)) * 1000
    col_values = col_offset + np.arange(cols)
    return row_values[:, None] + col_values[None, :]


def nearest_native_index(offset, step, count):
    """offset + i * step for i in range(count), floored.

    Reproduces the nearest-neighbor mapping from a region cell to
    a native file cell in Rast__create_window_mapping() (window_map.c).
    """
    return np.floor(offset + step * np.arange(count)).astype(int)


def native_indices_for_region(north, west, res, rows, cols):
    """Native (row, col) indices 'linked' resolves to for a region."""
    step = res / FILE_RES
    native_cols = nearest_native_index(
        (west - FILE_WEST + res / 2.0) / FILE_RES, step, cols
    )
    native_rows = nearest_native_index(
        (FILE_NORTH - north + res / 2.0) / FILE_RES, step, rows
    )
    return native_rows, native_cols


def wrapped_native_col_indices(region_west, region_east, res, file_west, file_cols):
    """Native (0-based) column indices of GRASS's lat/lon wraparound mapping.

    Mirrors Rast__create_window_mapping() (window_map.c).
    """
    west, east = region_west, region_east
    while west > file_west + 360.0:
        west -= 360.0
        east -= 360.0
    while west < file_west:
        west += 360.0
        east += 360.0

    cols = round((region_east - region_west) / res)

    def native_for(west):
        x = np.floor((west - file_west + res / 2.0) / res + np.arange(cols))
        x[(x < 0) | (x >= file_cols)] = -1
        return x.astype(int)

    native = native_for(west)
    while east - 360.0 > file_west:
        east -= 360.0
        west -= 360.0
        unresolved = native < 0
        native[unresolved] = native_for(west)[unresolved]
    return native


def test_region_fully_inside_source_extent(session):
    """A region fully inside the file reads the correct sub-window."""
    Tools(session=session).g_region(n=15, s=8, w=12, e=25, res=1)
    arr = np.array(garray.array("linked", null=NULL, env=session.env))
    assert np.array_equal(arr, expected_values(5, 12, *arr.shape))


def test_region_partially_outside_source_extent(session):
    """Columns outside the file's extent read as null, the rest as data."""
    Tools(session=session).g_region(n=10, s=5, w=-5, e=10, res=1)
    arr = np.array(garray.array("linked", null=NULL, env=session.env))
    assert np.all(arr[:, :5] == NULL)
    assert np.array_equal(arr[:, 5:], expected_values(10, 0, arr.shape[0], 10))


def test_region_fully_outside_source_extent(session):
    """A region with no overlap at all reads back as entirely null."""
    Tools(session=session).g_region(n=10, s=5, w=-50, e=-40, res=1)
    arr = np.array(garray.array("linked", null=NULL, env=session.env))
    assert np.all(arr == NULL)


def test_region_coarser_than_source_resolution(session):
    """A region coarser than the file's resolution reads the nearest cell."""
    Tools(session=session).g_region(n=16, s=6, w=10, e=24, res=2)
    arr = np.array(garray.array("linked", null=NULL, env=session.env))
    native_rows, native_cols = native_indices_for_region(16, 10, 2, *arr.shape)
    assert np.array_equal(arr, native_rows[:, None] * 1000 + native_cols[None, :])


def test_region_finer_than_source_resolution(session):
    """A region finer than the file's resolution duplicates the nearest cell."""
    Tools(session=session).g_region(n=16, s=11, w=10, e=15, res=0.5)
    arr = np.array(garray.array("linked", null=NULL, env=session.env))
    native_rows, native_cols = native_indices_for_region(16, 10, 0.5, *arr.shape)
    assert np.array_equal(arr, native_rows[:, None] * 1000 + native_cols[None, :])


def test_r_in_gdal_ignores_region(session, source_tif):
    """r.in.gdal imports the full file regardless of the current region."""
    tools = Tools(session=session)
    tools.g_region(n=15, s=8, w=12, e=25, res=1)
    tools.r_in_gdal(input=str(source_tif), output="imported")
    tools.g_region(raster="imported")
    arr = np.array(garray.array("imported", env=session.env))
    assert arr.shape == (ROWS, COLS)
    assert np.array_equal(arr, expected_values(0, 0, ROWS, COLS))


def test_region_wraps_across_antimeridian(latlon_mapset):
    """A region crossing the antimeridian reads correctly wrapped columns."""
    session = latlon_mapset
    Tools(session=session).g_region(n=5, s=-5, w=170, e=190, res=1)
    arr = np.array(garray.array("latlon", null=NULL, env=session.env))
    native_cols = wrapped_native_col_indices(
        region_west=170, region_east=190, res=1, file_west=-180, file_cols=360
    )
    assert np.all(native_cols >= 0)
    native_rows = np.arange(arr.shape[0])  # n=5, s=-5, res=1 matches the file
    assert np.array_equal(arr, native_rows[:, None] * 1000 + native_cols[None, :])


def test_r_mapcalc_parallel_matches_serial(session):
    """Results of r.mapcalc with linked data and nprocs = 1 and > 1 match."""
    tools = Tools(session=session)
    tools.g_region(n=15, s=8, w=12, e=25, res=1)
    tools.r_mapcalc(expression="serial = linked", nprocs=1)
    tools.r_mapcalc(expression="parallel = linked", nprocs=2)
    serial = np.array(garray.array("serial", env=session.env))
    parallel = np.array(garray.array("parallel", env=session.env))
    assert np.array_equal(parallel, expected_values(5, 12, *parallel.shape))
    assert np.array_equal(parallel, serial)


def test_r_univar_parallel_matches_serial(session):
    """Results of r.univar with  linked data and nprocs = 1 and > 1 match."""
    tools = Tools(session=session)
    tools.g_region(n=15, s=8, w=12, e=25, res=1)
    serial = tools.r_univar(map="linked", nprocs=1, format="json").json
    parallel = tools.r_univar(map="linked", nprocs=2, format="json").json
    assert parallel == serial
