"""
Created on Tue Jul 31 13:06:20 2012

@author: pietro
"""

import ctypes
import operator
from collections.abc import Iterable

import grass.lib.vector as libvect
from grass.pygrass.errors import GrassError
from grass.pygrass.shell.conversion import dict2html


class Bbox:
    """Instantiate a Bounding Box class that contains
    a ctypes pointer to the C struct bound_box, that could be used
    by C GRASS functions.

    >>> bbox = Bbox()
    >>> bbox
    Bbox(0.0, 0.0, 0.0, 0.0)

    The default parameters are 0. It is possible to set or change
    the parameters later, with:

    >>> bbox.north = 10
    >>> bbox.south = -10
    >>> bbox.east = -20
    >>> bbox.west = 20
    >>> bbox
    Bbox(10.0, -10.0, -20.0, 20.0)

    Or directly instantiate the class with the values, with:

    >>> bbox = Bbox(north=100, south=0, east=0, west=100)
    >>> bbox
    Bbox(100.0, 0.0, 0.0, 100.0)

    Two Bbox objects are equal when their coordinates are equal. Because the
    coordinates can be changed, Bbox objects are not hashable, so they cannot
    be put in a set or used as dictionary keys. Use the tuple returned by
    nsewtb() instead:

    >>> Bbox(1, 2, 3, 4) == Bbox(1, 2, 3, 4)
    True
    >>> {Bbox(1, 2, 3, 4).nsewtb(), Bbox(1, 2, 3, 4).nsewtb()}
    {(1.0, 2.0, 3.0, 4.0, 0.0, 0.0)}

    ..
    """

    def __init__(self, north=0, south=0, east=0, west=0, top=0, bottom=0):
        self.c_bbox = ctypes.pointer(libvect.bound_box())
        self.north = north
        self.south = south
        self.east = east
        self.west = west
        self.top = top
        self.bottom = bottom

    def _get_n(self):
        """Private method to obtain the north value"""
        return self.c_bbox.contents.N

    def _set_n(self, value):
        """Private method to set the north value"""
        self.c_bbox.contents.N = value

    north = property(fget=_get_n, fset=_set_n, doc="Set and obtain north value")

    def _get_s(self):
        """Private method to obtain the south value"""
        return self.c_bbox.contents.S

    def _set_s(self, value):
        """Private method to set the south value"""
        self.c_bbox.contents.S = value

    south = property(fget=_get_s, fset=_set_s, doc="Set and obtain south value")

    def _get_e(self):
        """Private method to obtain the east value"""
        return self.c_bbox.contents.E

    def _set_e(self, value):
        """Private method to set the east value"""
        self.c_bbox.contents.E = value

    east = property(fget=_get_e, fset=_set_e, doc="Set and obtain east value")

    def _get_w(self):
        """Private method to obtain the west value"""
        return self.c_bbox.contents.W

    def _set_w(self, value):
        """Private method to set the west value"""
        self.c_bbox.contents.W = value

    west = property(fget=_get_w, fset=_set_w, doc="Set and obtain west value")

    def _get_t(self):
        """Private method to obtain the top value"""
        return self.c_bbox.contents.T

    def _set_t(self, value):
        """Private method to set the top value"""
        self.c_bbox.contents.T = value

    top = property(fget=_get_t, fset=_set_t, doc="Set and obtain top value")

    def _get_b(self):
        """Private method to obtain the bottom value"""
        return self.c_bbox.contents.B

    def _set_b(self, value):
        """Private method to set the bottom value"""
        self.c_bbox.contents.B = value

    bottom = property(fget=_get_b, fset=_set_b, doc="Set and obtain bottom value")

    def __repr__(self):
        return "Bbox({n}, {s}, {e}, {w})".format(
            n=self.north, s=self.south, e=self.east, w=self.west
        )

    def _repr_html_(self):
        return dict2html(dict(self.items()), keys=self.keys(), border="1", kdec="b")

    def keys(self):
        return ["north", "south", "west", "east", "top", "bottom"]

    def contains(self, point):
        """Return True if the object is contained by the BoundingBox

        :param point: the point to analyze
        :type point: a Point object or a tuple with the coordinates

        >>> from grass.pygrass.vector.geometry import Point
        >>> poi = Point(5, 5)
        >>> bbox = Bbox(north=10, south=0, west=0, east=10)
        >>> bbox.contains(poi)
        True
        >>> bbox.contains((5, 5))
        True
        >>> bbox.contains((5, 5, 0))
        True

        """
        if hasattr(point, "x") and hasattr(point, "y"):
            coords = (point.x, point.y, getattr(point, "z", None) or 0.0)
        elif isinstance(point, (tuple, list)) and len(point) in {2, 3}:
            coords = (*point, 0.0) if len(point) == 2 else tuple(point)
        else:
            str_error = "The format of the point is not supported: {0!r}"
            raise ValueError(str_error.format(point))
        # Convert here so that non-numeric input fails with a clear error
        # instead of a ctypes error or garbage reaching the C function.
        try:
            x, y, z = (float(coord) for coord in coords)
        except (TypeError, ValueError) as error:
            str_error = "The coordinates of the point must be numbers: {0!r}"
            raise ValueError(str_error.format(point)) from error
        return bool(libvect.Vect_point_in_box(x, y, z, self.c_bbox))

    def items(self):
        return [(k, getattr(self, k)) for k in self.keys()]

    def nsewtb(self, tb=True):
        """Return a list of values from bounding box

        :param tb: if tb parameter is False return only: north, south, east,
                   west and not top and bottom
        :type tb: bool

        """
        if tb:
            return (self.north, self.south, self.east, self.west, self.top, self.bottom)
        return (self.north, self.south, self.east, self.west)

    def __eq__(self, other):
        if not isinstance(other, Bbox):
            return NotImplemented
        return self.nsewtb() == other.nsewtb()

    # Equality is based on the coordinates, which are mutable, so a hash
    # based on them would change while the object is in a set or a dict.
    __hash__ = None


