#!/usr/bin/env python3

############################################################################
#
# MODULE:	t.vect.list
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	List registered maps of a space time vector dataset
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
# % description: Lists registered maps of a space time vector dataset.
# % keyword: temporal
# % keyword: map management
# % keyword: vector
# % keyword: list
# % keyword: time
# %end

# %option G_OPT_STVDS_INPUT
# %end

# %option
# % key: order
# % type: string
# % description: Sort the space time dataset by category
# % guisection: Formatting
# % required: no
# % multiple: yes
# % options: id,name,layer,creator,mapset,temporal_type,creation_time,start_time,end_time,north,south,west,east,points,lines,boundaries,centroids,faces,kernels,primitives,nodes,areas,islands,holes,volumes
# % answer: start_time
# %end

# %option
# % key: columns
# % type: string
# % description: Columns to be printed to stdout
# % guisection: Selection
# % required: no
# % multiple: yes
# % options: id,name,layer,creator,mapset,temporal_type,creation_time,start_time,end_time,north,south,west,east,points,lines,boundaries,centroids,faces,kernels,primitives,nodes,areas,islands,holes,volumes
# % answer: name,layer,mapset,start_time,end_time
# %end

# %option G_OPT_T_WHERE
# % guisection: Selection
# %end

# %option
# % key: method
# % type: string
# % description: Method used for data listing
# % required: no
# % multiple: no
# % options: cols,comma,delta,deltagaps,gran
# % answer: cols
# %end

# %option G_OPT_F_FORMAT
# % options: plain,line,json,yaml,csv
# % descriptions: plain;Plain text output;line;Comma separated list of map names;json;JSON (JavaScript Object Notation);yaml;YAML (YAML Ain't Markup Language);csv;CSV (Comma Separated Values);
# % guisection: Formatting
# %end

# %option G_OPT_F_SEP
# % label: Field separator character between the output columns
# % guisection: Formatting
# %end

# %option G_OPT_F_OUTPUT
# % required: no
# %end

# %flag
# % key: u
# % description: Suppress printing of column names
# % guisection: Formatting
# %end

import grass.script as gs


def message_option_value_excludes_option_value(
    option_name, option_value, excluded_option_name, excluded_option_value, reason
):
    return _(
        "Combining {option_name}={option_value} and "
        "{excluded_option_name}={excluded_option_value} is not allowed. {reason}"
    ).format(
        option_name=option_name,
        option_value=option_value,
        excluded_option_name=excluded_option_name,
        excluded_option_value=excluded_option_value,
        reason=reason,
    )


def message_option_value_excludes_flag(option_name, option_value, flag_name, reason):
    return _(
        "The flag -{flag_name} is not allowed with {option_name}={option_value}."
        " {reason}"
    ).format(
        flag_name=flag_name,
        option_name=option_name,
        option_value=option_value,
        reason=reason,
    )


############################################################################


def main():
    options, flags = gs.parser()
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    columns = options["columns"]
    order = options["order"]
    where = options["where"]
    separator = gs.separator(options["separator"])
    method = options["method"]
    header = flags["u"]
    output = options["output"]
    output_format = options["format"]

    if output_format == "csv":
        if len(separator) > 1:
            gs.fatal(
                message_option_value_excludes_option_value(
                    option_name="format",
                    option_value=output_format,
                    excluded_option_name="separator",
                    excluded_option_value=separator,
                    reason=_(
                        "A standard CSV separator (delimiter) is only one character "
                        "long"
                    ),
                )
            )
        if separator == "|":
            # We use comma as the default for separator, so we override the pipe.
            # This does not allow for users to generate CSV with pipe, but unlike
            # the C API, the Python interface specs does not allow resetting the default
            # except for setting it to an empty string which does not have a precedence
            # in the current code and the behavior is unclear.
            separator = ","

    if output_format in {"json", "yaml"} and header:
        gs.fatal(
            message_option_value_excludes_flag(
                option_name="format",
                option_value=output_format,
                flag_name="u",
                reason=_("Column names are always included"),
            )
        )
        # We ignore when separator is set for JSON and YAML because of the default
        # value which is always there (see above). Having no default and producing
        # an error when set would be more clear and would fit with using different
        # defaults for plain and CSV formats.
    elif (output_format == "line") and separator == "|":
        # Same as for CSV: Custom default needed.
        # Pipe is currently not supported at all.
        separator = ","

    if output_format == "line":
        columns_list = columns.split(",")
        if len(columns_list) > 1:
            gs.fatal(
                message_option_value_excludes_option_value(
                    option_name="format",
                    option_value=output_format,
                    excluded_option_name="columns",
                    excluded_option_value=columns,
                    reason=_("Only one column is allowed (not {num_columns})").format(
                        num_columns=len(columns_list)
                    ),
                )
            )

    # Make sure the temporal database exists
    tgis.init()

    tgis.list_maps_of_stds(
        "stvds",
        input,
        columns,
        order,
        where,
        separator,
        method,
        header,
        outpath=output,
        output_format=output_format,
    )


if __name__ == "__main__":
    main()
