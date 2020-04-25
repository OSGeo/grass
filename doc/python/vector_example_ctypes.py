#!/usr/bin/env python3

"""
Sample Python script to access vector data using GRASS Ctypes
interface
"""

import os, sys

from grass.lib.gis    import *
from grass.lib.vector import *

if not os.environ.has_key("GISBASE"):
    sys.exit("You must be in GRASS GIS to run this program.")

if len(sys.argv) == 2:
  input = sys.argv[1]
else:
  input = raw_input("Name of vector map? ")

# initialize GRASS library
G_gisinit('')

# find vector map in the search path
mapset = G_find_vector2(input, "")
if not mapset:
    sys.exit("Vector map <%s> not found" % input)

# define map structure
map_info = pointer(Map_info())

# define open level (level 2: topology)
Vect_set_open_level(2)

# open existing vector map
Vect_open_old(map_info, input, mapset)

# query
print('Vector map        :', Vect_get_full_name(map_info))
print('Vector is 3D      :', Vect_is_3d(map_info))
print('Vector DB links   :', Vect_get_num_dblinks(map_info))
print('Map Scale         : 1:%d' % Vect_get_scale(map_info))

# vector box tests
box = bound_box()
c_easting1  = 599505.0
c_northing  = 4921010.0
c_easting2  = 4599505.0

Vect_get_map_box(map_info, byref(box))
print('Position 1 in box ?', Vect_point_in_box(c_easting1, c_northing, 0, byref(box)))
print('Position 2 in box ?', Vect_point_in_box(c_easting2, c_northing, 0, byref(box)))

print('Number of features:', Vect_get_num_lines(map_info))
print('Number of points  :', Vect_get_num_primitives(map_info, GV_POINT))
print('Number of lines   :', Vect_get_num_primitives(map_info, GV_LINE))
print('Number of areas   :', Vect_get_num_areas(map_info))

# close map
Vect_close(map_info)
