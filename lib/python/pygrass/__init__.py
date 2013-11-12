# -*- coding: utf-8 -*-
"""
Created on Fri May 25 12:55:14 2012

@author: pietro
"""
import grass.lib.gis as _libgis
_libgis.G_gisinit('')
import os as _os
import sys as _sys

import errors
import gis
import functions
import raster
import vector
import modules
import shell
import messages
