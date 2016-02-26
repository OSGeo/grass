#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.rast.export
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Export a space time raster dataset
# COPYRIGHT:    (C) 2011-2014 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (version 2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Exports space time raster dataset.
#% keyword: temporal
#% keyword: export
#% keyword: raster
#% keyword: time
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option G_OPT_F_OUTPUT
#% description: Name of a space time raster dataset archive
#%end

#%option G_OPT_M_DIR
#% key: directory
#% description: Path to the work directory, default is /tmp
#% required: no
#% answer: /tmp
#%end

#%option
#% key: compression
#% type: string
#% description: Compression method of the tar archive
#% required: no
#% multiple: no
#% options: no,gzip,bzip2
#% answer: bzip2
#%end

#%option
#% key: format
#% type: string
#% label: The export format of a single raster map
#% description: Supported are GTiff, AAIGrid via r.out.gdal and the GRASS package format of r.pack
#% required: no
#% multiple: no
#% options: GTiff,AAIGrid,pack
#% answer: GTiff
#%end

#%option
#% key: type
#% type: string
#% label: Data type
#% description: Supported only for GTiff
#% required: no
#% multiple: no
#% options: Byte,Int16,UInt16,Int32,UInt32,Float32,Float64,CInt16,CInt32,CFloat32,CFloat64
#%end

#%option G_OPT_T_WHERE
#%end

import grass.script as grass
import grass.temporal as tgis


############################################################################
def main():

    # Get the options
    _input = options["input"]
    output = options["output"]
    compression = options["compression"]
    directory = options["directory"]
    where = options["where"]
    _format = options["format"]
    _type = options["type"]

    if _type and _format in ["pack", "AAIGrid"]:
        grass.warning(_("Type options is not working with pack format, it will be skipped"))
    # Make sure the temporal database exists
    tgis.init()
    # Export the space time raster dataset
    tgis.export_stds(_input, output, compression, directory, where, _format,
                     "strds", _type)

############################################################################
if __name__ == "__main__":
    options, flags = grass.parser()
    main()
