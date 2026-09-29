"""Tests of TemporalAlgebraParser.compare_values.

These make the comparison behavior of the temporal algebra observable, in
particular the ``!=`` operator, which no expression-level test exercises.
"""

import datetime
from types import SimpleNamespace

import pytest

from grass.temporal.temporal_algebra import TemporalAlgebraParser

# compare_values does not use instance state, so build an instance without
# running __init__ (which would open a temporal database connection). Provide a
# stub dbif so __del__ (which checks dbif.connected) stays a no-op.
parser = TemporalAlgebraParser.__new__(TemporalAlgebraParser)
parser.dbif = SimpleNamespace(connected=False)


@pytest.mark.parametrize(
    ("left", "right"),
    [
        (5, 2),  # int
        (2, 2),  # equal ints
        (5.0, 2.5),  # float
        (datetime.date(2001, 1, 1), datetime.date(2000, 1, 1)),  # date
        (datetime.date(2001, 1, 1), datetime.date(2001, 1, 1)),  # equal dates
    ],
)
def test_compare_values_matches_python(left, right):
    """Each operator returns the plain Python comparison result."""
    assert parser.compare_values(left, "<", right) == (left < right)
    assert parser.compare_values(left, ">", right) == (left > right)
    assert parser.compare_values(left, "==", right) == (left == right)
    assert parser.compare_values(left, "!=", right) == (left != right)
    assert parser.compare_values(left, "<=", right) == (left <= right)
    assert parser.compare_values(left, ">=", right) == (left >= right)


def test_compare_values_not_equal():
    """The != branch, otherwise unexercised, behaves as expected."""
    assert parser.compare_values(1, "!=", 2) is True
    assert parser.compare_values(2, "!=", 2) is False


def test_compare_values_keeps_int_type():
    """No float coercion: integer comparisons stay exact."""
    assert parser.compare_values(2001, "==", 2001) is True
    assert parser.compare_values(3, "<", 10) is True


def test_compare_values_unknown_operator():
    with pytest.raises(ValueError, match="Unknown comparison operator"):
        parser.compare_values(1, "=<", 2)
