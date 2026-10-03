"""Tests for the sign, copy, equality and range ctypes bindings

datetime_set_negative(), datetime_copy(), datetime_is_same() and
datetime_is_between() all operate on values already in memory, so no GRASS
session is needed here.
"""

from ctypes import byref

import pytest

from grass.lib import date as libdate


def scan(text):
    """Parse text with datetime_scan(), returning the resulting DateTime"""
    dt = libdate.DateTime()
    assert libdate.datetime_scan(byref(dt), text.encode()) == 0
    return dt


def test_datetimes_are_positive_by_default() -> None:
    dt = scan("15 aug 2001")
    assert libdate.datetime_is_positive(byref(dt))
    assert not libdate.datetime_is_negative(byref(dt))


def test_set_negative_and_invert_sign() -> None:
    dt = scan("15 aug 2001")
    libdate.datetime_set_negative(byref(dt))
    assert libdate.datetime_is_negative(byref(dt))

    libdate.datetime_invert_sign(byref(dt))
    assert libdate.datetime_is_positive(byref(dt))


def test_is_same_compares_by_value() -> None:
    a = scan("15 aug 2001 12:30:45")
    b = scan("15 aug 2001 12:30:45")
    c = scan("16 aug 2001 12:30:45")
    assert libdate.datetime_is_same(byref(a), byref(b))
    assert not libdate.datetime_is_same(byref(a), byref(c))


def test_copy_is_independent_of_the_source() -> None:
    original = scan("15 aug 2001")
    copied = libdate.DateTime()
    libdate.datetime_copy(byref(copied), byref(original))
    assert libdate.datetime_is_same(byref(original), byref(copied))

    libdate.datetime_set_year(byref(copied), 1999)
    assert original.year == 2001
    assert copied.year == 1999


@pytest.mark.parametrize(
    ("x", "a", "b", "expected"),
    [
        # A normal ascending range, including both boundaries.
        (5, 1, 10, True),
        (1, 1, 10, True),
        (10, 1, 10, True),
        (0, 1, 10, False),
        # The range is also accepted reversed (a > b).
        (5, 10, 1, True),
    ],
)
def test_is_between(x, a, b, expected) -> None:
    assert bool(libdate.datetime_is_between(x, a, b)) == expected
