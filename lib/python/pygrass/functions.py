# -*- coding: utf-8 -*-
"""
Created on Tue Jun 26 12:38:48 2012

@author: pietro
"""

import fnmatch

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


def remove(oldname, maptype):
    """Remove a map"""
    grasscore.run_command('g.remove', quiet=True,
                          **{maptype: '{old}'.format(old=oldname)})


def rename(oldname, newname, maptype):
    """Rename a map"""
    grasscore.run_command('g.rename', quiet=True,
                          **{maptype: '{old},{new}'.format(old=oldname,
                                                           new=newname), })


def copy(existingmap, newmap, maptype):
    """Copy a map

    >>> copy('census', 'mycensus', 'vect')
    >>> rename('mycensus', 'mynewcensus', 'vect')
    >>> remove('mynewcensus', 'vect')

    """
    grasscore.run_command('g.copy', quiet=True,
                          **{maptype: '{old},{new}'.format(old=existingmap,
                                                           new=newmap), })


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
    return libgis.G_find_raster(mapname, '')


def get_mapset_vector(mapname, mapset=''):
    """Return the mapset of the vector map ::

    >>> get_mapset_vector('census')
    'PERMANENT'

    """
    return libgis.G_find_vector(mapname, '')


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
