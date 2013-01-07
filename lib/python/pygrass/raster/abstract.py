# -*- coding: utf-8 -*-
"""
Created on Fri Aug 17 16:05:25 2012

@author: pietro
"""


import ctypes

#
# import GRASS modules
#
from grass.script import fatal, warning, gisenv
from grass.script import core as grasscore
#from grass.script import core
#import grass.lib as grasslib
import grass.lib.gis as libgis
import grass.lib.raster as libraster

#
# import pygrass modules
#
from pygrass import functions
from pygrass.gis.region import Region
from pygrass.errors import must_be_open

#
# import raster classes
#
from raster_type import TYPE as RTYPE
from category import Category


## Define global variables to not exceed the 80 columns
WARN_OVERWRITE = "Raster map <{0}> already exists and will be overwritten"
INDXOUTRANGE = "The index (%d) is out of range, have you open the map?."


class RasterAbstractBase(object):
    """Raster_abstract_base: The base class from which all sub-classes
    inherit. It does not implement any row or map access methods:
    * Implements raster metadata information access (Type, ...)
    * Implements an open method that will be overwritten by the sub-classes
    * Implements the close method that might be overwritten by sub-classes
      (should work for simple row access)
    * Implements get and set region methods
    * Implements color, history and category handling
    * Renaming, deletion, ...
    """
    def __init__(self, name, mapset=""):
        """The constructor need at least the name of the map
        *optional* field is the `mapset`. ::

            >>> land = RasterAbstractBase('landcover_1m')
            >>> land.name
            'landcover_1m'
            >>> land.mapset
            ''
            >>> land.exist()
            True
            >>> land.mapset
            'PERMANENT'

        ..
        """
        self.mapset = mapset
        #self.region = Region()
        self.cats = Category()

        self._name = name
        ## Private attribute `_fd` that return the file descriptor of the map
        self._fd = None
        ## Private attribute `_rows` that return the number of rows
        # in active window, When the class is instanced is empty and it is set
        # when you open the file, using Rast_window_rows()
        self._rows = None
        ## Private attribute `_cols` that return the number of rows
        # in active window, When the class is instanced is empty and it is set
        # when you open the file, using Rast_window_cols()
        self._cols = None

    def _get_mtype(self):
        return self._mtype

    def _set_mtype(self, mtype):
        if mtype.upper() not in ('CELL', 'FCELL', 'DCELL'):
            #fatal(_("Raser type: {0} not supported".format(mtype) ) )
            str_err = "Raster type: {0} not supported ('CELL','FCELL','DCELL')"
            raise ValueError(_(str_err).format(mtype))
        self._mtype = mtype
        self._gtype = RTYPE[self.mtype]['grass type']

    mtype = property(fget=_get_mtype, fset=_set_mtype)

    def _get_mode(self):
        return self._mode

    def _set_mode(self, mode):
        if mode.upper() not in ('R', 'W'):
            str_err = _("Mode type: {0} not supported ('r', 'w')")
            raise ValueError(str_err.format(mode))
        self._mode = mode

    mode = property(fget=_get_mode, fset=_set_mode)

    def _get_overwrite(self):
        return self._overwrite

    def _set_overwrite(self, overwrite):
        if overwrite not in (True, False):
            str_err = _("Overwrite type: {0} not supported (True/False)")
            raise ValueError(str_err.format(overwrite))
        self._overwrite = overwrite

    overwrite = property(fget=_get_overwrite, fset=_set_overwrite)

    def _get_name(self):
        """Private method to return the Raster name"""
        return self._name

    def _set_name(self, newname):
        """Private method to change the Raster name"""
        #import pdb; pdb.set_trace()
        cleanname = functions.clean_map_name(newname)
        if self.exist():
            self.rename(cleanname)
        self._name = cleanname

    name = property(fget=_get_name, fset=_set_name)

    @must_be_open
    def _get_rows(self):
        """Private method to return the Raster name"""
        return self._rows

    def _set_unchangeable(self, new):
        """Private method to change the Raster name"""
        warning(_("Unchangeable attribute"))

    rows = property(fget=_get_rows, fset=_set_unchangeable)

    @must_be_open
    def _get_cols(self):
        """Private method to return the Raster name"""
        return self._cols

    cols = property(fget=_get_cols, fset=_set_unchangeable)

    @must_be_open
    def _get_range(self):
        if self.mtype == 'CELL':
            maprange = libraster.Range()
            libraster.Rast_read_range(self.name, self.mapset,
                                      ctypes.byref(maprange))
            self._min = libgis.CELL()
            self._max = libgis.CELL()
            self._min.value = maprange.min
            self._max.value = maprange.max
        else:
            maprange = libraster.FPRange()
            libraster.Rast_read_fp_range(self.name, self.mapset,
                                         ctypes.byref(maprange))
            self._min = libgis.DCELL()
            self._max = libgis.DCELL()
            libraster.Rast_get_fp_range_min_max(ctypes.byref(maprange),
                                                ctypes.byref(self._min),
                                                ctypes.byref(self._max))
        return self._min.value, self._max.value

    range = property(fget=_get_range, fset=_set_unchangeable)

    @must_be_open
    def _get_cats_title(self):
        return self.cats.title

    @must_be_open
    def _set_cats_title(self, newtitle):
        self.cats.title = newtitle

    cats_title = property(fget=_get_cats_title, fset=_set_cats_title)

    def __unicode__(self):
        return self.name_mapset()

    def __str__(self):
        """Return the string of the object"""
        return self.__unicode__()

    def __len__(self):
        return self._rows

    def __getitem__(self, key):
        """Return the row of Raster object, slice allowed."""
        if isinstance(key, slice):
            #import pdb; pdb.set_trace()
            #Get the start, stop, and step from the slice
            return (self.get_row(ii) for ii in xrange(*key.indices(len(self))))
        elif isinstance(key, tuple):
            x, y = key
            return self.get(x, y)
        elif isinstance(key, int):
            if key < 0:  # Handle negative indices
                key += self._rows
            if key >= self._rows:
                fatal(INDXOUTRANGE.format(key))
                raise IndexError
            return self.get_row(key)
        else:
            fatal("Invalid argument type.")

    def __iter__(self):
        """Return a constructor of the class"""
        return (self.__getitem__(irow) for irow in xrange(self._rows))

    def exist(self):
        """Return True if the map already exist, and
        set the mapset if were not set.

        call the C function `G_find_raster`."""
        if self.name:
            self.mapset = functions.get_mapset_raster(self.name, self.mapset)
        else:
            return False
        if self.mapset:
            return True
        else:
            return False

    def is_open(self):
        """Return True if the map is open False otherwise"""
        return True if self._fd is not None and self._fd >= 0 else False

    @must_be_open
    def close(self):
        """Close the map"""
        libraster.Rast_close(self._fd)
        # update rows and cols attributes
        self._rows = None
        self._cols = None
        self._fd = None

    def remove(self):
        """Remove the map"""
        if self.is_open():
            self.close()
        grasscore.run_command('g.remove', rast=self.name)

    def name_mapset(self, name=None, mapset=None):
        if name is None:
            name = self.name
        if mapset is None:
            self.exist()
            mapset = self.mapset

        gis_env = gisenv()

        if mapset and mapset != gis_env['MAPSET']:
            return "{name}@{mapset}".format(name=name, mapset=mapset)
        else:
            return name

    def rename(self, newname):
        """Rename the map"""
        if self.exist():
            functions.rename(self.name, newname, 'rast')
        self._name = newname

    def set_from_rast(self, rastname='', mapset=''):
        """Set the region that will use from a map, if rastername and mapset
        is not specify, use itself.

        call C function `Rast_get_cellhd`"""
        if self.is_open():
            fatal("You cannot change the region if map is open")
            raise
        region = Region()
        if rastname == '':
            rastname = self.name
        if mapset == '':
            mapset = self.mapset

        libraster.Rast_get_cellhd(rastname, mapset,
                                  ctypes.byref(region._region))
        # update rows and cols attributes
        self._rows = libraster.Rast_window_rows()
        self._cols = libraster.Rast_window_cols()

    @must_be_open
    def get_value(self, point, region=None):
        """This method returns the pixel value of a given pair of coordinates:

        Parameters
        ------------

        point = pair of coordinates in tuple object
        """
        if not region:
            region = Region()
        x, y = functions.coor2pixel(point.coords(), region)
        if x < 0 or x > region.cols or y < 0 or y > region.rows:
            return None
        line = self.get_row(int(x))
        return line[int(y)]

    @must_be_open
    def has_cats(self):
        """Return True if the raster map has categories"""
        if self.exist():
            self.cats.read(self)
            self.close()
            if len(self.cats) != 0:
                return True
        return False

    @must_be_open
    def num_cats(self):
        """Return the number of categories"""
        return len(self.cats)

    @must_be_open
    def copy_cats(self, raster):
        """Copy categories from another raster map object"""
        self.cats.copy(raster.cats)

    @must_be_open
    def sort_cats(self):
        """Sort categories order by range"""
        self.cats.sort()

    @must_be_open
    def read_cats(self):
        """Read category from the raster map file"""
        self.cats.read(self)

    @must_be_open
    def write_cats(self):
        """Write category to the raster map file"""
        self.cats.write(self)

    @must_be_open
    def read_cats_rules(self, filename, sep=':'):
        """Read category from the raster map file"""
        self.cats.read_rules(filename, sep)

    @must_be_open
    def write_cats_rules(self, filename, sep=':'):
        """Write category to the raster map file"""
        self.cats.write_rules(filename, sep)

    @must_be_open
    def get_cats(self):
        """Return a category object"""
        cat = Category()
        cat.read(self)
        return cat

    @must_be_open
    def set_cats(self, category):
        """The internal categories are copied from this object."""
        self.cats.copy(category)

    @must_be_open
    def get_cat(self, label):
        """Return a category given an index or a label"""
        return self.cats[label]

    @must_be_open
    def set_cat(self, label, min_cat, max_cat=None, index=None):
        """Set or update a category"""
        self.cats.set_cat(index, (label, min_cat, max_cat))