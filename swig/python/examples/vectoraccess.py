#!/usr/bin/python

# run within GRASS Spearfish session

import os, sys
import python_grass7 as g7lib

if not os.environ.has_key("GISBASE"):
    print "You must be in GRASS GIS to run this program."
    sys.exit(1)

if len(sys.argv)==2:
  input = sys.argv[1]
else:
  input = raw_input("Vector Map Name? ")

mapset = 'PERMANENT'

# initialize
g7lib.G_gisinit('')

# define map structure
map = g7lib.Map_info()

# define open level (level 2: topology)
g7lib.Vect_set_open_level (2)

# open existing map
g7lib.Vect_open_old(map, input, mapset)

# query
print 'Vect map: ', input
print 'Vect is 3D: ', g7lib.Vect_is_3d (map)
print 'Vect DB links: ', g7lib.Vect_get_num_dblinks(map)
print 'Map Scale:  1:', g7lib.Vect_get_scale(map)
# misleading:
# print 'Number of lines:', g7lib.Vect_get_num_lines(map)
print 'Number of points: ', g7lib.Vect_get_num_primitives(map,g7lib.GV_POINT)
# confusing:
#print 'Number of lines: ', g7lib.Vect_get_num_primitives(map,g7lib.GV_LINE)
#print 'Number of areas:', g7lib.Vect_get_num_primitives(map,g7lib.GV_AREA)
print 'Number of areas:', g7lib.Vect_get_num_areas(map)

# close map
g7lib.Vect_close(map)
## end of the python script

