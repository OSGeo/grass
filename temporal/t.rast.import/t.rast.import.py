#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:        t.rast.import
# AUTHOR(S):     Soeren Gebbert
#
# PURPOSE:        Import a space time raster dataset
# COPYRIGHT:        (C) 2011 by the GRASS Development Team
#
#                This program is free software under the GNU General Public
#                License (version 2). Read the file COPYING that comes with GRASS
#                for details.
#
#############################################################################

#%module
#% description: Imports space time raster dataset.
#% keywords: temporal
#% keywords: import
#%end

#%option G_OPT_F_INPUT
#%end

#%option G_OPT_STRDS_OUTPUT
#%end

#%option G_OPT_M_DIR
#% key: extrdir
#% description: Path to the extraction directory
#%end

#%option
#% key: title
#% type: string
#% description: Title of the new space time dataset
#% required: no
#% multiple: no
#%end

#%option
#% key: description
#% type: string
#% description: Description of the new space time dataset
#% required: no
#% multiple: no
#%end

#%option
#% key: location
#% type: string
#% description: Create a new location and import the data into it. Please do not run this module in parallel or interrupt it when a new location should be created.
#% required: no
#% multiple: no
#%end

#%flag
#% key: l
#% description: Link the raster files using r.external
#%end

#%flag
#% key: e
#% description: Extend location extents based on new dataset
#%end

#%flag
#% key: o
#% description: Override projection (use location's projection)
#%end

#%option G_OPT_R_BASE
#%end

#%flag
#% key: c
#% description: Create the location specified by the "location" parameter and exit. Do not import the space time raster datasets.
#%end

import grass.script as grass
import grass.temporal as tgis


def main():

    # Get the options
    input = options["input"]
    output = options["output"]
    extrdir = options["extrdir"]
    title = options["title"]
    descr = options["description"]
    location = options["location"]
    base = options["base"]
    link = flags["l"]
    exp = flags["e"]
    overr = flags["o"]
    create = flags["c"]
    
    tgis.init()

    tgis.import_stds(input, output, extrdir, title, descr, location,
                     link, exp, overr, create, "strds", base)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
