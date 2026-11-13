"""Tests for the box list (struct boxlist) ctypes bindings

The Vect_*boxlist*() functions in lib/vector/Vlib/list.c only allocate and
fill the list's own arrays and never read a project, mapset, or GISRC
variable, so no GRASS session is needed here.

These tests call the C functions directly, without grass.pygrass, to pin
down what the C side does on its own. In particular, ids are the key of the
list: appending an id which is already in the list succeeds without adding
anything, and deleting works by id, not by position.
"""

from contextlib import contextmanager
from ctypes import byref

from grass.lib import vector as libvect

# Vect_boxlist_append() grows the arrays by this many items at a time.
ALLOC_STEP = 1000


@contextmanager
def boxlist(have_boxes=1):
    """A list from Vect_new_boxlist(), destroyed afterwards"""
    blist = libvect.Vect_new_boxlist(have_boxes)
    assert blist
    try:
        yield blist
    finally:
        libvect.Vect_destroy_boxlist(blist)


def box(north):
    """A bound_box struct identified by its north, other coordinates are 0"""
    return libvect.bound_box(N=north)


def append(blist, box_id, north):
    """Call Vect_boxlist_append() with a box identified by its north"""
    return libvect.Vect_boxlist_append(blist, box_id, byref(box(north)))


def ids(blist):
    """The ids in the list, in order"""
    return [blist.contents.id[i] for i in range(blist.contents.n_values)]


def norths(blist):
    """The north coordinates of the boxes in the list, in order"""
    return [blist.contents.box[i].N for i in range(blist.contents.n_values)]


def test_new_boxlist_is_empty() -> None:
    with boxlist() as blist:
        assert blist.contents.n_values == 0
        assert blist.contents.alloc_values == 0
        assert blist.contents.have_boxes == 1


def test_new_boxlist_normalizes_have_boxes() -> None:
    with boxlist(have_boxes=5) as blist:
        assert blist.contents.have_boxes == 1
    with boxlist(have_boxes=0) as blist:
        assert blist.contents.have_boxes == 0


def test_append_stores_id_and_box() -> None:
    with boxlist() as blist:
        assert append(blist, 7, 1.0) == 0
        assert append(blist, 3, 2.0) == 0
        assert ids(blist) == [7, 3]
        assert norths(blist) == [1.0, 2.0]


def test_append_copies_the_box() -> None:
    with boxlist() as blist:
        source = box(1.0)
        libvect.Vect_boxlist_append(blist, 0, byref(source))
        source.N = 99.0
        assert norths(blist) == [1.0]


def test_append_skips_an_id_already_in_the_list() -> None:
    """The return value is 0 (success) even though nothing is added"""
    with boxlist() as blist:
        append(blist, 1, 1.0)
        assert append(blist, 1, 2.0) == 0
        assert ids(blist) == [1]
        assert norths(blist) == [1.0]


def test_append_after_delete_with_length_as_id_is_skipped() -> None:
    """Using the list length as the new id collides with an existing id
    once an earlier item is deleted, so the caller must pick unique ids"""
    with boxlist() as blist:
        for i in range(3):
            append(blist, i, float(i))
        libvect.Vect_boxlist_delete(blist, 0)
        assert ids(blist) == [1, 2]

        append(blist, blist.contents.n_values, 3.0)

        assert ids(blist) == [1, 2]


def test_append_grows_past_the_allocation_step() -> None:
    with boxlist() as blist:
        for i in range(ALLOC_STEP + 1):
            append(blist, i, float(i))
        assert blist.contents.n_values == ALLOC_STEP + 1
        assert blist.contents.alloc_values == 2 * ALLOC_STEP
        assert ids(blist) == list(range(ALLOC_STEP + 1))
        assert norths(blist) == [float(i) for i in range(ALLOC_STEP + 1)]


def test_append_to_ids_only_list_does_not_store_boxes() -> None:
    """The box is not read at all, so it may be NULL"""
    with boxlist(have_boxes=0) as blist:
        assert libvect.Vect_boxlist_append(blist, 4, None) == 0
        assert libvect.Vect_boxlist_append(blist, 2, None) == 0
        assert ids(blist) == [4, 2]
        assert not blist.contents.box


