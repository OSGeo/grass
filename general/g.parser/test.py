#!/usr/bin/env python

# g.parser demo script for python programing

#%Module
#% description: g.parser test script (python)
#% keyword: keyword1
#% keyword: keyword2
#%End
#%flag
#% key: f
#% description: A flag
#%end
#%option
#% key: raster
#% type: string
#% gisprompt: old,cell,raster
#% description: Raster input map
#% required : yes
#%end
#%option
#% key: vector
#% type: string
#% gisprompt: old,vector,vector
#% description: Vector input map
#% required : yes
#%end
#%option
#% key: option1
#% type: string
#% description: An option
#% required : no
#%end

import os
import sys

import grass.script as grass

def main():
    #### add your code here ####

    if flags['f']:
        print "Flag -f set"
    else:
        print "Flag -f not set"

    # test if parameter present:
    if options['option1']:
        print "Value of GIS_OPT_OPTION1: '%s'" % options['option1']

    print "Value of GIS_OPT_RASTER: '%s'" % options['raster']
    print "Value of GIS_OPT_VECTOR: '%s'" % options['vector']

    #### end of your code ####

    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
