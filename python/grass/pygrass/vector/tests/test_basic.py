"""
SPDX-FileCopyrightText: 2026 GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later

Unit tests for grass.pygrass.vector.basic.Ilist
"""

from unittest.mock import patch

import pytest

from grass.pygrass.errors import GrassError
from grass.pygrass.vector.basic import Ilist


def test_init_empty():
    """Test empty Ilist initialization."""
    ilist = Ilist()
    assert len(ilist) == 0
    assert list(ilist) == []


def test_init_with_list():
    """Test Ilist initialization with integer list."""
    ilist = Ilist([1, 2, 3])
    assert len(ilist) == 3
    assert list(ilist) == [1, 2, 3]


def test_init_with_ilist():
    """Test Ilist initialization from another Ilist instance."""
    orig = Ilist([1, 2, 3])
    ilist = Ilist(orig)
    assert len(ilist) == 3
    assert list(ilist) == [1, 2, 3]


def test_getitem():
    """Test __getitem__ with indexing and slicing."""
    ilist = Ilist([10, 20, 30, 40])
    assert ilist[0] == 10
    assert ilist[1] == 20
    assert ilist[-1] == 40
    assert ilist[1:3] == [20, 30]
    with pytest.raises(IndexError):
        _ = ilist[10]
    with pytest.raises(IndexError):
        _ = ilist[-10]


def test_setitem():
    """Test __setitem__."""
    ilist = Ilist([1, 2, 3])
    ilist[1] = 20
    assert list(ilist) == [1, 20, 3]
    with pytest.raises(ValueError, match="already in the list"):
        ilist[0] = 3


def test_contains():
    """Test contains and in operator."""
    ilist = Ilist([1, 2, 3])
    assert 1 in ilist
    assert 4 not in ilist
    assert ilist.contains(2)
    assert not ilist.contains(5)


def test_append():
    """Test appending an integer."""
    ilist = Ilist([1, 2])
    ilist.append(3)
    assert list(ilist) == [1, 2, 3]
    # Appending duplicate should not duplicate
    ilist.append(2)
    assert list(ilist) == [1, 2, 3]
    # Appending value convertible to int
    ilist.append(4.0)
    assert list(ilist) == [1, 2, 3, 4]


def test_append_error():
    """Test append raises GrassError when Vect_list_append fails."""
    ilist = Ilist()
    # Vect_list_append only fails on memory allocation failure (or NULL list),
    # which cannot be easily provoked in a unit test, so mock the return code.
    with (
        patch(
            "grass.pygrass.vector.basic.libvect.Vect_list_append",
            return_value=1,
        ),
        pytest.raises(GrassError, match="Cannot append value to list"),
    ):
        ilist.append(10)


def test_extend_with_python_list():
    """Test extending Ilist with Python list."""
    ilist = Ilist([1, 2])
    ilist.extend([3, 4])
    assert list(ilist) == [1, 2, 3, 4]


def test_extend_with_ilist():
    """Test extending Ilist with another Ilist instance."""
    ilist_a = Ilist([1, 2, 3])
    ilist_b = Ilist([4, 5, 6])
    ilist_a.extend(ilist_b)
    assert list(ilist_a) == [1, 2, 3, 4, 5, 6]


def test_remove_int():
    """Test removing an integer."""
    ilist = Ilist([1, 2, 3, 4])
    ilist.remove(2)
    assert list(ilist) == [1, 3, 4]


def test_remove_ilist():
    """Test removing items from another Ilist instance."""
    ilist_a = Ilist([1, 2, 3, 4, 5])
    ilist_b = Ilist([2, 4])
    ilist_a.remove(ilist_b)
    assert list(ilist_a) == [1, 3, 5]


def test_remove_iterable():
    """Test removing items with an iterable."""
    ilist = Ilist([1, 2, 3, 4, 5])
    ilist.remove([1, 3])
    assert list(ilist) == [2, 4, 5]


def test_remove_invalid():
    """Test removing unsupported value type raises ValueError."""
    ilist = Ilist([1, 2])
    # Pass non-iterable types so execution reaches the unsupported type branch
    with pytest.raises(ValueError, match="is not supported"):
        ilist.remove(1.5)
    with pytest.raises(ValueError, match="is not supported"):
        ilist.remove(None)


def test_reset():
    """Test resetting Ilist."""
    ilist = Ilist([1, 2, 3])
    ilist.reset()
    assert len(ilist) == 0
    assert list(ilist) == []


def test_repr():
    """Test __repr__."""
    ilist = Ilist([1, 2, 3])
    assert repr(ilist) == "Ilist([1, 2, 3])"
