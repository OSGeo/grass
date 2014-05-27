# -*- coding: utf-8 -*-
"""
Created on Fri May 25 12:56:33 2012

@author: pietro
"""
from __future__ import (nested_scopes, generators, division, absolute_import,
                        with_statement, print_function, unicode_literals)
import os
import ctypes
import numpy as np

#
# import GRASS modules
#
from grass.script import fatal, warning
from grass.script import core as grasscore

import grass.lib.gis as libgis
import grass.lib.raster as libraster
import grass.lib.rowio as librowio

#
# import pygrass modules
#
from grass.pygrass.errors import OpenError, must_be_open
from grass.pygrass.gis.region import Region
from grass.pygrass import functions

#
# import raster classes
#
from grass.pygrass.raster.abstract import RasterAbstractBase
from grass.pygrass.raster.raster_type import TYPE as RTYPE, RTYPE_STR
from grass.pygrass.raster.buffer import Buffer
from grass.pygrass.raster.segment import Segment
from grass.pygrass.raster.rowio import RowIO


class RasterRow(RasterAbstractBase):
    """Raster_row_access": Inherits: "Raster_abstract_base" and implements
    the default row access of the Rast library.
        * Implements row access using row id
        * The get_row() method must accept a Row object as argument that will
          be used for value storage, so no new buffer will be allocated
        * Implements sequential writing of rows
        * Implements indexed value read only access using the [row][col]
          operator
        * Implements the [row] read method that returns a new Row object
        * Writing is limited using the put_row() method which accepts a
          Row as argument
        * No mathematical operation like __add__ and stuff for the Raster
          object (only for rows), since r.mapcalc is more sophisticated and
          faster

    Examples
    --------

        >>> elev = RasterRow('elevation')
        >>> elev.exist()
        True
        >>> elev.is_open()
        False
        >>> elev.info.cols
        >>> elev.open()
        >>> elev.is_open()
        True
        >>> type(elev.info.cols)
        <type 'int'>
        >>> elev.has_cats()
        False
        >>> elev.mode
        u'r'
        >>> elev.mtype
        'FCELL'
        >>> elev.num_cats()
        0
        >>> elev.info.range
        (56, 156)

    Each Raster map have an attribute call ``cats`` that allow user
    to interact with the raster categories. ::

        >>> land = RasterRow('geology')
        >>> land.open()
        >>> land.cats               # doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
        [('Zml', 1.0, None),
         ...
         ('Tpyw', 1832.0, None)]

    Open a raster map using the *with statement*: ::

        >>> with RasterRow('elevation') as elev:
        ...     for row in elev[:3]:
        ...         row[:4]
        ...
        Buffer([ 141.99613953,  141.27848816,  141.37904358,  142.29821777], dtype=float32)
        Buffer([ 142.90461731,  142.39450073,  142.68611145,  143.59086609], dtype=float32)
        Buffer([ 143.81854248,  143.54707336,  143.83972168,  144.59527588], dtype=float32)
        >>> elev.is_open()
        False

    """
    def __init__(self, name, mapset='', *args, **kargs):
        super(RasterRow, self).__init__(name, mapset, *args, **kargs)

    # mode = "r", method = "row",
    @must_be_open
    def get_row(self, row, row_buffer=None):
        """Private method that return the row using the read mode
        call the `Rast_get_row` C function.

        :param row: the number of row to obtain
        :type row: int
        :param row_buffer: specify the Buffer object that will be instantiate
        :type row_buffer: bool

        >>> elev = RasterRow('elevation')
        >>> elev.open()
        >>> elev[0]                 # doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
        Buffer([ 141.99613953, 141.27848816,  141.37904358, ..., 58.40825272,
                 58.30711365,  58.18310547], dtype=float32)
        >>> elev.get_row(0)         # doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
        Buffer([ 141.99613953, 141.27848816, 141.37904358, ..., 58.40825272,
                 58.30711365, 58.18310547], dtype=float32)

        """
        if row_buffer is None:
            row_buffer = Buffer((self._cols,), self.mtype)
        libraster.Rast_get_row(self._fd, row_buffer.p, row, self._gtype)
        return row_buffer

    @must_be_open
    def put_row(self, row):
        """Private method to write the row sequentially.

        :param row: a Row object to insert into raster
        :type row: Buffer object
        """
        libraster.Rast_put_row(self._fd, row.p, self._gtype)

    def open(self, mode=None, mtype=None, overwrite=None):
        """Open the raster if exist or created a new one.

        :param mode: Specify if the map will be open with read or write mode
                     ('r', 'w')
        :type mode: str
        :param type: If a new map is open, specify the type of the map(`CELL`,
                     `FCELL`, `DCELL`)
        :type type: str
        :param overwrite: Use this flag to set the overwrite mode of existing
                          raster maps
        :type overwrite: bool


        if the map already exist, automatically check the type and set:
            * self.mtype

        Set all the privite, attributes:
            * self._fd;
            * self._gtype
            * self._rows and self._cols
        """
        self.mode = mode if mode else self.mode
        self.mtype = mtype if mtype else self.mtype
        self.overwrite = overwrite if overwrite is not None else self.overwrite

        # check if exist and instantiate all the private attributes
        if self.exist():
            self.info.read()
            self.cats.mtype = self.mtype
            self.cats.read()
            self.hist.read()
            if self.mode == 'r':
                # the map exist, read mode
                self._fd = libraster.Rast_open_old(self.name, self.mapset)
                self._gtype = libraster.Rast_get_map_type(self._fd)
                self.mtype = RTYPE_STR[self._gtype]
