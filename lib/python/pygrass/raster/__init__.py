# -*- coding: utf-8 -*-
"""
Created on Fri May 25 12:56:33 2012

@author: pietro
"""
import ctypes
import numpy as np

#
# import GRASS modules
#
from grass.script import fatal, warning
from grass.script import core as grasscore
#from grass.script import core
#import grass.lib as grasslib
import grass.lib.gis as libgis
import grass.lib.raster as libraster
import grass.lib.segment as libseg
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
from abstract import RasterAbstractBase, Info
from raster_type import TYPE as RTYPE, RTYPE_STR
from buffer import Buffer
from segment import Segment
from rowio import RowIO


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

    ::
        >>> elev = RasterRow('elevation')
        >>> elev.exist()
        True
        >>> elev.is_open()
        False
        >>> elev.cols
        >>> elev.open()
        >>> elev.is_open()
        True
        >>> type(elev.cols)
        <type 'int'>
        >>> elev.has_cats()
        False
        >>> elev.mode
        'r'
        >>> elev.mtype
        'FCELL'
        >>> elev.num_cats()
        0
        >>> elev.range
        (55.578792572021484, 156.32986450195312)

    Each Raster map have an attribute call ``cats`` that allow user
    to interact with the raster categories. ::

        >>> land = RasterRow('landcover_1m')
        >>> land.open()
        >>> land.cats
        []
        >>> land.read_cats()
        >>> land.cats
        [('pond', 1, None),
         ('forest', 2, None),
         ('developed', 3, None),
         ('bare', 4, None),
         ('paved road', 5, None),
         ('dirt road', 6, None),
         ('vineyard', 7, None),
         ('agriculture', 8, None),
         ('wetland', 9, None),
         ('bare ground path', 10, None),
         ('grass', 11, None)]

    Open a raster map using the *with statement*: ::

        >>> with RasterRow('elevation') as elev:
        ...     for row in elev[:3]:
        ...         print row[:4]
        ...
        [ 141.99613953  141.27848816  141.37904358  142.29821777]
        [ 142.90461731  142.39450073  142.68611145  143.59086609]
        [ 143.81854248  143.54707336  143.83972168  144.59527588]
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
        """
        libraster.Rast_put_row(self._fd, row.p, self._gtype)

    def open(self, mode='r', mtype='CELL', overwrite=False):
        """Open the raster if exist or created a new one.

        Parameters
        ------------

        mode: string
            Specify if the map will be open with read or write mode ('r', 'w')
        type: string
            If a new map is open, specify the type of the map(`CELL`, `FCELL`,
            `DCELL`)
        overwrite: Boolean
            Use this flag to set the overwrite mode of existing raster maps


        if the map already exist, automatically check the type and set:
            * self.mtype

        Set all the privite, attributes:
            * self._fd;
            * self._gtype
            * self._rows and self._cols
        """
        self.mode = mode
        self.mtype = mtype
        self.overwrite = overwrite

        # check if exist and instantiate all the private attributes
        if self.exist():
            self.info = Info(self.name, self.mapset)
            if self.mode == 'r':
                # the map exist, read mode
                self._fd = libraster.Rast_open_old(self.name, self.mapset)
                self._gtype = libraster.Rast_get_map_type(self._fd)
                self.mtype = RTYPE_STR[self._gtype]
                self.cats.read(self)
                self.hist.read(self.name)
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

    def open(self, mode='r', mtype='CELL', overwrite=False):
        super(RasterRowIO, self).open(mode, mtype, overwrite)
        self.rowio.open(self._fd, self._rows, self._cols, self.mtype)

    @must_be_open
    def close(self):
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
        if mode.lower() not in ('r', 'w', 'rw'):
            str_err = _("Mode type: {0} not supported ('r', 'w','rw')")
            raise ValueError(str_err.format(mode))
        self._mode = mode

    mode = property(fget=_get_mode, fset=_set_mode)

    def __setitem__(self, key, row):
        """Return the row of Raster object, slice allowed."""
        if isinstance(key, slice):
            #Get the start, stop, and step from the slice
            return [self.put_row(ii, row)
                    for ii in xrange(*key.indices(len(self)))]
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
        for row in xrange(self._rows):
            libraster.Rast_get_row(
                self._fd, row_buffer.p, row, self._gtype)
            self.segment.put_row(row, row_buffer)

    @must_be_open
    def segment2map(self):
        """Transform the segment file to a map.
        """
        row_buffer = Buffer((self._cols), self.mtype)
        for row in xrange(self._rows):
            row_buffer = self.segment.get_row(row, row_buffer)
            libraster.Rast_put_row(self._fd, row_buffer.p, self._gtype)

    @must_be_open
    def get_row(self, row, row_buffer=None):
        """Return the row using the `segment.get_row` method

        Parameters
        ------------

        row: integer
            Specify the row number;
        row_buffer: Buffer object, optional
            Specify the Buffer object that will be instantiate.
        """
        if row_buffer is None:
            row_buffer = Buffer((self._cols), self.mtype)
        return self.segment.get_row(row, row_buffer)

    @must_be_open
    def put_row(self, row, row_buffer):
        """Write the row using the `segment.put_row` method

        Parameters
        ------------

        row: integer
            Specify the row number;
        row_buffer: Buffer object
            Specify the Buffer object that will be write to the map.
        """
        self.segment.put_row(row, row_buffer)

    @must_be_open
    def get(self, row, col):
        """Return the map value using the `segment.get` method

        Parameters
        ------------

        row: integer
            Specify the row number;
        col: integer
            Specify the column number.
        """
        return self.segment.get(row, col)

    @must_be_open
    def put(self, row, col, val):
        """Write the value to the map using the `segment.put` method

        Parameters
        ------------

        row: integer
            Specify the row number;
        col: integer
            Specify the column number.
        val: value
            Specify the value that will be write to the map cell.
        """
        self.segment.val.value = val
        self.segment.put(row, col)

    def open(self, mode='r', mtype='DCELL', overwrite=False):
        """Open the map, if the map already exist: determine the map type
        and copy the map to the segment files;
        else, open a new segment map.

        Parameters
        ------------

        mode: string, optional
            Specify if the map will be open with read, write or read/write
            mode ('r', 'w', 'rw')
        mtype: string, optional
            Specify the map type, valid only for new maps: CELL, FCELL, DCELL;
        overwrite: Boolean, optional
            Use this flag to set the overwrite mode of existing raster maps
        """
        # read rows and cols from the active region
        self._rows = libraster.Rast_window_rows()
        self._cols = libraster.Rast_window_cols()

        self.overwrite = overwrite
        self.mode = mode
        self.mtype = mtype

        if self.exist():
            self.info = Info(self.name, self.mapset)
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

        Parameters
        ------------

        rm_temp_files: bool
            If True all the segments file will be removed.
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
    RasterNumpy([[ True,  True,  True],
       [ True,  True,  True],
       [ True,  True,  True],
       [False, False, False],
       [False, False, False]], dtype=bool)
    >>> el._write()
    0

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

    mode = property(fget=_get_mode, fset=_set_mode)

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
                raise ValueError(_('Invalid type {0}'.forma(kind)))
        else:
            raise ValueError(_('Invalid size {0}'.format(size)))

    def _read(self):
        """!Read raster map into array

        @return 0 on success
        @return non-zero code on failure
        """
        self.null = None

        size, kind = self._get_flags(self.dtype.itemsize, self.dtype.kind)
        kind = 'f' if kind == 'd' else kind
        ret = grasscore.run_command('r.out.bin', flags=kind,
                                    input=self._name, output=self.filename,
                                    bytes=size, null=self.null,
                                    quiet=True)
        return ret

    def _write(self):
        """
        r.in.bin input=/home/pietro/docdat/phd/thesis/gis/north_carolina/user1/.tmp/eraclito/14325.0 output=new title='' bytes=1,anull='' --verbose --overwrite north=228500.0 south=215000.0 east=645000.0 west=630000.0 rows=1350 cols=1500

        """
        self.tofile(self.filename)
        size, kind = self._get_flags(self.dtype.itemsize, self.dtype.kind)
        #print size, kind
        if kind == 'i':
            kind = None
            size = 4
        size = None if kind == 'f' else size

        # To be set in the future
        self.title = None
        self.null = None

        #import pdb; pdb.set_trace()
        if self.mode in ('w+', 'r+'):
            if not self._name:
                import os
                self._name = "doctest_%i" % os.getpid()
            ret = grasscore.run_command('r.in.bin', flags=kind,
                                        input=self.filename, output=self._name,
                                        title=self.title, bytes=size,
                                        anull=self.null,
                                        overwrite=self.overwrite,
                                        verbose=True,
                                        north=self.reg.north,
                                        south=self.reg.south,
                                        east=self.reg.east,
                                        west=self.reg.west,
                                        rows=self.reg.rows,
                                        cols=self.reg.cols)
            return ret

    def open(self, mtype='', null=None, overwrite=None):
        """Open the map, if the map already exist: determine the map type
        and copy the map to the segment files;
        else, open a new segment map.

        Parameters
        ------------

        mtype: string, optional
            Specify the map type, valid only for new maps: CELL, FCELL, DCELL;
        """
        if overwrite is not None:
            self.overwrite = overwrite
        self.null = null
        # rows and cols already set in __new__
        if self.exist():
            self._read()
        else:
            if mtype:
                self.mtype = mtype
            self._gtype = RTYPE[self.mtype]['grass type']
        # set _fd, because this attribute is used to check
        # if the map is open or not
        self._fd = 1

    def close(self):
        self._write()
        np.memmap._close(self)
        grasscore.try_remove(self.filename)
        self._fd = None

    def get_value(self, point, region=None):
        """This method returns the pixel value of a given pair of coordinates:

        Parameters
        ------------

        point = pair of coordinates in tuple object
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
    for _ in xrange(region.rows):
        random_map.put_row(row_buf)
    random_map.close()
    return random_map


def random_map(mapname, mtype, overwrite=True, factor=100):
    region = Region()
    random_map = RasterRow(mapname)
    random_map.open('w', mtype, overwrite)
    for _ in xrange(region.rows):
        row_buf = Buffer((region.cols, ), mtype,
                         buffer=(np.random.random(region.cols,) * factor).data)
        random_map.put_row(row_buf)
    random_map.close()
    return random_map
