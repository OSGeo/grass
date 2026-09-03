"""Tests for the basic statistics ctypes bindings

AS_basic_stats() and AS_eqdrt() only read a plain array of doubles and write
their result struct/output parameters, so no GRASS session is needed here.
"""

from ctypes import byref, c_double

import pytest

from grass.lib import arraystats as libas


def make_array(values):
    """Build a ctypes double array from a Python sequence"""
    return (c_double * len(values))(*values)


def basic_stats(values):
    """Call AS_basic_stats(), returning the filled GASTATS struct"""
    stats = libas.GASTATS()
    libas.AS_basic_stats(make_array(values), len(values), byref(stats))
    return stats


def test_basic_stats_on_sorted_data() -> None:
    stats = basic_stats([1.0, 2.0, 3.0, 4.0, 5.0])
    assert stats.min == 1.0
    assert stats.max == 5.0
    assert stats.mean == 3.0
    assert stats.stdev == pytest.approx(1.4142135623730951)


def test_basic_stats_assumes_the_data_is_already_sorted() -> None:
    """min/max read the first and last element rather than scanning

    AS_basic_stats() takes data[0] as the minimum and data[count - 1] as the
    maximum without checking the values in between. Called with unsorted
    data, min and max come out wrong even though sum-based statistics
    (mean, var, stdev) are still correct, since those do not depend on
    order. Callers are expected to sort the data before calling this.
    """
    stats = basic_stats([5.0, 1.0, 3.0, 2.0, 4.0])
    assert stats.min == 5.0
    assert stats.max == 4.0
    assert stats.mean == 3.0


@pytest.mark.parametrize(
    ("x1", "y1", "x2", "y2", "a", "b"),
    [
        # A line through (1, 2) and (3, 6): slope 2, intercept 0.
        (1.0, 2.0, 3.0, 6.0, 0.0, 2.0),
        # A horizontal line at y = 5.
        (0.0, 5.0, 10.0, 5.0, 5.0, 0.0),
    ],
)
def test_eqdrt_computes_the_line_through_two_points(x1, y1, x2, y2, a, b) -> None:
    """AS_eqdrt() fits y = a + b*x through (vectx[i1], vecty[i1]) and
    (vectx[i2], vecty[i2])"""
    vectx = make_array([x1, x2])
    vecty = make_array([y1, y2])
    ra, rb, rc = c_double(), c_double(), c_double()

    libas.AS_eqdrt(vectx, vecty, 1, 0, byref(ra), byref(rb), byref(rc))

    assert ra.value == pytest.approx(a)
    assert rb.value == pytest.approx(b)


def test_eqdrt_returns_a_vertical_line_as_c_instead_of_a_slope() -> None:
    """A vertical line has no slope, so AS_eqdrt() reports x = c instead"""
    vectx = make_array([3.0, 3.0])
    vecty = make_array([0.0, 10.0])
    a, b, c = c_double(), c_double(), c_double()

    libas.AS_eqdrt(vectx, vecty, 1, 0, byref(a), byref(b), byref(c))

    assert a.value == 0.0
    assert b.value == 0.0
    assert c.value == 3.0


def test_eqdrt_treats_index_zero_as_the_origin() -> None:
    """When i1 is 0, AS_eqdrt() forces that point to (0, 0), ignoring
    whatever is actually stored at vectx[0]/vecty[0]

    AS_class_discont(), the one caller of AS_eqdrt() in this codebase,
    relies on this: it passes index 0 as a start-of-range sentinel while
    its own vectx[0]/vecty[0] happen to already be 0.0, so this special
    case is what actually makes index 0 behave as "the origin" here.
    """
    # vectx[0]/vecty[0] are (100, 100), but AS_eqdrt() must ignore them and
    # use (0, 0) instead, giving the same line as through (0, 0) and (3, 6).
    vectx = make_array([100.0, 3.0])
    vecty = make_array([100.0, 6.0])
    a, b, c = c_double(), c_double(), c_double()

    libas.AS_eqdrt(vectx, vecty, 0, 1, byref(a), byref(b), byref(c))

    assert a.value == pytest.approx(0.0)
    assert b.value == pytest.approx(2.0)
