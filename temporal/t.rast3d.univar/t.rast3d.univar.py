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
# % keyword: parallel
# %end

# %option G_OPT_STR3DS_INPUT
# %end

# %option G_OPT_R3_INPUT
# % key: zones
# % label: Raster map with zones to compute statistics for
# % description: Raster map with zones to compute statistics for
# % required: no
# %end

# %option G_OPT_M_NPROCS
# % required: no
# %end

# %option G_OPT_F_OUTPUT
# % required: no
# %end

# %option
# % key: percentile
# % type: double
# % required: no
# % multiple: yes
# % options: 0-100
# % description: Percentile to calculate (requires extended statistics flag)
# %end

# %option G_OPT_T_WHERE
# % guisection: Selection
# %end

# %option
# % key: region_relation
# % description: Process only maps with this spatial relation to the current computational region
# % guisection: Selection
# % options: overlaps,contains,is_contained
# % required: no
# % multiple: no
# %end

# %option G_OPT_F_SEP
# % label: Field separator character between the output columns
# % answer:
# % guisection: Formatting
# %end

# %option G_OPT_F_FORMAT
# % options: plain,json,csv
# % descriptions: plain;Plain text output;json;JSON (JavaScript Object Notation);csv;CSV (Comma Separated Values)
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

# %rules
# % requires: percentile,-e
# %end

import grass.script as gs

############################################################################


def main():
    # Get the options
    options, flags = gs.parser()

    # Define variables
    input = options["input"]
    zones = options["zones"]
    output = options["output"]
    nprocs = gs.resolve_nprocs(options["nprocs"])
    where = options["where"]
    region_relation = options["region_relation"]
    extended = flags["e"]
    no_header = flags["s"]
    separator = gs.separator(options["separator"])
    output_format = options.get("format", "plain")

    if output_format == "csv":
        if not separator:
            separator = ","
        elif len(separator) > 1:
            gs.fatal(
                _("A standard CSV separator (delimiter) is only one character long")
            )

    elif output_format == "json":
        if no_header:
            gs.fatal(_("Column names are always included in JSON output"))
        if separator:
            gs.fatal(_("Separator option is not allowed with JSON format"))

    elif not separator:
        separator = "|"

    percentile = None
    if options["percentile"]:
        try:
            percentile = list(map(float, options["percentile"].split(",")))
        except ValueError:
            gs.fatal(
                _("<{}> is not valid input to the percentile option").format(
                    options["percentile"]
                )
            )

    # lazy imports
    import grass.temporal as tgis

    # Make sure the temporal database exists
    tgis.init()

    if not output:
        output = None
    if output == "-":
        output = None

    tgis.print_gridded_dataset_univar_statistics(
        "str3ds",
        input,
        output,
        where,
        extended,
        percentile=percentile,
        no_header=no_header,
        fs=separator,
        rast_region=False,
        region_relation=region_relation,
        zones=zones,
        nprocs=nprocs,
        format=output_format,
    )


if __name__ == "__main__":
    main()
