#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.rast.export
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Export a space time raster dataset
# COPYRIGHT:    (C) 2011 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (version 2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Exports space time raster dataset.
#% keywords: temporal
#% keywords: export
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option G_OPT_F_OUTPUT
#% description: Name of a space time raster dataset archive
#%end

#%option G_OPT_M_DIR
#% key: workdir
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
#% description: The export format of a single raster map. Supported are GTiff, AAIGrid via r.out.gdal and the GRASS package format of r.pack.
#% required: no
#% multiple: no
#% options: GTiff,AAIGrid,pack
#% answer: GTiff
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
    workdir = options["workdir"]
    where = options["where"]
    _format = options["format"]

    # Make sure the temporal database exists
    tgis.init()
    # Export the space time raster dataset
    tgis.export_stds(
        _input, output, compression, workdir, where, _format, "strds")

############################################################################
if __name__ == "__main__":
    options, flags = grass.parser()
    main()
