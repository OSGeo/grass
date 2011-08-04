#!/usr/bin/env python

############################################################################
#
# MODULE:       r.in.wms
#
# AUTHOR(S):    Cedric Shock, 2006
#               Upgraded for GRASS 7 by Martin Landa <landa.martin gmail.com>, 2009
#
# PURPOSE:      To import data from web mapping servers
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
#%  description: Downloads and imports data from WMS servers.
#%  keywords: raster
#%  keywords: import
#%  keywords: wms
#%end
#%flag
#% key: l
#% description: List available layers and exit
#% guisection: Request
#%end
#%flag
#% key: d
#% description: Skip to downloading (to resume downloads faster)
#% guisection: Download
#%end
#%flag
#% key: o
#% description: Don't request transparent data
#% guisection: Request
#%end
#%flag
#% key: c
#% description: Clean existing data out of download directory
#% guisection: Download
#%end
#%flag
#% key: k
#% description: Keep band numbers instead of using band color names
#% guisection: Import
#%end
#%flag
#% key: p
#% description: Don't reproject the data, just patch it
#% guisection: Import
#%end
#%flag
#% key: g
#% label: Use GET method instead of POST data method
#% description: This may be needed to connect to servers which lack POST capability
#% guisection: Request
#%end
#%flag
#% key: a
#% description: Use GDAL WMS driver
#% guisection: Request
#%end
#%option G_OPT_R_OUTPUT
#% required : no
#% guisection: Import
#%end
#%option
#% key: mapserver
#% type: string
#% description: Mapserver to request data from
#% required: yes
#% guisection: Request
#%end
#%option
#% key: layers
#% type: string
#% description: Layers to request from map server
#% multiple: yes
#% required: no
#% guisection: Request
#%end
#%option
#% key: styles
#% type: string
#% description: Styles to request from map server
#% multiple: yes
#% required: no
#% guisection: Request
#%end
#%option
#% key: srs
#% type: string
#% description: Source projection to request from server
#% answer:EPSG:4326
#% guisection: Request
#%end
#%option
#% key: format
#% type: string
#% description: Image format requested from the server
#% options: geotiff,tiff,jpeg,gif,png
#% answer: geotiff
#% required: yes
#% guisection: Request
#%end
#%option
#% key: wmsquery
#% type:string
#% description: Addition query options for server
#% answer: version=1.1.1
#% guisection: Request
#%end
#%option
#% key: maxcols
#% type: integer
#% description: Maximum columns to request at a time
#% answer: 1024
#% required : yes
#% guisection: Request
#%end
#%option
#% key: maxrows
#% type: integer
#% description: Maximum rows to request at a time
#% answer: 1024
#% required : yes
#% guisection: Request
#%end
#%option
#% key: tileoptions
#% type: string
#% description: Additional options for r.tileset
#% required : no
#%end
#%option
#% key: region
#% type: string
#% description: Named region to request data for. Current region used if omitted
#% required : no
#% guisection: Request
#%end
#%option
#% key: folder
#% type: string
#% description: Folder to save downloaded data to (default $GISDBASE/wms_download)
#% required : no
#% guisection: Download
#%end
#%option
#% key: method
#% type: string
#% description: Reprojection method to use
#% options:near,bilinear,cubic,cubicspline
#% answer:near
#% required: yes
#% guisection: Import
#%end
#%option
#% key: cap_file
#% type: string
#% label: Filename to save capabilities XML file to
#% description: Requires list available layers flag
#% required: no
#% guisection: Request
#%end

import os
import sys
import tempfile
import urllib
import xml.sax

import grass.script as grass

wmsPath = os.path.join(os.getenv('GISBASE'), 'etc', 'r.in.wms')
sys.path.append(wmsPath)
try:
    import wms_parse
    import wms_request
    import wms_download
    import gdalwarp
    import wms_gdal
except ImportError:
    pass

def list_layers():
    """Get list of available layers from WMS server"""
    qstring = "service=WMS&request=GetCapabilities&" + options['wmsquery']
    grass.debug("POST-data: %s" % qstring)
    
    # download capabilities file
    grass.verbose("List of layers for server <%s>:" % options['mapserver'])
    url = options['mapserver'] + '?' + qstring
    try:
        if options['cap_file']:
            cap_file, headers = urllib.urlretrieve(url, options['cap_file'])
        else:
            cap_file = urllib.urlopen(url, options['mapserver'] + '?' + qstring)
    except IOError:
        grass.fatal(_("Unable to get capabilities of '%s'") % options['mapserver'])
    
    # check DOCTYPE first
    if options['cap_file']:
        if headers['content-type'] != 'application/vnd.ogc.wms_xml':
            grass.fatal(_("Unable to get capabilities: %s") % url)
    else:
        if cap_file.info()['content-type'] != 'application/vnd.ogc.wms_xml':
            grass.fatal(_("Unable to get capabilities: %s") % url)

    # parse file with sax
    cap_xml = wms_parse.ProcessCapFile()
    try:
        xml.sax.parse(cap_file, cap_xml)
    except xml.sax.SAXParseException, err:
        grass.fatal(_("Reading capabilities failed. "
                      "Unable to parse XML document: %s") % err)
    
    cap_xml.getLayers()
    
def main():
    if flags['l']:
        # list of available layers
        list_layers()
        return 0
    elif not options['output']:
        grass.fatal(_("No output map specified"))
    
    if options['cap_file'] and not flags['l']:
        grass.warning(_("Option <cap_file> ignored. It requires '-l' flag."))
    
    # set directory for download
    if not options['folder']:
        options['folder'] = os.path.join(grass.gisenv()['GISDBASE'], 'wms_download')
    
    # region settings
    if options['region']:
        if not grass.find_file(name = options['region'], element = 'windows')['name']:
            grass.fatal(_("Region <%s> not found") % options['region'])

    request = wms_request.WMSRequest(flags, options)    
    if not flags['d']:
        # request data first
        request.GetTiles()
    if not request:
        grass.fatal(_("WMS request failed"))
    
    if flags['a']:
        # use GDAL WMS driver
        ### TODO: use GDAL Python bindings instead
        if not wms_gdal.checkGdalWms():
            grass.fatal(_("GDAL WMS driver is not available"))

        # create local service description XML file
        gdalWms = wms_gdal.GdalWms(options, request)
        options['input'] = gdalWms.GetFile()
    else:
        # download data
        download = wms_download.WMSDownload(flags, options)
        download.GetTiles(request.GetRequests())
    
        # list of files
        files = []
        for item in request.GetRequests():
            files.append(item['output'])
        files = ','.join(files)
        options['input'] = files

    # add flags for r.in.gdalwarp
    flags['e'] = False
    flags['c'] = True
    options['warpoptions'] = ''
    
    return gdalwarp.GDALWarp(flags, options).run()
    
    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
