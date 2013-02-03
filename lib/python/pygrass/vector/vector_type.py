# -*- coding: utf-8 -*-
"""
Created on Wed Jul 18 10:49:26 2012

@author: pietro
"""

import grass.lib.vector as libvect
import geometry as geo

MAPTYPE = {libvect.GV_FORMAT_NATIVE:     "native",
           libvect.GV_FORMAT_OGR:        "OGR",
           libvect.GV_FORMAT_OGR_DIRECT: "OGR",
           libvect.GV_FORMAT_POSTGIS:    "PostGIS"}

VTYPE = {'point':    libvect.GV_POINT,  # 1
         'line':     libvect.GV_LINE,   # 2
         'boundary': libvect.GV_BOUNDARY,  # 3
         'centroid': libvect.GV_CENTROID,  # 4
         'face':     libvect.GV_FACE,  # 5
         'kernel':   libvect.GV_KERNEL,  # 6
         'area':     libvect.GV_AREA,  # 7
         'volume':   libvect.GV_VOLUME}  # 8

GV_TYPE = {libvect.GV_POINT:    {'label': 'point',    'obj': geo.Point},
           libvect.GV_LINE:     {'label': 'line',     'obj': geo.Line},
           libvect.GV_BOUNDARY: {'label': 'boundary', 'obj': geo.Boundary},
           libvect.GV_CENTROID: {'label': 'centroid', 'obj': geo.Centroid},
           libvect.GV_FACE:     {'label': 'face',     'obj': None},
           libvect.GV_KERNEL:   {'label': 'kernel',   'obj': None},
           libvect.GV_AREA:     {'label': 'area',     'obj': geo.Area},
           libvect.GV_VOLUME:   {'label': 'volume',   'obj': None}, }
