#!/usr/bin/python

# run within GRASS Spearfish session

import os, sys
import swig.grass as grasslib
import swig.vector as grassvect

if not os.environ.has_key("GISBASE"):
    print "You must be in GRASS GIS to run this program."
    sys.exit(1)

if len(sys.argv)==2:
  input = sys.argv[1]
else:
  input = raw_input("Vector Map Name? ")

# initialize
grasslib.G_gisinit('')

# find map in search path
mapset = grasslib.G_find_vector2(input,'')

# define map structure
map = grassvect.Map_info()

# define open level (level 2: topology)
grassvect.Vect_set_open_level (2)

# open existing map
grassvect.Vect_open_old(map, input, mapset)

# query
print 'Vect map: ', input
print 'Vect is 3D: ', grassvect.Vect_is_3d (map)
print 'Vect DB links: ', grassvect.Vect_get_num_dblinks(map)
print 'Map Scale:  1:', grassvect.Vect_get_scale(map)
# misleading:
# print 'Number of lines:', grassvect.Vect_get_num_lines(map)
# how to access GV_POINT?
# print 'Number of points: ', grassvect.Vect_get_num_primitives(map,GV_POINT)
# confusing:
#print 'Number of lines: ', Vect_get_num_primitives(map,GV_LINE)
#print 'Number of areas:', Vect_get_num_primitives(map,GV_AREA)
print 'Number of areas:', grassvect.Vect_get_num_areas(map)

# close map
grassvect.Vect_close(map)
## end of the python script

