"""Tests for the calendar arithmetic ctypes bindings

datetime_is_leap_year(), datetime_days_in_year() and datetime_days_in_month()
take plain year/month/ad integers rather than a DateTime struct, so no GRASS
session is needed here.
"""

import pytest

from grass.lib import date as libdate

AD = 1
BC = 0


@pytest.mark.parametrize(
    ("year", "expected"),
    [
        # Divisible by 4 but not 100: a regular leap year.
        (2004, True),
        (2001, False),
        # Divisible by 100 but not 400: the Gregorian exception.
        (1900, False),
        # Divisible by 400: the exception to the exception.
        (2000, True),
    ],
)
def test_is_leap_year_follows_the_gregorian_rule(year, expected) -> None:
    assert bool(libdate.datetime_is_leap_year(year, AD)) == expected


def test_is_leap_year_treats_every_bc_year_as_non_leap() -> None:
    """BC years are never leap here, unlike a proleptic Gregorian calendar

    404 BC would be a leap year under a proleptic Gregorian calendar, but
    datetime_is_leap_year() special-cases BC to always return false. This
    is a simplification worth locking in rather than assuming leap-year
    rules apply symmetrically to BC dates.
    """
    assert libdate.datetime_is_leap_year(404, BC) == 0


@pytest.mark.parametrize(("year", "days"), [(2000, 366), (2001, 365)])
def test_days_in_year_matches_leap_year_status(year, days) -> None:
    assert libdate.datetime_days_in_year(year, AD) == days


@pytest.mark.parametrize(
    ("year", "month", "ad", "days"),
    [
        # February in and out of a leap year.
        (2000, 2, AD, 29),
        (1900, 2, AD, 28),
        # February in a BC year: always 28 days, since BC is never leap.
        (4, 2, BC, 28),
        # A 30-day and a 31-day month, unaffected by leap years.
        (2001, 4, AD, 30),
        (2001, 1, AD, 31),
    ],
)
def test_days_in_month(year, month, ad, days) -> None:
    assert libdate.datetime_days_in_month(year, month, ad) == days


@pytest.mark.parametrize("month", [0, 13])
def test_days_in_month_rejects_out_of_range_month(month) -> None:
    assert libdate.datetime_days_in_month(2000, month, AD) < 0
