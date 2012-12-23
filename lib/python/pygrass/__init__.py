# -*- coding: utf-8 -*-
"""
Created on Fri May 25 12:55:14 2012

@author: pietro
"""
import grass.lib.gis as _libgis
_libgis.G_gisinit('')
import os as _os
import sys as _sys

_pygrasspath = _os.path.dirname(_os.path.realpath(__file__)).split(_os.sep)

_sys.path.append(_os.path.join(_os.sep, *_pygrasspath[:-1]))

import gis
import raster
import vector
import modules
import errors
import functions