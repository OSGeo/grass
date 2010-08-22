#!/usr/bin/env python

"""
This is an example of how to use "ctypes" to access GRASS's C API from
within a Python script. See the GRASS Programmer's manual for details
on the e.g. G_*() functions in the GRASS C libraries (libgis et al.).

USAGE:  example_ctypes.py [Raster map name]
        If a raster map name is not given it will prompt you for one.
	This raster map is then opened, read, and data values printed
	to stdout. You may wish to use 'g.region' to set the rows x
	columns to something small (say 10 x 5) to avoid overwhelming
	yourself: `g.region rows=10 cols=5`
"""


# FIXME: as an example it should make extensive use of code comments and document
#  each and every step along the way.  (e.g. explain c_char_p().value memory pointer
#  to string conversion for Python programmers not familar with C pointers)
#
#  FIXME: explain at a basic level what ctypes is & does.

import os, sys, subprocess
# actiavate ctypes
from ctypes import *

# check if GRASS is running or not
if not os.environ.has_key("GISBASE"):
    print "You must be in GRASS GIS to run this program."
    sys.exit(1)

# load in the main GRASS C library
grass = CDLL("libgrass_gis.so")
rast = CDLL("libgrass_raster.so")

# parse command line arguements, prompt user for a raster map name if one wasn't given
if len(sys.argv)==2:
  input = sys.argv[1]
else:
  input = raw_input("Raster Map Name? ")

# initialize the C library
s = subprocess.Popen(['g.version','-r'], stdout=subprocess.PIPE).communicate()[0]
for line in s.splitlines():
    if line.startswith('Revision:'):
        version = '$' + line + '$'
grass.G__gisinit(version, '')


# find map in search path
mapset = grass.G_find_raster2(input, '')
mapset = c_char_p(mapset).value

if not mapset:
    print "Raster map <%s> not found." % input
    sys.exit(1)


# determine the inputmap type (CELL/FCELL/DCELL)
data_type = rast.Rast_map_type(input, mapset)

if data_type == 0:
    ptype = POINTER(c_int)
    type_name = 'CELL'
elif data_type == 1:
    ptype = POINTER(c_float)
    type_name = 'FCELL'
elif data_type == 2:
    ptype = POINTER(c_double)
    type_name = 'DCELL'

print "Raster map <%s> contains data type %s." % (input, type_name)

in_fd = rast.Rast_open_old(input, mapset)
in_rast = rast.Rast_allocate_buf(data_type)
in_rast = cast(c_void_p(in_rast), ptype)

rows = rast.Rast_window_rows()
cols = rast.Rast_window_cols()
print "Current region is %d rows x %d columns.\n" % (rows, cols)

# iterate through map rows
print "Map data:"
for row_n in xrange(rows):
    # read a row of raster data into memory, then print it
    rast.Rast_get_row(in_fd, in_rast, row_n, data_type)
    print row_n, in_rast[0:cols]

#TODO: NULL -> NaN


# closed map and cleanup memory allocation
rast.Rast_close(in_fd)
grass.G_free(in_rast)

