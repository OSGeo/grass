#!/usr/bin/env python3
############################################################################
#
# MODULE:       v.dissolve
# AUTHOR:       M. Hamish Bowman, Dept. Marine Science, Otago University,
#                 New Zealand
#               Markus Neteler for column support
#               Converted to Python by Glynn Clements
# PURPOSE:      Dissolve common boundaries between areas with common cat
#                 (frontend to v.extract -d)
# COPYRIGHT:    (c) 2006-2022 Hamish Bowman, and the GRASS Development Team
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

# %module
# % description: Dissolves boundaries between adjacent areas sharing a common category number or attribute.
# % keyword: vector
# % keyword: dissolve
# % keyword: area
# % keyword: line
# %end
# %option G_OPT_V_INPUT
# %end
# %option G_OPT_V_FIELD
# % label: Layer number or name.
# % required: no
# %end
# %option G_OPT_DB_COLUMN
# % description: Name of attribute column used to dissolve common boundaries
# %end
# %option G_OPT_V_OUTPUT
# %end
# %option G_OPT_DB_COLUMN
# % key: aggregate_column
# % description: Name of attribute columns to get aggregate statistics for
# % multiple: yes
# %end
# %option
# % key: aggregate_method
# % label: Aggregate statistics method
# % description: Default is all available basic statistics
# % multiple: yes
# %end
# %option G_OPT_DB_COLUMN
# % key: stats_column
# % description: New attribute column name for aggregate statistics results
# % description: Defaults to aggregate column name and statistics name
# % multiple: yes
# %end
# %rules
# % requires_all: aggregate_method,aggregate_column
# % requires_all: stats_column,aggregate_method,aggregate_column
# %end

"""Dissolve geometries and aggregate attribute values"""

import os
import atexit
import json

import grass.script as grass

# To use new style of import without changing old code.
import grass.script as gs  # pylint: disable=reimported
from grass.exceptions import CalledModuleError


def updates_to_sql(table, updates):
    """Create SQL from a list of dicts with column, value, where"""
    sql = ["BEGIN TRANSACTION"]
    for update in updates:
        sql.append(
            f"UPDATE {table} SET {update['column']} = {update['value']} "
            f"WHERE {update['where']};"
        )
    sql.append("END TRANSACTION")
    return "\n".join(sql)


def cleanup():
    nuldev = open(os.devnull, "w")
    grass.run_command(
        "g.remove",
        flags="f",
        type="vector",
        name="%s_%s" % (output, tmp),
        quiet=True,
        stderr=nuldev,
    )


def main():
    global output, tmp

    input = options["input"]
    output = options["output"]
    layer = options["layer"]
    column = options["column"]

    aggregate_columns = options["aggregate_column"]
    if aggregate_columns:
        aggregate_columns = aggregate_columns.split(",")
    else:
        aggregate_columns = None
    aggregate_methods = options["aggregate_method"]
    if aggregate_methods:
        aggregate_methods = aggregate_methods.split(",")
    stats_columns = options["stats_column"]
    if stats_columns:
        stats_columns = stats_columns.split(",")
        if len(stats_columns) != len(aggregate_columns) * len(aggregate_methods):
            gs.fatal(
                _(
                    "A column name is needed for each combination of aggregate_column "
                    "({num_columns}) and aggregate_method ({num_methods})"
                ).format(
                    num_columns=len(aggregate_columns),
                    num_methods=len(aggregate_methods),
                )
            )

    # setup temporary file
    tmp = str(os.getpid())

    # does map exist?
    if not grass.find_file(input, element="vector")["file"]:
        grass.fatal(_("Vector map <%s> not found") % input)

    if not column:
        grass.warning(
            _(
                "No '%s' option specified. Dissolving based on category values from layer <%s>."
            )
            % ("column", layer)
        )
        grass.run_command(
            "v.extract", flags="d", input=input, output=output, type="area", layer=layer
        )
    else:
        if int(layer) == -1:
            grass.warning(
                _(
                    "Invalid layer number (%d). "
                    "Parameter '%s' specified, assuming layer '1'."
                )
                % (int(layer), "column")
            )
            layer = "1"
        try:
            coltype = grass.vector_columns(input, layer)[column]
        except KeyError:
            grass.fatal(_("Column <%s> not found") % column)

        if coltype["type"] not in ("INTEGER", "SMALLINT", "CHARACTER", "TEXT"):
            grass.fatal(_("Key column must be of type integer or string"))
        column_is_str = bool(coltype["type"] in ("CHARACTER", "TEXT"))
        if aggregate_columns and not column_is_str:
            grass.fatal(
                _(
                    "Key column type must be string (text) "
                    "for aggregation method to work, not '{column_type}'"
                ).format(column_type=coltype["type"])
            )
        column_quote = column_is_str

        tmpfile = "%s_%s" % (output, tmp)

        try:
            grass.run_command(
                "v.reclass", input=input, output=tmpfile, layer=layer, column=column
            )
            grass.run_command(
                "v.extract",
                flags="d",
                input=tmpfile,
                output=output,
                type="area",
                layer=layer,
            )
            if aggregate_columns:
                records = json.loads(
                    gs.read_command(
                        "v.db.select",
                        map=input,
                        columns=column,
                        group=column,
                        format="json",
                    )
                )["records"]
                unique_values = [record[column] for record in records]
                created_columns = set()
                updates = []
                add_columns = []
                for value in unique_values:
                    for i, aggregate_column in enumerate(aggregate_columns):
                        if value is None:
                            where = f"{column} IS NULL"
                        elif column_quote:
                            where = f"{column}='{value}'"
                        else:
                            where = f"{column}={value}"
                        stats = json.loads(
                            gs.read_command(
                                "v.db.univar",
                                map=input,
                                column=aggregate_column,
                                format="json",
                                where=where,
                            )
                        )["statistics"]
                        if not aggregate_methods:
                            aggregate_methods = stats.keys()
                        if stats_columns:
                            current_stats_columns = stats_columns[
                                i
                                * len(aggregate_methods) : (i + 1)
                                * len(aggregate_methods)
                            ]
                        else:
                            current_stats_columns = [
                                f"{aggregate_column}_{method}"
                                for method in aggregate_methods
                            ]
                        for stats_column, key in zip(
                            current_stats_columns, aggregate_methods
                        ):
                            stats_value = stats[key]
                            # if stats_columns:
                            # stats_column = stats_columns[i * len(aggregate_methods) + j]
                            if stats_column not in created_columns:
                                if key == "n":
                                    stats_column_type = "INTEGER"
                                else:
                                    stats_column_type = "DOUBLE"
                                add_columns.append(
                                    f"{stats_column} {stats_column_type}"
                                )
                                created_columns.add(stats_column)
                            # TODO: Confirm that there is only one record in the table
                            # for a given attribute value after dissolve.
                            updates.append(
                                {
                                    "column": stats_column,
                                    "value": stats_value,
                                    "where": where,
                                }
                            )
                gs.run_command(
                    "v.db.addcolumn",
                    map=output,
                    columns=",".join(add_columns),
                )
                output_layer = 1
                db_info = gs.vector_db(output)[output_layer]
                sql = updates_to_sql(table=db_info["table"], updates=updates)
                gs.write_command(
                    "db.execute",
                    input="-",
                    database=db_info["database"],
                    driver=db_info["driver"],
                    stdin=sql,
                )

        except CalledModuleError as e:
            grass.fatal(
                _(
                    "Final extraction steps failed."
                    " Check above error messages and"
                    " see following details:\n%s"
                )
                % e
            )

    # write cmd history:
    grass.vector_history(output)


if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
