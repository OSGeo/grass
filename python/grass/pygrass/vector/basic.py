"""
Created on Tue Jul 31 13:06:20 2012

@author: pietro
"""
import ctypes
import grass.lib.vector as libvect
from collections.abc import Iterable

from grass.pygrass.shell.conversion import dict2html


class Bbox(object):
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

    Or directly istantiate the class with the values, with:

    >>> bbox = Bbox(north=100, south=0, east=0, west=100)
    >>> bbox
    Bbox(100.0, 0.0, 0.0, 100.0)

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
        >>> poi = Point(5,5)
        >>> bbox = Bbox(north=10, south=0, west=0, east=10)
        >>> bbox.contains(poi)
        True

        """
        return bool(
            libvect.Vect_point_in_box(
                point.x, point.y, point.z if point.z else 0, self.c_bbox
            )
        )

    def items(self):
        return [(k, self.__getattribute__(k)) for k in self.keys()]

    def nsewtb(self, tb=True):
        """Return a list of values from bounding box

        :param tb: if tb parameter is False return only: north, south, east,
                   west and not top and bottom
        :type tb: bool

        """
        if tb:
            return (self.north, self.south, self.east, self.west, self.top, self.bottom)
        else:
            return (self.north, self.south, self.east, self.west)


class BoxList(object):
    """Instantiate a BoxList class to create a list of Bounding Box"""

    def __init__(self, boxlist=None):
        self.c_boxlist = ctypes.pointer(libvect.boxlist())
        # if set to 0, the list will hold only ids and no boxes
        self.c_boxlist.contents.have_boxes = 1
        if boxlist is not None:
            for box in boxlist:
                self.append(box)

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

    def __getitem__(self, indx):
        bbox = Bbox()
        bbox.c_bbox = ctypes.pointer(self.c_boxlist.contents.box[indx])
        return bbox

    def __setitem__(self, indx, bbox):
        self.c_boxlist.contents.box[indx] = bbox

    def __iter__(self):
        return (self.__getitem__(box_id) for box_id in range(self.__len__()))

    def __str__(self):
        return self.__repr__()

    def append(self, box):
        """Append a Bbox object to a Boxlist object, using the
        ``Vect_boxlist_append`` C function.

        :param bbox: the bounding box to add to the list
        :param bbox: a Bbox object

        >>> box0 = Bbox()
        >>> box1 = Bbox(1,2,3,4)
        >>> box2 = Bbox(5,6,7,8)
        >>> boxlist = BoxList([box0, box1])
        >>> boxlist
        Boxlist([Bbox(0.0, 0.0, 0.0, 0.0), Bbox(1.0, 2.0, 3.0, 4.0)])
        >>> len(boxlist)
        2
        >>> boxlist.append(box2)
        >>> len(boxlist)
        3

        """
        indx = self.__len__()
        libvect.Vect_boxlist_append(self.c_boxlist, indx, box.c_bbox)

    #    def extend(self, boxlist):
    #        """Extend a boxlist with another boxlist or using a list of Bbox, using
    #        ``Vect_boxlist_append_boxlist`` c function. ::
    #
    #            >>> box0 = Bbox()
    #            >>> box1 = Bbox(1,2,3,4)
    #            >>> box2 = Bbox(5,6,7,8)
    #            >>> box3 = Bbox(9,8,7,6)
    #            >>> boxlist0 = BoxList([box0, box1])
    #            >>> boxlist0
    #            Boxlist([Bbox(0.0, 0.0, 0.0, 0.0), Bbox(1.0, 2.0, 3.0, 4.0)])
    #            >>> boxlist1 = BoxList([box2, box3])
    #            >>> len(boxlist0)
    #            2
    #            >>> boxlist0.extend(boxlist1)
    #            >>> len(boxlist0)
    #            4
    #            >>> boxlist1.extend([box0, box1])
    #            >>> len(boxlist1)
    #            4
    #
    #        ..
    #        """
    #        if hasattr(boxlist, 'c_boxlist'):
    #            #import pdb; pdb.set_trace()
    #            # FIXME: doesn't work
    #            libvect.Vect_boxlist_append_boxlist(self.c_boxlist,
    #                                                boxlist.c_boxlist)
    #        else:
    #            for box in boxlist:
    #                self.append(box)

    def remove(self, indx):
        """Remove Bbox from the boxlist, given an integer or a list of integer
        or a boxlist, using ``Vect_boxlist_delete`` C function or the
        ``Vect_boxlist_delete_boxlist``.

        :param indx: the index value of the Bbox to remove
        :param indx: int

        >>> boxlist = BoxList([Bbox(),
        ...                    Bbox(1, 0, 0, 1),
        ...                    Bbox(1, -1, -1, 1)])
        >>> boxlist.remove(0)
        >>> boxlist
        Boxlist([Bbox(1.0, 0.0, 0.0, 1.0), Bbox(1.0, -1.0, -1.0, 1.0)])

        """
        if hasattr(indx, "c_boxlist"):
            libvect.Vect_boxlist_delete_boxlist(self.c_boxlist, indx.c_boxlist)
        elif isinstance(indx, int):
            libvect.Vect_boxlist_delete(self.c_boxlist, indx)
        else:
            for ind in indx:
                libvect.Vect_boxlist_delete(self.c_boxlist, ind)

    def reset(self):
        """Reset the c_boxlist C struct, using the ``Vect_reset_boxlist`` C
        function.

        >>> boxlist = BoxList([Bbox(),
        ...                    Bbox(1, 0, 0, 1),
        ...                    Bbox(1, -1, -1, 1)])
        >>> len(boxlist)
        3
        >>> boxlist.reset()
        >>> len(boxlist)
        0

        """
        libvect.Vect_reset_boxlist(self.c_boxlist)


