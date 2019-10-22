#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.vect.export
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Export a space time vector dataset.as GRASS specific archive file
# COPYRIGHT:    (C) 2011-2017 by the GRASS Development Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#############################################################################

#%module
#% description: Exports a space time vector dataset as GRASS GIS specific archive file.
#% keyword: temporal
#% keyword: export
#% keyword: vector
#% keyword: time
#%end

#%option G_OPT_STVDS_INPUT
#%end

#%option G_OPT_F_OUTPUT
#% description: Name of a space time vector dataset archive
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
#% label: The export format of a single vector map
#% description: Supported are GML and GPKG via v.out.ogr and the GRASS package format of v.pack
#% required: no
#% multiple: no
#% options: GML,GPKG,pack
#% answer: GML
#%end

#%option G_OPT_T_WHERE
#%end

import grass.script as grass


############################################################################
def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    _input = options["input"]
    output = options["output"]
    compression = options["compression"]
    directory = options["directory"]
    where = options["where"]
    _format = options["format"]

    # Make sure the temporal database exists
    tgis.init()
    # Export the space time vector dataset
    tgis.export_stds(
        _input, output, compression, directory, where, _format, "stvds")

############################################################################
if __name__ == "__main__":
    options, flags = grass.parser()
    main()
