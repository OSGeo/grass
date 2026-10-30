"""Tests of r.timestamp"""

import pytest

from grass.tools import ToolError


def test_no_timestamp_by_default(session_tools, raster):
    """Reading a map with no timestamp set fails"""
    with pytest.raises(ToolError):
        session_tools.r_timestamp(map=raster)


def test_set_and_read_absolute_date(session_tools, raster):
    """A single absolute date round-trips through set and read"""
    session_tools.r_timestamp(map=raster, date="15 sep 1987")
    result = session_tools.r_timestamp(map=raster)
    assert result.text == "15 Sep 1987"


def test_set_and_read_date_range(session_tools, raster):
    """A start and end date round-trip through set and read"""
    session_tools.r_timestamp(map=raster, date="15 sep 1987/20 feb 1988")
    result = session_tools.r_timestamp(map=raster)
    assert result.text == "15 Sep 1987 / 20 Feb 1988"


def test_overwrite_timestamp(session_tools, raster):
    """Setting a new date replaces the previous one"""
    session_tools.r_timestamp(map=raster, date="15 sep 1987")
    session_tools.r_timestamp(map=raster, date="1 jan 2000")
    result = session_tools.r_timestamp(map=raster)
    assert result.text == "1 Jan 2000"


def test_remove_timestamp(session_tools, raster):
    """date=none removes an existing timestamp"""
    session_tools.r_timestamp(map=raster, date="15 sep 1987")
    session_tools.r_timestamp(map=raster, date="none")
    with pytest.raises(ToolError):
        session_tools.r_timestamp(map=raster)


def test_invalid_date_format(session_tools, raster):
    """An unparsable date string is rejected"""
    with pytest.raises(ToolError):
        session_tools.r_timestamp(map=raster, date="not a date")


def test_nonexistent_map_read(session_tools):
    """Reading the timestamp of a nonexistent map fails"""
    with pytest.raises(ToolError):
        session_tools.r_timestamp(map="doesnotexist")


def test_nonexistent_map_write(session_tools):
    """Setting the timestamp of a nonexistent map fails"""
    with pytest.raises(ToolError):
        session_tools.r_timestamp(map="doesnotexist", date="15 sep 1987")


@pytest.mark.parametrize("time_of_day", ["10", "10:30", "10:30:00", "10:30:23.34"])
def test_set_and_read_time_of_day(session_tools, raster, time_of_day):
    """Hour, hour:minute, hour:minute:second, and fractional seconds all round-trip"""
    date = f"18 feb 2005 {time_of_day}"
    session_tools.r_timestamp(map=raster, date=date)
    result = session_tools.r_timestamp(map=raster)
    assert result.text == f"18 Feb 2005 {time_of_day}"


@pytest.mark.parametrize("time_of_day", ["00:00", "12:00"], ids=["midnight", "noon"])
def test_set_and_read_midnight_and_noon(session_tools, raster, time_of_day):
    """Midnight and noon round-trip correctly

    These are the two hours where a naive 24h-to-12h conversion classically
    breaks (0 % 12 == 12 % 12 == 0), so they are worth pinning down
    explicitly rather than relying on an arbitrary hour value.
    """
    date = f"18 feb 2005 {time_of_day}"
    session_tools.r_timestamp(map=raster, date=date)
    result = session_tools.r_timestamp(map=raster)
    assert result.text == f"18 Feb 2005 {time_of_day}"


@pytest.mark.parametrize("date", ["18 feb 2005 2:30 pm", "18 feb 2005 02:30 PM"])
def test_twelve_hour_am_pm_suffix_is_rejected(session_tools, raster, date):
    """12-hour clock notation with an am/pm suffix is not accepted

    The timestamp format is documented as 24-hour only (hour 0-23). This
    guards against a future change silently accepting am/pm notation and
    misinterpreting it.
    """
    with pytest.raises(ToolError):
        session_tools.r_timestamp(map=raster, date=date)


@pytest.mark.parametrize("timezone", ["+0100", "-0500"])
def test_set_and_read_timezone(session_tools, raster, timezone):
    """A timezone offset on a timestamp round-trips correctly

    grass.temporal's conversion of timestamps to Python datetime objects
    currently drops timezone information (see GH-7166), but r.timestamp's
    own get/set round-trip, tested here, preserves it correctly.
    """
    date = f"18 feb 2005 10:30:00 {timezone}"
    session_tools.r_timestamp(map=raster, date=date)
    result = session_tools.r_timestamp(map=raster)
    assert result.text == f"18 Feb 2005 10:30:00 {timezone}"


def test_set_and_read_timezone_on_range(session_tools, raster):
    """A date range with a timezone offset on both ends round-trips correctly"""
    date = "18 feb 2005 10:30:00 +0100/20 jul 2007 20:30:00 +0100"
    session_tools.r_timestamp(map=raster, date=date)
    result = session_tools.r_timestamp(map=raster)
    assert result.text == "18 Feb 2005 10:30:00 +0100 / 20 Jul 2007 20:30:00 +0100"
