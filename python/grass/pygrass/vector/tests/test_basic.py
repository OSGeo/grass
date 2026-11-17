"""
SPDX-FileCopyrightText: 2026 GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later

Unit tests for grass.pygrass.vector.basic (Ilist, BoxList, Bbox)
"""

from unittest.mock import patch

import pytest

import grass.lib.vector as libvect
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
    box1 = Bbox(1, 2, 3, 4, 5, 6)
    box2 = Bbox(1, 2, 3, 4, 5, 6)
    box3 = Bbox(1, 2, 3, 4, 5, 7)
    assert box1 == box2
    assert box1 != box3
    assert box1 != (1, 2, 3, 4, 5, 6)
    assert box1 != "not_a_bbox"

    class EqualToAll:
        def __eq__(self, other):
            return True

    # Bbox returns NotImplemented for other types, so Python tries the
    # other operand.
    assert box1 == EqualToAll()


def test_bbox_subclass_equality():
    """Test that a Bbox subclass compares equal by coordinates."""

    class SubBbox(Bbox):
        pass

    assert SubBbox(1, 2, 3, 4) == Bbox(1, 2, 3, 4)
    assert Bbox(1, 2, 3, 4) == SubBbox(1, 2, 3, 4)


def test_bbox_unhashable():
    """Test that Bbox is unhashable because it is mutable and has __eq__."""
    with pytest.raises(TypeError, match="unhashable"):
        hash(Bbox())


def test_bbox_changes_through_its_list():
    """Test that a Bbox from a BoxList changes when the list changes.

    This is why Bbox cannot have a hash based on its coordinates.
    """
    boxlist = BoxList([Bbox(1, 2, 3, 4)])
    box = boxlist[0]
    boxlist[0] = Bbox(5, 6, 7, 8)
    assert box == Bbox(5, 6, 7, 8)


def test_bbox_nsewtb_is_hashable():
    """Test that nsewtb() can be used where a hashable value is needed."""
    box1 = Bbox(1, 2, 3, 4)
    box2 = Bbox(1, 2, 3, 4)
    assert {box1.nsewtb(), box2.nsewtb()} == {(1, 2, 3, 4, 0, 0)}
    assert {box1.nsewtb(): "a"}[box2.nsewtb()] == "a"


def test_boxlist_init_empty():
    """Test empty BoxList initialization."""
    boxlist = BoxList()
    assert len(boxlist) == 0
    assert list(boxlist) == []
    assert boxlist.have_boxes()


def test_boxlist_init_with_list():
    """Test BoxList initialization with list of Bbox."""
    box0 = Bbox(1, 2, 3, 4)
    box1 = Bbox(5, 6, 7, 8)
    boxlist = BoxList([box0, box1])
    assert len(boxlist) == 2
    assert boxlist[0] == box0
    assert boxlist[1] == box1
    assert boxlist.ids == [0, 1]


def test_boxlist_getitem():
    """Test BoxList.__getitem__ with indexing and slicing."""
    box0 = Bbox(1, 2, 3, 4)
    box1 = Bbox(5, 6, 7, 8)
    box2 = Bbox(9, 10, 11, 12)
    boxlist = BoxList([box0, box1, box2])
    assert boxlist[0] == box0
    assert boxlist[1] == box1
    assert boxlist[-1] == box2
    assert boxlist[-2] == box1
    assert boxlist[0:2] == [box0, box1]

    with pytest.raises(IndexError, match="Index out of range"):
        _ = boxlist[10]
    with pytest.raises(IndexError, match="Index out of range"):
        _ = boxlist[-10]
    with pytest.raises(TypeError, match="Index must be an integer"):
        _ = boxlist["invalid"]
    with pytest.raises(TypeError, match="Index must be an integer"):
        _ = boxlist[1.0]

    empty = BoxList()
    with pytest.raises(IndexError, match="Index out of range"):
        _ = empty[0]


def test_boxlist_numpy_index():
    """Test that numpy integers can be used as indices and ids."""
    np = pytest.importorskip("numpy")
    box0 = Bbox(1, 2, 3, 4)
    box1 = Bbox(5, 6, 7, 8)
    boxlist = BoxList([box0, box1])
    assert boxlist[np.int64(1)] == box1
    assert boxlist[np.int32(-2)] == box0
    boxlist[np.int64(0)] = box1
    assert boxlist[0] == box1
    boxlist.remove(np.int64(0))
    assert boxlist.ids == [1]


