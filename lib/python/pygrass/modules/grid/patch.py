# -*- coding: utf-8 -*-
"""
Created on Tue Apr  2 18:57:42 2013

@author: pietro
"""
from grass.pygrass.gis.region import Region
from grass.pygrass.raster import RasterRow
from grass.pygrass.functions import coor2pixel


def get_start_end_index(bbox_list):
    """Convert a Bounding Box to a list of the index of
    column start, end, row start and end
    """
    ss_list = []
    reg = Region()
    for bbox in bbox_list:
        r_start, c_start = coor2pixel((bbox.west, bbox.north), reg)
        r_end, c_end = coor2pixel((bbox.east, bbox.south), reg)
        ss_list.append((int(r_start), int(r_end), int(c_start), int(c_end)))
    return ss_list


def patch_row(rast, rasts, bboxes):
    """Patch a row of bound boxes."""
    sei = get_start_end_index(bboxes)
    # instantiate two buffer
    buff = rasts[0][0]
    rbuff = rasts[0][0]
    r_start, r_end, c_start, c_end = sei[0]
    for row in xrange(r_start, r_end):
        for col, ras in enumerate(rasts):
            r_start, r_end, c_start, c_end = sei[col]
            buff = ras.get_row(row, buff)
            rbuff[c_start:c_end] = buff[c_start:c_end]
        rast.put_row(rbuff)


def patch_map(raster, mapset, mset_str, bbox_list, overwrite=False,
              start_row=0, start_col=0):
    """Patch raster using a bounding box list to trim the raster."""
    # Instantiate the RasterRow input objects
    rast = RasterRow(raster, mapset)
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
        patch_row(rast, rrasts, rbbox)

    for rrast in rasts:
        for rast_ in rrast:
            rast_.close()
    rast.close()
    #import ipdb; ipdb.set_trace()
