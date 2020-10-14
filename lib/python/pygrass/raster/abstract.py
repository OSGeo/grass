# -*- coding: utf-8 -*-
"""
Created on Fri Aug 17 16:05:25 2012

@author: pietro
"""
from __future__ import (nested_scopes, generators, division, absolute_import,
                        with_statement, print_function, unicode_literals)
import ctypes

#
# import GRASS modules
#
from grass.script import fatal, gisenv
import grass.lib.gis as libgis
import grass.lib.raster as libraster

#
# import pygrass modules
#
from grass.pygrass import utils
from grass.pygrass.gis.region import Region
from grass.pygrass.errors import must_be_open, must_be_in_current_mapset
from grass.pygrass.shell.conversion import dict2html
from grass.pygrass.shell.show import raw_figure

#
# import raster classes
#
from grass.pygrass.raster.raster_type import TYPE as RTYPE, RTYPE_STR
from grass.pygrass.raster.category import Category
from grass.pygrass.raster.history import History

test_raster_name = "abstract_test_map"

# Define global variables to not exceed the 80 columns
INDXOUTRANGE = "The index (%d) is out of range, have you open the map?."
INFO = """{name}@{mapset}
rows: {rows}
cols: {cols}
north: {north} south: {south} nsres:{nsres}
east:  {east} west: {west} ewres:{ewres}
range: {min}, {max}
proj: {proj}
"""


