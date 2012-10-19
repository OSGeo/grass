#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.vect.export
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Export a space time vector dataset.as GRASS specific archive file
# COPYRIGHT:    (C) 2011 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (version 2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Export a space time vector dataset.as GRASS specific archive file
#% keywords: temporal
#% keywords: export
#%end

#%option G_OPT_STVDS_INPUT
#%end

#%option G_OPT_F_OUTPUT
#% description: Name of a space time vector dataset archive
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
#% description: The export format of a single raster map. Supported are GML via v.out.ogr and the GRASS package format of v.pack.
#% required: no
#% multiple: no
#% options: GML,pack
#% answer: GML
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
        _input, output, compression, workdir, where, _format, "stvds")

############################################################################
if __name__ == "__main__":
    options, flags = grass.parser()
    main()
