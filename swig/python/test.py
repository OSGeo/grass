#!/usr/bin/env python

# must be executed in the GRASS env
# test case for Spearfish location


import os, sys
import python_grass6 as g6lib

if not os.environ.has_key("GISBASE"):
    print "You must be in GRASS GIS to run this program."
    sys.exit(1)

rname = 'elevation.dem'
mapset = 'PERMANENT'

g6lib.G_gisinit('')
g6lib.G_find_cell2(rname,'')

print mapset

print 'prints 0 if map was found'

print 'roads:'
print g6lib.G_raster_map_type('roads',mapset)

print 'elevation.dem:'
print g6lib.G_raster_map_type(rname,mapset)

