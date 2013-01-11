# -*- coding: utf-8 -*-
"""
Created on Tue Jul 31 13:06:20 2012

@author: pietro
"""
import ctypes
import grass.lib.vector as libvect
from collections import Iterable


class Bbox(object):
    """Instantiate a Bounding Box class that contains
    a ctypes pointer to the C struct bound_box, that could be used
    by C GRASS functions. ::

        >>> bbox = Bbox()
        >>> bbox
        Bbox(0.0, 0.0, 0.0, 0.0)

    The default parameters are 0. It is possible to set or change
    the parameters later, with: ::

        >>> bbox.north = 10
        >>> bbox.south = -10
        >>> bbox.east = -20
        >>> bbox.west = 20
        >>> bbox
        Bbox(10.0, -10.0, -20.0, 20.0)

    Or directly istantiate the class with the values, with: ::

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
        return self.c_bbox.contents.N

    def _set_n(self, value):
        self.c_bbox.contents.N = value

    north = property(fget=_get_n, fset=_set_n)

    def _get_s(self):
        return self.c_bbox.contents.S

    def _set_s(self, value):
        self.c_bbox.contents.S = value

    south = property(fget=_get_s, fset=_set_s)

    def _get_e(self):
        return self.c_bbox.contents.E

    def _set_e(self, value):
        self.c_bbox.contents.E = value

    east = property(fget=_get_e, fset=_set_e)

    def _get_w(self):
        return self.c_bbox.contents.W

    def _set_w(self, value):
        self.c_bbox.contents.W = value

    west = property(fget=_get_w, fset=_set_w)

    def _get_t(self):
        return self.c_bbox.contents.T

    def _set_t(self, value):
        self.c_bbox.contents.T = value

    top = property(fget=_get_t, fset=_set_t)

    def _get_b(self):
        return self.c_bbox.contents.B

    def _set_b(self, value):
        self.c_bbox.contents.B = value

    bottom = property(fget=_get_b, fset=_set_b)

    def __repr__(self):
        return "Bbox({n}, {s}, {e}, {w})".format(n=self.north, s=self.south,
                                                 e=self.east, w=self.west)

    def contains(self, point):
        return bool(libvect.Vect_point_in_box(point.x, point.y,
                                              point.z if point.z else 0,
                                              self.c_bbox))


class BoxList(object):
    def __init__(self, boxlist=None):
        self.c_boxlist = ctypes.pointer(libvect.boxlist())
        # if set to 0, the list will hold only ids and no boxes
        self.c_boxlist.contents.have_boxes = 1
        if boxlist is not None:
            for box in boxlist:
                self.append(box)

    def __len__(self):
        return self.c_boxlist.contents.n_values

    def __repr__(self):
        return "Boxlist([%s])" % ", ".join([repr(box)
                                            for box in self.__iter__()])

    def __getitem__(self, indx):
        bbox = Bbox()
        bbox.c_bbox = ctypes.pointer(self.c_boxlist.contents.box[indx])
        return bbox

    def __setitem__(self, indx, bbox):
        self.c_boxlist.contents.box[indx] = bbox

    def __iter__(self):
        return (self.__getitem__(box_id) for box_id in xrange(self.__len__()))

    def __str__(self):
        return self.__repr__()

    def append(self, box):
        """Append a Bbox object to a Boxlist object, using the
        ``Vect_boxlist_append`` C fuction. ::

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

        ..
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
        ``Vect_boxlist_delete_boxlist``. ::

            >>> boxlist = BoxList([Bbox(),
            ...                    Bbox(1, 0, 0, 1),
            ...                    Bbox(1, -1, -1, 1)])
            >>> boxlist.remove(0)
            >>> boxlist
            Boxlist([Bbox(1.0, 0.0, 0.0, 1.0), Bbox(1.0, -1.0, -1.0, 1.0)])

        ..
        """
        if hasattr(indx, 'c_boxlist'):
            libvect.Vect_boxlist_delete_boxlist(self.c_boxlist, indx.c_boxlist)
        elif isinstance(indx, int):
            libvect.Vect_boxlist_delete(self.c_boxlist, indx)
        else:
            for ind in indx:
                libvect.Vect_boxlist_delete(self.c_boxlist, ind)

    def reset(self):
        """Reset the c_boxlist C struct, using the ``Vect_reset_boxlist`` C
        function. ::

            >>> boxlist = BoxList([Bbox(),
            ...                    Bbox(1, 0, 0, 1),
            ...                    Bbox(1, -1, -1, 1)])
            >>> len(boxlist)
            3
            >>> boxlist.reset()
            >>> len(boxlist)
            0

        ..
        """
        libvect.Vect_reset_boxlist(self.c_boxlist)


class Ilist(object):
    """Instantiate a list of integer using the C GRASS struct ``ilist``,
    the class contains this struct as ``c_ilist`` attribute. """
    def __init__(self, integer_list=None):
        self.c_ilist = ctypes.pointer(libvect.struct_ilist())
        if integer_list is not None:
            self.extend(integer_list)

    def __getitem__(self, key):
        if isinstance(key, slice):
            #import pdb; pdb.set_trace()
            #Get the start, stop, and step from the slice
            return [self.c_ilist.contents.value[indx]
                    for indx in xrange(*key.indices(len(self)))]
        elif isinstance(key, int):
            if key < 0:  # Handle negative indices
                key += self.c_ilist.contents.n_values
            if key >= self.c_ilist.contents.n_values:
                raise IndexError('Index out of range')
            return self.c_ilist.contents.value[key]
        else:
            raise ValueError("Invalid argument type: %r." % key)

    def __setitem__(self, key, value):
        if self.contains(value):
            raise ValueError('Integer already in the list')
        self.c_ilist.contents.value[key] = int(value)

    def __len__(self):
        return self.c_ilist.contents.n_values

    def __iter__(self):
        return [self.c_ilist.contents.value[i] for i in xrange(self.__len__())]

    def __repr__(self):
        return "Ilist(%r)" % repr(self.__iter__())

    def append(self, value):
        """Append an integer to the list"""
        if libvect.Vect_list_append(self.c_ilist, value):
            raise  # TODO

    def reset(self):
        """Reset the list"""
        libvect.Vect_reset_list(self.c_ilist)

    def extend(self, ilist):
        """Extend the list with another Ilist object or
        with a list of integers"""
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
            raise ValueError('Value: %r, is not supported' % value)

    def contains(self, value):
        """Check if value is in the list"""
        return bool(libvect.Vect_val_in_list(self.c_ilist, value))


class Cats(object):
    """Instantiate a Category class that contains a ctypes pointer
    to the Map_info C struct, and a ctypes pointer to that C cats struct,
    that could be used by C functions.
    """

    def __init__(self, c_mapinfo, v_id, c_cats=None):
        self.c_mapinfo = c_mapinfo
        self.id = v_id
        if c_cats is not None:
            self.c_cats = c_cats
        else:
            self.c_cats = ctypes.pointer(libvect.line_cats())
            self.get_area_cats()

    def get_area_cats(self):
        """Get area categories, set the c_cats struct given an area, using the
        ``Vect_get_area_cats`` function.
        """
        libvect.Vect_get_area_cats(self.c_mapinfo, self.id, self.c_cats)

    def reset(self):
        """Reset the C cats struct from previous values."""
        libvect.Vect_reset_cats(self.c_cats)
