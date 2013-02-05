# -*- coding: utf-8 -*-
"""
Created on Mon Jun 18 13:22:38 2012

@author: pietro
"""
import ctypes

import grass.lib.rowio as librowio
import grass.lib.raster as librast

from grass.pygrass.errors import GrassError
from raster_type import TYPE as RTYPE


CMPFUNC = ctypes.CFUNCTYPE(ctypes.c_int,
                           ctypes.c_int, ctypes.c_void_p,
                           ctypes.c_int, ctypes.c_int)


def getmaprow_CELL(fd, buf, row, l):
    librast.Rast_get_c_row(fd, ctypes.cast(buf, ctypes.POINTER(librast.CELL)),
                           row)
    return 1


def getmaprow_FCELL(fd, buf, row, l):
    librast.Rast_get_f_row(fd, ctypes.cast(buf, ctypes.POINTER(librast.FCELL)),
                           row)
    return 1


def getmaprow_DCELL(fd, buf, row, l):
    librast.Rast_get_d_row(fd, ctypes.cast(buf, ctypes.POINTER(librast.DCELL)),
                           row)
    return 1

get_row = {
    'CELL':  CMPFUNC(getmaprow_CELL),
    'FCELL': CMPFUNC(getmaprow_FCELL),
    'DCELL': CMPFUNC(getmaprow_DCELL),
}


class RowIO(object):

    def __init__(self):
        self.c_rowio = librowio.ROWIO()
        self.fd = None
        self.rows = None
        self.cols = None
        self.mtype = None
        self.row_size = None

    def open(self, fd, rows, cols, mtype):
        self.fd = fd
        self.rows = rows
        self.cols = cols
        self.mtype = mtype
        self.row_size = ctypes.sizeof(RTYPE[mtype]['grass def'] * cols)
        if (librowio.Rowio_setup(ctypes.byref(self.c_rowio), self.fd,
                                 self.rows,
                                 self.row_size,
                                 get_row[self.mtype],
                                 get_row[self.mtype]) == -1):
            raise GrassError('Fatal error, Rowio not setup correctly.')

    def release(self):
        librowio.Rowio_release(ctypes.byref(self.c_rowio))
        self.fd = None
        self.rows = None
        self.cols = None
        self.mtype = None

    def get(self, row_index, buf):
        rowio_buf = librowio.Rowio_get(ctypes.byref(self.c_rowio), row_index)
        ctypes.memmove(buf.p, rowio_buf, self.row_size)
        return buf