def test_boxlist_ids_only():
    """Test that indexing a list without boxes raises instead of crashing."""
    boxlist = BoxList()
    boxlist.c_boxlist.contents.have_boxes = 0
    libvect.Vect_boxlist_append(boxlist.c_boxlist, 1, Bbox().c_bbox)
    with pytest.raises(GrassError, match="only ids"):
        _ = boxlist[0]
    with pytest.raises(GrassError, match="only ids"):
        boxlist[0] = Bbox()


def test_boxlist_setitem():
    """Test BoxList.__setitem__."""
    box0 = Bbox(1, 2, 3, 4)
    box1 = Bbox(5, 6, 7, 8)
    boxlist = BoxList([box0, box1])

    new_box = Bbox(10, 20, 30, 40)
    boxlist[0] = new_box
    assert boxlist[0] == new_box

    neg_box = Bbox(50, 60, 70, 80)
    boxlist[-1] = neg_box
    assert boxlist[1] == neg_box

    # The list holds a copy, so changing the original box does not change it.
    new_box.north = 99
    assert boxlist[0].north == 10

    with pytest.raises(IndexError, match="Index out of range"):
        boxlist[10] = new_box
    with pytest.raises(IndexError, match="Index out of range"):
        boxlist[-10] = new_box
    with pytest.raises(TypeError, match="Expected Bbox instance"):
        boxlist[0] = "not_a_bbox"
    with pytest.raises(TypeError, match="Index must be an integer"):
        boxlist["invalid"] = new_box


def test_boxlist_contains():
    """Test BoxList contains / in operator."""
    box0 = Bbox(1, 2, 3, 4)
    box1 = Bbox(5, 6, 7, 8)
    boxlist = BoxList([box0, box1])
    assert box0 in boxlist
    assert box1 in boxlist
    assert Bbox(9, 9, 9, 9) not in boxlist


def test_boxlist_append():
    """Test appending Bbox to BoxList."""
    boxlist = BoxList()
    box0 = Bbox(1, 2, 3, 4)
    boxlist.append(box0)
    assert len(boxlist) == 1
    assert boxlist[0] == box0

    with pytest.raises(TypeError, match="Expected Bbox instance"):
        boxlist.append("not_a_bbox")


def test_boxlist_append_subclass():
    """Test that instances of Bbox subclasses are accepted."""

    class SubBbox(Bbox):
        pass

    boxlist = BoxList([SubBbox(1, 2, 3, 4)])
    assert boxlist[0] == Bbox(1, 2, 3, 4)


def test_boxlist_append_after_remove():
    """Test that append after remove does not silently drop the box."""
    box3 = Bbox(4, 4, 4, 4)
    boxlist = BoxList([Bbox(1, 1, 1, 1), Bbox(2, 2, 2, 2), Bbox(3, 3, 3, 3)])
    boxlist.remove(0)
    assert boxlist.ids == [1, 2]
    boxlist.append(box3)
    assert len(boxlist) == 3
    assert boxlist.ids == [1, 2, 3]
    assert boxlist[2] == box3


def test_boxlist_append_to_query_ids():
    """Test appending to a list with ids which are not 0..n-1."""
    boxlist = BoxList()
    for box_id in (5, 6, 8, 9):
        libvect.Vect_boxlist_append(boxlist.c_boxlist, box_id, Bbox().c_bbox)
    boxlist.append(Bbox(1, 1, 1, 1))
    # The length, now 5, is taken by a query id, so the largest id + 1 is used.
    boxlist.append(Bbox(2, 2, 2, 2))
    assert boxlist.ids == [5, 6, 8, 9, 4, 10]
    assert boxlist[-2:] == [Bbox(1, 1, 1, 1), Bbox(2, 2, 2, 2)]


def test_boxlist_append_view_of_same_list():
    """Test appending a box from the list itself while the list grows."""
    # 1000 is the allocation step of Vect_boxlist_append, so the next append
    # reallocates the box array.
    boxlist = BoxList([Bbox(i, 0, 0, 0) for i in range(1, 1001)])
    boxlist.append(boxlist[0])
    assert len(boxlist) == 1001
    assert boxlist[1000] == Bbox(1, 0, 0, 0)


def test_boxlist_append_error():
    """Test BoxList.append raises GrassError when the box is not added."""
    boxlist = BoxList()
    box0 = Bbox(1, 2, 3, 4)
    with (
        patch(
            "grass.pygrass.vector.basic.libvect.Vect_boxlist_append",
            return_value=0,
        ),
        pytest.raises(GrassError, match="Cannot append box to list"),
    ):
        boxlist.append(box0)
    assert len(boxlist) == 0


