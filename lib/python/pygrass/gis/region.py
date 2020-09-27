# -*- coding: utf-8 -*-
"""
Created on Fri May 25 12:57:10 2012

@author: Pietro Zambelli
"""
from __future__ import (nested_scopes, generators, division, absolute_import,
                        with_statement, print_function, unicode_literals)
import ctypes
import grass.lib.gis as libgis
import grass.lib.raster as libraster
import grass.script as grass

from grass.pygrass.errors import GrassError
from grass.pygrass.shell.conversion import dict2html
from grass.pygrass.utils import get_mapset_vector, get_mapset_raster

test_vector_name = "Region_test_vector"
test_raster_name = "Region_test_raster"

class Region(object):
    """This class is design to easily access and modify GRASS computational
    region. ::

        >>> r = Region()
        >>> r.north
        40.0
        >>> r.south
        0.0
        >>> r.east
        40.0
        >>> r.west
        0.0
        >>> r.cols
        20
        >>> r.rows
        20
        >>> r.nsres
        2.0
        >>> r.ewres
        2.0

        >>> r.north = 100
        >>> r.east = 100
        >>> r.adjust(rows=True, cols=True)
        >>> r.nsres
        5.0
        >>> r.ewres
        5.0
        >>> r.cols
        20
        >>> r.rows
        20

        >>> r.read()
        >>> r.north = 100
        >>> r.east = 100
        >>> r.adjust(rows=False, cols=True)
        >>> r.nsres
        2.0
        >>> r.ewres
        5.0
        >>> r.cols
        20
        >>> r.rows
        50

        >>> r.read()
        >>> r.north = 100
        >>> r.east = 100
        >>> r.adjust(rows=True, cols=False)
        >>> r.nsres
        5.0
        >>> r.ewres
        2.0
        >>> r.cols
        50
        >>> r.rows
        20

        >>> r.read()
        >>> r.north = 100
        >>> r.east = 100
        >>> r.adjust(rows=False, cols=False)
        >>> r.nsres
        2.0
        >>> r.ewres
        2.0
        >>> r.cols
        50
        >>> r.rows
        50

        >>> r.read()
        >>> r.cols = 1000
        >>> r.ewres
        0.04
        >>> r.rows = 1000
        >>> r.nsres
        0.04

    ..
    """

    def __init__(self, default=False):
        self.c_region = libgis.Cell_head()
        if default:
            self.read_default()
        else:
            self.read()

    def byref(self):
        """Return the internal region representation as pointer"""
        return ctypes.pointer(self.c_region)

    def _set_param(self, key, value):
        grass.run_command('g.region', **{key: value})

    #----------LIMITS----------
    def _get_n(self):
        """Private function to obtain north value"""
        return self.c_region.north

    def _set_n(self, value):
        """Private function to set north value"""
        self.c_region.north = value

    north = property(fget=_get_n, fset=_set_n,
                     doc="Set and obtain north coordinate")

    def _get_s(self):
        """Private function to obtain south value"""
        return self.c_region.south

    def _set_s(self, value):
        """Private function to set south value"""
        self.c_region.south = value

    south = property(fget=_get_s, fset=_set_s,
                     doc="Set and obtain south coordinate")

    def _get_e(self):
        """Private function to obtain east value"""
        return self.c_region.east

    def _set_e(self, value):
        """Private function to set east value"""
        self.c_region.east = value

    east = property(fget=_get_e, fset=_set_e,
                    doc="Set and obtain east coordinate")

    def _get_w(self):
        """Private function to obtain west value"""
        return self.c_region.west

    def _set_w(self, value):
        """Private function to set west value"""
        self.c_region.west = value

    west = property(fget=_get_w, fset=_set_w,
                    doc="Set and obtain west coordinate")

    def _get_t(self):
        """Private function to obtain top value"""
        return self.c_region.top

    def _set_t(self, value):
        """Private function to set top value"""
        self.c_region.top = value

    top = property(fget=_get_t, fset=_set_t,
                   doc="Set and obtain top value")

    def _get_b(self):
        """Private function to obtain bottom value"""
        return self.c_region.bottom

    def _set_b(self, value):
        """Private function to set bottom value"""
        self.c_region.bottom = value

    bottom = property(fget=_get_b, fset=_set_b,
                      doc="Set and obtain bottom value")

    #----------RESOLUTION----------
    def _get_rows(self):
        """Private function to obtain rows value"""
        return self.c_region.rows

    def _set_rows(self, value):
        """Private function to set rows value"""
        self.c_region.rows = value
        self.adjust(rows=True)

    rows = property(fget=_get_rows, fset=_set_rows,
                    doc="Set and obtain number of rows")

    def _get_cols(self):
        """Private function to obtain columns value"""
        return self.c_region.cols

    def _set_cols(self, value):
        """Private function to set columns value"""
        self.c_region.cols = value
        self.adjust(cols=True)

    cols = property(fget=_get_cols, fset=_set_cols,
                    doc="Set and obtain number of columns")

    def _get_depths(self):
        """Private function to obtain depths value"""
        return self.c_region.depths

    def _set_depths(self, value):
        """Private function to set depths value"""
        self.c_region.depths = value

    depths = property(fget=_get_depths, fset=_set_depths,
                      doc="Set and obtain number of depths")

    def _get_nsres(self):
        """Private function to obtain north-south value"""
        return self.c_region.ns_res

    def _set_nsres(self, value):
        """Private function to obtain north-south value"""
        self.c_region.ns_res = value
        self.adjust()

    nsres = property(fget=_get_nsres, fset=_set_nsres,
                     doc="Set and obtain north-south resolution value")

    def _get_ewres(self):
        """Private function to obtain east-west value"""
        return self.c_region.ew_res

    def _set_ewres(self, value):
        """Private function to set east-west value"""
        self.c_region.ew_res = value
        self.adjust()

    ewres = property(fget=_get_ewres, fset=_set_ewres,
                     doc="Set and obtain east-west resolution value")

    def _get_tbres(self):
        """Private function to obtain top-botton 3D value"""
        return self.c_region.tb_res

    def _set_tbres(self, value):
        """Private function to set top-bottom 3D value"""
        self.c_region.tb_res = value
        self.adjust()

    tbres = property(fget=_get_tbres, fset=_set_tbres,
                     doc="Set and obtain top-bottom 3D value")

    @property
    def zone(self):
        """Return the zone of projection
        """
        return self.c_region.zone

    @property
    def proj(self):
        """Return a code for projection
        """
        return self.c_region.proj

    @property
    def cells(self):
        """Return the number of cells"""
        return self.rows * self.cols

    #----------MAGIC METHODS----------
    def __repr__(self):
        rg = "Region(north=%g, south=%g, east=%g, west=%g, "\
            "nsres=%g, ewres=%g, rows=%i, cols=%i, "\
            "cells=%i, zone=%i, proj=%i)"
        return rg % (self.north, self.south, self.east, self.west,
                     self.nsres, self.ewres, self.rows, self.cols,
                     self.cells, self.zone, self.proj)

    def _repr_html_(self):
        return dict2html(dict(self.items()), keys=self.keys(),
                         border='1', kdec='b')

    def __unicode__(self):
        return self.__repr__()

    def __str__(self):
        return self.__unicode__()

    def __eq__(self, reg):
        """Compare two region. ::

        >>> r0 = Region()
        >>> r1 = Region()
        >>> r2 = Region()
        >>> r2.nsres = 5
        >>> r0 == r1
        True
        >>> r1 == r2
        False

        ..
        """
        attrs = ['north', 'south', 'west', 'east', 'top', 'bottom',
                 'nsres', 'ewres', 'tbres', 'rows', 'cols', 'cells',
                 'zone', 'proj']
        for attr in attrs:
            if getattr(self, attr) != getattr(reg, attr):
                return False
        return True

    def __ne__(self, other):
        return not self == other

    # Restore Python 2 hashing beaviour on Python 3
    __hash__ = object.__hash__

    def keys(self):
        """Return a list of valid keys. ::

            >>> reg = Region()
            >>> reg.keys()                               # doctest: +ELLIPSIS
            ['proj', 'zone', ..., 'cols', 'cells']

        ..
        """
        return ['proj', 'zone', 'north', 'south', 'west', 'east',
                'top', 'bottom', 'nsres', 'ewres', 'tbres', 'rows',
                'cols', 'cells']

    def items(self):
        """Return a list of tuple with key and value.
        """
        return [(k, self.__getattribute__(k)) for k in self.keys()]

    #----------METHODS----------
    def zoom(self, raster_name):
        """Shrink region until it meets non-NULL data from this raster map

        Warning: This will change the user GRASS region settings

        :param raster_name: the name of raster
        :type raster_name: str
        """
        self._set_param('zoom', str(raster_name))
        self.read()

    def align(self, raster_name):
        """Adjust region cells to cleanly align with this raster map

        Warning: This will change the user GRASS region settings

        :param raster_name: the name of raster
        :type raster_name: str
        """
        self._set_param('align', str(raster_name))
        self.read()

    def adjust(self, rows=False, cols=False):
        """Adjust rows and cols number according with the nsres and ewres
        resolutions. If rows or cols parameters are True, the adjust method
        update nsres and ewres according with the rows and cols numbers.
        """
        libgis.G_adjust_Cell_head(self.byref(), bool(rows), bool(cols))

    def from_vect(self, vector_name):
        """Adjust bounding box of region using a vector

            :param vector_name: the name of vector
            :type vector_name: str

            Example ::

            >>> reg = Region()
            >>> reg.from_vect(test_vector_name)
            >>> reg.get_bbox()
            Bbox(6.0, 0.0, 14.0, 0.0)
            >>> reg.read()
            >>> reg.get_bbox()
            Bbox(40.0, 0.0, 40.0, 0.0)

            ..
        """
        from grass.pygrass.vector import VectorTopo
        with VectorTopo(vector_name, mode='r') as vect:
            bbox = vect.bbox()
            self.set_bbox(bbox)

    def from_rast(self, raster_name):
        """Set the region from the computational region
            of a raster map layer.

            :param raster_name: the name of raster
            :type raster_name: str

            :param mapset: the mapset of raster
            :type mapset: str

            call C function `Rast_get_cellhd`

            Example ::

            >>> reg = Region()
            >>> reg.from_rast(test_raster_name)
            >>> reg.get_bbox()
            Bbox(50.0, 0.0, 60.0, 0.0)
            >>> reg.read()
            >>> reg.get_bbox()
            Bbox(40.0, 0.0, 40.0, 0.0)

            ..
           """
        if not raster_name:
            raise ValueError("Raster name or mapset are invalid")


        mapset = get_mapset_raster(raster_name)

        if mapset:
            libraster.Rast_get_cellhd(raster_name, mapset,
                                      self.byref())

    def set_raster_region(self):
        """Set the computational region (window) for all raster maps in the current process.
           
           Attention: All raster objects must be closed or the
                      process will be terminated.
                      
           The Raster library C function Rast_set_window() is called.
        
        """
        libraster.Rast_set_window(self.byref())

    def get_current(self):
        """Get the current working region of this process
           and store it into this Region object

           Previous calls to set_current() affects values returned by this function.
           Previous calls to read() affects values returned by this function
           only if the current working region is not initialized.

            Example:

            >>> r = Region()
            >>> r.north
            40.0

            >>> r.north = 30
            >>> r.north
            30.0
            >>> r.get_current()
            >>> r.north
            40.0

        """
        libgis.G_get_set_window(self.byref())

    def set_current(self):
        """Set the current working region from this region object

           This function adjusts the values before setting the region
           so you don't have to call G_adjust_Cell_head().

           Attention: Only the current process is affected.
                      The GRASS computational region is not affected.

            Example::

            >>> r = Region()
            >>> r.north
            40.0
            >>> r.south
            0.0

            >>> r.north = 30
            >>> r.south = 20
            >>> r.set_current()
            >>> r.north
            30.0
            >>> r.south
            20.0
            >>> r.get_current()
            >>> r.north
            30.0
            >>> r.south
            20.0

            >>> r.read(force_read=False)
            >>> r.north
            40.0
            >>> r.south
            0.0

            >>> r.read(force_read=True)
            >>> r.north
            40.0
            >>> r.south
            0.0

        """
        libgis.G_set_window(self.byref())

    def read(self, force_read=True):
        """
          Read the region into this region object

          Reads the region as stored in the WIND file in the user's current
          mapset into region.

          3D values are set to defaults if not available in WIND file.  An
          error message is printed and exit() is called if there is a problem
          reading the region.

          <b>Note:</b> GRASS applications that read or write raster maps
          should not use this routine since its use implies that the active
          module region will not be used. Programs that read or write raster
          map data (or vector data) can query the active module region using
          Rast_window_rows() and Rast_window_cols().

          :param force_read: If True the WIND file of the current mapset
                             is re-readed, otherwise the initial region
                             set at process start will be loaded from the internal
                             static variables.
          :type force_read: boolean

        """
        # Force the reading of the WIND file
        if force_read:
            libgis.G_unset_window()
        libgis.G_get_window(self.byref())

    def write(self):
        """Writes the region from this region object

           This function writes this region to the Region file (WIND)
           in the users current mapset. This function should be
           carefully used, since the user will ot notice if his region
           was changed and would expect that only g.region will do this.

            Example ::

            >>> from copy import deepcopy
            >>> r = Region()
            >>> rn = deepcopy(r)
            >>> r.north = 20
            >>> r.south = 10

            >>> r.write()
            >>> r.read()
            >>> r.north
            20.0
            >>> r.south
            10.0

            >>> rn.write()
            >>> r.read()
            >>> r.north
            40.0
            >>> r.south
            0.0

            >>> r.read_default()
            >>> r.write()

            ..
        """
        self.adjust()
        if libgis.G_put_window(self.byref()) < 0:
            raise GrassError("Cannot change region (WIND file).")


    def read_default(self):
        """
          Get the default region

          Reads the default region for the location in this Region object.
          3D values are set to defaults if not available in WIND file.

          An error message is printed and exit() is called if there is a
          problem reading the default region.
        """
        libgis.G_get_default_window(self.byref())

    def get_bbox(self):
        """Return a Bbox object with the extension of the region. ::

            >>> reg = Region()
            >>> reg.get_bbox()
            Bbox(40.0, 0.0, 40.0, 0.0)

        ..
        """
        from grass.pygrass.vector.basic import Bbox
        return Bbox(north=self.north, south=self.south,
                    east=self.east, west=self.west,
                    top=self.top, bottom=self.bottom)

    def set_bbox(self, bbox):
        """Set region extent from Bbox

        :param bbox: a Bbox object to set the extent
        :type bbox: Bbox object

        ::

            >>> from grass.pygrass.vector.basic import Bbox
            >>> b = Bbox(230963.640878, 212125.562878, 645837.437393, 628769.374393)
            >>> reg = Region()
            >>> reg.set_bbox(b)
            >>> reg.get_bbox()
            Bbox(230963.640878, 212125.562878, 645837.437393, 628769.374393)
            >>> reg.get_current()

        ..
        """
        self.north = bbox.north
        self.south = bbox.south
        self.east = bbox.east
        self.west = bbox.west

if __name__ == "__main__":

    import doctest
    from grass.pygrass import utils
    from grass.script.core import run_command

    utils.create_test_vector_map(test_vector_name)
    run_command("g.region", n=50, s=0, e=60, w=0, res=1)
    run_command("r.mapcalc", expression="%s = 1" % (test_raster_name),
                             overwrite=True)
    run_command("g.region", n=40, s=0, e=40, w=0, res=2)

    doctest.testmod()

    """Remove the generated vector map, if exist"""
    mset = utils.get_mapset_vector(test_vector_name, mapset='')
    if mset:
        run_command("g.remove", flags='f', type='vector', name=test_vector_name)
    mset = utils.get_mapset_raster(test_raster_name, mapset='')
    if mset:
        run_command("g.remove", flags='f', type='raster', name=test_raster_name)
