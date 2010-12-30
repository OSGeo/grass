#!/usr/bin/python

"""
Sample Python script to access vector dada using GRASS Ctypes interface

Run this before starting python to append module search path:
   export PYTHONPATH=$PYTHONPATH:/usr/local/grass-7.0.svn/etc/python
check with "import sys; sys.path"

or
   sys.path.append("/usr/local/grass-7.0.svn/etc/python")
"""

import os, sys

from grass.lib.grass  import *
from grass.lib.vector import *

if not os.environ.has_key("GISBASE"):
    sys.exit("You must be in GRASS GIS to run this program.")

if len(sys.argv) == 2:
  input = sys.argv[1]
else:
  input = raw_input("Vector Map Name? ")

# initialize GRASS library
G_gisinit('')

# find vector map in the search path
mapset = G_find_vector2(input, '')
if mapset:
    print "Vector map found in mapset <%s>" % mapset
else:
    sys.exit("Vector map <%s> not found" % input)

# define map structure
map = Map_info()

# define open level (level 2: topology)
Vect_set_open_level(2)

# open existing vector map
Vect_open_old(map, input, mapset)

# query
print 'Vect map: ',      input
print 'Vect is 3D: ',    Vect_is_3d(map)
print 'Vect DB links: ', Vect_get_num_dblinks(map)
print 'Map Scale:  1:',  Vect_get_scale(map)

# vector box tests
box = bound_box()
c_easting1  = 599505.0
c_northing  = 4921010.0
c_easting2  = 4599505.0

Vect_get_map_box(map, box)
print 'Position 1   in box? ', Vect_point_in_box(c_easting1, c_northing, 0, box)
print 'Position 2   in box? ', Vect_point_in_box(c_easting2, c_northing, 0, box)
print 'Feature id 2 in box? ', Vect_get_line_box(map, 2, box)

print 'Number of features:', Vect_get_num_lines(map)
# how to access GV_POINT?
# print 'Number of points: ', grassvect.Vect_get_num_primitives(map,GV_POINT)
# confusing:
#print 'Number of lines: ', grassvect.Vect_get_num_primitives(map,GV_LINE)
#print 'Number of areas:', grassvect.Vect_get_num_primitives(map,GV_AREA)
print 'Number of areas:',   Vect_get_num_areas(map)

# close map
Vect_close(map)
