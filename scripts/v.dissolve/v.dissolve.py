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
# %option
# % key: aggregate_backend
# % label: Aggregate statistics method
# % description: Default is all available basic statistics
# % multiple: no
# % required: no
# % options: univar,sql
# %end
# %rules
# % requires_all: aggregate_method,aggregate_column
# % requires_all: stats_column,aggregate_method,aggregate_column
# %end

"""Dissolve geometries and aggregate attribute values"""

import atexit
import json
import subprocess

import grass.script as grass

# To use new style of import without changing old code.
import grass.script as gs  # pylint: disable=reimported
from grass.exceptions import CalledModuleError


# Methods supported by v.db.univar by default.
UNIVAR_METHODS = [
    "n",
    "min",
    "max",
    "range",
    "mean",
    "mean_abs",
    "variance",
    "stddev",
    "coeff_var",
    "sum",
]

# Basic SQL aggregate function common between SQLite and PostgreSQL
# (and the SQL standard) using their proper names and order from
# their documentation.
# Notably, this does not include SQLite total which returns zero
# when all values are NULL.
STANDARD_SQL_FUNCTIONS = ["avg", "count", "max", "min", "sum"]


def get_methods_and_backend(methods, backend):
    """Get methods and backed based on user-provided methods and backend"""
    if methods:
        if not backend:
            in_univar = False
            neither_in_sql_nor_univar = False
            for method in methods:
                if method not in STANDARD_SQL_FUNCTIONS:
                    if method in UNIVAR_METHODS:
                        in_univar = True
                    else:
                        neither_in_sql_nor_univar = True
            # If all the non-basic functions are available in univar, use it.
            if in_univar and not neither_in_sql_nor_univar:
                backend = "univar"
    elif backend == "sql":
        methods = STANDARD_SQL_FUNCTIONS
    elif backend == "univar":
        methods = UNIVAR_METHODS
    else:
        # This is the default SQL functions but using the univar names (and order).
        methods = ["n", "min", "max", "mean", "sum"]
        backend = "sql"
    if not backend:
        backend = "sql"
    return methods, backend


def modify_methods_for_backend(methods, backend):
    """Modify list of methods to fit the backend if they do not

    This allows for support of the same method names for both backends.
    It works both ways.
    """
    new_methods = []
    if backend == "sql":
        for method in methods:
            if method == "n":
                new_methods.append("count")
            elif method == "mean":
                new_methods.append("avg")
            else:
                new_methods.append(method)
    elif backend == "univar":
        for method in methods:
            if method == "count":
                new_methods.append("n")
            elif method == "avg":
                new_methods.append("mean")
            else:
                new_methods.append(method)
    return new_methods


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


def update_columns(output, output_layer, updates, add_columns):
    """Update attribute values based on a list of updates"""
    if add_columns:
        gs.run_command(
            "v.db.addcolumn",
            map=output,
            columns=",".join(add_columns),
        )
    db_info = gs.vector_db(output)[output_layer]
    sql = updates_to_sql(table=db_info["table"], updates=updates)
    gs.write_command(
        "db.execute",
        input="-",
        database=db_info["database"],
        driver=db_info["driver"],
        stdin=sql,
    )


def column_value_to_where(column, value, *, quote):
    """Create SQL where clause without the where keyword for column and its value"""
    if value is None:
        return f"{column} IS NULL"
    if quote:
        return f"{column}='{value}'"
    return f"{column}={value}"


def check_aggregate_methods_or_fatal(methods, backend):
    if backend == "univar":
        for method in methods:
            if method not in UNIVAR_METHODS:
                gs.fatal(
                    _(
                        "Method <{method}> is not available for backend <{backend}>"
                    ).format(method=method, backend=backend)
                )
    # We don't have a list of available SQL functions. It is long for PostgreSQL
    # and open for SQLite depending on its extensions.


# TODO: Confirm that there is only one record in the table
# for a given attribute value after dissolve.


def aggregate_attributes_sql(
    input_name,
    column,
    quote_column,
    columns_to_aggregate,
    methods,
    result_columns,
):
    """Aggregate values in selected columns grouped by column using SQL backend"""
    select_columns = [
        f"{method}({agg_column})"
        for method in methods
        for agg_column in columns_to_aggregate
    ]
    column_types = [
        "INTEGER" if method == "count" else "DOUBLE" for method in methods
    ] * len(columns_to_aggregate)
    records = json.loads(
        gs.read_command(
            "v.db.select",
            map=input_name,
            columns=",".join([column] + select_columns),
            group=column,
            format="json",
        )
    )["records"]
    updates = []
    add_columns = []
    for result_column, column_type in zip(result_columns, column_types):
        add_columns.append(f"{result_column} {column_type}")
    for row in records:
        where = column_value_to_where(column, row[column], quote=quote_column)
        for (
            result_column,
            key,
        ) in zip(result_columns, select_columns):
            updates.append(
                {
                    "column": result_column,
                    "value": row[key],
                    "where": where,
                }
            )
    return updates, add_columns


