"""Tests for the 2D and 3D spatial extent relations

Extents are plain value objects, so no GRASS session and no temporal database
are needed here.
"""

import pytest

from grass.temporal.spatial_extent import SpatialExtent

# The extent every case below is compared against.
REFERENCE = {"north": 80, "south": 20, "east": 60, "west": 10, "bottom": -50, "top": 50}


def extent(**kwargs):
    """Return the reference extent with the given bounds overridden

    Called with no arguments it returns the reference extent itself; each
    keyword argument (north, south, east, west, bottom, top) replaces that
    one bound.
    """
    return SpatialExtent(**{**REFERENCE, **kwargs})


@pytest.mark.parametrize(
    "other",
    [
        # The intersection of the reference extent with an extent it fully
        # covers is that extent itself, in 2D and in 3D.
        {},
        {"north": 40, "south": 30},
        {"north": 40, "south": 30, "west": 30},
        {"north": 40, "south": 30, "west": 30, "bottom": -30},
        {"north": 40, "south": 30, "west": 30, "bottom": -30, "top": 30},
    ],
)
def test_intersect_with_covered_extent_returns_that_extent(other) -> None:
    covered = extent(**other)
    result = extent().intersect(covered)

    assert result.get_north() == covered.get_north()
    assert result.get_south() == covered.get_south()
    assert result.get_west() == covered.get_west()
    assert result.get_east() == covered.get_east()
    assert result.get_bottom() == covered.get_bottom()
    assert result.get_top() == covered.get_top()


@pytest.mark.parametrize(
    ("other", "expected_2d", "expected_3d"),
    [
        # Identical extents are equivalent in both 2D and 3D.
        ({}, "equivalent", "equivalent"),
        # Sharing a boundary while being smaller in one direction is "cover".
        ({"north": 70}, "cover", "cover"),
        ({"north": 70, "south": 30}, "cover", "cover"),
        ({"north": 70, "south": 30, "east": 50}, "cover", "cover"),
        # Strictly inside in 2D, but the top and bottom still touch, so the 3D
        # relation stays "cover" rather than becoming "contain".
        ({"north": 70, "south": 30, "east": 50, "west": 20}, "contain", "cover"),
        # Strictly inside in every direction is "contain".
        (
            {
                "north": 70,
                "south": 30,
                "east": 50,
                "west": 20,
                "bottom": -40,
                "top": 40,
            },
            "contain",
            "contain",
        ),
    ],
)
def test_spatial_relation_of_smaller_extent(other, expected_2d, expected_3d) -> None:
    assert extent().spatial_relation_2d(extent(**other)) == expected_2d
    assert extent().spatial_relation(extent(**other)) == expected_3d


@pytest.mark.parametrize(
    ("other", "expected"),
    [
        # The inverse of "cover" is "covered" and the inverse of "contain" is "in".
        ({"north": 70, "south": 30}, "covered"),
        (
            {
                "north": 70,
                "south": 30,
                "east": 50,
                "west": 20,
                "bottom": -40,
                "top": 40,
            },
            "in",
        ),
    ],
)
def test_spatial_relation_is_inverted_when_arguments_swap(other, expected) -> None:
    assert extent(**other).spatial_relation(extent()) == expected


@pytest.mark.parametrize(
    ("other", "expected_2d", "expected_3d"),
    [
        # Reaching past the reference extent in one direction only is an overlap.
        (
            {
                "north": 90,
                "south": 30,
                "east": 50,
                "west": 20,
                "bottom": -40,
                "top": 40,
            },
            "overlap",
            "overlap",
        ),
        # The reference extent lies inside the other one in 2D, but sticks out
        # in the third dimension, so in 3D the two only overlap.
        (
            {"north": 90, "south": 5, "east": 70, "west": 5, "bottom": -40, "top": 40},
            "in",
            "overlap",
        ),
    ],
)
def test_spatial_relation_of_overlapping_extent(
    other, expected_2d, expected_3d
) -> None:
    assert extent().spatial_relation_2d(extent(**other)) == expected_2d
    assert extent().spatial_relation(extent(**other)) == expected_3d


@pytest.mark.parametrize(
    ("one", "other", "expected"),
    [
        # Extents that only touch along a face "meet", never overlap.
        (
            {"north": 80, "south": 60},
            {"north": 60, "south": 20},
            "meet",
        ),
        (
            {"north": 60, "south": 40},
            {"north": 80, "south": 60},
            "meet",
        ),
        (
            {"north": 80, "south": 40, "east": 60, "west": 40},
            {"north": 80, "south": 40, "east": 40, "west": 20},
            "meet",
        ),
        # Touching only along the top and bottom faces also "meets".
        (
            {"east": 60, "west": 20, "north": 80, "south": 40, "bottom": 0, "top": 50},
            {"east": 60, "west": 20, "north": 80, "south": 40, "bottom": -50, "top": 0},
            "meet",
        ),
    ],
)
def test_spatial_relation_of_touching_extents(one, other, expected) -> None:
    assert extent(**one).spatial_relation(extent(**other)) == expected


def test_spatial_relation_of_separate_extents_is_disjoint() -> None:
    one = extent(north=80, south=40, east=40, west=20)
    other = extent(north=40, south=20, east=60, west=40)

    assert one.spatial_relation_2d(other) == "disjoint"
    assert one.spatial_relation(other) == "disjoint"
