#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:        t.vect.import
# AUTHOR(S):     Soeren Gebbert
#
# PURPOSE:        Import a space time vector dataset archive file
# COPYRIGHT:        (C) 2011-2017 by the GRASS Development Team
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
#% description: Imports a space time vector dataset from a GRASS GIS specific archive file.
#% keyword: temporal
#% keyword: import
#% keyword: vector
#% keyword: time
#% keyword: create location
#%end

#%option G_OPT_F_INPUT
#%end

#%option G_OPT_STVDS_OUTPUT
#%end

#%option
#% key: basename
#% type: string
#% label: Basename of the new generated output maps
#% description: A numerical suffix separated by an underscore will be attached to create a unique identifier
#% required: no
#% multiple: no
#%end

#%option G_OPT_M_DIR
#% key: directory
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
#% description: Create a new location and import the data into it. Do not run this module in parallel or interrupt it when a new location should be created
#% required: no
#% multiple: no
#%end

#%flag
#% key: e
#% description: Extend location extents based on new dataset
#%end

#%flag
#% key: o
#% label: Override projection check (use current location's projection)
#% description: Assume that the dataset has same projection as the current location
#%end

#%flag
#% key: c
#% description: Create the location specified by the "location" parameter and exit. Do not import the space time vector datasets.
#%end

import grass.script as grass


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    output = options["output"]
    directory = options["directory"]
    title = options["title"]
    descr = options["description"]
    location = options["location"]
    base = options["basename"]
    exp = flags["e"]
    overr = flags["o"]
    create = flags["c"]

    tgis.init()

    tgis.import_stds(input, output, directory, title, descr, location,
                     None, exp, overr, create, "stvds", base)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
