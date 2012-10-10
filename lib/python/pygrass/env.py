# -*- coding: utf-8 -*-
"""
Created on Tue Jun 26 12:38:48 2012

@author: pietro
"""
import grass.lib.gis as libgis
import fnmatch
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