#                try:
#                    self.cats.read(self)
#                    self.hist.read(self.name)
#                except:
#                    import ipdb; ipdb.set_trace()
            elif self.overwrite:
                if self._gtype is None:
                    raise OpenError(_("Raster type not defined"))
                self._fd = libraster.Rast_open_new(self.name, self._gtype)
            else:
                str_err = _("Raster map <{0}> already exists")
                raise OpenError(str_err.format(self))
        else:
            # Create a new map
            if self.mode == 'r':
                # check if we are in read mode
                str_err = _("The map does not exist, I can't open in 'r' mode")
                raise OpenError(str_err)
            self._fd = libraster.Rast_open_new(self.name, self._gtype)
        # read rows and cols from the active region
        self._rows = libraster.Rast_window_rows()
        self._cols = libraster.Rast_window_cols()


class RasterRowIO(RasterRow):
    """Raster_row_cache_access": The same as "Raster_row_access" but uses
    the ROWIO library for cached row access
    """
    def __init__(self, name, *args, **kargs):
        self.rowio = RowIO()
        super(RasterRowIO, self).__init__(name, *args, **kargs)

    def open(self, mode=None, mtype=None, overwrite=False):
        """Open the raster if exist or created a new one.

        :param mode: specify if the map will be open with read or write mode
                     ('r', 'w')
        :type mode: str
        :param type: if a new map is open, specify the type of the map(`CELL`,
                     `FCELL`, `DCELL`)
        :type type: str
        :param overwrite: use this flag to set the overwrite mode of existing
                          raster maps
        :type overwrite: bool
        """
        super(RasterRowIO, self).open(mode, mtype, overwrite)
        self.rowio.open(self._fd, self._rows, self._cols, self.mtype)

    @must_be_open
    def close(self):
        """Function to close the raster"""
        self.rowio.release()
        libraster.Rast_close(self._fd)
        # update rows and cols attributes
        self._rows = None
        self._cols = None
        self._fd = None

    @must_be_open
    def get_row(self, row, row_buffer=None):
        """This method returns the row using:

            * the read mode and
            * `rowcache` method

        :param row: the number of row to obtain
        :type row: int
        :param row_buffer: Specify the Buffer object that will be instantiate
        :type row_buffer: Buffer object
        """
        if row_buffer is None:
            row_buffer = Buffer((self._cols,), self.mtype)
        rowio_buf = librowio.Rowio_get(ctypes.byref(self.rowio.c_rowio), row)
        ctypes.memmove(row_buffer.p, rowio_buf, self.rowio.row_size)
        return row_buffer


