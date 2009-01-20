#!/usr/bin/python

# run within GRASS Spearfish session

import os, sys
import swig.grass as grasslib
import swig.raster as grassrast

if not os.environ.has_key("GISBASE"):
    print "You must be in GRASS GIS to run this program."
    sys.exit(1)

if len(sys.argv)==2:
  input = sys.argv[1]
else:
  input = raw_input("Raster Map Name? ")

# initialize
grasslib.G_gisinit('')

# find map in search path
mapset = grasslib.G_find_cell2(input,'')

# determine the inputmap type (CELL/FCELL/DCELL) */
data_type = grasslib.G_raster_map_type(input, mapset)

infd = grasslib.G_open_cell_old(input, mapset)
inrast = grasslib.G_allocate_raster_buf(data_type)

rown=0
while 1:
    myrow = grasslib.G_get_raster_row(infd, inrast, rown, data_type)
    print rown,myrow[0:10]
    rown = rown+1
    if rown==476:break

grasslib.G_close_cell(inrast)
grasslib.G_free(cell)
