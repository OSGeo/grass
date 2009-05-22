#!/usr/bin/env python

"""
Run within GRASS session
Run this before starting python to append module search path:

@code
export PYTHONPATH=/usr/src/grass70/swig/python
@endcode

Check with "import sys; sys.path"
or:

@code
sys.path.append("/usr/src/grass70/swig/python")
@endcode

\todo install the grass bindings in $GISBASE/lib/ ?
"""

import os, sys
from grass.lib import grass

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
data_type = grass.G_raster_map_type(input, mapset)

infd = grass.G_open_cell_old(input, mapset)
inrast = grass.G_allocate_raster_buf(data_type)

rown = 0
while True:
    myrow = grass.G_get_raster_row(infd, inrast, rown, data_type)
    print rown, myrow[0:10]
    rown += 1
    if rown == 476:
        break

grass.G_close_cell(inrast)
grass.G_free(cell)
