#!/usr/bin/env python3

"""
Sample Python script to access raster data using GRASS Ctypes
interface

You may wish to use 'g.region' to set the rows x columns to something
small (say 10 x 5) to avoid overwhelming yourself: `g.region rows=10
cols=5`
"""

# FIXME: as an example it should make extensive use of code comments and document
#  each and every step along the way.  (e.g. explain c_char_p().value memory pointer
#  to string conversion for Python programmers not familar with C pointers)
#
#  FIXME: explain at a basic level what ctypes is & does.

import os
import sys

from grass.lib.gis    import *
from grass.lib.raster import *

# check if GRASS is running or not
if not os.environ.has_key("GISBASE"):
    sys.exit("You must be in GRASS GIS to run this program")

# parse command line arguments, prompt user for a raster map name if one wasn't given
if len(sys.argv) == 2:
  input = sys.argv[1]
else:
  input = raw_input("Name of raster map? ")

# initialize GRASS library
G_gisinit('')

# find map in search path
mapset = G_find_raster2(input, '')
if not mapset:
    sys.exit("Raster map <%s> not found" % input)

# determine the inputmap type (CELL/FCELL/DCELL)
data_type = Rast_map_type(input, mapset)

if data_type == CELL_TYPE:
    ptype = POINTER(c_int)
    type_name = 'CELL'
elif data_type == FCELL_TYPE:
    ptype = POINTER(c_float)
    type_name = 'FCELL'
elif data_type == DCELL_TYPE:
    ptype = POINTER(c_double)
    type_name = 'DCELL'

print("Raster map <%s> contains data type %s." % (input, type_name))

in_fd   = Rast_open_old(input, mapset)
in_rast = Rast_allocate_buf(data_type)
in_rast = cast(c_void_p(in_rast), ptype)

rows = Rast_window_rows()
cols = Rast_window_cols()
print("Current region is %d rows x %d columns" % (rows, cols))

# iterate through map rows
print("Map data:")
for row_n in range(rows):
    # read a row of raster data into memory, then print it
    Rast_get_row(in_fd, in_rast, row_n, data_type)
    print(row_n, in_rast[0:cols])
    # TODO check for NULL

# closed map and cleanup memory allocation
Rast_close(in_fd)
G_free(in_rast)

def check_null(value):
    if math.isnan(value):
        return 'null'
    return value
