#!/usr/bin/python
############################################################################
#
# MODULE:       m.distance
#
# AUTHOR(S):    Hamish Bowman, Dunedin, New Zealand
#
# PURPOSE:      Find distance between two points
#               If the projection is latitude-longitude, this distance
#                 is measured along the geodesic.
#               Demonstrates GRASS SWIG-Python interface
#
# COPYRIGHT:    (c) 2008 Hamish Bowman, and The GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
############################################################################
#
# Requires GRASS SWIG-Python interface
# Requires Numeric module (NumPy) from  http://numpy.scipy.org/
# Requires NumPrt module from  http://geosci.uchicago.edu/csc/numptr/
#

"""
     ****  FIXME: needs to be ported from SWIG to Ctypes!  ****
"""


#%Module
#%  label: Finds the distance between two or more points.
#%  description: If the projection is latitude-longitude, this distance is measured along the geodesic.
#%  keywords: miscellaneous, distance, measure
#%End
#%Option
#%  key: coord
#%  type: string
#%  required: no
#%  multiple: yes
#%  key_desc: x,y
#%  description: Comma separated list of coordinate pairs
#%End
#%Flag
#%  key: i
#%  description: Read coordinate pairs from stdin
#%End


import os, sys

if not os.environ.has_key("GISBASE"):
    print "You must be in GRASS GIS to run this program."
    sys.exit(1)


def main(): 

    #### add your code here ####

    # run this before starting python to append module search path:
    #   export PYTHONPATH=$PYTHONPATH:/usr/local/grass-7.0.svn/etc/python
    #   check with "import sys; sys.path"
    # or:
    sys.path.append("/usr/local/grass-7.0.svn/etc/python")
    from grass.lib import grass as g7lib
 
    # for passing pointers
    import Numeric
    import NumPtr

    g7lib.G_gisinit('m.distance')
    # returns 0 on success


    ### calc distance ###
    
    proj_type = g7lib.G_begin_distance_calculations()
    # returns 0 if projection has no metrix (ie. imagery)
    # returns 1 if projection is planimetric
    # returns 2 if projection is latitude-longitude


    # parser always creates at least an empty variable, and sys.argv is
    #  toast, so no way to check if option was given. So it hangs if
    #  --q was the only option given and there is no data from stdin.
    coord_ans  = os.getenv("GIS_OPT_COORD")
    stdin_flag = bool(int(os.getenv("GIS_FLAG_I")))

    if stdin_flag is True:
        coords = []
        # read line by line from stdin
        while 1:
            line = sys.stdin.readline().strip()
            if not line:   # EOF
                break
            else:
                coords += line.split(',')
    else:
        # read from coord= command line option
        coords = coord_ans.split(',')


    if len(coords) < 4:
       print "A minimum of two input coordinate pairs are needed"
       return


    # init variables
    overall_distance = 0.0

    if proj_type == 2:
        # lat/lon scan for DDD:MM:SS.SSSS
        easting = Numeric.array(0., Numeric.Float64)
        eastPtr = NumPtr.getpointer(easting)
        northing = Numeric.array(0., Numeric.Float64)
        northPtr = NumPtr.getpointer(northing)

        # TODO: for clarity, figure out how to replace "3" with
        #       the defined LOCATION_LL constant from gis.i
        g7lib.G_scan_easting(coords[0], eastPtr, 3)
        g7lib.G_scan_northing(coords[1], northPtr, 3)
        x1 = float(easting)
        y1 = float(northing)
    else:
        # plain old coordinates
        x1 = float(coords[0])
        y1 = float(coords[1])

    x = [x1]
    y = [y1]

    for i in range(1, (len(coords) / 2)):

        if proj_type == 2:
            g7lib.G_scan_easting (coords[ i*2 + 0 ], eastPtr, 3)
            g7lib.G_scan_northing(coords[ i*2 + 1 ], northPtr, 3)
            x2 = float(easting)
            y2 = float(northing)
        else:
            x2 = float(coords[ i*2 + 0 ])
            y2 = float(coords[ i*2 + 1 ])

        segment_distance = g7lib.G_distance(x1, y1, x2, y2)
        overall_distance += segment_distance

        print "segment %d distance is %.2f meters" % (i, segment_distance)

        # add to the area array

        # setup for the next loop
        x1 = x2
        y1 = y2

        x += [x2]
        y += [y2]
  
    print
    print " total distance is %.2f meters" % overall_distance
    print


    ### calc area ###
    if len(coords) < 6:
        return
 
    g7lib.G_begin_polygon_area_calculations()
    # returns 0 if the projection is not measurable (ie. imagery or xy)
    # returns 1 if the projection is planimetric (ie. UTM or SP)
    # returns 2 if the projection is non-planimetric (ie. latitude-longitude)

    # do not need to close polygon (but it doesn't hurt if you do)
    npoints = len(x)

    # unset variables:
    #del [Xs, Xptr, Ys, Yptr]
    #   or
    #Xs = Xptr = Ys = Yptr = None

    Xs = Numeric.array(x, Numeric.Float64)
    Xptr = NumPtr.getpointer(Xs)
    Ys = Numeric.array(y, Numeric.Float64)
    Yptr = NumPtr.getpointer(Ys)
    
    area = g7lib.G_area_of_polygon(Xptr, Yptr, npoints)
    print "AREA:  %10.2f square meters" % area
    print


    # we don't need this, but just to have a look
    if False:
        if proj_type == 1:
            g7lib.G_database_units_to_meters_factor()
            # 1.0
        print "Location units are", g7lib.G_database_unit_name(True)


    #### end of your code ####
    return 

if __name__ == "__main__":
    if ( len(sys.argv) <= 1 or sys.argv[1] != "@ARGS_PARSED@" ):
        os.execvp("g.parser", [sys.argv[0]] + sys.argv)
    else:
        main();