class RasterSegment(RasterAbstractBase):
    """Raster_segment_access": Inherits "Raster_abstract_base" and uses the
    segment library for cached randomly reading and writing access.
        * Implements the [row][col] operator for read and write access using
          segement_get() and segment_put() functions internally
        * Implements row read and write access with the [row] operator using
          segment_get_row() segment_put_row() internally
        * Implements the get_row() and put_row() method  using
          segment_get_row() segment_put_row() internally
        * Implements the flush_segment() method
        * Implements the copying of raster maps to segments and vice verse
        * Overwrites the open and close methods
        * No mathematical operation like __add__ and stuff for the Raster
          object (only for rows), since r.mapcalc is more sophisticated and
          faster
    """
    def __init__(self, name, srows=64, scols=64, maxmem=100,
                 *args, **kargs):
        self.segment = Segment(srows, scols, maxmem)
        super(RasterSegment, self).__init__(name, *args, **kargs)

    def _get_mode(self):
        return self._mode

    def _set_mode(self, mode):
        if mode and mode.lower() not in ('r', 'w', 'rw'):
            str_err = _("Mode type: {0} not supported ('r', 'w','rw')")
            raise ValueError(str_err.format(mode))
        self._mode = mode

    mode = property(fget=_get_mode, fset=_set_mode,
                    doc="Set or obtain the opening mode of raster")

    def __setitem__(self, key, row):
        """Return the row of Raster object, slice allowed."""
        if isinstance(key, slice):
            #Get the start, stop, and step from the slice
            return [self.put_row(ii, row)
                    for ii in range(*key.indices(len(self)))]
        elif isinstance(key, tuple):
            x, y = key
            return self.put(x, y, row)
        elif isinstance(key, int):
            if key < 0:  # Handle negative indices
                key += self._rows
            if key >= self._rows:
                raise IndexError(_("Index out of range: %r.") % key)
            return self.put_row(key, row)
        else:
            raise TypeError("Invalid argument type.")

    @must_be_open
    def map2segment(self):
        """Transform an existing map to segment file.
        """
        row_buffer = Buffer((self._cols), self.mtype)
        for row in range(self._rows):
            libraster.Rast_get_row(
                self._fd, row_buffer.p, row, self._gtype)
            self.segment.put_row(row, row_buffer)

    @must_be_open
    def segment2map(self):
        """Transform the segment file to a map.
        """
        row_buffer = Buffer((self._cols), self.mtype)
        for row in range(self._rows):
            row_buffer = self.segment.get_row(row, row_buffer)
            libraster.Rast_put_row(self._fd, row_buffer.p, self._gtype)

    @must_be_open
    def get_row(self, row, row_buffer=None):
        """Return the row using the `segment.get_row` method

        :param row: specify the row number
        :type row: int
        :param row_buffer: specify the Buffer object that will be instantiate
        :type row_buffer: Buffer object
        """
        if row_buffer is None:
            row_buffer = Buffer((self._cols), self.mtype)
        return self.segment.get_row(row, row_buffer)

    @must_be_open
    def put_row(self, row, row_buffer):
        """Write the row using the `segment.put_row` method

        :param row: a Row object to insert into raster
        :type row: Buffer object
        """
        self.segment.put_row(row, row_buffer)

    @must_be_open
    def get(self, row, col):
        """Return the map value using the `segment.get` method

        :param row: Specify the row number
        :type row: int
        :param col: Specify the column number
        :type col: int
        """
        return self.segment.get(row, col)

    @must_be_open
    def put(self, row, col, val):
        """Write the value to the map using the `segment.put` method

        :param row: Specify the row number
        :type row: int
        :param col: Specify the column number
        :type col: int
        :param val: Specify the value that will be write to the map cell
        :type val: value
        """
        self.segment.val.value = val
        self.segment.put(row, col)

    def open(self, mode=None, mtype=None, overwrite=None):
        """Open the map, if the map already exist: determine the map type
        and copy the map to the segment files;
        else, open a new segment map.

        :param mode: specify if the map will be open with read, write or
                     read/write mode ('r', 'w', 'rw')
        :type mode: str
        :param mtype: specify the map type, valid only for new maps: CELL,
                      FCELL, DCELL
        :type mtype: str
        :param overwrite: use this flag to set the overwrite mode of existing
                          raster maps
        :type overwrite: bool
        """
        # read rows and cols from the active region
        self._rows = libraster.Rast_window_rows()
        self._cols = libraster.Rast_window_cols()

        self.mode = mode if mode else self.mode
        self.mtype = mtype if mtype else self.mtype
        self.overwrite = overwrite if overwrite is not None else self.overwrite

        if self.exist():
            self.info.read()
            self.cats.mtype = self.mtype
            self.cats.read()
            self.hist.read()
            if ((self.mode == "w" or self.mode == "rw") and
                    self.overwrite is False):
                str_err = _("Raster map <{0}> already exists. Use overwrite.")
                fatal(str_err.format(self))

            # We copy the raster map content into the segments
            if self.mode == "rw" or self.mode == "r":
                self._fd = libraster.Rast_open_old(self.name, self.mapset)
                self._gtype = libraster.Rast_get_map_type(self._fd)
                self.mtype = RTYPE_STR[self._gtype]
                # initialize the segment, I need to determine the mtype of the
                # map
                # before to open the segment
                self.segment.open(self)
                self.map2segment()
                self.segment.flush()
                self.cats.read(self)
                self.hist.read(self.name)

                if self.mode == "rw":
                    warning(_(WARN_OVERWRITE.format(self)))
                    # Close the file descriptor and open it as new again
                    libraster.Rast_close(self._fd)
                    self._fd = libraster.Rast_open_new(
                        self.name, self._gtype)
            # Here we simply overwrite the existing map without content copying
            elif self.mode == "w":
                #warning(_(WARN_OVERWRITE.format(self)))
                self._gtype = RTYPE[self.mtype]['grass type']
                self.segment.open(self)
                self._fd = libraster.Rast_open_new(self.name, self._gtype)
        else:
            if self.mode == "r":
                str_err = _("Raster map <{0}> does not exists")
                raise OpenError(str_err.format(self.name))

            self._gtype = RTYPE[self.mtype]['grass type']
            self.segment.open(self)
            self._fd = libraster.Rast_open_new(self.name, self._gtype)

    @must_be_open
    def close(self, rm_temp_files=True):
        """Close the map, copy the segment files to the map.

        :param rm_temp_files: if True all the segments file will be removed
        :type rm_temp_files: bool
        """
        if self.mode == "w" or self.mode == "rw":
            self.segment.flush()
            self.segment2map()
        if rm_temp_files:
            self.segment.close()
        else:
            self.segment.release()
        libraster.Rast_close(self._fd)
        # update rows and cols attributes
        self._rows = None
        self._cols = None
        self._fd = None


