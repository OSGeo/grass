"""Tests for datetime arithmetic and granularity adjustment

Both functions are pure datetime arithmetic, so no GRASS session and no
temporal database are needed here.
"""

from datetime import datetime

import pytest

from grass.temporal.datetime_math import (
    adjust_datetime_to_granularity,
    decrement_datetime_by_string,
    increment_datetime_by_string,
)


@pytest.mark.parametrize(
    ("start", "increment", "expected"),
    [
        # A single increment string may combine every supported unit. Seconds
        # carry into minutes and weeks are added on top of the days.
        (
            datetime(2001, 9, 1),
            "60 seconds, 4 minutes, 12 hours, 10 days, 1 weeks, 5 months, 1 years",
            datetime(2003, 2, 18, 12, 5),
        ),
        # A month increment lands on the same day of the following month.
        (datetime(2001, 11, 1), "1 months", datetime(2001, 12, 1)),
        # More than twelve months has to roll the year over.
        (datetime(2001, 11, 1), "13 months", datetime(2002, 12, 1)),
        (datetime(2001, 1, 1), "72 months", datetime(2007, 1, 1)),
    ],
)
def test_increment_datetime_by_string(start, increment, expected) -> None:
    assert increment_datetime_by_string(start, increment) == expected


@pytest.mark.parametrize(
    ("granularity", "expected"),
    [
        # The date is truncated to the granularity unit, so a granularity
        # finer than or equal to the seconds of the date leaves it unchanged.
        ("5 seconds", datetime(2001, 8, 8, 12, 30, 30)),
        ("20 minutes", datetime(2001, 8, 8, 12, 30)),
        ("3 hours", datetime(2001, 8, 8, 12, 0)),
        ("5 days", datetime(2001, 8, 8, 0, 0)),
        # Weeks snap back to the Monday of the week, 2001-08-08 is a Wednesday.
        ("2 weeks", datetime(2001, 8, 6, 0, 0)),
        ("6 months", datetime(2001, 8, 1, 0, 0)),
        ("2 years", datetime(2001, 1, 1, 0, 0)),
        # The singular unit names work the same way.
        ("1 second", datetime(2001, 8, 8, 12, 30, 30)),
        ("1 minute", datetime(2001, 8, 8, 12, 30)),
        ("1 hour", datetime(2001, 8, 8, 12, 0)),
        ("1 day", datetime(2001, 8, 8, 0, 0)),
        ("1 week", datetime(2001, 8, 6, 0, 0)),
        ("1 month", datetime(2001, 8, 1, 0, 0)),
        ("1 year", datetime(2001, 1, 1, 0, 0)),
        # With several units the finest one present wins, because truncating to
        # it already satisfies all the coarser ones.
        (
            "2 years, 3 months, 5 days, 3 hours, 3 minutes, 2 seconds",
            datetime(2001, 8, 8, 12, 30, 30),
        ),
        ("3 months, 5 days, 3 minutes", datetime(2001, 8, 8, 12, 30)),
        ("3 weeks, 5 days", datetime(2001, 8, 8, 0, 0)),
    ],
)
def test_adjust_datetime_to_granularity(granularity, expected) -> None:
    assert (
        adjust_datetime_to_granularity(datetime(2001, 8, 8, 12, 30, 30), granularity)
        == expected
    )


@pytest.mark.parametrize(
    ("increment", "expected"),
    [
        # Each unit is accepted in singular and plural form.
        ("1 second", datetime(2001, 1, 1, 0, 0, 1)),
        ("9 seconds", datetime(2001, 1, 1, 0, 0, 9)),
        ("1 minute", datetime(2001, 1, 1, 0, 1)),
        ("7 minutes", datetime(2001, 1, 1, 0, 7)),
        ("1 hour", datetime(2001, 1, 1, 1)),
        ("5 hours", datetime(2001, 1, 1, 5)),
        ("1 day", datetime(2001, 1, 2)),
        ("2 days", datetime(2001, 1, 3)),
        # A week is seven days, also for counts other than one.
        ("1 week", datetime(2001, 1, 8)),
        ("2 weeks", datetime(2001, 1, 15)),
        ("1 month", datetime(2001, 2, 1)),
        ("1 year", datetime(2002, 1, 1)),
        ("3 years", datetime(2004, 1, 1)),
    ],
)
def test_increment_datetime_by_string_single_unit(increment, expected) -> None:
    assert increment_datetime_by_string(datetime(2001, 1, 1), increment) == expected


@pytest.mark.parametrize(
    ("decrement", "expected"),
    [
        ("1 second", datetime(2002, 3, 1, 12, 30, 29)),
        ("1 minute", datetime(2002, 3, 1, 12, 29, 30)),
        ("1 hour", datetime(2002, 3, 1, 11, 30, 30)),
        ("1 day", datetime(2002, 2, 28, 12, 30, 30)),
        ("2 days", datetime(2002, 2, 27, 12, 30, 30)),
        ("1 week", datetime(2002, 2, 22, 12, 30, 30)),
        ("2 weeks", datetime(2002, 2, 15, 12, 30, 30)),
        ("1 month", datetime(2002, 2, 1, 12, 30, 30)),
        # More than twelve months has to roll the year back.
        ("13 months", datetime(2001, 2, 1, 12, 30, 30)),
        ("1 year", datetime(2001, 3, 1, 12, 30, 30)),
        ("3 years", datetime(1999, 3, 1, 12, 30, 30)),
    ],
)
def test_decrement_datetime_by_string(decrement, expected) -> None:
    start = datetime(2002, 3, 1, 12, 30, 30)
    assert decrement_datetime_by_string(start, decrement) == expected
