#!/usr/bin/env python

"""
Run within GRASS session
Run this before starting python to append module search path:

@code
export PYTHONPATH=$PYTHONPATH:/usr/local/grass-7.0.svn/etc/python/
@endcode

Check with "import sys; sys.path"
or:

@code
sys.path.append("/usr/local/grass-7.0.svn/etc/python")
@endcode
"""

import os, sys
from grass.lib import grass
from grass.raster import grassrast

if not os.environ.has_key("GISBASE"):
    print "You must be in GRASS GIS to run this program."
    sys.exit(1)

if len(sys.argv)==2:
  input = sys.argv[1]
else:
  input = raw_input("Raster Map Name? ")

# initialize
grass.G_gisinit('')

# find map in search path
mapset = grass.G_find_cell2(input, '')

# determine the inputmap type (CELL/FCELL/DCELL) */
data_type = grassrast.Rast_map_type(input, mapset)

infd = grassrast.Rast_open_cell_old(input, mapset)
inrast = grassrast.Rast_allocate_raster_buf(data_type)

rown = 0
while True:
    myrow = grassrast.Rast_get_raster_row(infd, inrast, rown, data_type)
    print rown, myrow[0:10]
    rown += 1
    if rown == 476:
        break

grassrast.Rast_close_cell(inrast)
