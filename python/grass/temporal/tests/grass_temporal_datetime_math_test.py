"""Tests for datetime arithmetic and granularity adjustment

Both functions are pure datetime arithmetic, so no GRASS session and no
temporal database are needed here.
"""

from datetime import datetime

import pytest

from grass.temporal.datetime_math import (
    adjust_datetime_to_granularity,
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