class Info(object):
    def __init__(self, name, mapset=''):
        """Read the information for a raster map. ::

            >>> info = Info(test_raster_name)
            >>> info.read()
            >>> info          # doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
            abstract_test_map@
            rows: 4
            cols: 4
            north: 40.0 south: 0.0 nsres:10.0
            east:  40.0 west: 0.0 ewres:10.0
            range: 11, 44
            ...
            <BLANKLINE>

        """
        self.name = name
        self.mapset = mapset
        self.c_region = ctypes.pointer(libraster.struct_Cell_head())
        self.c_range = None

    def _get_range(self):
        if self.mtype == 'CELL':
            self.c_range = ctypes.pointer(libraster.Range())
            libraster.Rast_read_range(self.name, self.mapset, self.c_range)
        else:
            self.c_range = ctypes.pointer(libraster.FPRange())
            libraster.Rast_read_fp_range(self.name, self.mapset, self.c_range)

    def _get_raster_region(self):
        libraster.Rast_get_cellhd(self.name, self.mapset, self.c_region)

    def read(self):
        self._get_range()
        self._get_raster_region()

    @property
    def north(self):
        return self.c_region.contents.north

    @property
    def south(self):
        return self.c_region.contents.south

    @property
    def east(self):
        return self.c_region.contents.east

    @property
    def west(self):
        return self.c_region.contents.west

    @property
    def top(self):
        return self.c_region.contents.top

    @property
    def bottom(self):
        return self.c_region.contents.bottom

    @property
    def rows(self):
        return self.c_region.contents.rows

    @property
    def cols(self):
        return self.c_region.contents.cols

    @property
    def nsres(self):
        return self.c_region.contents.ns_res

    @property
    def ewres(self):
        return self.c_region.contents.ew_res

    @property
    def tbres(self):
        return self.c_region.contents.tb_res

    @property
    def zone(self):
        return self.c_region.contents.zone

    @property
    def proj(self):
        return self.c_region.contents.proj

    @property
    def min(self):
        if self.c_range is None:
            return None
        return self.c_range.contents.min

    @property
    def max(self):
        if self.c_range is None:
            return None
        return self.c_range.contents.max

    @property
    def range(self):
        if self.c_range is None:
            return None, None
        return self.c_range.contents.min, self.c_range.contents.max

    @property
    def mtype(self):
        return RTYPE_STR[libraster.Rast_map_type(self.name, self.mapset)]

    def _get_band_reference(self):
        """Get band reference identifier.

        :return str: band identifier (eg. S2_1) or None
        """
        band_ref = None
        p_filename = ctypes.c_char_p()
        p_band_ref = ctypes.c_char_p()
        ret = libraster.Rast_read_band_reference(self.name, self.mapset,
                                                 ctypes.byref(p_filename),
                                                 ctypes.byref(p_band_ref))
        if ret:
            band_ref = utils.decode(p_band_ref.value)
            libgis.G_free(p_filename)
            libgis.G_free(p_band_ref)

        return band_ref

    @must_be_in_current_mapset
    def _set_band_reference(self, band_reference):
        """Set/Unset band reference identifier.

        :param str band_reference: band reference to assign or None to remove (unset)
        """
        if band_reference:
            # assign
            from grass.bandref import BandReferenceReader, BandReferenceReaderError
            reader = BandReferenceReader()
            # determine filename (assuming that band_reference is unique!)
            try:
                filename = reader.find_file(band_reference)
            except BandReferenceReaderError as e:
                fatal("{}".format(e))
                raise
            if not filename:
                fatal("Band reference <{}> not found".format(band_reference))
                raise

            # write band reference
            libraster.Rast_write_band_reference(self.name,
                                                filename,
                                                band_reference)
        else:
            libraster.Rast_remove_band_reference(self.name)

    band_reference = property(fget=_get_band_reference, fset=_set_band_reference)

    def _get_units(self):
        return libraster.Rast_read_units(self.name, self.mapset)

    def _set_units(self, units):
        libraster.Rast_write_units(self.name, units)

    units = property(_get_units, _set_units)

    def _get_vdatum(self):
        return libraster.Rast_read_vdatum(self.name, self.mapset)

    def _set_vdatum(self, vdatum):
        libraster.Rast_write_vdatum(self.name, vdatum)

    vdatum = property(_get_vdatum, _set_vdatum)

    def __repr__(self):
        return INFO.format(name=self.name, mapset=self.mapset,
                           rows=self.rows, cols=self.cols,
                           north=self.north, south=self.south,
                           east=self.east, west=self.west,
                           top=self.top, bottom=self.bottom,
                           nsres=self.nsres, ewres=self.ewres,
                           tbres=self.tbres, zone=self.zone,
                           proj=self.proj, min=self.min, max=self.max)

    def keys(self):
        return ['name', 'mapset', 'rows', 'cols', 'north', 'south',
                'east', 'west', 'top', 'bottom', 'nsres', 'ewres', 'tbres',
                'zone', 'proj', 'min', 'max']

    def items(self):
        return [(k, self.__getattribute__(k)) for k in self.keys()]
        
    def __iter__(self):
        return ((k, self.__getattribute__(k)) for k in self.keys())
        
    def _repr_html_(self):
        return dict2html(dict(self.items()), keys=self.keys(),
                         border='1', kdec='b')


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

    def __init__(self, name, mapset="", *aopen, **kwopen):
        """The constructor need at least the name of the map
        *optional* field is the `mapset`.

        >>> ele = RasterAbstractBase(test_raster_name)
        >>> ele.name
        'abstract_test_map'
        >>> ele.exist()
        True

        ..
        """
        self.mapset = mapset
        if not mapset:
            # note that @must_be_in_current_mapset requires mapset to be set
            mapset = libgis.G_find_raster(name, mapset)
            if mapset is not None:
                self.mapset = utils.decode(mapset)

        self._name = name
        # Private attribute `_fd` that return the file descriptor of the map
        self._fd = None
        # Private attribute `_rows` that return the number of rows
        # in active window, When the class is instanced is empty and it is set
        # when you open the file, using Rast_window_rows()
        self._rows = None
        # Private attribute `_cols` that return the number of rows
        # in active window, When the class is instanced is empty and it is set
        # when you open the file, using Rast_window_cols()
        self._cols = None
        # self.region = Region()
        self.hist = History(self.name, self.mapset)
        self.cats = Category(self.name, self.mapset)
        self.info = Info(self.name, self.mapset)
        self._aopen = aopen
        self._kwopen = kwopen
        self._mtype = 'CELL'
        self._mode = 'r'
        self._overwrite = False

    def __enter__(self):
        self.open(*self._aopen, **self._kwopen)
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    def _get_mtype(self):
        """Private method to get the Raster type"""
        return self._mtype

    def _set_mtype(self, mtype):
        """Private method to change the Raster type"""
        if mtype.upper() not in ('CELL', 'FCELL', 'DCELL'):
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
        if not utils.is_clean_name(newname):
            str_err = _("Map name {0} not valid")
            raise ValueError(str_err.format(newname))
        if self.exist():
            self.rename(newname)
        self._name = newname

    name = property(fget=_get_name, fset=_set_name)

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
            # Get the start, stop, and step from the slice
            return (self.get_row(ii) for ii in range(*key.indices(len(self))))
        elif isinstance(key, tuple):
            x, y = key
            return self.get(x, y)
        elif isinstance(key, int):
            if not self.is_open():
                raise IndexError("Can not operate on a closed map. Call open() first.")
            if key < 0:  # Handle negative indices
                key += self._rows
            if key >= self._rows:
                raise IndexError("The row index {0} is out of range [0, {1}).".format(key, self._rows))
            return self.get_row(key)
        else:
            fatal("Invalid argument type.")

    def __iter__(self):
        """Return a constructor of the class"""
        return (self.__getitem__(irow) for irow in range(self._rows))

    def _repr_png_(self):
        return raw_figure(utils.r_export(self))

    def exist(self):
        """Return True if the map already exist, and
        set the mapset if were not set.

        call the C function `G_find_raster`.

        >>> ele = RasterAbstractBase(test_raster_name)
        >>> ele.exist()
        True
        """
        if self.name:
            if self.mapset == '':
                mapset = utils.get_mapset_raster(self.name, self.mapset)
                self.mapset = mapset if mapset else ''
                return True if mapset else False
            return bool(utils.get_mapset_raster(self.name, self.mapset))
        else:
            return False

    def is_open(self):
        """Return True if the map is open False otherwise.

        >>> ele = RasterAbstractBase(test_raster_name)
        >>> ele.is_open()
        False

        """
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
        utils.remove(self.name, 'rast')

    def fullname(self):
        """Return the full name of a raster map: name@mapset"""
        return "{name}@{mapset}".format(name=self.name, mapset=self.mapset)

    def name_mapset(self, name=None, mapset=None):
        """Return the full name of the Raster.

        >>> ele = RasterAbstractBase(test_raster_name)
        >>> name = ele.name_mapset().split("@")
        >>> name
        ['abstract_test_map']

        """
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
            utils.rename(self.name, newname, 'rast')
        self._name = newname

    def set_region_from_rast(self, rastname='', mapset=''):
        """Set the computational region from a map,
           if rastername and mapset is not specify, use itself.
           This region will be used by all
           raster map layers that are opened in the same process.

           The GRASS region settings will not be modified.

           call C function `Rast_get_cellhd`, `Rast_set_window`

           """
        if self.is_open():
            fatal("You cannot change the region if map is open")
            raise
        region = Region()
        if rastname == '':
            rastname = self.name
        if mapset == '':
            mapset = self.mapset

        libraster.Rast_get_cellhd(rastname, mapset,
                                  region.byref())
        self._set_raster_window(region)

    def set_region(self, region):
        """Set the computational region that can be different from the
           current region settings. This region will be used by all
           raster map layers that are opened in the same process.

           The GRASS region settings will not be modified.
        """
        if self.is_open():
            fatal("You cannot change the region if map is open")
            raise
        self._set_raster_window(region)

    def _set_raster_window(self, region):
        libraster.Rast_set_window(region.byref())
        # update rows and cols attributes
        self._rows = libraster.Rast_window_rows()
        self._cols = libraster.Rast_window_cols()

    @must_be_open
    def get_value(self, point, region=None):
        """This method returns the pixel value of a given pair of coordinates:

        :param point: pair of coordinates in tuple object or class object with coords() method
        """
        # Check for tuple
        if not isinstance(point, list) and not isinstance(point, tuple):
            point = point.coords()

        if not region:
            region = Region()
        row, col = utils.coor2pixel(point, region)
        if col < 0 or col > region.cols or row < 0 or row > region.rows:
            return None
        line = self.get_row(int(row))
        return line[int(col)]

    @must_be_open
    def has_cats(self):
        """Return True if the raster map has categories"""
        if self.exist():
            self.cats.read()
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
        cat = Category(name=self.name, mapset=self.mapset)
        cat.read()
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

if __name__ == "__main__":

    import doctest
    from grass.pygrass.modules import Module
    Module("g.region", n=40, s=0, e=40, w=0, res=10)
    Module("r.mapcalc", expression="%s = row() + (10 * col())" % (test_raster_name),
        overwrite=True)

    doctest.testmod()

    """Remove the generated vector map, if exist"""
    mset = utils.get_mapset_raster(test_raster_name, mapset='')
    if mset:
        Module("g.remove", flags='f', type='raster', name=test_raster_name)
