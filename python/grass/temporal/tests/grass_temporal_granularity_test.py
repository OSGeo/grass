"""Tests for the absolute time granularity computation

The granularity of a map list is the largest unit in which the start and end
times of every map can be expressed. Maps are built in memory only, so no
GRASS session and no temporal database are needed here.
"""

from datetime import datetime

import pytest

from grass.temporal.datetime_math import increment_datetime_by_string
from grass.temporal.space_time_datasets import RasterDataset
from grass.temporal.temporal_granularity import compute_absolute_time_granularity


def interval_maps(start, increment, count):
    """Return count maps covering consecutive intervals of the given increment"""
    maps = []
    for i in range(count):
        map_ = RasterDataset(None)
        map_.set_absolute_time(
            increment_datetime_by_string(start, increment, i),
            increment_datetime_by_string(start, increment, i + 1),
        )
        maps.append(map_)
    return maps


def point_maps(start, increment, count):
    """Return count maps which are time points spaced by the given increment"""
    maps = []
    for i in range(count):
        map_ = RasterDataset(None)
        map_.set_absolute_time(increment_datetime_by_string(start, increment, i), None)
        maps.append(map_)
    return maps


@pytest.mark.parametrize(
    ("start", "increment", "count", "expected"),
    [
        (datetime(2001, 1, 1), "1 year", 10, "1 year"),
        (datetime(2001, 1, 1), "3 years", 10, "3 years"),
        (datetime(2001, 5, 1), "1 month", 20, "1 month"),
        (datetime(2001, 1, 1), "3 months", 20, "3 months"),
        (datetime(2001, 1, 1), "1 day", 6, "1 day"),
        (datetime(2001, 1, 14), "14 days", 6, "14 days"),
        (datetime(2001, 6, 12), "6 hours", 20, "6 hours"),
        (datetime(2001, 1, 1), "20 minutes", 20, "20 minutes"),
        (datetime(2001, 12, 31, 12, 30, 30), "3600 seconds", 24, "3600 seconds"),
        # Mixed units collapse to the finest unit that can express every
        # boundary. Months and days cannot be combined, so days remain.
        (datetime(2001, 3, 1), "1 month, 4 days", 20, "1 day"),
        (datetime(2001, 2, 11), "1 days, 1 hours", 20, "25 hours"),
        (datetime(2001, 1, 1), "5 hours, 25 minutes", 20, "325 minutes"),
        (datetime(2001, 1, 1), "5 minutes, 30 seconds", 20, "330 seconds"),
        (datetime(2001, 12, 31), "60 minutes, 30 seconds", 24, "3630 seconds"),
    ],
)
def test_granularity_of_intervals(start, increment, count, expected) -> None:
    assert (
        compute_absolute_time_granularity(interval_maps(start, increment, count))
        == expected
    )


@pytest.mark.parametrize(
    ("start", "increment", "count", "expected"),
    [
        (datetime(2001, 12, 31, 12, 30, 30), "3600 seconds", 24, "3600 seconds"),
        (datetime(2001, 12, 31), "20 days", 24, "20 days"),
        (datetime(2001, 12, 1), "5 months", 24, "5 months"),
    ],
)
def test_granularity_of_time_points(start, increment, count, expected) -> None:
    assert (
        compute_absolute_time_granularity(point_maps(start, increment, count))
        == expected
    )


@pytest.mark.parametrize(
    ("interval_start", "point_start", "increment", "count", "expected"),
    [
        (
            datetime(2001, 12, 31, 12, 30, 30),
            datetime(2002, 2, 1, 12, 30, 30),
            "3600 seconds",
            24,
            "3600 seconds",
        ),
        (datetime(2001, 1, 1), datetime(2001, 2, 2), "2 days", 8, "2 days"),
    ],
)
def test_granularity_of_intervals_and_time_points(
    interval_start, point_start, increment, count, expected
) -> None:
    """A list may mix intervals and time points and still have a granularity"""
    maps = interval_maps(interval_start, increment, count) + point_maps(
        point_start, increment, count
    )
    assert compute_absolute_time_granularity(maps) == expected