class BoxList:
    """Instantiate a BoxList class to create a list of Bounding Box"""

    def __init__(self, boxlist=None):
        self.c_boxlist = ctypes.pointer(libvect.boxlist())
        # if set to 0, the list will hold only ids and no boxes
        self.c_boxlist.contents.have_boxes = 1
        if boxlist is not None:
            self._extend_with_new_ids(boxlist)

    @property
    def ids(self):
        return [self.c_boxlist.contents.id[i] for i in range(self.n_values)]

    @property
    def n_values(self):
        return self.c_boxlist.contents.n_values

    def have_boxes(self):
        return bool(self.c_boxlist.contents.have_boxes)

    def __len__(self):
        return self.c_boxlist.contents.n_values

    def __repr__(self):
        return "Boxlist([%s])" % ", ".join([repr(box) for box in self.__iter__()])

    def _box_index(self, indx):
        """Return a valid non-negative position in the C box array.

        The box array is a raw C pointer, so indexing it is not checked by
        ctypes and must not go out of bounds.
        """
        try:
            indx = operator.index(indx)
        except TypeError:
            msg = f"Index must be an integer, got {type(indx).__name__}"
            raise TypeError(msg) from None
        if not self.have_boxes():
            msg = "BoxList holds only ids and no boxes"
            raise GrassError(msg)
        if indx < 0:
            indx += len(self)
        if indx >= len(self) or indx < 0:
            msg = "Index out of range"
            raise IndexError(msg)
        return indx

    def __getitem__(self, indx):
        """Return a Bbox which is a view into the list, not a copy.

        The view is valid only until the list grows, because growing the
        list may move the boxes to a new memory location.
        """
        if isinstance(indx, slice):
            return [self[i] for i in range(*indx.indices(len(self)))]
        indx = self._box_index(indx)
        bbox = Bbox()
        bbox.c_bbox = ctypes.pointer(self.c_boxlist.contents.box[indx])
        return bbox

    def __setitem__(self, indx, bbox):
        indx = self._box_index(indx)
        if not isinstance(bbox, Bbox):
            msg = f"Expected Bbox instance, got {type(bbox).__name__}"
            raise TypeError(msg)
        # Assigning the struct copies its values into the C array.
        self.c_boxlist.contents.box[indx] = bbox.c_bbox.contents

    def __iter__(self):
        return (self.__getitem__(box_id) for box_id in range(self.__len__()))

    def __str__(self):
        return self.__repr__()

    def append(self, box):
        """Append a Bbox object to a Boxlist object, using the
        ``Vect_boxlist_append`` C function.

        The box gets the length of the list as its id. If that id is
        already in the list, e.g., after remove() or with feature ids from
        a spatial query, it gets an id one larger than the largest id.

        :param box: the bounding box to add to the list
        :type box: a Bbox object

        >>> box0 = Bbox()
        >>> box1 = Bbox(1, 2, 3, 4)
        >>> box2 = Bbox(5, 6, 7, 8)
        >>> boxlist = BoxList([box0, box1])
        >>> boxlist
        Boxlist([Bbox(0.0, 0.0, 0.0, 0.0), Bbox(1.0, 2.0, 3.0, 4.0)])
        >>> len(boxlist)
        2
        >>> boxlist.append(box2)
        >>> len(boxlist)
        3

        """
        c_box = self._copy_c_box(box)
        # Try the length first because finding the largest id is a slow
        # pass over all ids in Python.
        if self._append_c_box(len(self), c_box):
            return
        if not self._append_c_box(self._next_id(), c_box):
            msg = "Cannot append box to list"
            raise GrassError(msg)

    def _next_id(self):
        """Return an id which is not yet in the list."""
        return max(self.ids, default=-1) + 1

    @staticmethod
    def _copy_c_box(box):
        """Return a copy of the C struct of a Bbox.

        The box may be a view into a BoxList (from boxlist[i]), which
        becomes invalid when that list grows, so it is copied before
        appending to any list.
        """
        if not isinstance(box, Bbox):
            msg = f"Expected Bbox instance, got {type(box).__name__}"
            raise TypeError(msg)
        return libvect.bound_box.from_buffer_copy(box.c_bbox.contents)

    def _append_c_box(self, box_id, c_box):
        """Append a C box, return False if the id is already in the list"""
        n_before = len(self)
        if libvect.Vect_boxlist_append(self.c_boxlist, box_id, ctypes.pointer(c_box)):
            msg = "Cannot append box to list"
            raise GrassError(msg)
        return len(self) == n_before + 1

    def extend(self, boxlist):
        """Extend a boxlist with another BoxList or iterable of Bbox objects.

        Boxes from another BoxList keep their ids, so ids of features from
        a spatial query still point to the right features. If any of these
        ids is already in this list, ValueError is raised and nothing is
        added. Boxes from any other iterable get new ids as in append().
        Use, e.g., ``list(other)`` to add boxes from a BoxList with new ids.

        >>> box0 = Bbox()
        >>> box1 = Bbox(1, 2, 3, 4)
        >>> box2 = Bbox(5, 6, 7, 8)
        >>> box3 = Bbox(9, 8, 7, 6)
        >>> boxlist0 = BoxList([box0, box1])
        >>> boxlist0
        Boxlist([Bbox(0.0, 0.0, 0.0, 0.0), Bbox(1.0, 2.0, 3.0, 4.0)])
        >>> boxlist1 = BoxList([box2, box3])
        >>> len(boxlist0)
        2
        >>> boxlist0.extend(list(boxlist1))
        >>> len(boxlist0)
        4
        >>> boxlist0.ids
        [0, 1, 2, 3]

        """
        if isinstance(boxlist, BoxList):
            duplicates = sorted(set(self.ids) & set(boxlist.ids))
            if duplicates:
                shown = ", ".join(str(i) for i in duplicates[:5])
                if len(duplicates) > 5:
                    shown += ", ..."
                msg = f"Ids already in the list: {shown}"
                raise ValueError(msg)
            n_expected = len(self) + len(boxlist)
            if (
                libvect.Vect_boxlist_append_boxlist(self.c_boxlist, boxlist.c_boxlist)
                or len(self) != n_expected
            ):
                msg = "Cannot extend list"
                raise GrassError(msg)
        else:
            self._extend_with_new_ids(boxlist)

    def _extend_with_new_ids(self, boxes):
        # Copy all boxes before appending any, because they may be views
        # into this list which the first append can invalidate.
        c_boxes = [self._copy_c_box(box) for box in boxes]
        first_id = self._next_id()
        for offset, c_box in enumerate(c_boxes):
            if not self._append_c_box(first_id + offset, c_box):
                msg = "Cannot append box to list"
                raise GrassError(msg)

    def remove(self, indx):
        """Remove Bbox from the boxlist, given an id or a list of ids
        or a boxlist, using ``Vect_boxlist_delete`` C function or the
        ``Vect_boxlist_delete_boxlist``.

        The boxes are removed by their id, not by their position in the
        list. Ids which are not in the list are ignored.

        :param indx: the id of the Bbox to remove, an iterable of ids, or
                     a BoxList with the ids to remove
        :type indx: int, an iterable of int, or a BoxList

        >>> boxlist = BoxList([Bbox(), Bbox(1, 0, 0, 1), Bbox(1, -1, -1, 1)])
        >>> boxlist.remove(0)
        >>> boxlist
        Boxlist([Bbox(1.0, 0.0, 0.0, 1.0), Bbox(1.0, -1.0, -1.0, 1.0)])
        >>> boxlist.ids
        [1, 2]

        """
        if isinstance(indx, BoxList):
            libvect.Vect_boxlist_delete_boxlist(self.c_boxlist, indx.c_boxlist)
            return
        try:
            ids = [operator.index(indx)]
        except TypeError:
            if not isinstance(indx, Iterable):
                msg = f"Value {indx!r} of type {type(indx).__name__} is not supported"
                raise TypeError(msg) from None
            ids = []
            for ind in indx:
                try:
                    ids.append(operator.index(ind))
                except TypeError:
                    msg = f"Id must be an integer, got {type(ind).__name__}"
                    raise TypeError(msg) from None
        for box_id in ids:
            libvect.Vect_boxlist_delete(self.c_boxlist, box_id)

    def reset(self):
        """Reset the c_boxlist C struct, using the ``Vect_reset_boxlist`` C
        function.

        >>> boxlist = BoxList([Bbox(), Bbox(1, 0, 0, 1), Bbox(1, -1, -1, 1)])
        >>> len(boxlist)
        3
        >>> boxlist.reset()
        >>> len(boxlist)
        0

        """
        libvect.Vect_reset_boxlist(self.c_boxlist)