class Ilist(object):
    """Instantiate a list of integer using the C GRASS struct ``ilist``,
    the class contains this struct as ``c_ilist`` attribute."""

    def __init__(self, integer_list=None):
        self.c_ilist = ctypes.pointer(libvect.struct_ilist())
        if integer_list is not None:
            self.extend(integer_list)

    def __getitem__(self, key):
        if isinstance(key, slice):
            # import pdb; pdb.set_trace()
            # Get the start, stop, and step from the slice
            return [
                self.c_ilist.contents.value[indx]
                for indx in range(*key.indices(len(self)))
            ]
        elif isinstance(key, int):
            if key < 0:  # Handle negative indices
                key += self.c_ilist.contents.n_values
            if key >= self.c_ilist.contents.n_values:
                raise IndexError("Index out of range")
            return self.c_ilist.contents.value[key]
        else:
            raise ValueError("Invalid argument type: %r." % key)

    def __setitem__(self, key, value):
        if self.contains(value):
            raise ValueError("Integer already in the list")
        self.c_ilist.contents.value[key] = int(value)

    def __len__(self):
        return self.c_ilist.contents.n_values

    def __iter__(self):
        return (self.c_ilist.contents.value[i] for i in range(self.__len__()))

    def __repr__(self):
        return "Ilist(%r)" % [i for i in self.__iter__()]

    def __contains__(self, item):
        return item in self.__iter__()

    def append(self, value):
        """Append an integer to the list"""
        if libvect.Vect_list_append(self.c_ilist, value):
            raise  # TODO

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
            libvect.Vect_list_append_list(self.c_ilist, ilist.ilist)
        else:
            for i in ilist:
                self.append(i)

    def remove(self, value):
        """Remove a value from a list"""
        if isinstance(value, int):
            libvect.Vect_list_delete(self.c_ilist, value)
        elif isinstance(value, Ilist):
            libvect.Vect_list_delete_list(self.c_ilist, value.ilist)
        elif isinstance(value, Iterable):
            for i in value:
                libvect.Vect_list_delete(self.c_ilist, int(i))
        else:
            raise ValueError("Value: %r, is not supported" % value)

    def contains(self, value):
        """Check if value is in the list"""
        return bool(libvect.Vect_val_in_list(self.c_ilist, value))


class Cats(object):
    """Instantiate a Category class that contains a ctypes pointer
    to the C line_cats struct.

    >>> cats = Cats()
    >>> for cat in range(100, 110): cats.set(cat, layer=cat-50)
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
        self.c_cats = c_cats if c_cats else ctypes.pointer(libvect.line_cats())

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
            err_msg = "Layer(%d)/category(%d) number does not exist"
            err_msg = err_msg % (layer, cat)
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


class CatsList(object):
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
        self.c_cat_list = (
            c_cat_list if c_cat_list else ctypes.pointer(libvect.cat_list())
        )

    def from_string(self, string):
        """Converts string of categories and cat ranges separated by commas
        to cat_list.

        :param string: a string containing the cats separated by commas
        :type string: str
        """
        num_errors = libvect.Vect_str_to_cat_list(string, self.c_cat_list)
        if num_errors:
            from grass.pygrass.errors import GrassError

            raise GrassError("%d number of errors in ranges" % num_errors)

    def from_array(self, array):
        """Convert ordered array of integers to cat_list structure.

        :param array: the input array containing the cats
        :type array: array
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
