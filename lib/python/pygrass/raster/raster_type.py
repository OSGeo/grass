# -*- coding: utf-8 -*-
"""
Created on Wed Jun 13 19:42:22 2012

@author: pietro
"""
import grass.lib.raster as libraster
import ctypes
import numpy as np

## Private dictionary to convert RASTER_TYPE into type string.
RTYPE_STR = {libraster.CELL_TYPE:  'CELL',
             libraster.FCELL_TYPE: 'FCELL',
             libraster.DCELL_TYPE: 'DCELL'}


TYPE = {'CELL':  {'grass type':  libraster.CELL_TYPE,
                  'grass def':   libraster.CELL,
                  'numpy':       np.int32,
                  'ctypes':      ctypes.c_int},
        'FCELL': {'grass type':  libraster.FCELL_TYPE,
                  'grass def':   libraster.FCELL,
                  'numpy':       np.float32,
                  'ctypes':      ctypes.c_float},
        'DCELL': {'grass type':  libraster.DCELL_TYPE,
                  'grass def':   libraster.DCELL,
                  'numpy':       np.float64,
                  'ctypes':      ctypes.c_double}}
