#!/usr/bin/env python

############################################################################
#
# MODULE:       r.in.wms
#
# AUTHOR(S):    Cedric Shock, 2006
#               Pythonized by Martin Landa <landa.martin gmail.com>, 2009
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
#%  keywords: raster, import, wms
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
#%option
#% key: output
#% type: string
#% description: Name for output raster map
#% gisprompt: new,cell,raster
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
#% key: wgetoptions
#% type: string
#% description: Additional options for wget
#% answer: -c -t 5 -nv
#% required : no
#% guisection: Download
#%end
#%option
#% key: curloptions
#% type: string
#% description: Additional options for curl
#% answer: -C - --retry 5 -s -S
#% required : no
#% guisection: Download
#%end
#%option
#% key: method
#% type: string
#% description: Reprojection method to use
#% options:nearest,bilinear,cubic,cubicspline
#% answer:nearest
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
import xml.sax.handler
HandlerBase=xml.sax.handler.ContentHandler
from xml.sax import make_parser

import grass

wmsPath = os.path.join(os.getenv('GISBASE'), 'etc', 'r.in.wms')
sys.path.append(wmsPath)
print wmsPath
import wms_request
import wms_download

class ProcessCapFile(HandlerBase):
    """
    A SAX handler for the capabilities file
    """
    def __init__(self):
        self.inTag = {}
        for tag in ('layer', 'name', 'style',
                    'title', 'srs'):
            self.inTag[tag] = False
        self.value = ''
        
        self.layers = []
        
    def startElement(self, name, attrs):
        if self.inTag.has_key(name.lower()):
            self.inTag[name.lower()] = True
        
        if name.lower() == 'layer':
            self.layers.append({})
        
    def endElement(self, name):
        if self.inTag.has_key(name.lower()):
            self.inTag[name.lower()] = False

        for tag in ('name', 'title', 'srs'):
            if name.lower() != tag:
                continue
            if self.inTag['style']:
                if not self.layers[-1].has_key('style'):
                    self.layers[-1]['style'] = {}
                if not self.layers[-1]['style'].has_key(tag):
                    self.layers[-1]['style'][tag] = []
                self.layers[-1]['style'][tag].append(self.value)
            elif self.inTag['layer']:
                self.layers[-1][tag] = self.value
            
        if name.lower() in ('name', 'title', 'srs'):
            self.value = ''
        
    def characters(self, ch):
        if self.inTag['name'] or \
                self.inTag['title'] or \
                self.inTag['srs']:
            self.value += ch
    
    def getLayers(self):
        """Print list of layers"""
        for ly in self.layers:
            print "LAYER: " + ly['name']
            if ly.has_key('title'):
                print "  Title: " + ly['title']
            if ly.has_key('srs'):
                print "  SRS: " + ly['srs']
            if ly.has_key('style'):
                for idx in range(len(ly['style']['name'])):
                    print "  STYLE: " + ly['style']['name'][idx]
                    print "    Style title: " + ly['style']['title'][idx]
        
def list_layers():
    """Get available layers from WMS server"""
    qstring = "service=WMS&request=GetCapabilities&" + options['wmsquery']
    grass.debug("POST-data: %s" % qstring)
    
    # download capabilities file
    grass.verbose("List of layers for server <%s>:" % options['mapserver'])
    cap_file = urllib.urlopen(options['mapserver'] + qstring)
    if not cap_file:
        grass.fatal("Unable to get capabilities of <%s>" % options['mapserver'])

    # parse file with sax
    cap_xml = ProcessCapFile()
    try:
        xml.sax.parse(cap_file, cap_xml)
    except xml.sax.SAXParseException, err:
        grass.fatal("Reading capabilities failed. "
                    "Unable to parse XML document: %s" % err)

    cap_xml.getLayers()
    
def main():
    if flags['l']:
        # list of available layers
        list_layers()
        return 0
    elif not options['output']:
        grass.fatal("No output map specified")

    # set directory for download
    if not options['folder']:
        options['folder'] = os.path.join(grass.gisenv()['GISDBASE'], 'wms_download')
    
    # region settings
    if options['region']:
        if not grass.find_file(name = options['region'], element = 'windows')['name']:
            grass.fatal("Region <%s> not found" % options['region'])
    
    if not flags['d']:
        # request data first
        request = wms_request.WMSRequest(flags, options)
        request.GetTiles()
        if not request:
            grass.fatal("WMS request failed")
    
    # download data
    download = wms_download.WMSDownload(flags, options)
    download.GetTiles(request.GetRequests()) ## ??
    
    # list of files
    files = []
    for item in request.GetRequests():
        files.append(item['output'])
    files = ','.join(files)
    
    gdalwarp.GDALWarp(flags, options)
    
    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
