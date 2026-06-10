#!/usr/bin/env python3

############################################################################
#
# MODULE:   t.rast3d.list
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:  List registered maps of a space time raster3d dataset
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
# % description: Lists registered maps of a space time raster3d dataset.
# % keyword: temporal
# % keyword: map management
# % keyword: list
# % keyword: raster3d
# % keyword: voxel
# % keyword: time
# %end

# %option G_OPT_STR3DS_INPUT
# %end

# %option
# % key: order
# % type: string
# % description: Order the space time dataset by category
# % guisection: Formatting
# % required: no
# % multiple: yes
# % options: id,name,creator,mapset,temporal_type,creation_time,start_time,end_time,north,south,west,east,nsres,tbres,ewres,cols,rows,depths,number_of_cells,min,max
# %end

# %option
# % key: granule
# % type: string
# % description: The granule to be used for listing. The granule must be specified as string eg.: absolute time "1 months" or relative time "1"
# % required: no
# % multiple: no
# %end

# %option
# % key: columns
# % type: string
# % description: Columns to be printed to stdout
# % guisection: Selection
# % required: no
# % multiple: yes
# % options: id,name,creator,mapset,temporal_type,creation_time,start_time,end_time,north,south,west,east,nsres,tbres,ewres,cols,rows,depths,number_of_cells,min,max,interval_length,distance_from_begin
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

# %option G_OPT_F_SEP
# % label: Field separator character between the output columns
# % answer:
# % guisection: Formatting
# %end

# %option G_OPT_F_FORMAT
# % options: plain,line,json,yaml,csv
# % descriptions: plain;Plain text output;line;Comma separated list of map names;json;JSON (JavaScript Object Notation);yaml;YAML (YAML Ain't Markup Language);csv;CSV (Comma Separated Values);
# % guisection: Formatting
# %end

# %option G_OPT_F_OUTPUT
# % required: no
# %end

# %flag
# % key: s
# % description: Suppress printing of column names
# % guisection: Formatting
# %end


############################################################################

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


def message_option_value_excludes_option(
    option_name, option_value, excluded_option_name, reason
):
    return _(
        "The option {excluded_option_name} is not allowed with "
        "{option_name}={option_value}. {reason}"
    ).format(
        excluded_option_name=excluded_option_name,
        option_name=option_name,
        option_value=option_value,
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


def main():
    options, flags = gs.parser()

    # Get the options
    input = options["input"]
    columns = options["columns"]
    order = options["order"]
    where = options["where"]
    separator = gs.separator(options["separator"])
    method = options["method"]
    header = flags["s"]
    granule = options["granule"]
    output = options["output"]
    output_format = options["format"]

    if output_format == "csv":
        if not separator:
            separator = ","
        elif len(separator) > 1:
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
    elif output_format in {"json", "yaml"}:
        if header:
            gs.fatal(
                message_option_value_excludes_flag(
                    option_name="format",
                    option_value=output_format,
                    flag_name="s",
                    reason=_("Column names are always included"),
                )
            )
        if separator:
            gs.fatal(
                message_option_value_excludes_option_value(
                    option_name="format",
                    option_value=output_format,
                    excluded_option_name="separator",
                    excluded_option_value=separator,
                    reason=_("Separator option is not allowed with this format"),
                )
            )
    elif output_format == "line" or method == "comma":
        output_format = "line"
        if not separator:
            separator = ","
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
    elif not separator:  # output_format = "plain"
        separator = "|"

    # lazy imports
    import grass.temporal as tgis

    if method in {"delta", "deltagaps", "gran"}:
        if order:
            gs.fatal(
                message_option_value_excludes_option(
                    option_name="method",
                    option_value=method,
                    excluded_option_name="order",
                    reason=_("Values are always ordered by start_time"),
                )
            )
        if columns:
            columns_list = columns.split(",")
            for column in [
                "creator",
                "temporal_type",
                "creation_time",
                "north",
                "south",
                "west",
                "east",
                "nsres",
                "tbres",
                "ewres",
                "cols",
                "rows",
                "depths",
                "number_of_cells",
                "min",
                "max",
            ]:
                if column in columns_list:
                    gs.fatal(
                        message_option_value_excludes_option_value(
                            option_name="method",
                            option_value=method,
                            excluded_option_name="columns",
                            excluded_option_value=columns,
                            reason=_(
                                "Column '{name}' is not available with the method "
                                "'{method}'"
                            ).format(name=column, method=method),
                        )
                    )
    elif columns:
        columns_list = columns.split(",")
        for column in ["interval_length", "distance_from_begin"]:
            if column in columns_list:
                gs.fatal(
                    message_option_value_excludes_option_value(
                        option_name="method",
                        option_value=method,
                        excluded_option_name="columns",
                        excluded_option_value=columns,
                        reason=_(
                            "Column '{name}' is not available with the method "
                            "'{method}'"
                        ).format(name=column, method=method),
                    )
                )

    if method == "gran" and where:
        gs.fatal(
            message_option_value_excludes_option(
                option_name="method",
                option_value=method,
                excluded_option_name="where",
                reason=_("All maps are always listed"),
            )
        )

    # Make sure the temporal database exists
    tgis.init()

    tgis.list_maps_of_stds(
        "str3ds",
        input,
        columns,
        order,
        where,
        separator,
        method,
        header,
        gran=granule,
        outpath=output,
        output_format=output_format,
    )


if __name__ == "__main__":
    main()
