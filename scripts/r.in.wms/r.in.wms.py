#!/usr/bin/env python
"""
MODULE:    r.in.wms

AUTHOR(S): Stepan Turek <stepan.turek AT seznam.cz>

PURPOSE:   Downloads and imports data from WMS server.

COPYRIGHT: (C) 2012 Stepan Turek, and by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.
"""

#%module
#% description: Downloads and imports data from WMS servers.
#% keywords: raster
#% keywords: import
#% keywords: wms
#%end

#%option
#% key: mapserver
#% type: string
#% description:URL of WMS server 
#% required: yes
#%end

#%option
#% key: layers
#% type: string
#% description: Layers to request from map server
#% multiple: yes
#% required: yes
#%end

#%option G_OPT_R_OUTPUT
#% description: Name for output raster map
#%end

#%option
#% key: srs
#% type: integer
#% description: EPSG number of source projection for request 
#% guisection: Request properties
#% required: yes
#%end

#%option
#% key: region
#% type: string
#% description: Named region to request data for. Current region used if omitted
#% guisection: Request properties
#%end

#%option
#% key: wms_version
#% type:string
#% description:WMS standard
#% options:1.1.1,1.3.0
#% answer:1.1.1
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
#% description: Addition query parameters for server (only with 'd' flag)
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
#% description: Color of map background (only with 'd' flag)
#% guisection: Map style
#%end

#%flag
#% key: o
#% description: Don't request transparent data
#% guisection: Map style
#%end

#%flag
#% key: c
#% description: Get capabilities
#% guisection: Request properties
#% suppress_required: yes
#%end

#%flag
#% key: d
#% description: Do not use GDAL WMS driver
#%end

import os
import sys
sys.path.insert(1, os.path.join(os.path.dirname(sys.path[0]), 'etc', 'r.in.wms'))

import grass.script as grass

def main():
    if flags['d']:
        grass.debug("Using own driver")
        from wms_drv import WMSDrv
        wms = WMSDrv()
    else:
        grass.debug("Using GDAL WMS driver")
        from wms_gdal_drv import WMSGdalDrv
        wms = WMSGdalDrv()
    
    if flags['c']:
        wms.GetCapabilities(options)
    else:
        wms.GetMap(options, flags)  
    
    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
