# -*- coding: utf-8 -*-
"""
Created on Tue Jun 26 12:38:48 2012

@author: pietro
"""
import itertools
import fnmatch
import os
from sqlite3 import OperationalError

import grass.lib.gis as libgis
import grass.lib.raster as libraster
from grass.script import core as grasscore

from grass.pygrass.errors import GrassError
from grass.pygrass.gis.region import Region


def looking(obj, filter_string):
    """
    >>> import grass.lib.vector as libvect
    >>> sorted(looking(libvect, '*by_box*'))  # doctest: +NORMALIZE_WHITESPACE
    ['Vect_select_areas_by_box', 'Vect_select_isles_by_box',
     'Vect_select_lines_by_box', 'Vect_select_nodes_by_box']

    """
    word_list = dir(obj)
    word_list.sort()
    return fnmatch.filter(word_list, filter_string)


def findfiles(dirpath, match=None):
    """Return a list of the files"""
    res = []
    for f in sorted(os.listdir(dirpath)):
        abspath = os.path.join(dirpath, f)
        if os.path.isdir(abspath):
            res.extend(findfiles(abspath, match))

        if match:
            if fnmatch.fnmatch(abspath, match):
                res.append(abspath)
        else:
            res.append(abspath)
    return res


def findmaps(type, pattern=None, mapset='', location='', gisdbase=''):
    """Return a list of tuple contining the names of the:
        * map
        * mapset,
        * location,
        * gisdbase

    """
    from grass.pygrass.gis import Gisdbase, Location, Mapset

    def find_in_location(type, pattern, location):
        res = []
        for msetname in location.mapsets():
            mset = Mapset(msetname, location.name, location.gisdbase)
            res.extend([(m, mset.name, mset.location, mset.gisdbase)
                        for m in mset.glist(type, pattern)])
        return res

    def find_in_gisdbase(type, pattern, gisdbase):
        res = []
        for loc in gisdbase.locations():
            res.extend(find_in_location(type, pattern,
                                        Location(loc, gisdbase.name)))
        return res

    if gisdbase and location and mapset:
        mset = Mapset(mapset, location, gisdbase)
        return [(m, mset.name, mset.location, mset.gisdbase)
                for m in mset.glist(type, pattern)]
    elif gisdbase and location:
        loc = Location(location, gisdbase)
        return find_in_location(type, pattern, loc)
    elif gisdbase:
        gis = Gisdbase(gisdbase)
        return find_in_gisdbase(type, pattern, gis)
    elif location:
        loc = Location(location)
        return find_in_location(type, pattern, loc)
    elif mapset:
        mset = Mapset(mapset)
        return [(m, mset.name, mset.location, mset.gisdbase)
                for m in mset.glist(type, pattern)]
    else:
        gis = Gisdbase()
        return find_in_gisdbase(type, pattern, gis)


def remove(oldname, maptype, **kwargs):
    """Remove a map"""
    kwargs.update({maptype: '{old}'.format(old=oldname)})
    grasscore.run_command('g.remove', quiet=True, **kwargs)


def rename(oldname, newname, maptype, **kwargs):
    """Rename a map"""
    kwargs.update({maptype: '{old},{new}'.format(old=oldname, new=newname), })
    grasscore.run_command('g.rename', quiet=True, **kwargs)


def copy(existingmap, newmap, maptype, **kwargs):
    """Copy a map

    >>> copy('census', 'mycensus', 'vect')
    >>> rename('mycensus', 'mynewcensus', 'vect')
    >>> remove('mynewcensus', 'vect')

    """
    kwargs.update({maptype: '{old},{new}'.format(old=existingmap, new=newmap)})
    grasscore.run_command('g.copy', quiet=True, **kwargs)


def getenv(env):
    """Return the current grass environment variables ::

        >>> getenv("MAPSET")
        'user1'

    """
    return libgis.G__getenv(env)


def get_mapset_raster(mapname, mapset=''):
    """Return the mapset of the raster map ::

    >>> get_mapset_raster('elevation')
    'PERMANENT'

    """
    return libgis.G_find_raster2(mapname, mapset)


def get_mapset_vector(mapname, mapset=''):
    """Return the mapset of the vector map ::

    >>> get_mapset_vector('census')
    'PERMANENT'

    """
    return libgis.G_find_vector2(mapname, mapset)


def is_clean_name(name):
    """Return if the name is valid ::

    >>> is_clean_name('census')
    True
    >>> is_clean_name('0census')
    False
    >>> is_clean_name('census&')
    False

    """
    if name[0].isdigit():
        return False
    for char in ' @#^?°,;%&/':
        if name.find(char) != -1:
            return False
    return True


def coor2pixel((east, north), region):
    """Convert coordinates into a pixel row and col ::

        >>> reg = Region()
        >>> coor2pixel((reg.west, reg.north), reg)
        (0.0, 0.0)
        >>> coor2pixel((reg.east, reg.south), reg) == (reg.rows, reg.cols)
        True
    """
    return (libraster.Rast_northing_to_row(north, region.c_region),
            libraster.Rast_easting_to_col(east, region.c_region))


