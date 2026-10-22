"""Tests for the datetime_change_from_to() ctypes binding

datetime_change_from_to() only reads and writes a DateTime struct already in
memory, so no GRASS session is needed here.

This covers only the straightforward truncate/extend cases (round == -1,
i.e. floor, or extending to a lower precision than before). The function
also supports rounding up when precision is lost (round > 0) and an
"increment by half" mode (round == 0); both involve substantially more
elaborate carry logic that is out of scope for this first pass.
"""

from ctypes import byref, create_string_buffer

from grass.lib import date as libdate


def scan(text):
    """Parse text with datetime_scan(), returning the resulting DateTime"""
    dt = libdate.DateTime()
    assert libdate.datetime_scan(byref(dt), text.encode()) == 0, text
    return dt


def format_datetime(dt):
    """Format a DateTime with datetime_format(), returning the text"""
    buf = create_string_buffer(128)
    assert libdate.datetime_format(byref(dt), buf) == 0
    return buf.value.decode()


def test_truncating_to_a_coarser_precision_drops_the_finer_fields() -> None:
    """round=-1 (floor) drops the time of day rather than rounding it away"""
    dt = scan("15 aug 2001 23:45:30")
    ret = libdate.datetime_change_from_to(
        byref(dt), libdate.DATETIME_YEAR, libdate.DATETIME_DAY, -1
    )
    assert ret == 0
    assert format_datetime(dt) == "15 Aug 2001"


def test_extending_to_a_finer_precision_zero_fills_the_new_fields() -> None:
    dt = scan("15 aug 2001")
    ret = libdate.datetime_change_from_to(
        byref(dt), libdate.DATETIME_YEAR, libdate.DATETIME_SECOND, 0
    )
    assert ret == 0
    assert format_datetime(dt) == "15 Aug 2001 00:00:00"


def test_truncating_a_relative_interval() -> None:
    incr = libdate.DateTime()
    libdate.datetime_set_type(
        byref(incr),
        libdate.DATETIME_RELATIVE,
        libdate.DATETIME_DAY,
        libdate.DATETIME_SECOND,
        0,
    )
    libdate.datetime_set_day(byref(incr), 2)
    libdate.datetime_set_hour(byref(incr), 5)

    ret = libdate.datetime_change_from_to(
        byref(incr), libdate.DATETIME_DAY, libdate.DATETIME_DAY, -1
    )
    assert ret == 0
    assert format_datetime(incr) == "2 days"


def test_rejects_an_uninitialized_datetime() -> None:
    """A DateTime that was never given a type via datetime_set_type() is
    not a valid input: its mode is neither ABSOLUTE nor RELATIVE"""
    dt = libdate.DateTime()
    assert (
        libdate.datetime_change_from_to(
            byref(dt), libdate.DATETIME_YEAR, libdate.DATETIME_DAY, 0
        )
        == -1
    )


def test_rejects_an_invalid_from_for_the_datetimes_mode() -> None:
    """An absolute datetime must always count from YEAR"""
    dt = scan("15 aug 2001")
    assert (
        libdate.datetime_change_from_to(
            byref(dt), libdate.DATETIME_MONTH, libdate.DATETIME_DAY, 0
        )
        == -2
    )
