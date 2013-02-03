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


def remove(**kargs):
    grasscore.run_command('g.remove', **kargs)


def rename(oldname, newname, maptype):
    grasscore.run_command('g.rename',
                          **{maptype: '{old},{new}'.format(old=oldname,
                                                           new=newname), })


def copy(existingmap, newmap, maptype):
    grasscore.run_command('g.copy',
                          **{maptype: '{old},{new}'.format(old=existingmap,
                                                           new=newmap), })


def getenv(env):
    """Return the current grass environment variables:

        >>> getenv("MAPSET")
        'user1'

    .."""
    return libgis.G__getenv(env)


def get_mapset_raster(mapname, mapset=''):
    return libgis.G_find_raster(mapname, '')


def get_mapset_vector(mapname, mapset=''):
    return libgis.G_find_vector(mapname, '')


def exist(mapname, mapset=''):
    mapset = get_mapset_raster(mapname, mapset)
    if mapset != '':
        return True
    else:
        mapset = get_mapset_vector(mapname, mapset)
        if mapset:
            return True
    return False


def clean_map_name(name):
    name.strip()
    for char in ' @#^?°,;%&/':
        name = name.replace(char, '')
    return name


def coor2pixel((east, north), region):
    """Convert coordinates into a pixel row and col ::

        >>> reg = Region()
        >>> coor2pixel((reg.west, reg.north), reg)
        (0.0, 0.0)
        >>> coor2pixel((reg.east, reg.south), reg) == (reg.cols, reg.rows)
        True
    """
    return (libraster.Rast_northing_to_row(north, region.c_region),
            libraster.Rast_easting_to_col(east, region.c_region))


def pixel2coor((col, row), region):
    """Convert row and col of a pixel into a coordinates ::

        >>> reg = Region()
        >>> pixel2coor((0, 0), reg) == (reg.north, reg.west)
        True
        >>> pixel2coor((reg.cols, reg.rows), reg) == (reg.east, reg.south)
        True
    """
    return (libraster.Rast_row_to_northing(row, region.c_region),
            libraster.Rast_col_to_easting(col, region.c_region))


def get_raster_for_points(point, raster):
    """Query a raster map for each point feature of a vector

    Parameters
    -------------

    point: point vector object

    raster: raster object
    """
    reg = Region()
    if not point.is_open():
        point.open()
    if point.num_primitive_of('point') == 0:
        raise GrassError(_("Vector doesn't contain points"))
    values = [raster.get_value(poi.coords, reg) for poi in point.viter('point')]
    return values
