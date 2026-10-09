"""Test tile index bounds checking in Rast3d_write_tile"""

import os
import subprocess
import sys

import pytest

import grass.script as gs

WRITE_TILE_SCRIPT = """\
import sys
from ctypes import POINTER, byref, c_float, cast

from grass.lib.gis import G_gisinit
from grass.lib.raster import FCELL_TYPE
from grass.lib.raster3d import (
    RASTER3D_NO_CACHE,
    RASTER3D_Map,
    RASTER3D_Region,
    Rast3d_adjust_region,
    Rast3d_init_defaults,
    Rast3d_open_cell_new,
    Rast3d_write_tile,
)

index_offset = int(sys.argv[1])

G_gisinit("test")
Rast3d_init_defaults()

region = RASTER3D_Region()
region.north = 100
region.south = 0
region.east = 100
region.west = 0
region.top = 100
region.bottom = 0
region.rows = 10
region.cols = 10
region.depths = 10
Rast3d_adjust_region(byref(region))

handle = Rast3d_open_cell_new(
    "bounds_check_3d", FCELL_TYPE, RASTER3D_NO_CACHE, byref(region)
)
if not handle:
    sys.exit("Rast3d_open_cell_new failed")
map_ptr = cast(handle, POINTER(RASTER3D_Map))
tile = (c_float * map_ptr.contents.tileSize)()
Rast3d_write_tile(map_ptr, map_ptr.contents.nTiles + index_offset, tile, FCELL_TYPE)
print("tile write accepted")
"""


@pytest.fixture
def session(tmp_path):
    """Active session in an XY project (scope: function)"""
    project = tmp_path / "xy_test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield session


def write_tile(index_offset, session, tmp_path):
    """Call Rast3d_write_tile with tile index nTiles + index_offset.

    Runs in a subprocess because an invalid index triggers a fatal error
    which exits the calling process.
    """
    script = tmp_path / "write_tile.py"
    script.write_text(WRITE_TILE_SCRIPT, encoding="utf-8")
    return subprocess.run(
        [sys.executable, str(script), str(index_offset)],
        env=session.env,
        capture_output=True,
        text=True,
        timeout=60,
        check=False,
    )


def test_write_tile_accepts_last_index(session, tmp_path):
    """Writing the last valid tile index (nTiles - 1) is accepted"""
    result = write_tile(-1, session, tmp_path)
    assert result.returncode == 0, result.stderr
    assert "tile write accepted" in result.stdout


def test_write_tile_rejects_index_past_end(session, tmp_path):
    """A tile index equal to nTiles is rejected as out of range"""
    result = write_tile(0, session, tmp_path)
    assert result.returncode != 0, "out-of-range tile index was accepted"
    assert "tileIndex out of range" in result.stderr
