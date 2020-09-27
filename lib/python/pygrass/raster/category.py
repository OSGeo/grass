# -*- coding: utf-8 -*-
"""
Created on Thu Jun 28 17:44:14 2012

@author: pietro
"""
import ctypes
from operator import itemgetter

import grass.lib.raster as libraster
from grass.exceptions import ImplementationError

from grass.pygrass.errors import GrassError
from grass.pygrass.utils import decode
from grass.pygrass.raster.raster_type import TYPE as RTYPE


class Category(list):
    """
    I would like to add the following functions:

    Getting the umber of cats:
    Rast_number_of_cats() <- Important for ith access

    Getting and setting the title:
    Rast_get_cats_title()
    Rast_set_cats_title()

    Do not use these functions for category access:
    Rast_get_cat()
    and the specialized types for CELL, FCELL and DCELL.
    Since these functions are working on hidden static buffer.

    Use the ith-get methods:
    Rast_get_ith_c_cat()
    Rast_get_ith_f_cat()
    Rast_get_ith_d_cat()

    This can be implemented using an iterator too. So that the category object
    provides the [] access operator to the categories, returning a tuple
    (label, min, max).
    Using this, the category object must be aware of its raster map type.

    Set categories using:
    Rast_set_c_cat()
    Rast_set_f_cat()
    Rast_set_d_cat()

    Misc:
    Rast_sort_cats()
    Rast_copy_cats() <- This should be wrapped so that categories from an
    existing Python category class are copied.

    """

    def __init__(self, name, mapset='', mtype='CELL', *args, **kargs):
        self.name = name
        self.mapset = mapset
        self.c_cats = libraster.Categories()
        libraster.Rast_init_cats("", ctypes.byref(self.c_cats))
        self._mtype = mtype
        self._gtype = None if mtype is None else RTYPE[mtype]['grass type']
        super(Category, self).__init__(*args, **kargs)

    def _get_mtype(self):
        return self._mtype

    def _set_mtype(self, mtype):
        if mtype.upper() not in ('CELL', 'FCELL', 'DCELL'):
            raise ValueError(_("Raster type: {0} not supported".format(mtype)))
        self._mtype = mtype
        self._gtype = RTYPE[self.mtype]['grass type']

    mtype = property(fget=_get_mtype, fset=_set_mtype,
                     doc="Set or obtain raster data type")

    def _get_title(self):
        return libraster.Rast_get_cats_title(ctypes.byref(self.c_cats))

    def _set_title(self, newtitle):
        return libraster.Rast_set_cats_title(newtitle,
                                             ctypes.byref(self.c_cats))

    title = property(fget=_get_title, fset=_set_title,
                     doc="Set or obtain raster title")

    def __str__(self):
        return self.__repr__()

    def __list__(self):
        cats = []
        for cat in self.__iter__():
            cats.append(cat)
        return cats

    def __dict__(self):
        diz = dict()
        for cat in self.__iter__():
            label, min_cat, max_cat = cat
            diz[(min_cat, max_cat)] = label
        return diz

    def __repr__(self):
        cats = []
        for cat in self.__iter__():
            cats.append(repr(cat))
        return "[{0}]".format(',\n '.join(cats))

    def _chk_index(self, index):
        if type(index) == str:
            try:
                index = self.labels().index(index)
            except ValueError:
                raise KeyError(index)
        return index

    def _chk_value(self, value):
        if type(value) == tuple:
            length = len(value)
            if length == 2:
                label, min_cat = value
                value = (label, min_cat, None)
            elif length < 2 or length > 3:
                raise TypeError('Tuple with a length that is not supported.')
        else:
            raise TypeError('Only Tuple are supported.')
        return value

    def __getitem__(self, index):
        return super(Category, self).__getitem__(self._chk_index(index))

    def __setitem__(self, index, value):
        return super(Category, self).__setitem__(self._chk_index(index),
                                                 self._chk_value(value))

    def _get_c_cat(self, index):
        """Returns i-th description and i-th data range from the list of
        category descriptions with corresponding data ranges. end points of
        data interval.

        Rast_get_ith_cat(const struct Categories * 	pcats,
                         int 	i,
                         void * 	rast1,
                         void * 	rast2,
                         RASTER_MAP_TYPE 	data_type
                         )
        """
        min_cat = ctypes.pointer(RTYPE[self.mtype]['grass def']())
        max_cat = ctypes.pointer(RTYPE[self.mtype]['grass def']())
        lab = decode(libraster.Rast_get_ith_cat(ctypes.byref(self.c_cats),
                                                index,
                                                ctypes.cast(min_cat, ctypes.c_void_p),
                                                ctypes.cast(max_cat, ctypes.c_void_p),
                                                self._gtype))
        # Manage C function Errors
        if lab == '':
            raise GrassError(_("Error executing: Rast_get_ith_cat"))
        if max_cat.contents.value == min_cat.contents.value:
            max_cat = None
        else:
            max_cat = max_cat.contents.value
        return lab, min_cat.contents.value, max_cat

    def _set_c_cat(self, label, min_cat, max_cat=None):
        """Adds the label for range min through max in category structure cats.

        int Rast_set_cat(const void * 	rast1,
                         const void * 	rast2,
                         const char * 	label,
                         struct Categories * 	pcats,
                         RASTER_MAP_TYPE 	data_type
                         )
        """
        max_cat = min_cat if max_cat is None else max_cat
        min_cat = ctypes.pointer(RTYPE[self.mtype]['grass def'](min_cat))
        max_cat = ctypes.pointer(RTYPE[self.mtype]['grass def'](max_cat))
        err = libraster.Rast_set_cat(ctypes.cast(min_cat, ctypes.c_void_p),
                                     ctypes.cast(max_cat, ctypes.c_void_p),
                                     label,
                                     ctypes.byref(self.c_cats), self._gtype)
        # Manage C function Errors
        if err == 1:
            return None
        elif err == 0:
            raise GrassError(_("Null value detected"))
        elif err == -1:
            raise GrassError(_("Error executing: Rast_set_cat"))

    def __del__(self):
        libraster.Rast_free_cats(ctypes.byref(self.c_cats))

    def get_cat(self, index):
        return self.__getitem__(index)

    def set_cat(self, index, value):
        if index is None:
            self.append(value)
        elif index < self.__len__():
            self.__setitem__(index, value)
        else:
            raise TypeError("Index outside range.")

    def reset(self):
        for i in range(len(self) - 1, -1, -1):
            del(self[i])
        libraster.Rast_init_cats("", ctypes.byref(self.c_cats))

    def _read_cats(self):
        """Copy from the C struct to the list"""
        for i in range(self.c_cats.ncats):
            self.append(self._get_c_cat(i))

    def _write_cats(self):
        """Copy from the list data to the C struct"""
        # reset only the C struct
        libraster.Rast_init_cats("", ctypes.byref(self.c_cats))
        # write to the c struct
        for cat in self.__iter__():
            label, min_cat, max_cat = cat
            if max_cat is None:
                max_cat = min_cat
            self._set_c_cat(label, min_cat, max_cat)

    def read(self):
        """Read categories from a raster map

        The category file for raster map name in mapset is read into the
        cats structure. If there is an error reading the category file,
        a diagnostic message is printed.

        int Rast_read_cats(const char * 	name,
                           const char * 	mapset,
                           struct Categories * 	pcats
                           )
        """
        self.reset()
        err = libraster.Rast_read_cats(self.name, self.mapset,
                                       ctypes.byref(self.c_cats))
        if err == -1:
            raise GrassError("Can not read the categories.")
        # copy from C struct to list
        self._read_cats()

    def write(self):
        """Writes the category file for the raster map name in the current
           mapset from the cats structure.

        void Rast_write_cats(const char * 	name,
                             struct Categories * 	cats
                             )
        """
        # copy from list to C struct
        self._write_cats()
        # write to the map
        libraster.Rast_write_cats(self.name, ctypes.byref(self.c_cats))

    def copy(self, category):
        """Copy from another Category class

        :param category: Category class to be copied
        :type category: Category object
        """
        libraster.Rast_copy_cats(ctypes.byref(self.c_cats),     # to
                                 ctypes.byref(category.c_cats))  # from
        self._read_cats()

    def ncats(self):
        return self.__len__()

    def set_cats_fmt(self, fmt, m1, a1, m2, a2):
        """Not implemented yet.
        void Rast_set_cats_fmt()
        """
        # TODO: add
        raise ImplementationError("set_cats_fmt() is not implemented yet.")

    def read_rules(self, filename, sep=':'):
        """Copy categories from a rules file, default separetor is ':', the
        columns must be: min and/or max and label. ::

            1:forest
            2:road
            3:urban

            0.:0.5:forest
            0.5:1.0:road
            1.0:1.5:urban

        :param str filename: the name of file with categories rules
        :param str sep: the separator used to divide values and category

        """
        self.reset()
        with open(filename, 'r') as f:
            for row in f.readlines():
                cat = row.strip().split(sep)
                if len(cat) == 2:
                    label, min_cat = cat
                    max_cat = None
                elif len(cat) == 3:
                    label, min_cat, max_cat = cat
                else:
                    raise TypeError("Row length is greater than 3")
                self.append((label, min_cat, max_cat))

    def write_rules(self, filename, sep=':'):
        """Copy categories from a rules file, default separetor is ':', the
        columns must be: min and/or max and label. ::

            1:forest
            2:road
            3:urban

            0.:0.5:forest
            0.5:1.0:road
            1.0:1.5:urban

        :param str filename: the name of file with categories rules
        :param str sep: the separator used to divide values and category
        """
        with open(filename, 'w') as f:
            cats = []
            for cat in self.__iter__():
                if cat[-1] is None:
                    cat = cat[:-1]
                cats.append(sep.join([str(i) for i in cat]))
            f.write('\n'.join(cats))

    def sort(self):
        libraster.Rast_sort_cats(ctypes.byref(self.c_cats))

    def labels(self):
        return list(map(itemgetter(0), self))
