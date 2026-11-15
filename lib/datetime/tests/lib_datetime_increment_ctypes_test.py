"""Tests for the increment ctypes bindings

datetime_increment() and its supporting functions only read and write
DateTime structs already in memory, so no GRASS session is needed here.
"""

from ctypes import byref, c_int, create_string_buffer

import pytest

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


def relative(from_, to):
    """A relative DateTime spanning from_ to to, both zeroed"""
    incr = libdate.DateTime()
    assert (
        libdate.datetime_set_type(byref(incr), libdate.DATETIME_RELATIVE, from_, to, 0)
        == 0
    )
    return incr


@pytest.mark.parametrize(
    ("text", "days", "expected"),
    [
        # Adding days can carry into the next month.
        ("30 jan 2001", 5, "4 Feb 2001"),
        ("15 aug 2001", 5, "20 Aug 2001"),
    ],
)
def test_increment_by_days(text, days, expected) -> None:
    dt = scan(text)
    incr = relative(libdate.DATETIME_DAY, libdate.DATETIME_DAY)
    libdate.datetime_set_day(byref(incr), days)
    assert libdate.datetime_increment(byref(dt), byref(incr)) == 0
    assert format_datetime(dt) == expected


def test_decrement_by_days_borrows_from_the_previous_month() -> None:
    """A negative day increment can carry back across a month boundary"""
    dt = scan("3 mar 2001")
    incr = relative(libdate.DATETIME_DAY, libdate.DATETIME_DAY)
    libdate.datetime_set_day(byref(incr), 5)
    libdate.datetime_set_negative(byref(incr))
    assert libdate.datetime_increment(byref(dt), byref(incr)) == 0
    assert format_datetime(dt) == "26 Feb 2001"


@pytest.mark.parametrize(
    ("text", "months", "expected"),
    [
        # Adding months can carry into the next year.
        ("nov 2001", 3, "Feb 2002"),
        ("nov 2001", 5, "Apr 2002"),
    ],
)
def test_increment_by_months_on_a_year_month_datetime(text, months, expected) -> None:
    """Month increments only apply to a datetime whose own precision is in
    the year/month group; a full calendar date (day precision) cannot take
    a month increment directly, see
    test_increment_rejects_a_month_increment_on_a_day_precision_datetime.
    """
    dt = scan(text)
    incr = relative(libdate.DATETIME_YEAR, libdate.DATETIME_MONTH)
    libdate.datetime_set_month(byref(incr), months)
    assert libdate.datetime_increment(byref(dt), byref(incr)) == 0
    assert format_datetime(dt) == expected


def test_increment_by_an_hour_carries_into_the_next_day() -> None:
    dt = scan("15 aug 2001 23:30:00")
    incr = relative(libdate.DATETIME_DAY, libdate.DATETIME_SECOND)
    libdate.datetime_set_hour(byref(incr), 1)
    assert libdate.datetime_increment(byref(dt), byref(incr)) == 0
    assert format_datetime(dt) == "16 Aug 2001 00:30:00"


def test_increment_rejects_a_month_increment_on_a_day_precision_datetime() -> None:
    """Year/month and day/second increments cannot mix

    A month has no fixed number of days, so a relative DateTime cannot span
    both groups (enforced by datetime_set_type()'s own -5 error). The same
    split applies to increments: a datetime with day precision can only be
    incremented within the day/second group, even though "N months" is a
    perfectly valid relative DateTime on its own.
    """
    dt = scan("15 nov 2001")
    incr = relative(libdate.DATETIME_YEAR, libdate.DATETIME_MONTH)
    libdate.datetime_set_month(byref(incr), 3)
    assert libdate.datetime_increment(byref(dt), byref(incr)) != 0


def test_check_increment_rejects_a_more_precise_increment() -> None:
    src = scan("15 aug 2001")  # day precision
    too_precise = relative(libdate.DATETIME_DAY, libdate.DATETIME_SECOND)
    assert libdate.datetime_check_increment(byref(src), byref(too_precise)) == -2


def test_check_increment_rejects_an_absolute_increment() -> None:
    src = scan("15 aug 2001")
    incr = libdate.DateTime()
    libdate.datetime_set_type(
        byref(incr),
        libdate.DATETIME_ABSOLUTE,
        libdate.DATETIME_YEAR,
        libdate.DATETIME_YEAR,
        0,
    )
    assert libdate.datetime_check_increment(byref(src), byref(incr)) == -1


def test_get_increment_type_matches_the_source_precision() -> None:
    """The recommended increment type mirrors the source's own to/fracsec,
    using YEAR as the increment's from for a year/month source and DAY for
    a day/second source (see datetime_increment tests above for why)."""
    mode, from_, to, fracsec = c_int(), c_int(), c_int(), c_int()

    full_datetime = scan("15 aug 2001 12:30:45")
    assert (
        libdate.datetime_get_increment_type(
            byref(full_datetime), byref(mode), byref(from_), byref(to), byref(fracsec)
        )
        == 0
    )
    assert (mode.value, from_.value, to.value) == (
        libdate.DATETIME_RELATIVE,
        libdate.DATETIME_DAY,
        libdate.DATETIME_SECOND,
    )

    year_month = scan("aug 2001")
    assert (
        libdate.datetime_get_increment_type(
            byref(year_month), byref(mode), byref(from_), byref(to), byref(fracsec)
        )
        == 0
    )
    assert (mode.value, from_.value, to.value) == (
        libdate.DATETIME_RELATIVE,
        libdate.DATETIME_YEAR,
        libdate.DATETIME_MONTH,
    )
