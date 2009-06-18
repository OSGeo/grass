#!/usr/bin/python

# g.parser demo script for python programing

import os
import sys
import grass.script as grass

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
    if flags['f']:
        print "Flag -f set"
    else:
        print "Flag -f not set"

    #test if parameter present:
    if options['option1']:
        print "Value of option1=: '%s'" % options['option1']

    print "Value of raster=: '%s'" % options['raster']
    print "Value of vector=: '%s'" % options['vector']

    #end of your code 

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
