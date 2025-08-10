#!/usr/bin/env python3

############################################################################
#
# MODULE:        t.rast.import
# AUTHOR(S):     Soeren Gebbert
#
# PURPOSE:       Import a space time raster dataset
# COPYRIGHT:     (C) 2011-2017 by the GRASS Development Team
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

# %module
# % description: Imports space time raster dataset.
# % keyword: temporal
# % keyword: import
# % keyword: raster
# % keyword: time
# % keyword: create project
# %end

# %option G_OPT_F_INPUT
# %end

# %option G_OPT_STRDS_OUTPUT
# %end

# %option
# % key: basename
# % type: string
# % label: Basename of the new generated output maps
# % description: A numerical suffix separated by an underscore will be attached to create a unique identifier
# % required: no
# % multiple: no
# %end

# %option G_OPT_M_DIR
# % key: directory
# % description: Path to the extraction directory
# % answer: /tmp
# %end

# %option
# % key: title
# % type: string
# % description: Title of the new space time dataset
# % required: no
# % multiple: no
# %end

# %option
# % key: description
# % type: string
# % description: Description of the new space time dataset
# % required: no
# % multiple: no
# %end

# %option
# % key: project
# % type: string
# % description: Create a new project and import the data into it. Do not run this module in parallel or interrupt it when a new project should be created
# % required: no
# % multiple: no
# %end

# %option G_OPT_MEMORYMB
# %end

# %flag
# % key: r
# % description: Set the current region from the last map that was imported
# %end

# %flag
# % key: l
# % description: Link the raster files using r.external
# %end

# %flag
# % key: e
# % description: Extend project extents based on new dataset
# %end

# %flag
# % key: o
# % label: Override projection check (use current projects's CRS)
# % description: Assume that the dataset has same coordinate reference system as the current location
# %end

# %flag
# % key: c
# % description: Create the project specified by the "project" parameter and exit. Do not import the space time raster datasets.
# %end

import grass.script as gs


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    output = options["output"]
    directory = options["directory"]
    title = options["title"]
    descr = options["description"]
    location = options["project"]
    base = options["basename"]
    memory = options["memory"]
    set_current_region = flags["r"]
    link = flags["l"]
    exp = flags["e"]
    overr = flags["o"]
    create = flags["c"]

    tgis.init()

    tgis.import_stds(
        input,
        output,
        directory,
        title,
        descr,
        location,
        link,
        exp,
        overr,
        create,
        "strds",
        base,
        set_current_region,
        memory,
    )


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