class Ilist:
    """Instantiate a list of integer using the C GRASS struct ``ilist``,
    the class contains this struct as ``c_ilist`` attribute."""

    def __init__(self, integer_list=None):
        self.c_ilist = ctypes.pointer(libvect.struct_ilist())
        if integer_list is not None:
            self.extend(integer_list)

    def __getitem__(self, key):
        if isinstance(key, slice):
            # Get the start, stop, and step from the slice
            return [
                self.c_ilist.contents.value[indx]
                for indx in range(*key.indices(len(self)))
            ]
        if isinstance(key, int):
            if key < 0:  # Handle negative indices
                key += self.c_ilist.contents.n_values
            if key >= self.c_ilist.contents.n_values or key < 0:
                msg = "Index out of range"
                raise IndexError(msg)
            return self.c_ilist.contents.value[key]
        raise ValueError("Invalid argument type: %r." % key)

    def __setitem__(self, key, value):
        if self.contains(value):
            msg = "Integer already in the list"
            raise ValueError(msg)
        self.c_ilist.contents.value[key] = int(value)

    def __len__(self):
        return self.c_ilist.contents.n_values

    def __iter__(self):
        return (self.c_ilist.contents.value[i] for i in range(self.__len__()))

    def __repr__(self):
        return "Ilist(%r)" % list(self.__iter__())

    def __contains__(self, item):
        return self.contains(item)

    def append(self, value):
        """Append an integer to the list"""
        if libvect.Vect_list_append(self.c_ilist, int(value)):
            msg = "Cannot append value to list"
            raise GrassError(msg)

    def reset(self):
        """Reset the list"""
        libvect.Vect_reset_list(self.c_ilist)

    def extend(self, ilist):
        """Extend the list with another Ilist object or
        with a list of integers

        :param ilist: the ilist to append
        :type ilist: a Ilist object
        """
        if isinstance(ilist, Ilist):
            libvect.Vect_list_append_list(self.c_ilist, ilist.c_ilist)
        else:
            for i in ilist:
                self.append(i)

    def remove(self, value):
        """Remove a value from a list"""
        if isinstance(value, int):
            libvect.Vect_list_delete(self.c_ilist, value)
        elif isinstance(value, Ilist):
            libvect.Vect_list_delete_list(self.c_ilist, value.c_ilist)
        elif isinstance(value, Iterable):
            for i in value:
                libvect.Vect_list_delete(self.c_ilist, int(i))
        else:
            raise ValueError("Value: %r, is not supported" % value)

    def contains(self, value):
        """Check if value is in the list"""
        return bool(libvect.Vect_val_in_list(self.c_ilist, value))


