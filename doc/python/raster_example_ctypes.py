#!/usr/bin/env python
import os, sys, subprocess
from ctypes import *
grass = CDLL("libgrass_gis.so")
rast = CDLL("libgrass_raster.so")

if not os.environ.has_key("GISBASE"):
    print "You must be in GRASS GIS to run this program."
    sys.exit(1)

if len(sys.argv)==2:
  input = sys.argv[1]
else:
  input = raw_input("Raster Map Name? ")
 
# initialize
s = subprocess.Popen(['g.version','-r'], stdout=subprocess.PIPE).communicate()[0]
for line in s.splitlines():
    if line.startswith('Revision:'):
        version = '$' + line + '$'
grass.G__gisinit(version, '')
 
# find map in search path
mapset = grass.G_find_raster2(input, '')
mapset = c_char_p(mapset).value
 
# determine the inputmap type (CELL/FCELL/DCELL) */
data_type = rast.Rast_map_type(input, mapset)

if data_type == 0:
    ptype = POINTER(c_int)
elif data_type == 1:
    ptype = POINTER(c_float)
elif data_type == 2:
    ptype = POINTER(c_double)
 
infd = rast.Rast_open_old(input, mapset)
inrast = rast.Rast_allocate_buf(data_type)
inrast = cast(c_void_p(inrast), ptype)

rows = rast.Rast_window_rows()
cols = rast.Rast_window_cols()

for rown in xrange(rows):
    rast.Rast_get_row(infd, inrast, rown, data_type)
    print rown, inrast[0:cols]
 
rast.Rast_close(infd)
grass.G_free(inrast)

