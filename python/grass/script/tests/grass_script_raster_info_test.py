"""Test grass.script.raster_info"""

import pytest

import grass.script as gs


def test_raster_info_extent_and_resolution_are_floats(session_2x2):
    """Test the extent and the resolution are numbers, not text"""
    info = gs.raster_info("ones", env=session_2x2.env)
    assert info["north"] == pytest.approx(1.0)
    assert info["south"] == pytest.approx(0.0)
    assert info["east"] == pytest.approx(1.0)
    assert info["west"] == pytest.approx(0.0)
    assert info["nsres"] == pytest.approx(0.5)
    assert info["ewres"] == pytest.approx(0.5)


def test_raster_info_range_of_integer_raster(session_2x2):
    """Test the minimum and the maximum of an integer raster are floats"""
    info = gs.raster_info("ones", env=session_2x2.env)
    assert info["min"] == pytest.approx(1.0)
    assert info["max"] == pytest.approx(1.0)
    assert isinstance(info["min"], float)
    assert isinstance(info["max"], float)


def test_raster_info_range_of_floating_point_raster(session_2x2):
    """Test the minimum and the maximum of a floating point raster"""
    gs.mapcalc("quarters = float(row()) / 4", env=session_2x2.env)
    info = gs.raster_info("quarters", env=session_2x2.env)
    assert info["min"] == pytest.approx(0.25)
    assert info["max"] == pytest.approx(0.5)
    assert info["datatype"] == "FCELL"


def test_raster_info_range_of_raster_with_only_nulls(session_2x2):
    """Test the minimum and the maximum are None when there are no values"""
    gs.mapcalc("only_nulls = null()", env=session_2x2.env)
    info = gs.raster_info("only_nulls", env=session_2x2.env)
    assert info["min"] is None
    assert info["max"] is None


def test_raster_info_other_values_are_text(session_2x2):
    """Test the values which are not converted to numbers"""
    info = gs.raster_info("ones", env=session_2x2.env)
    assert info["datatype"] == "CELL"
    assert info["rows"] == "2"
    assert info["cols"] == "2"
    assert info["map"] == "ones"
    assert info["mapset"] == "PERMANENT"
