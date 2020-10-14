# -*- coding: utf-8 -*-
"""
Created on Tue Apr  2 19:00:15 2013

@author: pietro
"""
from __future__ import (nested_scopes, generators, division, absolute_import,
                        with_statement, print_function, unicode_literals)
from grass.pygrass.gis.region import Region
from grass.pygrass.vector.basic import Bbox


def get_bbox(reg, row, col, width, height, overlap):
    """Return a Bbox

    :param reg: a Region object to split
    :type reg: Region object
    :param row: the number of row
    :type row: int
    :param col: the number of row
    :type col: int
    :param width: the width of tiles
    :type width: int
    :param height: the width of tiles
    :type height: int
    :param overlap: the value of overlap between tiles
    :type overlap: int
    """
    north = reg.north - (row * height - overlap) * reg.nsres
    south = reg.north - ((row + 1) * height + overlap) * reg.nsres
    east = reg.west + ((col + 1) * width + overlap) * reg.ewres
    west = reg.west + (col * width - overlap) * reg.ewres
    return Bbox(north=north if north <= reg.north else reg.north,
                south=south if south >= reg.south else reg.south,
                east=east if east <= reg.east else reg.east,
                west=west if west >= reg.west else reg.west,)


def split_region_tiles(region=None, width=100, height=100, overlap=0):
    """Spit a region into a list of Bbox.

    :param region: a Region object to split
    :type region: Region object
    :param width: the width of tiles
    :type width: int
    :param height: the width of tiles
    :type height: int
    :param overlap: the value of overlap between tiles
    :type overlap: int

    >>> reg = Region()
    >>> reg.north = 1350
    >>> reg.south = 0
    >>> reg.nsres = 1
    >>> reg.east = 1500
    >>> reg.west = 0
    >>> reg.ewres = 1
    >>> reg.cols
    1500
    >>> reg.rows
    1350
    >>> split_region_tiles(region=reg, width=1000, height=700, overlap=0) # doctest: +NORMALIZE_WHITESPACE
    [[Bbox(1350.0, 650.0, 1000.0, 0.0), Bbox(1350.0, 650.0, 1500.0, 1000.0)],
     [Bbox(650.0, 0.0, 1000.0, 0.0), Bbox(650.0, 0.0, 1500.0, 1000.0)]]
    >>> split_region_tiles(region=reg, width=1000, height=700, overlap=10) # doctest: +NORMALIZE_WHITESPACE
    [[Bbox(1350.0, 640.0, 1010.0, 0.0), Bbox(1350.0, 640.0, 1500.0, 990.0)],
     [Bbox(660.0, 0.0, 1010.0, 0.0), Bbox(660.0, 0.0, 1500.0, 990.0)]]
    """
    reg = region if region else Region()
    ncols = (reg.cols + width - 1) // width
    nrows = (reg.rows + height - 1) // height
    box_list = []
    #print reg
    for row in range(nrows):
        row_list = []
        for col in range(ncols):
            #print 'c', c, 'r', r
            row_list.append(get_bbox(reg, row, col, width, height, overlap))
        box_list.append(row_list)
    return box_list


def get_overlap_region_tiles(region=None, width=100, height=100, overlap=0):
    """Get the Bbox of the overlapped region.

    :param region: a Region object to split
    :type region: Region object
    :param width: the width of tiles
    :type width: int
    :param height: the width of tiles
    :type height: int
    :param overlap: the value of overlap between tiles
    :type overlap: int
    """
    reg = region if region else Region()
    ncols = (reg.cols + width - 1) // width
    nrows = (reg.rows + height - 1) // height
    box_list = []
    #print reg
    for row in range(nrows):
        row_list = []
        for col in range(ncols):
            #print 'c', c, 'r', r
            row_list.append(get_bbox(reg, row, col, width, height, -overlap))
        box_list.append(row_list)
    return box_list
