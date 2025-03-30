#!/usr/bin/env python3
"""
MODULE:    r.in.wms

AUTHOR(S): Stepan Turek <stepan.turek AT seznam.cz>

PURPOSE:   Downloads and imports data from WMS/WMTS/NASA OnEarth server.

COPYRIGHT: (C) 2012-2021 Stepan Turek, and by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.
"""

# %module
# % description: Downloads and imports data from OGC WMS and OGC WMTS web mapping servers.
# % keyword: raster
# % keyword: import
# % keyword: OGC web services
# % keyword: OGC WMS
# % keyword: OGC WMTS
# %end

# %option
# % key: url
# % type: string
# % description: Typically starts with "http://"
# % required: yes
# %end

# %option G_OPT_R_OUTPUT
# % description: Name for output raster map
# %end

# %option
# % key: layers
# % type: string
# % description: Layer(s) to request from the map server
# % multiple: yes
# % required: yes
# %end

# %option
# % key: styles
# % type: string
# % description: Layer style(s) to request from the map server
# % multiple: yes
# % guisection: Map style
# %end

# %option
# % key: format
# % type: string
# % description: Image format requested from the server
# % options: geotiff,tiff,jpeg,gif,png,png8
# % answer: png
# % guisection: Request
# %end

# %option
# % key: srs
# % type: integer
# % description: EPSG code of requested source projection
# % answer:4326
# % guisection: Request
# %end

# %option
# % key: driver
# % type:string
# % description: Driver used for communication with the server
# % descriptions: WMS_GDAL;Download data using GDAL WMS driver;WMS_GRASS;Download data using native GRASS-WMS driver;WMTS_GRASS;Download data using native GRASS-WMTS driver;OnEarth_GRASS;Download data using native GRASS-OnEarth driver;
# % options:WMS_GDAL, WMS_GRASS, WMTS_GRASS, OnEarth_GRASS
# % answer:WMS_GRASS
# % guisection: Connection
# %end

# %option
# % key: wms_version
# % type:string
# % description: WMS standard version
# % options: 1.1.0,1.1.1,1.3.0
# % answer: 1.1.1
# % guisection: Request
# %end

# %option
# % key: maxcols
# % type:integer
# % description: Maximum columns to request at a time
# % answer:512
# % guisection: Request
# %end

# %option
# % key: maxrows
# % type: integer
# % description: Maximum rows to request at a time
# % answer: 512
# % guisection: Request
# %end

# %option
# % key: urlparams
# % type:string
# % description: Additional query parameters to pass to the server
# % guisection: Request
# %end

# %option
# % key: username
# % type:string
# % description: Username for server connection
# % guisection: Connection
# %end

# %option
# % key: password
# % type:string
# % description: Password for server connection
# % guisection: Connection
# %end

# %option
# % key: method
# % type: string
# % description: Interpolation method to use in reprojection
# % options:nearest,linear,cubic,cubicspline
# % answer:nearest
# % required: no
# %end

# %option
# % key: gdal_createopt
# % type: string
# % required: no
# % multiple: yes
# % label: GDAL creation option(s) to pass to the output format driver
# % description: In the form of "NAME=VALUE", separate multiple entries with a comma
# % guisection: Request
# %end

# %option
# % key: region
# % type: string
# % description: Request data for this named region instead of the current region bounds
# % guisection: Request
# %end

# %option
# % key: bgcolor
# % type: string
# % label: Background color
# % description: Format: 0xRRGGBB
# % guisection: Map style
# %end

# %option
# % key: proxy
# % label: HTTP proxy only GDAL driver (GDAL_HTTP_PROXY)
# % type: string
# % description: HTTP proxy
# %end

# %option
# % key: proxy_user_pw
# % label: User and password for HTTP proxy only for GDAL driver (GDAL_HTTP_PROXYUSERPWD). Must be in the form of [user name]:[password].
# % type: string
# % description: User and password for HTTP proxy
# %end

# %option G_OPT_F_BIN_INPUT
# % key: capfile
# % required: no
# % description: Capabilities file to parse (input). It is relevant for WMTS_GRASS and OnEarth_GRASS drivers
# %end

# %option G_OPT_F_OUTPUT
# % key: capfile_output
# % required: no
# % description: File where the server capabilities will be saved ('c' flag)
# %end

# %flag
# % key: c
# % description: Get the server capabilities, print them out, then exit
# % guisection: Request
# % suppress_required: yes
# %end

# %flag
# % key: o
# % description: Do not request transparent data
# % guisection: Map style
# %end

# %flag
# % key: b
# % description: Keep original bands (default: create composite)
# % guisection: Map style
# %end

# %rules
# % exclusive: capfile_output, capfile
# %end

import os
import sys

import grass.script as gs
from grass.script.utils import decode


def GetRegionParams(opt_region):
    # set region
    if opt_region:
        reg_spl = opt_region.strip().split("@", 1)
        reg_mapset = "."
        if len(reg_spl) > 1:
            reg_mapset = reg_spl[1]

        if not gs.find_file(name=reg_spl[0], element="windows", mapset=reg_mapset)[
            "name"
        ]:
            gs.fatal(_("Region <%s> not found") % opt_region)

    if opt_region:
        s = gs.read_command("g.region", quiet=True, flags="ug", region=opt_region)
        region_params = gs.parse_key_val(decode(s), val_type=float)
    else:
        region_params = gs.region()

    return region_params


def main():
    sys.path.insert(1, os.path.join(os.path.dirname(sys.path[0]), "etc", "r.in.wms"))

    if "GRASS" in options["driver"]:
        gs.debug("Using GRASS driver")
        from wms_drv import WMSDrv

        wms = WMSDrv()
    elif "GDAL" in options["driver"]:
        gs.debug("Using GDAL WMS driver")
        from wms_gdal_drv import WMSGdalDrv

        if options["gdal_createopt"]:
            create_options = options["gdal_createopt"].split(",")
        else:
            create_options = None
        wms = WMSGdalDrv(create_options)

    if flags["c"]:
        wms.GetCapabilities(options)
    else:
        from wms_base import GRASSImporter

        # set proxy
        if options["proxy"] and options["proxy_user_pw"]:
            wms.setProxy(options["proxy"], options["proxy_user_pw"])
        if options["proxy"]:
            wms.setProxy(options["proxy"])
            if "GRASS" in options["driver"]:
                gs.warning(
                    _(
                        "The proxy will be ignored by the chosen GRASS driver. It is "
                        "only used with the GDAL driver."
                    )
                )

        options["region"] = GetRegionParams(options["region"])
        fetched_map = wms.GetMap(options, flags)

        gs.message(_("Importing raster map into GRASS..."))
        if not fetched_map:
            gs.warning(
                _("Nothing to import.\nNo data has been downloaded from wms server.")
            )
            return None
        importer = GRASSImporter(options["output"], (not flags["b"]))
        importer.ImportMapIntoGRASS(fetched_map)

    return 0


if __name__ == "__main__":
    options, flags = gs.parser()
    sys.exit(main())
