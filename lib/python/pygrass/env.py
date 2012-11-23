# -*- coding: utf-8 -*-
"""
Created on Tue Jun 26 12:38:48 2012

@author: pietro
"""

import fnmatch

import grass.lib.gis as libgis
import grass.lib.raster as libraster
from grass.script import core as grasscore


def looking(filter_string, obj):
    """
    >>> import grass.lib.vector as libvect
    >>> sorted(looking('*by_box*', libvect))  # doctest: +NORMALIZE_WHITESPACE
    ['Vect_select_areas_by_box', 'Vect_select_isles_by_box',
     'Vect_select_lines_by_box', 'Vect_select_nodes_by_box']

    """
    word_list = [i for i in dir(obj)]
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

        >>> from pygrass.region import Region
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

        >>> from pygrass.region import Region
        >>> reg = Region()
        >>> pixel2coor((0, 0), reg) == (reg.north, reg.west)
        True
        >>> pixel2coor((reg.cols, reg.rows), reg) == (reg.east, reg.south)
        True
    """
    return (libraster.Rast_row_to_northing(row, region.c_region),
            libraster.Rast_col_to_easting(col, region.c_region))