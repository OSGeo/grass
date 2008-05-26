#!/usr/bin/python

# run within GRASS Spearfish session

import os, sys
import python_grass7 as g7lib

if not os.environ.has_key("GISBASE"):
    print "You must be in GRASS GIS to run this program."
    sys.exit(1)

input = 'elevation.dem'
mapset = 'PERMANENT'

g7lib.G_gisinit('')
infd = g7lib.G_open_cell_old(input, mapset)

cell = g7lib.G_allocate_cell_buf()

rown=0
while 1:
    myrow = g7lib.G_get_map_row_nomask(infd, cell, rown)
    print rown,myrow[0:10]
    rown = rown+1
    if rown==476:break

g7lib.G_close_cell(infd)
g7lib.G_free(cell)
