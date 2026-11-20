"""Regression tests for negative month offsets crossing a year boundary."""

from datetime import datetime

import pytest

from grass.temporal.datetime_math import decrement_datetime_by_string, modify_datetime


@pytest.mark.parametrize(
    ("start", "months", "expected"),
    [
        (datetime(2001, 1, 1), 13, datetime(1999, 12, 1)),
        (datetime(2001, 1, 1), 25, datetime(1998, 12, 1)),
        (datetime(2001, 2, 1), 14, datetime(1999, 12, 1)),
        (datetime(2001, 12, 1), 24, datetime(1999, 12, 1)),
        (datetime(2001, 1, 1), 1, datetime(2000, 12, 1)),
        (datetime(2001, 1, 1), 12, datetime(2000, 1, 1)),
        (datetime(2001, 3, 15, 12, 30), 15, datetime(1999, 12, 15, 12, 30)),
    ],
)
def test_decrement_months_across_year_boundary(start, months, expected):
    assert modify_datetime(start, months=-months) == expected
    assert decrement_datetime_by_string(start, f"{months} months") == expected


@pytest.mark.parametrize(
    ("start", "months"),
    [(datetime(2001, 3, 31), -1), (datetime(2004, 2, 29), -12)],
)
def test_decrement_months_preserves_invalid_day_error(start, months):
    with pytest.raises(ValueError, match="day is out of range for month"):
        modify_datetime(start, months=months)
