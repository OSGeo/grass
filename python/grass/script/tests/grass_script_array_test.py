"""Tests for grass.script.array"""

import os

import pytest
import numpy as np

import grass.script as gs
from grass.script import array as garray
from grass.tools import Tools


@pytest.fixture
def session_3x4(tmp_path):
    """Set up a GRASS session with a 3x4 region and rasters of different types."""
    project = tmp_path / "test_project"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.g_region(rows=3, cols=4)
        tools.r_mapcalc(expression="int_map = int(row() + col())")
        tools.r_mapcalc(expression="float_map = float(row() + col())")
        tools.r_mapcalc(expression="double_map = double(row() + col())")
        yield session


@pytest.fixture
def session_3d(tmp_path):
    """Set up a GRASS session with a 3D region and 3D rasters."""
    project = tmp_path / "test_project_3d"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.g_region(n=2, s=0, e=3, w=0, res3=1, b=0, t=2)
        tools.r3_mapcalc(expression="float3d = float(row() + col() + depth())")
        tools.r3_mapcalc(expression="double3d = double(row() + col() + depth())")
        yield session


class TestArrayDtypeAutoDetection:
    """Test automatic dtype detection when reading raster maps."""

    def test_auto_detect_cell(self, session_3x4):
        """Reading a CELL map without dtype should give int32."""
        arr = garray.array(mapname="int_map", env=session_3x4.env)
        assert arr.dtype == np.int32

    def test_auto_detect_fcell(self, session_3x4):
        """Reading an FCELL map without dtype should give float32."""
        arr = garray.array(mapname="float_map", env=session_3x4.env)
        assert arr.dtype == np.float32

    def test_auto_detect_dcell(self, session_3x4):
        """Reading a DCELL map without dtype should give float64."""
        arr = garray.array(mapname="double_map", env=session_3x4.env)
        assert arr.dtype == np.float64

    def test_explicit_dtype_overrides_detection(self, session_3x4):
        """Explicit dtype should override auto-detection."""
        arr = garray.array(mapname="int_map", dtype=np.float64, env=session_3x4.env)
        assert arr.dtype == np.float64

    def test_no_mapname_defaults_to_double(self, session_3x4):
        """Empty array without mapname should default to float64."""
        arr = garray.array(env=session_3x4.env)
        assert arr.dtype == np.float64


class TestArrayWriteReadRoundTrip:
    """Test that dtype is preserved through write and read-back."""

    def test_roundtrip_int(self, session_3x4):
        """Write an int32 array, read it back, check dtype and values."""
        arr = garray.array(dtype=np.int32, env=session_3x4.env)
        arr[:] = np.arange(12, dtype=np.int32).reshape(3, 4)
        arr.write(mapname="roundtrip_int", overwrite=True)

        arr2 = garray.array(mapname="roundtrip_int", env=session_3x4.env)
        assert arr2.dtype == np.int32
        np.testing.assert_array_equal(arr, arr2)

    def test_roundtrip_float(self, session_3x4):
        """Write a float32 array, read it back, check dtype and values."""
        arr = garray.array(dtype=np.float32, env=session_3x4.env)
        arr[:] = np.arange(12, dtype=np.float32).reshape(3, 4) * 0.5
        arr.write(mapname="roundtrip_float", overwrite=True)

        arr2 = garray.array(mapname="roundtrip_float", env=session_3x4.env)
        assert arr2.dtype == np.float32
        np.testing.assert_array_almost_equal(arr, arr2)

    def test_roundtrip_double(self, session_3x4):
        """Write a float64 array, read it back, check dtype and values."""
        arr = garray.array(dtype=np.float64, env=session_3x4.env)
        arr[:] = np.arange(12, dtype=np.float64).reshape(3, 4) * 0.1
        arr.write(mapname="roundtrip_double", overwrite=True)

        arr2 = garray.array(mapname="roundtrip_double", env=session_3x4.env)
        assert arr2.dtype == np.float64
        np.testing.assert_array_equal(arr, arr2)


class TestArrayInt64Rejected:
    """Test that 64-bit integers are rejected with actionable error messages."""

    def test_read_with_int64_dtype_raises(self, session_3x4):
        """Passing dtype=int64 with a mapname should raise ValueError."""
        with pytest.raises(ValueError, match="64-bit integers are not supported"):
            garray.array(mapname="int_map", dtype=np.int64, env=session_3x4.env)

    def test_write_int64_array_raises(self, session_3x4):
        """Writing an int64 array should raise ValueError with cast hint."""
        arr = garray.array(dtype=np.int64, env=session_3x4.env)
        arr[:] = np.arange(12, dtype=np.int64).reshape(3, 4)
        with pytest.raises(ValueError, match=r"array\.astype"):
            arr.write(mapname="should_fail", overwrite=True)


class TestArray3dDtypeAutoDetection:
    """Test automatic dtype detection for 3D raster arrays."""

    def test_auto_detect_fcell_3d(self, session_3d):
        """Reading an FCELL 3D map without dtype should give float32."""
        arr = garray.array3d(mapname="float3d", env=session_3d.env)
        assert arr.dtype == np.float32

    def test_auto_detect_dcell_3d(self, session_3d):
        """Reading a DCELL 3D map without dtype should give float64."""
        arr = garray.array3d(mapname="double3d", env=session_3d.env)
        assert arr.dtype == np.float64

    def test_explicit_dtype_overrides_3d(self, session_3d):
        """Explicit dtype should override auto-detection for 3D."""
        arr = garray.array3d(mapname="double3d", dtype=np.float32, env=session_3d.env)
        assert arr.dtype == np.float32

    def test_no_mapname_defaults_to_double_3d(self, session_3d):
        """Empty 3D array without mapname should default to float64."""
        arr = garray.array3d(env=session_3d.env)
        assert arr.dtype == np.float64
