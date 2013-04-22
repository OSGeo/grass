#!/usr/bin/env python
############################################################################
#
# MODULE:       m.distance
#
# AUTHOR(S):    Hamish Bowman, Dunedin, New Zealand
#               Updated for Ctypes by Martin Landa <landa.martin gmail.com>
#
# PURPOSE:      Find distance between two points
#               If the projection is latitude-longitude, this distance
#                 is measured along the geodesic.
#               Demonstrates GRASS Python Ctypes interface
#
# COPYRIGHT:    (c) 2008-2011 Hamish Bowman, and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
############################################################################
#
# Requires GRASS Python Ctypes interface
# Requires Numeric module (NumPy) from  http://numpy.scipy.org/
#

#%module
#% label: Finds the distance between two or more points.
#% description: If the projection is latitude-longitude, this distance is measured along the geodesic.
#% keywords: miscellaneous, distance, measure
#%end
#%option
#% key: coord
#% type: string
#% required: yes
#% multiple: yes
#% key_desc: easting,northing
#% description: Comma separated list of coordinate pairs
#%end
#%flag
#% key: i
#% description: Read coordinate pairs from stdin
#% suppress_required: yes
#%end

import os, sys

import grass.script as grass

from grass.lib.gis import *

def main(): 
    G_gisinit('m.distance')

    # calc distance
    
    proj_type = G_begin_distance_calculations()
    # returns 0 if projection has no metrix (ie. imagery)
    # returns 1 if projection is planimetric
    # returns 2 if projection is latitude-longitude

    # parser always creates at least an empty variable, and sys.argv is
    # toast, so no way to check if option was given. So it hangs if
    # --q was the only option given and there is no data from stdin.
    coords = []
    if flags['i']:
        # read line by line from stdin
        while True:
            line = sys.stdin.readline().strip()
            if not line: # EOF
                break
            else:
                coords.append(line.split(','))
    else:
        # read from coord= command line option
        p = None
        for c in options['coord'].split(','):
            if not p:
                p = [c]
            else:
                p.append(c)
                coords.append(p)
                p = None
    
    if len(coords) < 2:
       grass.fatal("A minimum of two input coordinate pairs are needed")
    
    # init variables
    overall_distance = 0.0
    coord_array = c_double * len(coords)
    x = coord_array()
    y = coord_array()
    if proj_type == 2:
        # lat/lon scan for DDD:MM:SS.SSSS
        easting = c_double()
        northing = c_double()
        G_scan_easting(coords[0][0], byref(easting), PROJECTION_LL)
        G_scan_northing(coords[0][1], byref(northing), PROJECTION_LL)
        x[0] = float(easting.value)
        y[0] = float(northing.value)
    else:
        # plain coordinates
        x[0] = float(coords[0][0])
        y[0] = float(coords[0][1])
    
    for i in range(1, len(coords)):
        if proj_type == 2:
            easting = c_double()
            northing = c_double()
            G_scan_easting(coords[i][0], byref(easting), PROJECTION_LL)
            G_scan_northing(coords[i][1], byref(northing), PROJECTION_LL)
            x[i] = float(easting.value)
            y[i] = float(northing.value)
        else:
            x[i] = float(coords[i][0])
            y[i] = float(coords[i][1])
        
        segment_distance = G_distance(x[i-1], y[i-1], x[i], y[i])
        overall_distance += segment_distance
        
        print "segment %d distance is %.2f meters" % (i, segment_distance)
        
        # add to the area array
    
    print "\ntotal distance is %.2f meters\n" % overall_distance
    
    # calc area
    if len(coords) < 3:
       return 0
    
    G_begin_polygon_area_calculations()
    # returns 0 if the projection is not measurable (ie. imagery or xy)
    # returns 1 if the projection is planimetric (ie. UTM or SP)
    # returns 2 if the projection is non-planimetric (ie. latitude-longitude)

    # do not need to close polygon (but it doesn't hurt if you do)
    area = G_area_of_polygon(x, y, len(coords))
    print "area is %.2f square meters\n" % area
    
    # we don't need this, but just to have a look
    if proj_type == 1:
        G_database_units_to_meters_factor()
        grass.message("Location units are %s" % G_database_unit_name(True).lower())
    
    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
