#!/usr/bin/env python3

############################################################################
#
# MODULE:	v.in.wfs
# AUTHOR(S):	Markus Neteler. neteler itc it
#               Hamish Bowman
#               Converted to Python by Glynn Clements
#               German ALKIS support added by Veronica Köß
# PURPOSE:	WFS support
# COPYRIGHT:	(C) 2006-2024 Markus Neteler and the GRASS Development Team
#
# 		This program is free software under the GNU General
# 		Public License (>=v2). Read the file COPYING that
# 		comes with GRASS for details.
#
# GetFeature example:
# http://mapserver.gdf-hannover.de/cgi-bin/grassuserwfs?REQUEST=GetFeature&SERVICE=WFS&VERSION=1.0.0
#############################################################################

#
# TODO: suggest to depend on the OWSLib for OGC web service needs
#       http://pypi.python.org/pypi/OWSLib
#

# %Module
# % description: Imports GetFeature from a WFS server.
# % keyword: vector
# % keyword: import
# % keyword: OGC web services
# % keyword: OGC WFS
# %end
# %option
# % key: url
# % type: string
# % description: Base URL starting with 'http' and ending in '?'
# % required: yes
# %end
# %option G_OPT_V_OUTPUT
# %end
# %option
# % key: name
# % type: string
# % description: Comma separated names of data layers to download
# % multiple: yes
# % required: no
# %end
# %option
# % key: layer
# % type: string
# % description: Name of data layers to import
# % multiple: yes
# % required: no
# %end
# %option
# % key: srs
# % type: string
# % label: Specify alternate spatial reference system (example: EPSG:4326)
# % description: The given code must be supported by the server, consult the capabilities file
# % required: no
# %end
# %option
# % key: maximum_features
# % type: integer
# % label: Maximum number of features to download
# % description: (default: unlimited)
# %end
# %option
# % key: start_index
# % type: integer
# % label: Skip earlier feature IDs and start downloading at this one
# % description: (default: start with the first feature)
# %end
# %option
# % key: version
# % type: string
# % required: no
# % multiple: no
# % description: version of WFS, e.g.:1.0.0 or 2.0.0
# % answer: 1.0.0
# %end
# %option
# % key: username
# % type: string
# % required: no
# % multiple: no
# % label: Username or file with username or environment variable name with username
# %end
# %option
# % key: password
# % type: string
# % required: no
# % multiple: no
# % label: Password or file with password or environment variable name with password
# %end
# %flag
# % key: l
# todo #% description: List available layers and exit
# % description: Download server capabilities to 'wms_capabilities.xml' in the current directory and exit
# % suppress_required: yes
# %end
# %flag
# % key: r
# % description: Restrict fetch to features which touch the current region
# %end


import os
from pathlib import Path
import sys
from grass.script.utils import try_remove
from grass.script import core as grass

from owslib.wfs import WebFeatureService

def main():
    out = options["output"]
    wfs_url = options["url"]
    version_num = options["version"]

    this_wfs = wfs11 = WebFeatureService(url=wfs_url, version=version_num)
    print(f'Connected to {this_wfs.identification.title}.')

    if 'GetCapabilities' in list(this_wfs.operations):
        if flags["l"]:
            wfs_url = wfs.getcapabilities().geturl()

        print(wfs_url)

        tmp = grass.tempfile()
        tmpxml = tmp + ".xml"

        grass.debug(wfs_url)

        # GTC Downloading WFS features
        grass.message(_("Retrieving data..."))
        try:
            inf = wfs.getcapabilities() #get capabilities at this stage
        except HTTPError as e:
            # GTC WFS request HTTP failure
            grass.fatal(_("Server couldn't fulfill the request.\nError code: %s") % e.code)
        except URLError as e:
            # GTC WFS request network failure
            grass.fatal(_("Failed to reach the server.\nReason: %s") % e.reason)

        outf = open(tmpxml, "wb")
        while True:
            s = inf.read()
            if not s:
                break
            outf.write(s)
        inf.close()
        outf.close()

        if flags["l"]:
            import shutil

            if os.path.exists("wms_capabilities.xml"): #wfs_capabilities?
                grass.fatal(_('A file called "wms_capabilities.xml" already exists here'))
            # os.move() might fail if the temp file is on another volume, so we copy instead
            shutil.copy(tmpxml, "wms_capabilities.xml")
            try_remove(tmpxml)
            sys.exit(0)
    else:
        grass.message(_("Get capabilities not supported by server..."))


    """set the variable stuff here"""
    if options["name"]:
        if ',' in options["name"]: #multiple layers separated by ,
            bulk_download = True
            this_layer_name = options["name"].split(',') #an array here
        else:
            this_layer_name = options["name"]

    this_srs = None
    if options["srs"]:
        srs_check = tuple(int(x) for x in version_num.split("."))
        if srs_check == (1, 1, 0) or srs_check == (2, 0, 0):
            if 'urn:x-ogc:def:crs:EPSG:' in options["srs"] :
                this_srs = options["srs"] #check the SRS by version
        elif srs_check == (1, 0, 0):
            if 'www.opengis.net/gml/srs/epsg.xml#' in options["srs"] :
                this_srs = options["srs"]
        else:
            grass.message(_("Specified SRS couldnot be set."))

    this_max_features = None    
    if options["maximum_features"]:
        this_max_features = options["maximum_features"]
        if int(options["maximum_features"]) < 1:
            # GTC Invalid WFS maximum features parameter
            grass.fatal(_("Invalid maximum number of features"))
        
    this_start_index = None
    if options["start_index"]:
        this_start_index = options["start_index"]
        if int(options["start_index"]) < 1:
            # GTC Invalid WFS start index parameter
            grass.fatal(_('Features begin with index "1"'))
        
    this_bbox = None
    if flags["r"]:
        this_bbox = grass.read_command("g.region", flags="w").split("=")[1]

    if 'GetFeature' in this_wfs.operations :
        grass.message(_("Importing data..."))
        try:
            if options["layer"]:
                if options["layer"] in list(this_wfs.contents):
                    response = this_wfs.getfeature(typename=options["layer"],
                        bbox=this_bbox,
                        srsname=this_srs,
                        maxfeatures=this_max_features,
                        startindex=this_start_index)
                    if response:
                        file_out = open(out, 'wb')
                        file_out.write(bytes(response.read(), 'UTF-8'))
                        file_out.close()
            else:
                grass.run_command(
                    "v.in.ogr",
                    flags="o",
                    input=tmpxml,
                    output=out)
                print('bulk download')

            grass.message(_("Vector map <%s> imported from WFS.") % out)
        except Exception:
            grass.message(_("WFS import failed"))
        finally:
            try_remove(tmpxml)
        """
        use v.in.ogr for file format conversion than fetching the data
        """
    else:
        grass.message(_("Get features not supported by server..."))

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
