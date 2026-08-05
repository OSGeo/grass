"""Tests of r.timestamp"""

import pytest

from grass.tools import ToolError


def test_no_timestamp_by_default(session_tools):
    """Reading a map with no timestamp set fails"""
    session_tools.r_mapcalc(expression="raster = 1")
    with pytest.raises(ToolError):
        session_tools.r_timestamp(map="raster")


def test_set_and_read_absolute_date(session_tools):
    """A single absolute date round-trips through set and read"""
    session_tools.r_mapcalc(expression="raster = 1")
    session_tools.r_timestamp(map="raster", date="15 sep 1987")
    result = session_tools.r_timestamp(map="raster")
    assert result.text == "15 Sep 1987"


def test_set_and_read_date_range(session_tools):
    """A start and end date round-trip through set and read"""
    session_tools.r_mapcalc(expression="raster = 1")
    session_tools.r_timestamp(map="raster", date="15 sep 1987/20 feb 1988")
    result = session_tools.r_timestamp(map="raster")
    assert result.text == "15 Sep 1987 / 20 Feb 1988"


def test_overwrite_timestamp(session_tools):
    """Setting a new date replaces the previous one"""
    session_tools.r_mapcalc(expression="raster = 1")
    session_tools.r_timestamp(map="raster", date="15 sep 1987")
    session_tools.r_timestamp(map="raster", date="1 jan 2000")
    result = session_tools.r_timestamp(map="raster")
    assert result.text == "1 Jan 2000"


def test_remove_timestamp(session_tools):
    """date=none removes an existing timestamp"""
    session_tools.r_mapcalc(expression="raster = 1")
    session_tools.r_timestamp(map="raster", date="15 sep 1987")
    session_tools.r_timestamp(map="raster", date="none")
    with pytest.raises(ToolError):
        session_tools.r_timestamp(map="raster")


def test_invalid_date_format(session_tools):
    """An unparsable date string is rejected"""
    session_tools.r_mapcalc(expression="raster = 1")
    with pytest.raises(ToolError):
        session_tools.r_timestamp(map="raster", date="not a date")


def test_nonexistent_map_read(session_tools):
    """Reading the timestamp of a nonexistent map fails"""
    with pytest.raises(ToolError):
        session_tools.r_timestamp(map="doesnotexist")


def test_nonexistent_map_write(session_tools):
    """Setting the timestamp of a nonexistent map fails"""
    with pytest.raises(ToolError):
        session_tools.r_timestamp(map="doesnotexist", date="15 sep 1987")
