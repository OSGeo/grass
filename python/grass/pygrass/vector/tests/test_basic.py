"""
SPDX-FileCopyrightText: 2026 GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later

Unit tests for grass.pygrass.vector.basic (Ilist, BoxList, Bbox)
"""

from unittest.mock import patch

import pytest

from grass.pygrass.errors import GrassError
from grass.pygrass.vector.basic import Bbox, BoxList, Ilist


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


def test_bbox_equality():
    """Test Bbox equality comparisons."""
    b1 = Bbox(1, 2, 3, 4, 5, 6)
    b2 = Bbox(1, 2, 3, 4, 5, 6)
    b3 = Bbox(1, 2, 3, 4, 5, 7)
    assert b1 == b2
    assert b1 != b3
    assert b1 != (1, 2, 3, 4, 5, 6)
    assert b1 != "not_a_bbox"


def test_boxlist_init_empty():
    """Test empty BoxList initialization."""
    bl = BoxList()
    assert len(bl) == 0
    assert list(bl) == []
    assert bl.have_boxes()


def test_boxlist_init_with_list():
    """Test BoxList initialization with list of Bbox."""
    b0 = Bbox(1, 2, 3, 4)
    b1 = Bbox(5, 6, 7, 8)
    bl = BoxList([b0, b1])
    assert len(bl) == 2
    assert bl[0] == b0
    assert bl[1] == b1


def test_boxlist_getitem():
    """Test BoxList.__getitem__ with indexing and slicing."""
    b0 = Bbox(1, 2, 3, 4)
    b1 = Bbox(5, 6, 7, 8)
    b2 = Bbox(9, 10, 11, 12)
    bl = BoxList([b0, b1, b2])
    assert bl[0] == b0
    assert bl[1] == b1
    assert bl[-1] == b2
    assert bl[-2] == b1
    assert bl[0:2] == [b0, b1]

    with pytest.raises(IndexError, match="Index out of range"):
        _ = bl[10]
    with pytest.raises(IndexError, match="Index out of range"):
        _ = bl[-10]
    with pytest.raises(TypeError, match="Index must be an integer or slice"):
        _ = bl["invalid"]

    empty = BoxList()
    with pytest.raises(IndexError, match="Index out of range"):
        _ = empty[0]


def test_boxlist_setitem():
    """Test BoxList.__setitem__."""
    b0 = Bbox(1, 2, 3, 4)
    b1 = Bbox(5, 6, 7, 8)
    bl = BoxList([b0, b1])

    new_box = Bbox(10, 20, 30, 40)
    bl[0] = new_box
    assert bl[0] == new_box

    neg_box = Bbox(50, 60, 70, 80)
    bl[-1] = neg_box
    assert bl[1] == neg_box

    with pytest.raises(IndexError, match="Index out of range"):
        bl[10] = new_box
    with pytest.raises(IndexError, match="Index out of range"):
        bl[-10] = new_box
    with pytest.raises(TypeError, match="Expected Bbox instance"):
        bl[0] = "not_a_bbox"
    with pytest.raises(TypeError, match="Index must be an integer"):
        bl["invalid"] = new_box


def test_boxlist_contains():
    """Test BoxList contains / in operator."""
    b0 = Bbox(1, 2, 3, 4)
    b1 = Bbox(5, 6, 7, 8)
    bl = BoxList([b0, b1])
    assert b0 in bl
    assert b1 in bl
    assert Bbox(9, 9, 9, 9) not in bl


def test_boxlist_append():
    """Test appending Bbox to BoxList."""
    bl = BoxList()
    b0 = Bbox(1, 2, 3, 4)
    bl.append(b0)
    assert len(bl) == 1
    assert bl[0] == b0

    with pytest.raises(TypeError, match="Expected Bbox instance"):
        bl.append("not_a_bbox")


def test_boxlist_append_error():
    """Test BoxList.append raises GrassError on failure."""
    bl = BoxList()
    b0 = Bbox(1, 2, 3, 4)
    with (
        patch(
            "grass.pygrass.vector.basic.libvect.Vect_boxlist_append",
            return_value=1,
        ),
        pytest.raises(GrassError, match="Cannot append box to list"),
    ):
        bl.append(b0)


def test_boxlist_extend():
    """Test extending BoxList."""
    b0 = Bbox(1, 2, 3, 4)
    b1 = Bbox(5, 6, 7, 8)
    bl = BoxList([b0])
    bl.extend([b1])
    assert len(bl) == 2
    assert bl[1] == b1

    b2 = Bbox(9, 10, 11, 12)
    bl2 = BoxList([b2])
    bl.extend(bl2)
    assert len(bl) == 3
    assert bl[2] == b2


def test_boxlist_remove():
    """Test removing Bbox from BoxList."""
    b0 = Bbox()
    b1 = Bbox(1, 0, 0, 1)
    b2 = Bbox(1, -1, -1, 1)
    bl = BoxList([b0, b1, b2])

    bl.remove(0)
    assert len(bl) == 2

    bl.remove([1])
    assert len(bl) == 1

    bl_other = BoxList([Bbox(1, -1, -1, 1)])
    bl.remove(bl_other)

    with pytest.raises(TypeError, match="is not supported"):
        bl.remove(1.5)


def test_boxlist_reset():
    """Test resetting BoxList."""
    bl = BoxList([Bbox(), Bbox(1, 2, 3, 4)])
    bl.reset()
    assert len(bl) == 0
    assert list(bl) == []


def test_boxlist_repr():
    """Test BoxList __repr__."""
    bl = BoxList([Bbox(1, 2, 3, 4)])
    assert repr(bl) == "Boxlist([Bbox(1.0, 2.0, 3.0, 4.0)])"