def test_boxlist_extend_iterable():
    """Test extending BoxList with an iterable of Bbox."""
    box0 = Bbox(1, 2, 3, 4)
    box1 = Bbox(5, 6, 7, 8)
    box2 = Bbox(9, 10, 11, 12)
    boxlist = BoxList([box0])
    boxlist.extend([box1])
    assert len(boxlist) == 2
    assert boxlist[1] == box1

    other = BoxList([box2])
    boxlist.extend(list(other))
    assert len(boxlist) == 3
    assert boxlist[2] == box2
    assert boxlist.ids == [0, 1, 2]

    # Nothing is added when any of the values is not a Bbox.
    with pytest.raises(TypeError, match="Expected Bbox instance"):
        boxlist.extend([Bbox(), "not_a_bbox"])
    assert len(boxlist) == 3


def test_boxlist_extend_keeps_ids():
    """Test that extending with a BoxList keeps its ids, e.g., feature ids."""
    box5 = Bbox(5, 5, 5, 5)
    box9 = Bbox(9, 9, 9, 9)
    other = BoxList()
    libvect.Vect_boxlist_append(other.c_boxlist, 5, box5.c_bbox)
    libvect.Vect_boxlist_append(other.c_boxlist, 9, box9.c_bbox)
    boxlist = BoxList([Bbox(1, 1, 1, 1)])
    boxlist.extend(other)
    assert boxlist.ids == [0, 5, 9]
    assert boxlist[1] == box5
    assert boxlist[2] == box9
    # New boxes do not collide with the kept ids.
    boxlist.extend([Bbox(), Bbox()])
    assert boxlist.ids == [0, 5, 9, 10, 11]


def test_boxlist_extend_duplicate_ids():
    """Test that extending with a BoxList with colliding ids raises."""
    boxlist = BoxList([Bbox(1, 1, 1, 1), Bbox(2, 2, 2, 2)])
    other = BoxList([Bbox(3, 3, 3, 3)])
    with pytest.raises(ValueError, match=r"Ids already in the list: 0$"):
        boxlist.extend(other)
    assert len(boxlist) == 2


def test_boxlist_extend_self():
    """Test extending a list with itself while the list grows."""
    boxlist = BoxList([Bbox(i, 0, 0, 0) for i in range(1, 1001)])
    with pytest.raises(
        ValueError, match=r"Ids already in the list: 0, 1, 2, 3, 4, \.\.\.$"
    ):
        boxlist.extend(boxlist)
    assert len(boxlist) == 1000

    boxlist.extend(list(boxlist))
    assert len(boxlist) == 2000
    assert boxlist[1000] == Bbox(1, 0, 0, 0)
    assert boxlist[1999] == Bbox(1000, 0, 0, 0)


def test_boxlist_remove():
    """Test removing Bbox from BoxList by id."""
    box0 = Bbox()
    box1 = Bbox(1, 0, 0, 1)
    box2 = Bbox(1, -1, -1, 1)
    boxlist = BoxList([box0, box1, box2])

    boxlist.remove(0)
    assert boxlist.ids == [1, 2]
    assert list(boxlist) == [box1, box2]

    # Ids are not positions, so removing id 0 again does nothing.
    boxlist.remove(0)
    assert boxlist.ids == [1, 2]

    boxlist.remove([2])
    assert boxlist.ids == [1]
    assert list(boxlist) == [box1]

    with pytest.raises(TypeError, match="is not supported"):
        boxlist.remove(1.5)
    with pytest.raises(TypeError, match="Id must be an integer"):
        boxlist.remove([1.5])
    assert boxlist.ids == [1]


def test_boxlist_remove_boxlist():
    """Test removing the ids of another BoxList."""
    boxlist = BoxList([Bbox(1, 1, 1, 1), Bbox(2, 2, 2, 2), Bbox(3, 3, 3, 3)])
    other = BoxList([Bbox(), Bbox()])
    boxlist.remove(other)
    assert boxlist.ids == [2]
    assert boxlist[0] == Bbox(3, 3, 3, 3)


def test_boxlist_reset():
    """Test resetting BoxList."""
    boxlist = BoxList([Bbox(), Bbox(1, 2, 3, 4)])
    boxlist.reset()
    assert len(boxlist) == 0
    assert list(boxlist) == []


def test_boxlist_repr():
    """Test BoxList __repr__."""
    boxlist = BoxList([Bbox(1, 2, 3, 4)])
    assert repr(boxlist) == "Boxlist([Bbox(1.0, 2.0, 3.0, 4.0)])"