class Cats:
    """Instantiate a Category class that contains a ctypes pointer
    to the C line_cats struct.

    >>> cats = Cats()
    >>> for cat in range(100, 110):
    ...     cats.set(cat, layer=cat - 50)
    >>> cats.n_cats
    10
    >>> cats.cat
    [100, 101, 102, 103, 104, 105, 106, 107, 108, 109]
    >>> cats.layer
    [50, 51, 52, 53, 54, 55, 56, 57, 58, 59]
    >>> cats.get()  # default layer is 1
    (-1, 0)
    >>> cats.get(50)
    (100, 1)
    >>> cats.get(51)
    (101, 1)
    >>> cats.set(1001, 52)
    >>> cats.cat
    [100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 1001]
    >>> cats.layer
    [50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 52]
    >>> cats.get(52)
    (102, 2)
    >>> cats.reset()
    >>> cats.layer
    []
    >>> cats.cat
    []

    """

    @property
    def layer(self):
        field = self.c_cats.contents.field
        return [field[i] for i in range(self.n_cats)]

    @property
    def cat(self):
        cat = self.c_cats.contents.cat
        return [cat[i] for i in range(self.n_cats)]

    @property
    def n_cats(self):
        """Return the number of categories"""
        return self.c_cats.contents.n_cats

    def __init__(self, c_cats=None):
        self.c_cats = c_cats or ctypes.pointer(libvect.line_cats())

    def reset(self):
        """Reset the C cats struct from previous values."""
        libvect.Vect_reset_cats(self.c_cats)

    def get(self, layer=1):
        """Return the first found category of given layer
        and the number of category found.

        :param layer: the number of layer
        :type layer: int
        """
        cat = ctypes.c_int()
        n_cats = libvect.Vect_cat_get(self.c_cats, layer, ctypes.byref(cat))
        return cat.value, n_cats

    def set(self, cat, layer=1):
        """Add new field/cat to category structure if doesn't exist yet.

        :param cat: the cat to add
        :type cat: int
        :param layer: the number of layer
        :type layer: int
        """
        libvect.Vect_cat_set(self.c_cats, layer, cat)

    def delete(self, cat=None, layer=1):
        """If cat is given delete cat from line_cats structure
        (using Vect_field_cat_del) else delete all categories of given layer
        (using Vect_cat_del).

        :param cat: the cat to add
        :type cat: int
        :param layer: the number of layer
        :type layer: int
        """
        if cat:
            self.n_del = libvect.Vect_field_cat_del(self.c_cats, layer, cat)
            err_msg = "Layer(%d)/category(%d) number does not exist" % (layer, cat)
        else:
            self.n_del = libvect.Vect_cat_del(self.c_cats, layer)
            err_msg = "Layer: %r does not exist" % layer
        if self.n_del == 0:
            raise ValueError(err_msg)

    def check_cats_constraints(self, cats_list, layer=1):
        """Check if categories match with category constraints

        :param cats_list: a list of categories
        :type cats_list: list
        :param layer: the number of layer
        :type layer: int
        """
        return bool(
            libvect.Vect_cats_in_constraint(self.c_cats, layer, cats_list.c_cat_list)
        )

    def get_list(self, layer=1):
        """Get list of categories of given field.

        :param layer: the number of layer
        :type layer: int
        """
        ilist = Ilist()
        if libvect.Vect_field_cat_get(self.c_cats, layer, ilist.c_ilist) < 0:
            raise ValueError("Layer: %r does not exist" % layer)
        return ilist