def pixel2coor((col, row), region):
    """Convert row and col of a pixel into a coordinates ::

        >>> reg = Region()
        >>> pixel2coor((0, 0), reg) == (reg.north, reg.west)
        True
        >>> pixel2coor((reg.cols, reg.rows), reg) == (reg.south, reg.east)
        True
    """
    return (libraster.Rast_row_to_northing(row, region.c_region),
            libraster.Rast_col_to_easting(col, region.c_region))


def get_raster_for_points(poi_vector, raster, column=None):
    """Query a raster map for each point feature of a vector

    Example ::

        >>> from grass.pygrass.vector import VectorTopo
        >>> from grass.pygrass.raster import RasterRow
        >>> ele = RasterRow('elevation')
        >>> copy('schools','myschools','vect')
        >>> sch = VectorTopo('myschools')
        >>> get_raster_for_points(sch, ele)               # doctest: +ELLIPSIS
        [(1, 633649.2856743174, 221412.94434781274, 145.06602)...
        >>> sch.table.columns.add('elevation','double precision')
        >>> 'elevation' in sch.table.columns
        True
        >>> get_raster_for_points(sch, ele, 'elevation')
        True
        >>> sch.table.filters.select('NAMESHORT','elevation')
        Filters('SELECT NAMESHORT, elevation FROM myschools;')
        >>> cur = sch.table.execute()
        >>> cur.fetchall()                                # doctest: +ELLIPSIS
        [(u'SWIFT CREEK', 145.06602), ... (u'9TH GRADE CTR', None)]
        >>> remove('myschools','vect')


    Parameters
    -------------

    point: point vector object

    raster: raster object

    column: column name to update
    """
    from math import isnan
    if not column:
        result = []
    reg = Region()
    if not poi_vector.is_open():
        poi_vector.open()
    if not raster.is_open():
        raster.open()
    if poi_vector.num_primitive_of('point') == 0:
        raise GrassError(_("Vector doesn't contain points"))
    for poi in poi_vector.viter('points'):
        val = raster.get_value(poi, reg)
        if column:
            if val is not None and not isnan(val):
                poi.attrs[column] = val
        else:
            if val is not None and not isnan(val):
                result.append((poi.id, poi.x, poi.y, val))
            else:
                result.append((poi.id, poi.x, poi.y, None))
    if not column:
        return result
    else:
        poi.attrs.commit()
        return True


def r_export(rast, output='', fmt='png', **kargs):
    from grass.pygrass.modules import Module
    if rast.exist():
        output = output if output else "%s_%s.%s" % (rast.name, rast.mapset,
                                                     fmt)
        Module('r.out.%s' % fmt, input=rast.fullname(), output=output,
               overwrite=True, **kargs)
        return output
    else:
        raise ValueError('Raster map does not exist.')


def get_lib_path(modname, libname):
    """Return the path of the libname contained in the module. ::

        >>> get_lib_path(modname='r.modis', libname='libmodis')
    """
    from os.path import isdir, join
    from os import getenv

    if isdir(join(getenv('GISBASE'), 'etc', modname)):
        path = join(os.getenv('GISBASE'), 'etc', modname)
    elif getenv('GRASS_ADDON_BASE') and \
            isdir(join(getenv('GRASS_ADDON_BASE'), 'etc', modname)):
        path = join(getenv('GRASS_ADDON_BASE'), 'etc', modname)
    elif getenv('GRASS_ADDON_BASE') and \
            isdir(join(getenv('GRASS_ADDON_BASE'), modname, modname)):
        path = join(os.getenv('GRASS_ADDON_BASE'), modname, modname)
    elif isdir(join('..', libname)):
        path = join('..', libname)
    else:
        path = None
    return path


def split_in_chunk(iterable, lenght=10):
    """Split a list in chunk.

    >>> for chunk in split_in_chunk(range(25)): print chunk
    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    [10, 11, 12, 13, 14, 15, 16, 17, 18, 19]
    [20, 21, 22, 23, 24]
    >>> for chunk in split_in_chunk(range(25), 3): print chunk
    [0, 1, 2]
    [3, 4, 5]
    [6, 7, 8]
    [9, 10, 11]
    [12, 13, 14]
    [15, 16, 17]
    [18, 19, 20]
    [21, 22, 23]
    [24]
    """
    it = iter(iterable)
    while True:
        chunk = tuple(itertools.islice(it, lenght))
        if not chunk:
            return
        yield chunk


def table_exist(cursor, table_name):
    """Return True if the table exist False otherwise"""
    try:
        # sqlite
        cursor.execute("SELECT name FROM sqlite_master"
                       " WHERE type='table' AND name='%s';" % table_name)
    except OperationalError:
        try:
            # pg
            cursor.execute("SELECT EXISTS(SELECT * FROM "
                           "information_schema.tables "
                           "WHERE table_name=%s)" % table_name)
        except OperationalError:
            return False
    one = cursor.fetchone() if cursor else None
    return True if one and one[0] else False
