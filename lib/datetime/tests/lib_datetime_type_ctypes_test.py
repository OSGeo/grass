"""Tests for the DateTime type (mode/from/to/fracsec) ctypes bindings

datetime_set_type() and datetime_check_type() only validate and write the
type fields of a DateTime struct, so no GRASS session is needed here.
"""

from ctypes import byref

import pytest

from grass.lib import date as libdate


def set_type(mode, from_, to, fracsec=0):
    """Call datetime_set_type(), returning (return_code, DateTime)"""
    dt = libdate.DateTime()
    ret = libdate.datetime_set_type(byref(dt), mode, from_, to, fracsec)
    return ret, dt


@pytest.mark.parametrize(
    ("mode", "from_", "to"),
    [
        (libdate.DATETIME_ABSOLUTE, libdate.DATETIME_YEAR, libdate.DATETIME_SECOND),
        (libdate.DATETIME_ABSOLUTE, libdate.DATETIME_YEAR, libdate.DATETIME_YEAR),
        (libdate.DATETIME_RELATIVE, libdate.DATETIME_YEAR, libdate.DATETIME_MONTH),
        (libdate.DATETIME_RELATIVE, libdate.DATETIME_DAY, libdate.DATETIME_SECOND),
    ],
)
def test_set_type_accepts_valid_combinations(mode, from_, to) -> None:
    ret, dt = set_type(mode, from_, to)
    assert ret == 0
    assert libdate.datetime_is_valid_type(byref(dt))


def test_set_type_rejects_unknown_mode() -> None:
    ret, _dt = set_type(0, libdate.DATETIME_YEAR, libdate.DATETIME_SECOND)
    assert ret == -1


def test_set_type_rejects_from_after_to() -> None:
    ret, _dt = set_type(
        libdate.DATETIME_ABSOLUTE, libdate.DATETIME_SECOND, libdate.DATETIME_YEAR
    )
    assert ret == -4


def test_set_type_rejects_absolute_from_other_than_year() -> None:
    """An absolute datetime always counts from a year, never a smaller unit"""
    ret, _dt = set_type(
        libdate.DATETIME_ABSOLUTE, libdate.DATETIME_MONTH, libdate.DATETIME_SECOND
    )
    assert ret == -6


def test_set_type_rejects_relative_interval_crossing_the_month_day_gap() -> None:
    """A relative interval cannot span from the year/month group into the
    day/second group, since there is no fixed number of days in a month"""
    ret, _dt = set_type(
        libdate.DATETIME_RELATIVE, libdate.DATETIME_YEAR, libdate.DATETIME_DAY
    )
    assert ret == -5


def test_set_type_rejects_negative_fracsec_with_seconds() -> None:
    ret, _dt = set_type(
        libdate.DATETIME_ABSOLUTE,
        libdate.DATETIME_YEAR,
        libdate.DATETIME_SECOND,
        -1,
    )
    assert ret == -7


def test_is_absolute_and_is_relative_are_mutually_exclusive() -> None:
    _, absolute = set_type(
        libdate.DATETIME_ABSOLUTE, libdate.DATETIME_YEAR, libdate.DATETIME_YEAR
    )
    _, relative = set_type(
        libdate.DATETIME_RELATIVE, libdate.DATETIME_YEAR, libdate.DATETIME_YEAR
    )
    assert libdate.datetime_is_absolute(byref(absolute))
    assert not libdate.datetime_is_relative(byref(absolute))
    assert libdate.datetime_is_relative(byref(relative))
    assert not libdate.datetime_is_absolute(byref(relative))
