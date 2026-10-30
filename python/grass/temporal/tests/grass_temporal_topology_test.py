"""Tests for the temporal topology builder and the map list sorting keys

The relations follow Allen and Ferguson 1994, as cited by
grass.temporal.temporal_extent: an interval starting earlier and ending inside
a later one "overlaps" it, and the later one is "overlapped" by it.

Maps are built in memory only, so no GRASS session and no temporal database
are needed here.
"""

from datetime import datetime

import pytest

from grass.temporal.abstract_dataset import (
    AbstractDatasetComparisonKeyEndTime,
    AbstractDatasetComparisonKeyStartTime,
)
from grass.temporal.space_time_datasets import RasterDataset
from grass.temporal.spatio_temporal_relationships import SpatioTemporalTopologyBuilder


def maps_from(intervals):
    """Return maps for a list of (identifier, start, end) tuples"""
    maps = []
    for ident, start, end in intervals:
        map_ = RasterDataset(ident=ident)
        map_.set_absolute_time(start, end)
        maps.append(map_)
    return maps


@pytest.fixture
def consecutive_maps():
    """Five maps covering consecutive months without gaps"""
    return maps_from(
        [
            ("d_%i@a" % (i + 1), datetime(2001, i + 1, 1), datetime(2001, i + 2, 1))
            for i in range(5)
        ]
    )


@pytest.fixture
def overlapping_maps():
    """Five maps with overlapping, contained and disjoint intervals"""
    return maps_from(
        [
            ("d_1@b", datetime(2001, 1, 14), datetime(2001, 3, 14)),
            ("d_2@b", datetime(2001, 2, 1), datetime(2001, 4, 1)),
            ("d_3@b", datetime(2001, 2, 14), datetime(2001, 4, 30)),
            ("d_4@b", datetime(2001, 4, 2), datetime(2001, 4, 30)),
            ("d_5@b", datetime(2001, 5, 1), datetime(2001, 5, 14)),
        ]
    )


def related_ids(maps, relation):
    """Return the identifiers related to each map by the given relation"""
    return [
        [related.get_id() for related in (getattr(map_, "get_" + relation)() or [])]
        for map_ in maps
    ]


def test_builder_iterates_in_input_order(consecutive_maps) -> None:
    builder = SpatioTemporalTopologyBuilder()
    builder.build(consecutive_maps)

    assert [map_.get_id() for map_ in builder] == [
        map_.get_id() for map_ in consecutive_maps
    ]


def test_consecutive_maps_only_follow_and_precede(consecutive_maps) -> None:
    """Intervals that touch but do not overlap only follow and precede"""
    builder = SpatioTemporalTopologyBuilder()
    builder.build(consecutive_maps)

    assert related_ids(consecutive_maps, "precedes") == [
        ["d_2@a"],
        ["d_3@a"],
        ["d_4@a"],
        ["d_5@a"],
        [],
    ]
    assert related_ids(consecutive_maps, "follows") == [
        [],
        ["d_1@a"],
        ["d_2@a"],
        ["d_3@a"],
        ["d_4@a"],
    ]
    assert related_ids(consecutive_maps, "overlaps") == [[]] * 5


def test_overlaps_points_from_the_earlier_to_the_later_interval(
    overlapping_maps,
) -> None:
    """d_1@b starts before d_2@b and d_3@b and ends inside them, so it overlaps them"""
    builder = SpatioTemporalTopologyBuilder()
    builder.build(overlapping_maps)

    assert related_ids(overlapping_maps, "overlaps") == [
        ["d_2@b", "d_3@b"],
        ["d_3@b"],
        [],
        [],
        [],
    ]
    assert related_ids(overlapping_maps, "overlapped") == [
        [],
        ["d_1@b"],
        ["d_1@b", "d_2@b"],
        [],
        [],
    ]


def test_contains_and_during_are_inverse(overlapping_maps) -> None:
    """d_4@b lies inside d_3@b, so d_3@b contains it and it is during d_3@b"""
    builder = SpatioTemporalTopologyBuilder()
    builder.build(overlapping_maps)

    assert related_ids(overlapping_maps, "contains") == [[], [], ["d_4@b"], [], []]
    assert related_ids(overlapping_maps, "during") == [[], [], [], ["d_3@b"], []]


def test_topology_between_two_map_lists(consecutive_maps, overlapping_maps) -> None:
    """Building with two lists relates the maps of one list to the other"""
    builder = SpatioTemporalTopologyBuilder()
    builder.build(consecutive_maps, overlapping_maps)

    # d_4@a covers April, so it starts inside d_3@b and ends after it.
    assert related_ids(consecutive_maps, "overlapped")[3] == ["d_3@b"]
    assert related_ids(consecutive_maps, "follows")[3] == ["d_2@b"]
    assert related_ids(consecutive_maps, "precedes")[3] == ["d_5@b"]
    assert related_ids(consecutive_maps, "contains")[3] == ["d_4@b"]
    # d_3@a covers March, which lies inside both d_2@b and d_3@b.
    assert related_ids(consecutive_maps, "during")[2] == ["d_2@b", "d_3@b"]


@pytest.fixture
def unsorted_maps():
    """Three maps whose order in the list is not their temporal order"""
    return maps_from(
        [
            ("d_1@a", datetime(2001, 2, 1), datetime(2001, 3, 1)),
            ("d_2@a", datetime(2001, 1, 1), datetime(2001, 2, 1)),
            ("d_3@a", datetime(2001, 3, 1), datetime(2001, 4, 1)),
        ]
    )


@pytest.mark.parametrize(
    "key",
    [AbstractDatasetComparisonKeyStartTime, AbstractDatasetComparisonKeyEndTime],
)
def test_map_list_sorting(unsorted_maps, key) -> None:
    """The maps do not overlap, so start and end time sort them the same way"""
    assert [map_.get_id() for map_ in sorted(unsorted_maps, key=key)] == [
        "d_2@a",
        "d_1@a",
        "d_3@a",
    ]