def aggregate_attributes_univar(
    input_name,
    column,
    quote_column,
    columns_to_aggregate,
    methods,
    result_columns,
):
    """Aggregate values in selected columns grouped by column using v.db.univar"""
    records = json.loads(
        gs.read_command(
            "v.db.select",
            map=input_name,
            columns=column,
            group=column,
            format="json",
        )
    )["records"]
    column_types = [
        "INTEGER" if method == "n" else "DOUBLE" for method in methods
    ] * len(columns_to_aggregate)
    add_columns = []
    for result_column, column_type in zip(result_columns, column_types):
        add_columns.append(f"{result_column} {column_type}")
    unique_values = [record[column] for record in records]
    updates = []
    for value in unique_values:
        where = column_value_to_where(column, value, quote=quote_column)
        for i, aggregate_column in enumerate(columns_to_aggregate):
            stats = json.loads(
                gs.read_command(
                    "v.db.univar",
                    map=input_name,
                    column=aggregate_column,
                    format="json",
                    where=where,
                )
            )["statistics"]
            current_result_columns = result_columns[
                i * len(methods) : (i + 1) * len(methods)
            ]
            for result_column, key in zip(current_result_columns, methods):
                updates.append(
                    {
                        "column": result_column,
                        "value": stats[key],
                        "where": where,
                    }
                )
    return updates, add_columns


def cleanup(name):
    """Remove temporary vector silently"""
    grass.run_command(
        "g.remove",
        flags="f",
        type="vector",
        name=name,
        quiet=True,
        stderr=subprocess.DEVNULL,
    )


def option_as_list(options, name):
    """Get value of an option as a list"""
    option = options[name]
    if option:
        return option.split(",")
    return []


def main():
    """Run the dissolve operation based on command line parameters"""
    options, unused_flags = grass.parser()
    input = options["input"]
    output = options["output"]
    layer = options["layer"]
    column = options["column"]
    aggregate_backend = options["aggregate_backend"]

    columns_to_aggregate = option_as_list(options, "aggregate_column")
    user_aggregate_methods = option_as_list(options, "aggregate_method")
    user_aggregate_methods, aggregate_backend = get_methods_and_backend(
        user_aggregate_methods, aggregate_backend
    )
    aggregate_methods = modify_methods_for_backend(
        user_aggregate_methods, backend=aggregate_backend
    )
    check_aggregate_methods_or_fatal(aggregate_methods, backend=aggregate_backend)
    result_columns = option_as_list(options, "stats_column")
    if result_columns:
        if len(result_columns) != len(columns_to_aggregate) * len(
            user_aggregate_methods
        ):
            gs.fatal(
                _(
                    "A column name is needed for each combination of aggregate_column "
                    "({num_columns}) and aggregate_method ({num_methods})"
                ).format(
                    num_columns=len(columns_to_aggregate),
                    num_methods=len(user_aggregate_methods),
                )
            )
    else:
        result_columns = [
            f"{aggregate_column}_{method}"
            for aggregate_column in columns_to_aggregate
            for method in user_aggregate_methods
        ]

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
        if columns_to_aggregate and not column_is_str:
            grass.fatal(
                _(
                    "Key column type must be string (text) "
                    "for aggregation method to work, not '{column_type}'"
                ).format(column_type=coltype["type"])
            )

        tmpfile = gs.append_node_pid(output)
        atexit.register(cleanup, tmpfile)

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
            if columns_to_aggregate:
                if aggregate_backend == "sql":
                    updates, add_columns = aggregate_attributes_sql(
                        input_name=input,
                        column=column,
                        quote_column=column_is_str,
                        columns_to_aggregate=columns_to_aggregate,
                        methods=aggregate_methods,
                        result_columns=result_columns,
                    )
                else:
                    updates, add_columns = aggregate_attributes_univar(
                        input_name=input,
                        column=column,
                        quote_column=column_is_str,
                        columns_to_aggregate=columns_to_aggregate,
                        methods=aggregate_methods,
                        result_columns=result_columns,
                    )
                update_columns(
                    output=output,
                    output_layer=1,
                    updates=updates,
                    add_columns=add_columns,
                )
        except CalledModuleError as error:
            grass.fatal(
                _(
                    "Final extraction steps failed."
                    " Check above error messages and"
                    " see following details:\n{error}"
                ).format(error=error)
            )

    # write cmd history:
    grass.vector_history(output)


if __name__ == "__main__":
    main()
