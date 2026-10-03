"""Tests for the R-tree search through the ctypes bindings

The R-tree supports one to four dimensions. Each case below inserts the ten
rectangles [i, i + 1] along every dimension and searches with [2, 7] along
every dimension, so the same ten rectangles and the same expected result apply
to all dimensions. Rectangles that only touch the search rectangle at a
boundary are part of the result, which is why [1, 2] and [7, 8] are included.

The R-tree is a pure in-memory structure, so no GRASS session is needed here.
"""

from ctypes import byref

import pytest

from grass.lib import gis, rtree, vector

# The identifiers the search is expected to return. Identifiers are one-based,
# so the rectangle [i, i + 1] has the identifier i + 1.
EXPECTED_IDS = [2, 3, 4, 5, 6, 7, 8]

# The function setting the bounds of a rectangle, one per dimensionality.
SET_RECT = {
    1: rtree.RTreeSetRect1D,
    2: rtree.RTreeSetRect2D,
    3: rtree.RTreeSetRect3D,
    4: rtree.RTreeSetRect4D,
}


def set_bounds(rect, tree, minimum, maximum) -> None:
    """Set the same bounds along every dimension of the tree"""
    dimensions = tree.contents.ndims
    SET_RECT[dimensions](rect, tree, *([minimum, maximum] * dimensions))


def search(tree, minimum, maximum):
    """Return the identifiers of the rectangles overlapping the search bounds"""
    rect = rtree.RTreeAllocRect(tree)
    set_bounds(rect, tree, minimum, maximum)

    found = gis.ilist()
    number = vector.RTreeSearch2(tree, rect, byref(found))
    rtree.RTreeFreeRect(rect)

    identifiers = sorted(found.value[i] for i in range(found.n_values))
    # The return value counts the rectangles the search put into the list.
    assert number == len(identifiers)
    return identifiers


@pytest.fixture(params=[1, 2, 3, 4], ids=["1d", "2d", "3d", "4d"])
def tree(request):
    """An R-tree of the given dimensionality holding the ten rectangles"""
    tree = rtree.RTreeCreateTree(-1, 0, request.param)
    for i in range(10):
        rect = rtree.RTreeAllocRect(tree)
        set_bounds(rect, tree, float(i), float(i + 1))
        rtree.RTreeInsertRect(rect, i + 1, tree)
    yield tree
    rtree.RTreeDestroyTree(tree)


def test_search_returns_the_overlapping_rectangles(tree) -> None:
    assert search(tree, 2.0, 7.0) == EXPECTED_IDS


def test_search_without_a_match_returns_nothing(tree) -> None:
    """A search rectangle beyond every inserted rectangle finds nothing"""
    assert search(tree, 20.0, 30.0) == []
