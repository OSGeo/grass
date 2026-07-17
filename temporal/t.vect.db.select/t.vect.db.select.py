#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.vect.db.select
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Prints attributes of vector maps registered in a space time vector dataset.
# SPDX-FileCopyrightText: 2011-2017 Soeren Gebbert
# SPDX-FileCopyrightText: Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#
#############################################################################

# %module
# % description: Prints attributes of vector maps registered in a space time vector dataset.
# % keyword: temporal
# % keyword: attribute table
# % keyword: vector
# % keyword: database
# % keyword: select
# % keyword: time
# %end

# %option G_OPT_STVDS_INPUT
# %end

# %option G_OPT_DB_COLUMNS
# %end

# %option G_OPT_F_FORMAT
# % options: plain,csv,json
# % descriptions: plain;Plain text output;csv;CSV (Comma Separated Values);json;JSON (JavaScript Object Notation)
# % answer: plain
# % guisection: Formatting
# %end

# %option G_OPT_F_SEP
# % label: Field separator character between the output columns
# % answer:
# %end

# %option G_OPT_V_FIELD
# %end

# %option G_OPT_DB_WHERE
# %end

# %option G_OPT_T_WHERE
# % key: t_where
# %end

import json

import grass.script as gs
from grass.tools import Tools

############################################################################


def main():

    # Get the options
    input = options["input"]
    where = options["where"]
    columns = options["columns"]
    tempwhere = options["t_where"]
    layer = options["layer"]
    separator = gs.separator(options["separator"])
    output_format = options.get("format", "plain")

    if where in {"", " ", "\n"}:
        where = None

    if columns in {"", " ", "\n"}:
        columns = None

    if output_format == "csv":
        if not separator:
            separator = ","
        elif len(separator) > 1:
            gs.fatal(
                _("A standard CSV separator (delimiter) is only one character long")
            )

    elif output_format == "json":
        if separator:
            gs.fatal(_("Separator option is not allowed with JSON format"))

    elif not separator:  # output_format == "plain"
        separator = "|"

    # lazy imports
    import grass.temporal as tgis

    # Make sure the temporal database exists
    tgis.init()

    sp = tgis.open_old_stds(input, "stvds")

    rows = sp.get_registered_maps(
        "name,layer,mapset,start_time,end_time", tempwhere, "start_time", None
    )

    tools = Tools()
    json_output = []
    col_names = ""

    if rows:
        for row in rows:
            vector_name = "%s@%s" % (row["name"], row["mapset"])
            # In case a layer is defined in the vector dataset,
            # we override the option layer
            if row["layer"]:
                layer = row["layer"]

            res = tools.v_db_select(
                map=vector_name,
                layer=layer,
                columns=columns,
                separator=separator,
                where=where,
                format=output_format,
            )

            if output_format == "json":
                records = res["records"]
                for record in records:
                    new_record = {
                        "start_time": row["start_time"],
                        "end_time": row["end_time"],
                    }
                    new_record.update(record)
                    json_output.append(new_record)

            else:
                select = res.text
                # The first line are the column names
                list = select.split("\n")
                count = 0
                for entry in list:
                    if entry.strip() != "":
                        # print the column names in case they change
                        if count == 0:
                            col_names_new = "start_time%send_time%s%s" % (
                                separator,
                                separator,
                                entry,
                            )
                            if col_names != col_names_new:
                                col_names = col_names_new
                                print(col_names)
                        elif row["end_time"]:
                            print(
                                "%s%s%s%s%s"
                                % (
                                    row["start_time"],
                                    separator,
                                    row["end_time"],
                                    separator,
                                    entry,
                                )
                            )
                        else:
                            print(
                                "%s%s%s%s"
                                % (row["start_time"], separator, separator, entry)
                            )
                        count += 1

    if output_format == "json":
        print(json.dumps(json_output, indent=4, default=str))


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
