#!/usr/bin/env python3

############################################################################
#
# MODULE:	t.vect.univar
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Calculates univariate statistics of attributes for each registered vector map of a space time vector dataset
# COPYRIGHT:	(C) 2011-2017 by the GRASS Development Team
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
# % description: Calculates univariate statistics of attributes for each registered vector map of a space time vector dataset
# % keyword: temporal
# % keyword: statistics
# % keyword: vector
# % keyword: time
# %end

# %option G_OPT_STVDS_INPUT
# %end

# %option G_OPT_F_OUTPUT
# % required: no
# %end

# %option G_OPT_V_FIELD
# %end

# %option G_OPT_DB_COLUMN
# % required: yes
# %end

# %option G_OPT_T_WHERE
# % guisection: Selection
# % key: twhere
# %end

# %option G_OPT_DB_WHERE
# % guisection: Selection
# %end

# %option G_OPT_V_TYPE
# % options: point,line,boundary,centroid,area
# % multiple: no
# % answer: point
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
# % key: u
# % description: Suppress printing of column names
# % guisection: Formatting
# %end

import grass.script as gs

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    output = options["output"]
    twhere = options["twhere"]
    layer = options["layer"]
    type = options["type"]
    column = options["column"]
    where = options["where"]
    extended = flags["e"]
    no_header = flags["u"]
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

    # Make sure the temporal database exists
    tgis.init()

    if not output:
        output = None
    if output == "-":
        output = None

    tgis.print_vector_dataset_univar_statistics(
        input,
        output,
        twhere,
        layer,
        type,
        column,
        where,
        extended,
        no_header,
        separator,
        format=output_format,
    )


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
