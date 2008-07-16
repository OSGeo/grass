#!/usr/bin/python

# g.parser demo script for python programing

import os
import sys

#%Module
#%  description: g.parser test script (python)
#%End
#%flag
#%  key: f
#%  description: a flag
#%END
#%option
#% key: raster
#% type: string
#% gisprompt: old,cell,raster
#% description: raster input map
#% required : yes
#%end
#%option
#% key: vector
#% type: string
#% gisprompt: old,vector,vector
#% description: vector input map
#% required : yes
#%end
#%option
#% key: option1
#% type: string
#% description: an option
#% required : yes
#%end

def main():

    #add your code here

    print ""
    if ( os.getenv('GIS_FLAG_F') == "1" ):
        print "Flag -f set"
    else:
        print "Flag -f not set"

    #test if parameter present:
    if ( os.getenv("GIS_OPT_OPTION1") != "" ):
        print "Value of GIS_OPT_OPTION1: '%s'" % os.getenv('GIS_OPT_OPTION1')

    print "Value of GIS_OPT_RASTER: '%s'" % os.getenv('GIS_OPT_RASTER')
    print "Value of GIS_OPT_VECTOR: '%s'" % os.getenv('GIS_OPT_VECTOR')

    #end of your code 
    
if __name__ == "__main__":
    if not os.getenv("GISBASE"):
        print >> sys.stderr, "You must be in GRASS GIS to run this program."
        sys.exit(0)

    try:
        if len(sys.argv) < 2 or sys.argv[1] != "@ARGS_PARSED@":
            os.execvp("g.parser", ["g.parser"] + sys.argv)
    except IndexError:
	os.execvp("g.parser", ["g.parser"] + sys.argv)

    if sys.argv[1] == "@ARGS_PARSED@":
        main();
