#!/usr/bin/env python
#
############################################################################
#
# MODULE:       d.wms
#
# AUTHOR(S):    Stepan Turek <stepan.turek seznam.cz> (mentor: Martin Landa)
#
# PURPOSE:      Wrapper for wxGUI to add WMS into layer tree
#
# COPYRIGHT:    (C) 2012 by Stepan Turek, and the GRASS Development Team
#
#   This program is free software under the GNU General Public License
#   (>=v2). Read the file COPYING that comes with GRASS for details.
#
#############################################################################

#%module
#% description: Downloads and displays data from WMS server.
#% keywords: raster
#% keywords: import
#% keywords: wms
#%end

#%option
#% key: url
#% type: string
#% description: URL of WMS server 
#% required: yes
#%end

#%option
#% key: layers
#% type: string
#% description: Layers to request from WMS server
#% multiple: yes
#% required: yes
#%end

#%option
#% key: map
#% type: string
#% description: Name for output WMS layer in the layer tree
#% required : yes
#%end

#%option
#% key: srs
#% type: integer
#% description: EPSG number of source projection for request 
#% answer:4326 
#% guisection: Request properties
#%end

#%option
#% key: wms_version
#% type: string
#% description: WMS standard
#% options: 1.1.1,1.3.0
#% answer: 1.1.1
#% guisection: Request properties
#%end

#%option
#% key: format
#% type: string
#% description: Image format requested from the server
#% options: geotiff,tiff,jpeg,gif,png
#% answer: geotiff
#% guisection: Request properties
#%end

#%option
#% key: method
#% type: string
#% description: Reprojection method to use
#% options:near,bilinear,cubic,cubicspline
#% answer:near
#% guisection: Request properties
#%end

#%option
#% key: maxcols
#% type:integer
#% description: Maximum columns to request at a time
#% answer:400
#% guisection: Request properties
#%end

#%option
#% key: maxrows
#% type: integer
#% description: Maximum rows to request at a time
#% answer: 300
#% guisection: Request properties
#%end

#%option
#% key: urlparams
#% type:string
#% description: Additional query parameters for server
#% guisection: Request properties
#%end

#%option
#% key: username
#% type:string
#% description: Username for server connection
#% guisection: Request properties
#%end

#%option
#% key: password
#% type:string
#% description: Password for server connection
#% guisection: Request properties
#%end

#%option
#% key: styles
#% type: string
#% description: Styles to request from map server
#% multiple: yes
#% guisection: Map style
#%end

#%option
#% key: bgcolor
#% type: string
#% description: Color of map background
#% guisection: Map style
#%end

#%flag
#% key: o
#% description: Don't request transparent data
#% guisection: Map style
#%end

#%option
#% key: driver
#% type:string
#% description: Driver for communication with server
#% descriptions: WMS_GDAL;Download data using GDAL WMS driver;WMS_GRASS;Download data using native GRASS-WMS driver;WMTS_GRASS;Download data using native GRASS-WMTS driver;OnEarth_GRASS;Download data using native GRASS-OnEarth driver;
#% options:WMS_GDAL, WMS_GRASS, WMTS_GRASS, OnEarth_GRASS
#% answer:WMS_GRASS
#%end

#%option G_OPT_F_INPUT
#% key: capfile
#% required: no
#% gisprompt: old,file,bin_input
#% description: Capabilities file to load

import os
import sys

from grass.script import core as grass

sys.path.append(os.path.join(os.getenv("GISBASE"), "etc", "r.in.wms"))
                
def GetRegion():
    """!Parse region from GRASS_REGION env var.
    """
    region = os.environ["GRASS_REGION"]
    conv_reg_vals = {'east' : 'e',
                     'north' : 'n',
                     'west' : 'w',
                     'south' : 's',
                     'rows' : 'rows',
                     'cols' : 'cols',
                     'e-w resol' : 'ewres',
                     'n-s resol' : 'nsres'}

    keys_to_convert = conv_reg_vals.keys()

    conv_region = {}
    region = region.split(';')

    for r in region:
        r = r.split(':')
        r[0] = r[0].strip()
        
        if r[0] in keys_to_convert:
            conv_region[conv_reg_vals[r[0]]] = float(r[1])

    return conv_region

def main():
    options['region'] = GetRegion()

    if 'GRASS' in options['driver']:
        grass.debug("Using GRASS driver")
        from wms_drv import WMSDrv
        wms = WMSDrv()
    elif 'GDAL' in options['driver']:
        grass.debug("Using GDAL WMS driver")
        from wms_gdal_drv import WMSGdalDrv
        wms = WMSGdalDrv()
    
    temp_map = wms.GetMap(options, flags) 
    os.rename(temp_map, os.environ["GRASS_PNGFILE"])

    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
