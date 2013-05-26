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
#% description: Downloads and imports data from WMS server.
#% keywords: raster
#% keywords: import
#% keywords: WMS
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

#%option G_OPT_R_OUTPUT
#% description: Name for output raster map
#%end

#%option
#% key: srs
#% type: integer
#% description: EPSG number of source projection for request 
#% answer:4326 
#% guisection: Request properties
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
#% options:nearest,linear,cubic,cubicspline
#% answer:nearest
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

#%flag
#% key: c
#% description: Get capabilities
#% guisection: Request properties
#% suppress_required: yes
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
#% gisprompt: old,bin,file
#% description: Capabilities file to load
#%end

#%option G_OPT_F_OUTPUT
#% key: capfile_output
#% required: no
#% description: File where capabilities will be saved (only with 'c' flag).
#%end

import os
import sys
sys.path.insert(1, os.path.join(os.path.dirname(sys.path[0]), 'etc', 'r.in.wms'))

import grass.script as grass

def GetRegionParams(opt_region):

    # set region 
    if opt_region:  
        reg_spl = opt_region.strip().split('@', 1)
        reg_mapset = '.'
        if len(reg_spl) > 1:
            reg_mapset = reg_spl[1]
            
        if not grass.find_file(name = reg_spl[0], element = 'windows', mapset = reg_mapset)['name']:
             grass.fatal(_("Region <%s> not found") % opt_region)
    
    if opt_region:
        s = grass.read_command('g.region',
                                quiet = True,
                                flags = 'ug',
                                region = opt_region)
        region_params = grass.parse_key_val(s, val_type = float)
    else:
        region_params = grass.region()

    return region_params

def main():


    if 'GRASS' in options['driver']:
        grass.debug("Using GRASS driver")
        from wms_drv import WMSDrv
        wms = WMSDrv()
    elif 'GDAL' in options['driver']:
        grass.debug("Using GDAL WMS driver")
        from wms_gdal_drv import WMSGdalDrv
        wms = WMSGdalDrv()
    
    if flags['c']:
        wms.GetCapabilities(options)
    else:
        from wms_base import GRASSImporter
        options['region'] = GetRegionParams(options['region'])
        fetched_map = wms.GetMap(options, flags)

        grass.message(_("Importing raster map into GRASS..."))
        if not fetched_map:
            grass.warning(_("Nothing to import.\nNo data has been downloaded from wms server."))
            return
        importer = GRASSImporter(options['output'])
        importer.ImportMapIntoGRASS(fetched_map)

    return 0


if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
