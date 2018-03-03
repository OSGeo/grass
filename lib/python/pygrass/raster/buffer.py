# -*- coding: utf-8 -*-
from grass.pygrass.raster.raster_type import TYPE as RTYPE
import ctypes
import numpy as np


_CELL = ('int', 'int0', 'int8', 'int16', 'int32', 'int64')
CELL = tuple([getattr(np, attr) for attr in _CELL if hasattr(np, attr)])
_FCELL = 'float', 'float16', 'float32'
FCELL = tuple([getattr(np, attr) for attr in _FCELL if hasattr(np, attr)])
_DCELL = 'float64', 'float128'
DCELL = tuple([getattr(np, attr) for attr in _DCELL if hasattr(np, attr)])


class Buffer(np.ndarray):
    """shape, mtype='FCELL', buffer=None, offset=0,
    strides=None, order=None
    """
    @property
    def mtype(self):
        if self.dtype in CELL:
            return 'CELL'
        elif self.dtype in FCELL:
            return 'FCELL'
        elif self.dtype in DCELL:
            return DCELL
        else:
            err = "Raster type: %r not supported by GRASS."
            raise TypeError(err % self.dtype)

    def __new__(cls, shape, mtype='FCELL', buffer=None, offset=0,
                strides=None, order=None):
        obj = np.ndarray.__new__(cls, shape, RTYPE[mtype]['numpy'],
                                 buffer, offset, strides, order)
        obj.pointer_type = ctypes.POINTER(RTYPE[mtype]['ctypes'])
        obj.p = obj.ctypes.data_as(obj.pointer_type)
        return obj

    def __array_finalize__(self, obj):
        if obj is None:
            return
        self.pointer_type = getattr(obj, 'pointer_type', None)
        self.p = getattr(obj, 'p', None)

    def __array_wrap__(self, out_arr, context=None):
        """See:
        http://docs.scipy.org/doc/numpy/user/
        basics.subclassing.html#array-wrap-for-ufuncs"""
        if out_arr.dtype == np.bool:
            # there is not support for boolean maps, so convert into integer
            out_arr = out_arr.astype(np.int32)
        out_arr.p = out_arr.ctypes.data_as(out_arr.pointer_type)
        return np.ndarray.__array_wrap__(self, out_arr, context)
