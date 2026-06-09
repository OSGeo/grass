#!/usr/bin/env python3

############################################################################
#
# MODULE:    t.rast.univar
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:    Calculates univariate statistics from the non-null cells for each registered raster map of a space time raster dataset
# COPYRIGHT:    (C) 2011-2017, Soeren Gebbert and the GRASS Development Team
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
# % description: Calculates univariate statistics from the non-null cells for each registered raster map of a space time raster dataset.
# % keyword: temporal
# % keyword: statistics
# % keyword: raster
# % keyword: time
# % keyword: parallel
# %end

# %option G_OPT_STRDS_INPUT
# %end

# %option G_OPT_R_INPUT
# % key: zones
# % description: Raster map used for zoning, must be of type CELL
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
# % key: r
# % description: Use the raster map regions for univar statistical calculation instead of the current region
# %end

# %flag
# % key: u
# % description: Suppress printing of column names
# % guisection: Formatting
# %end

# %rules
# % requires: percentile,-e
# %end

import grass.script as gs

############################################################################


def main():
    # Get the options and flags
    options, flags = gs.parser()

    # Define variables
    input = options["input"]
    zones = options["zones"]
    output = options["output"]
    nprocs = int(options["nprocs"])
    where = options["where"]
    region_relation = options["region_relation"]
    extended = flags["e"]
    no_header = flags["u"]
    rast_region = bool(flags["r"])
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

    # lazy imports
    import grass.temporal as tgis

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
    # Make sure the temporal database exists
    tgis.init()

    if not output or output == "-":
        output = None

    # Check if zones map exists and is of type CELL
    if zones:
        if gs.raster.raster_info(zones)["datatype"] != "CELL":
            gs.fatal(_("Zoning raster must be of type CELL"))

    tgis.print_gridded_dataset_univar_statistics(
        "strds",
        input,
        output,
        where,
        extended,
        percentile=percentile,
        no_header=no_header,
        fs=separator,
        zones=zones,
        rast_region=rast_region,
        region_relation=region_relation,
        nprocs=nprocs,
        format=output_format,
    )


if __name__ == "__main__":
    main()
