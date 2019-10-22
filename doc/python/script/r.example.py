#!/usr/bin/env python3

#%module
#% description: Selects values from raster above value of mean plus standard deviation
#% keyword: raster
#% keyword: select
#% keyword: standard deviation
#%end
#%option G_OPT_R_INPUT
#%end
#%option G_OPT_R_OUTPUT
#%end


import sys

import grass.script as gscript
from grass.exceptions import CalledModuleError


def main():
    options, flags = gscript.parser()
    input_raster = options['input']
    output_raster = options['output']

    try:
        stats = gscript.parse_command('r.univar', map=input_raster, flags='g')
    except CalledModuleError as e:
        gscript.fatal('{0}'.format(e))
    raster_mean = float(stats['mean'])
    raster_stddev = float(stats['stddev'])
    raster_high = raster_mean + raster_stddev
    gscript.mapcalc('{r} = {i} > {v}'.format(r=output_raster, i=input_raster,
                                             v=raster_high))
    return 0


if __name__ == "__main__":
    sys.exit(main())