def test_append_boxlist_keeps_ids() -> None:
    with boxlist() as alist, boxlist() as blist:
        append(alist, 0, 0.0)
        append(blist, 5, 5.0)
        append(blist, 9, 9.0)

        assert libvect.Vect_boxlist_append_boxlist(alist, blist) == 0

        assert ids(alist) == [0, 5, 9]
        assert norths(alist) == [0.0, 5.0, 9.0]


def test_append_boxlist_skips_ids_already_in_the_list() -> None:
    """Two lists with the same ids, e.g., both numbered 0..n-1, add nothing"""
    with boxlist() as alist, boxlist() as blist:
        for i in range(2):
            append(alist, i, float(i))
            append(blist, i, float(i + 10))

        assert libvect.Vect_boxlist_append_boxlist(alist, blist) == 0

        assert ids(alist) == [0, 1]
        assert norths(alist) == [0.0, 1.0]


def test_append_boxlist_from_ids_only_list_adds_empty_boxes() -> None:
    with boxlist() as alist, boxlist(have_boxes=0) as blist:
        libvect.Vect_boxlist_append(blist, 3, None)

        libvect.Vect_boxlist_append_boxlist(alist, blist)

        assert ids(alist) == [3]
        assert norths(alist) == [0.0]


def test_delete_works_by_id_not_position() -> None:
    with boxlist() as blist:
        for box_id in (10, 20, 30):
            append(blist, box_id, float(box_id))

        assert libvect.Vect_boxlist_delete(blist, 20) == 0

        assert ids(blist) == [10, 30]
        assert norths(blist) == [10.0, 30.0]


def test_delete_of_missing_id_does_nothing() -> None:
    with boxlist() as blist:
        append(blist, 1, 1.0)
        assert libvect.Vect_boxlist_delete(blist, 0) == 0
        assert ids(blist) == [1]


def test_delete_boxlist_removes_ids_of_other_list() -> None:
    """Only the ids of the other list matter, not its boxes"""
    with boxlist() as alist, boxlist(have_boxes=0) as blist:
        for box_id in range(4):
            append(alist, box_id, float(box_id))
        libvect.Vect_boxlist_append(blist, 1, None)
        libvect.Vect_boxlist_append(blist, 3, None)
        libvect.Vect_boxlist_append(blist, 7, None)

        assert libvect.Vect_boxlist_delete_boxlist(alist, blist) == 0

        assert ids(alist) == [0, 2]
        assert norths(alist) == [0.0, 2.0]


def test_val_in_boxlist() -> None:
    with boxlist() as blist:
        append(blist, 5, 5.0)
        assert libvect.Vect_val_in_boxlist(blist, 5) == 1
        assert libvect.Vect_val_in_boxlist(blist, 0) == 0


def test_reset_keeps_the_allocation() -> None:
    with boxlist() as blist:
        append(blist, 0, 0.0)
        alloc_values = blist.contents.alloc_values

        assert libvect.Vect_reset_boxlist(blist) == 0

        assert blist.contents.n_values == 0
        assert blist.contents.alloc_values == alloc_values
        append(blist, 0, 1.0)
        assert ids(blist) == [0]
        assert norths(blist) == [1.0]


def test_null_list_is_an_error() -> None:
    with boxlist() as blist:
        assert libvect.Vect_boxlist_append(None, 0, byref(box(0))) == 1
        assert libvect.Vect_boxlist_append_boxlist(None, blist) == 1
        assert libvect.Vect_boxlist_append_boxlist(blist, None) == 1
        assert libvect.Vect_boxlist_delete(None, 0) == 1
        assert libvect.Vect_boxlist_delete_boxlist(None, blist) == 1
        assert libvect.Vect_boxlist_delete_boxlist(blist, None) == 1
        assert libvect.Vect_val_in_boxlist(None, 0) == 0
