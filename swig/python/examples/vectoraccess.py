#!/usr/bin/python

# run within GRASS Spearfish session
# run this before starting python to append module search path:
#   export PYTHONPATH=/usr/src/grass70/swig/python
#   check with "import sys; sys.path"
# or:
#   sys.path.append("/usr/src/grass70/swig/python")
# FIXME: install the grass bindings in $GISBASE/lib/ ?

import os, sys
import grass

if not os.environ.has_key("GISBASE"):
    print "You must be in GRASS GIS to run this program."
    sys.exit(1)

if len(sys.argv)==2:
  input = sys.argv[1]
else:
  input = raw_input("Vector Map Name? ")

# initialize
grass.G_gisinit('')

# find map in search path
mapset = grass.G_find_vector2(input,'')

# define map structure
map = grass.Map_info()

# define open level (level 2: topology)
grass.Vect_set_open_level (2)

# open existing map
grass.Vect_open_old(map, input, mapset)

# query
print 'Vect map: ', input
print 'Vect is 3D: ', grass.Vect_is_3d (map)
print 'Vect DB links: ', grass.Vect_get_num_dblinks(map)
print 'Map Scale:  1:', grass.Vect_get_scale(map)
# misleading:
# print 'Number of lines:', grass.Vect_get_num_lines(map)
# how to access GV_POINT?
# print 'Number of points: ', grass.Vect_get_num_primitives(map,GV_POINT)
# confusing:
#print 'Number of lines: ', grass.Vect_get_num_primitives(map,GV_LINE)
#print 'Number of areas:', grass.Vect_get_num_primitives(map,GV_AREA)
print 'Number of areas:', grass.Vect_get_num_areas(map)

# close map
grass.Vect_close(map)
## end of the python script

