"""Tests for the field get/set ctypes bindings (year, month, day, ...)

datetime_set_year(), datetime_set_day() and friends only validate and write
a single field of a DateTime struct, so no GRASS session is needed here.
"""

from ctypes import byref

import pytest

from grass.lib import date as libdate


def absolute_datetime():
    """A fresh absolute DateTime with year-to-second precision"""
    dt = libdate.DateTime()
    libdate.datetime_set_type(
        byref(dt),
        libdate.DATETIME_ABSOLUTE,
        libdate.DATETIME_YEAR,
        libdate.DATETIME_SECOND,
        0,
    )
    return dt


def test_set_year_rejects_zero() -> None:
    """Year 0 does not exist in the AD/BC calendar: 1 BC is followed by 1 AD"""
    dt = absolute_datetime()
    assert libdate.datetime_set_year(byref(dt), 0) != 0


@pytest.mark.parametrize("month", [0, 13])
def test_set_month_rejects_out_of_range(month) -> None:
    dt = absolute_datetime()
    assert libdate.datetime_set_month(byref(dt), month) != 0


@pytest.mark.parametrize(
    ("year", "month", "day", "valid"),
    [
        # 2000 is a leap year, so February has 29 days.
        (2000, 2, 29, True),
        (2000, 2, 30, False),
        # 1900 is not a leap year, so February has only 28 days.
        (1900, 2, 29, False),
        (1900, 2, 28, True),
    ],
)
def test_set_day_is_validated_against_the_month_and_year(
    year, month, day, valid
) -> None:
    """A day is only valid relative to the month and year already set"""
    dt = absolute_datetime()
    libdate.datetime_set_year(byref(dt), year)
    libdate.datetime_set_month(byref(dt), month)
    ret = libdate.datetime_set_day(byref(dt), day)
    assert (ret == 0) == valid


def test_setting_the_year_resets_the_day() -> None:
    """Changing the year invalidates any day already set for the old year

    datetime_set_year() zeroes the day field on an absolute datetime rather
    than leaving a day that might not exist in the new year (e.g. keeping
    day 29 after moving from a leap year to a non-leap one).
    """
    dt = absolute_datetime()
    libdate.datetime_set_year(byref(dt), 2000)
    libdate.datetime_set_month(byref(dt), 6)
    libdate.datetime_set_day(byref(dt), 15)
    assert dt.day == 15

    libdate.datetime_set_year(byref(dt), 2001)
    assert dt.day == 0


@pytest.mark.parametrize(("hour", "valid"), [(23, True), (24, False)])
def test_set_hour_rejects_24(hour, valid) -> None:
    dt = absolute_datetime()
    assert (libdate.datetime_set_hour(byref(dt), hour) == 0) == valid


@pytest.mark.parametrize(("minute", "valid"), [(59, True), (60, False)])
def test_set_minute_rejects_60(minute, valid) -> None:
    dt = absolute_datetime()
    assert (libdate.datetime_set_minute(byref(dt), minute) == 0) == valid


@pytest.mark.parametrize(("second", "valid"), [(59.999, True), (60.0, False)])
def test_set_second_rejects_60(second, valid) -> None:
    dt = absolute_datetime()
    assert (libdate.datetime_set_second(byref(dt), second) == 0) == valid


def test_set_fracsec_rejects_negative() -> None:
    dt = absolute_datetime()
    assert libdate.datetime_set_fracsec(byref(dt), -1) != 0
    assert libdate.datetime_set_fracsec(byref(dt), 3) == 0


def test_check_year_reports_a_missing_year_component() -> None:
    """A DateTime whose range does not include YEAR has no year to check"""
    dt = libdate.DateTime()
    libdate.datetime_set_type(
        byref(dt),
        libdate.DATETIME_RELATIVE,
        libdate.DATETIME_HOUR,
        libdate.DATETIME_SECOND,
        0,
    )
    assert libdate.datetime_check_year(byref(dt), 2000) == -2
