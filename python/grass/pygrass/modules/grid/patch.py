# -*- coding: utf-8 -*-
"""
Created on Tue Apr  2 18:57:42 2013

@author: pietro
"""
from __future__ import (nested_scopes, generators, division, absolute_import,
                        with_statement, print_function, unicode_literals)
from grass.pygrass.gis.region import Region
from grass.pygrass.raster import RasterRow
from grass.pygrass.utils import coor2pixel


def get_start_end_index(bbox_list):
    """Convert a Bounding Box to a list of the index of
    column start, end, row start and end

    :param bbox_list: a list of BBox object to convert
    :type bbox_list: list of BBox object

    """
    ss_list = []
    reg = Region()
    for bbox in bbox_list:
        r_start, c_start = coor2pixel((bbox.west, bbox.north), reg)
        r_end, c_end = coor2pixel((bbox.east, bbox.south), reg)
        ss_list.append((int(r_start), int(r_end), int(c_start), int(c_end)))
    return ss_list


def rpatch_row(rast, rasts, bboxes):
    """Patch a row of bound boxes.

    :param rast: a Raster object to write
    :type rast: Raster object
    :param rasts: a list of Raster object to read
    :type rasts: list of Raster object
    :param bboxes: a list of BBox object
    :type bboxes: list of BBox object
    """
    sei = get_start_end_index(bboxes)
    # instantiate two buffer
    buff = rasts[0][0]
    rbuff = rasts[0][0]
    r_start, r_end, c_start, c_end = sei[0]
    for row in range(r_start, r_end):
        for col, ras in enumerate(rasts):
            r_start, r_end, c_start, c_end = sei[col]
            buff = ras.get_row(row, buff)
            rbuff[c_start:c_end] = buff[c_start:c_end]
        rast.put_row(rbuff)


def rpatch_map(raster, mapset, mset_str, bbox_list, overwrite=False,
               start_row=0, start_col=0, prefix=''):
    # TODO is prefix useful??
    """Patch raster using a bounding box list to trim the raster.

    :param raster: the name of output raster
    :type raster: str
    :param mapset: the name of mapset to use
    :type mapset: str
    :param mset_str:
    :type mset_str: str
    :param bbox_list: a list of BBox object to convert
    :type bbox_list: list of BBox object
    :param overwrite: overwrite existing raster
    :type overwrite: bool
    :param start_row: the starting row of original raster
    :type start_row: int
    :param start_col: the starting column of original raster
    :type start_col: int
    :param prefix: the prefix of output raster
    :type prefix: str
    """
    # Instantiate the RasterRow input objects
    rast = RasterRow(prefix + raster, mapset)
    rtype = RasterRow(name=raster, mapset=mset_str % (0, 0))
    rtype.open('r')
    rast.open('w', mtype=rtype.mtype, overwrite=overwrite)
    rtype.close()
    rasts = []
    for row, rbbox in enumerate(bbox_list):
        rrasts = []
        for col in range(len(rbbox)):
            rrasts.append(RasterRow(name=raster,
                                    mapset=mset_str % (start_row + row,
                                                       start_col + col)))
            rrasts[-1].open('r')
        rasts.append(rrasts)
        rpatch_row(rast, rrasts, rbbox)

        for rst in rrasts:
            rst.close()
            del(rst)

    rast.close()
