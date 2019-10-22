#!/usr/bin/env python3

# g.parser demo script for python programming

#%module
#% description: g.parser test script (python)
#% keyword: keyword1
#% keyword: keyword2
#%end
#%flag
#% key: f
#% description: A flag
#%end
#%option G_OPT_R_MAP
#% key: raster
#% required: yes
#%end
#%option G_OPT_V_MAP
#% key: vector
#%end
#%option
#% key: option1
#% type: string
#% description: An option
#% required: no
#%end

import sys
import atexit

import grass.script as grass

def cleanup():
    # add some cleanup code
    grass.message(_("Inside cleanup function..."))

def main():
    flag_f = flags['f']
    option1 = options['option1']
    raster = options['raster']
    vector = options['vector']

    #### add your code here ####
    exitcode = 0

    if flag_f:
        grass.message(_("Flag -f set"))
    else:
        grass.message(_("Flag -f not set"))

    # test if parameter present:
    if option1:
        grass.message(_("Value of option1 option: '%s'" % option1))

    grass.message(_("Value of raster option: '%s'" % raster))
    grass.message(_("Value of vector option: '%s'" % vector))

    #### end of your code ####

    sys.exit(exitcode)

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
