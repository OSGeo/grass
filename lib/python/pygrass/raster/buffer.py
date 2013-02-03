# -*- coding: utf-8 -*-
"""
Created on Fri Jun  8 18:46:34 2012

@author: pietro
"""
from raster_type import TYPE as RTYPE
import ctypes
import numpy as np


class Buffer(np.ndarray):
    """shape, mtype='FCELL', buffer=None, offset=0,
    strides=None, order=None
    """

    def __new__(cls, shape, mtype='FCELL', buffer=None, offset=0,
                strides=None, order=None):
        #import pdb; pdb.set_trace()
        obj = np.ndarray.__new__(cls, shape, RTYPE[mtype]['numpy'],
                                 buffer, offset, strides, order)
        obj.pointer_type = ctypes.POINTER(RTYPE[mtype]['ctypes'])
        obj.p = obj.ctypes.data_as(obj.pointer_type)
        obj.mtype = mtype
        return obj

    def __array_finalize__(self, obj):
        if obj is None:
            return
        self.mtype = getattr(obj, 'mtype', None)
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
