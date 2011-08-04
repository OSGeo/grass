#!/usr/bin/env python

############################################################################
#
# MODULE:       r.in.gdalwarp
#
# AUTHOR(S):    Cedric Shock, 2006
#               Pythonized by Martin Landa <landa.martin gmail.com>, 2009
#
# PURPOSE:      To warp and import data
#               (based on Bash script by Cedric Shock)
#
# COPYRIGHT:    (C) 2009 Martin Landa, and GRASS development team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

#%module
#% description: Warps and imports GDAL supported raster file complete with correct NULL values
#% keywords: raster
#% keywords: rotate
#% keywords: reproject
#%end
#%flag
#% key: p
#% description: Don't reproject the data, just patch it
#%end
#%flag
#% key: e
#% description: Extend location extents based on new dataset
#%end
#%flag
#% key: c
#% description: Make color composite image if possible
#%end
#%flag
#% key: k
#% description: Keep band numbers instead of using band color names
#%end
#%option G_OPT_F_INPUT
#% label: Name of raster file or files to be imported
#% description: If multiple files are specified they will be patched together.
#% multiple: yes
#%end
#%option
#% key: output
#% type: string
#% label: Prefix for resultant raster maps.
#% description: Each band will be name output.bandname
#% required : yes
#%end
#%option
#% key: s_srs
#% type: string
#% description: Source projection in gdalwarp format
#% required : yes
#%end
#%option
#% key: method
#% type: string
#% description: Reprojection method to use
#% options:near,bilinear,cubic,cubicspline
#% answer:near
#% required: yes
#%end
#%option
#% key: warpoptions
#% type: string
#% description: Additional options for gdalwarp
#% required : no
#%end

import os
import sys
import atexit

from grass.script import core as grass

path = os.path.join(os.getenv('GISBASE'), 'etc', 'r.in.wms')
sys.path.append(path)
import gdalwarp

def cleanup():
    if tmp:
	grass.run_command('g.remove', rast = tmp, quiet = True)

def main():
    # show progress infromation and grass.info() by default
    os.environ['GRASS_VERBOSE'] = '1'

    return gdalwarp.GDALWarp(flags, options).run()
    
if __name__ == "__main__":
    options, flags = grass.parser()
    tmp = None
    atexit.register(cleanup)

    sys.exit(main())
