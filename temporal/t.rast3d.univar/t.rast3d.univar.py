#!/usr/bin/env python3

############################################################################
#
# MODULE:	t.rast3d.univar
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Calculates univariate statistics from the non-null cells for each registered 3D
#               raster map of a space time 3D raster dataset
# COPYRIGHT:	(C) 2011-2017, Soeren Gebbert and the GRASS Development Team
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
# % description: Calculates univariate statistics from the non-null cells for each registered 3D raster map of a space time 3D raster dataset.
# % keyword: temporal
# % keyword: statistics
# % keyword: raster3d
# % keyword: voxel
# % keyword: time
# %end

# %option G_OPT_STR3DS_INPUT
# %end

# %option G_OPT_R_INPUT
# % key: zones
# % label: Raster map withh zones to compute statistics for
# % description: Raster map withh zones to compute statistics for (needs to be CELL)
# % required: no
# %end

# %option G_OPT_F_OUTPUT
# % required: no
# %end

# %option G_OPT_T_WHERE
# % guisection: Selection
# %end

# %option G_OPT_F_SEP
# % label: Field separator character between the output columns
# % guisection: Formatting
# %end

# %flag
# % key: e
# % description: Calculate extended statistics
# %end

# %flag
# % key: s
# % description: Suppress printing of column names
# % guisection: Formatting
# %end

import grass.script as grass


############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    zones = options["zones"]
    output = options["output"]
    where = options["where"]
    extended = flags["e"]
    no_header = flags["s"]
    separator = grass.separator(options["separator"])

    # Make sure the temporal database exists
    tgis.init()

    if not output:
        output = None
    if output == "-":
        output = None

    # Check if zones map exists and is of type CELL
    if zones:
        if grass.raster.raster_info()["datatype"] != "CELL":
            grass.fatal(_("Zoning raster must be of type CELL"))


if __name__ == "__main__":
    options, flags = grass.parser()
    main()