FLAGS = {1: {'b': 'i', 'i': 'i', 'u': 'i'},
         2: {'b': 'i', 'i': 'i', 'u': 'i'},
         4: {'f': 'f', 'i': 'i', 'b': 'i', 'u': 'i'},
         8: {'f': 'd'}, }


class RasterNumpy(np.memmap, RasterAbstractBase):
    """Raster_cached_narray": Inherits "Raster_abstract_base" and
    "numpy.memmap". Its purpose is to allow numpy narray like access to
    raster maps without loading the map into the main memory.
    * Behaves like a numpy array and supports all kind of mathematical
      operations: __add__, ...
    * Overrides the open and close methods
    * Be aware of the 2Gig file size limit

    >>> import grass.pygrass as pygrass
    >>> elev = pygrass.raster.RasterNumpy('elevation')
    >>> elev.open()
    >>> elev[:5, :3]
    RasterNumpy([[ 141.99613953,  141.27848816,  141.37904358],
           [ 142.90461731,  142.39450073,  142.68611145],
           [ 143.81854248,  143.54707336,  143.83972168],
           [ 144.56524658,  144.58493042,  144.86477661],
           [ 144.99488831,  145.22894287,  145.57142639]], dtype=float32)
    >>> el = elev < 144
    >>> el[:5, :3]
    RasterNumpy([[1, 1, 1],
           [1, 1, 1],
           [1, 1, 1],
           [0, 0, 0],
           [0, 0, 0]], dtype=int32)
    >>> el._write()
    """
    def __new__(cls, name, mapset="", mtype='CELL', mode='r+',
                overwrite=False):
        reg = Region()
        shape = (reg.rows, reg.cols)
        mapset = libgis.G_find_raster(name, mapset)
        gtype = None
        if mapset:
            # map exist, set the map type
            gtype = libraster.Rast_map_type(name, mapset)
            mtype = RTYPE_STR[gtype]
        filename = grasscore.tempfile()
        obj = np.memmap.__new__(cls, filename=filename,
                                dtype=RTYPE[mtype]['numpy'],
                                mode=mode,
                                shape=shape)
        obj.mtype = mtype.upper()
        obj.gtype = gtype if gtype else RTYPE[mtype]['grass type']
        obj._rows = reg.rows
        obj._cols = reg.cols
        obj.filename = filename
        obj._name = name
        obj.mapset = mapset
        obj.reg = reg
        obj.overwrite = overwrite
        return obj

    def __array_finalize__(self, obj):
        if hasattr(obj, '_mmap'):
            self._mmap = obj._mmap
            self.filename = grasscore.tempfile()
            self.offset = obj.offset
            self.mode = obj.mode
            self._rows = obj._rows
            self._cols = obj._cols
            self._name = None
            self.mapset = ''
            self.reg = obj.reg
            self.overwrite = obj.overwrite
            self.mtype = obj.mtype
            self._fd = obj._fd
        else:
            self._mmap = None

    def _get_mode(self):
        return self._mode

    def _set_mode(self, mode):
        if mode.lower() not in ('r', 'w+', 'r+', 'c'):
            raise ValueError(_("Mode type: {0} not supported.").format(mode))
        self._mode = mode

    mode = property(fget=_get_mode, fset=_set_mode,
                    doc="Set or obtain the opening mode of raster")

    def __array_wrap__(self, out_arr, context=None):
        """See:
        http://docs.scipy.org/doc/numpy/user/
        basics.subclassing.html#array-wrap-for-ufuncs"""
        if out_arr.dtype.kind in 'bui':
            # there is not support for boolean maps, so convert into integer
            out_arr = out_arr.astype(np.int32)
            out_arr.mtype = 'CELL'
        #out_arr.p = out_arr.ctypes.data_as(out_arr.pointer_type)
        return np.ndarray.__array_wrap__(self, out_arr, context)

    def __init__(self, name, *args, **kargs):
        ## Private attribute `_fd` that return the file descriptor of the map
        self._fd = None
        rows, cols = self._rows, self._cols
        RasterAbstractBase.__init__(self, name)
        self._rows, self._cols = rows, cols

    def __unicode__(self):
        return RasterAbstractBase.__unicode__(self)

    def __str__(self):
        return self.__unicode__()

    def _get_flags(self, size, kind):
        if size in FLAGS:
            if kind in FLAGS[size]:
                return size, FLAGS[size][kind]
            else:
                raise ValueError(_('Invalid type {0}'.format(kind)))
        else:
            raise ValueError(_('Invalid size {0}'.format(size)))

    def _read(self):
        """!Read raster map into array

        @return 0 on success
        @return non-zero code on failure
        """
        with RasterRow(self.name, self.mapset, mode='r') as rst:
            buff = rst[0]
            for i in range(len(rst)):
                self[i] = rst.get_row(i, buff)

    def _write(self):
        """Write the numpy array into map
        """
        #r.in.bin input=/home/pietro/docdat/phd/thesis/gis/north_carolina/user1/.tmp/eraclito/14325.0 output=new title='' bytes=1,anull='' --verbose --overwrite north=228500.0 south=215000.0 east=645000.0 west=630000.0 rows=1350 cols=1500
        if not self.exist() or self.mode != 'r':
            self.flush()
            buff = Buffer(self[0].shape, mtype=self.mtype)
            with RasterRow(self.name, self.mapset, mode='w',
                           mtype=self.mtype) as rst:
                for i in range(len(rst)):
                    buff[:] = self[i][:]
                    rst.put_row(buff[:])

    def open(self, mtype='', null=None, overwrite=None):
        """Open the map, if the map already exist: determine the map type
        and copy the map to the segment files;
        else, open a new segment map.

        :param mtype: specify the map type, valid only for new maps: CELL,
                      FCELL, DCELL;
        :type mtype: str
        :param null:
        :type null:
        :param overwrite: use this flag to set the overwrite mode of existing
                          raster maps
        :type overwrite: bool
        """
        if overwrite is not None:
            self.overwrite = overwrite
        self.null = null
        # rows and cols already set in __new__
        if self.exist():
            self.info.read()
            self.cats.mtype = self.mtype
            self.cats.read()
            self.hist.read()
            self._read()
        else:
            if mtype:
                self.mtype = mtype
            self._gtype = RTYPE[self.mtype]['grass type']
        # set _fd, because this attribute is used to check
        # if the map is open or not
        self._fd = 1

    def close(self, name=''):
        """Function to close the map

        :param name: the name of raster
        :type name: str        
        """
        if self.is_open():
            name = name if name else self.name
            if not name:
                raise RuntimeError('Raster name not set neither '
                                   'given as parameter.')
            self._write()
            os.remove(self.filename)
            self._fd = None

    def get_value(self, point, region=None):
        """This method returns the pixel value of a given pair of coordinates:

        :param point: pair of coordinates in tuple object
        :type point: tuple
        :param region: the region to crop the request
        :type region: Region object
        """
        if not region:
            region = Region()
        x, y = functions.coor2pixel(point.coords(), region)
        return self[x][y]


def random_map_only_columns(mapname, mtype, overwrite=True, factor=100):
    region = Region()
    random_map = RasterRow(mapname)
    row_buf = Buffer((region.cols, ), mtype,
                     buffer=(np.random.random(region.cols,) * factor).data)
    random_map.open('w', mtype, overwrite)
    for _ in range(region.rows):
        random_map.put_row(row_buf)
    random_map.close()
    return random_map


def random_map(mapname, mtype, overwrite=True, factor=100):
    region = Region()
    random_map = RasterRow(mapname)
    random_map.open('w', mtype, overwrite)
    for _ in range(region.rows):
        row_buf = Buffer((region.cols, ), mtype,
                         buffer=(np.random.random(region.cols,) * factor).data)
        random_map.put_row(row_buf)
    random_map.close()
    return random_map