class CatsList:
    """

    >>> cats_list = CatsList()
    >>> cats_list.min
    []
    >>> cats_list.max
    []
    >>> cats_list.n_ranges
    0
    >>> cats_list.layer
    0
    >>> string = "2,3,5-9,20"
    >>> cats_list.from_string(string)
    >>> cats_list.min
    [2, 3, 5, 20]
    >>> cats_list.max
    [2, 3, 9, 20]
    >>> cats_list.n_ranges
    4

    """

    @property
    def layer(self):
        """Return the layer number"""
        return self.c_cat_list.contents.field

    @property
    def n_ranges(self):
        """Return the ranges number"""
        return self.c_cat_list.contents.n_ranges

    @property
    def min(self):
        """Return the minimum value"""
        min_values = self.c_cat_list.contents.min
        return [min_values[i] for i in range(self.n_ranges)]

    @property
    def max(self):
        """Return the maximum value"""
        max_values = self.c_cat_list.contents.max
        return [max_values[i] for i in range(self.n_ranges)]

    def __init__(self, c_cat_list=None):
        self.c_cat_list = c_cat_list or ctypes.pointer(libvect.cat_list())

    def from_string(self, string):
        """Converts string of categories and cat ranges separated by commas
        to cat_list.

        :param string: a string containing the cats separated by commas
        :type string: str
        """
        num_errors = libvect.Vect_str_to_cat_list(string, self.c_cat_list)
        if num_errors:
            raise GrassError("%d number of errors in ranges" % num_errors)

    def from_array(self, array):
        """Convert ordered array of integers to cat_list structure.

        :param array: the input array containing the cats
        :type array: ~grass.script.array.array
        """
        # Vect_array_to_cat_list(const int *vals, int nvals, ***)
        # TODO: it's not working
        libvect.Vect_array_to_cat_list(array, len(array), self.c_cat_list)

    def __contains__(self, cat):
        """Check if category number is in list.

        :param cat: the category number
        :type cat: int
        """
        return bool(libvect.Vect_cat_in_cat_list(cat, self.c_cat_list))


if __name__ == "__main__":
    import doctest

    doctest.testmod()
