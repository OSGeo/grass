#!/usr/bin/env python3
############################################################################
#
# MODULE:       v.dissolve
# AUTHOR:       M. Hamish Bowman, Dept. Marine Science, Otago University,
#                 New Zealand
#               Markus Neteler for column support
#               Converted to Python by Glynn Clements
#               Vaclav Petras <wenzeslaus gmail com> (aggregate statistics)
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
# % label: Name of attribute columns to get aggregate statistics for
# % description: One column per method if result columns are specified
# % multiple: yes
# %end
# %option
# % key: aggregate_method
# % label: Aggregate statistics method (e.g., sum)
# % description: Default is all available basic statistics for a given backend
# % multiple: yes
# %end
# %option G_OPT_DB_COLUMN
# % key: result_column
# % label: New attribute column name for aggregate statistics results
# % description: Defaults to aggregate column name and statistics name
# % multiple: yes
# %end
# %option
# % key: aggregate_backend
# % label: Backend for attribute aggregation
# % description: Default is sql unless the methods are for univar
# % multiple: no
# % required: no
# % options: sql,univar
# %end
# %rules
# % requires_all: aggregate_method,aggregate_column
# % requires_all: result_column,aggregate_method,aggregate_column
# %end

"""Dissolve geometries and aggregate attribute values"""

import atexit
import json
import subprocess
from collections import defaultdict

import grass.script as gs
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
            in_univar = 0
            neither_in_sql_nor_univar = 0
            for method in methods:
                if method not in STANDARD_SQL_FUNCTIONS:
                    if method in UNIVAR_METHODS:
                        in_univar += 1
                    else:
                        neither_in_sql_nor_univar += 1
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


def update_columns(output_name, output_layer, updates, add_columns):
    """Update attribute values based on a list of updates"""
    if add_columns:
        gs.run_command(
            "v.db.addcolumn",
            map=output_name,
            layer=output_layer,
            columns=",".join(add_columns),
        )
    db_info = gs.vector_db(output_name)[int(output_layer)]
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
    """Check for known methods if possible or fail"""
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


def match_columns_and_methods(columns, methods):
    """Return all combinations of columns and methods"""
    new_columns = []
    new_methods = []
    for column in columns:
        for method in methods:
            new_columns.append(column)
            new_methods.append(method)
    return new_columns, new_methods


def create_or_check_result_columns_or_fatal(
    result_columns, columns_to_aggregate, methods
):
    """Create result columns from input if not provided or check them"""
    if result_columns:
        if len(columns_to_aggregate) != len(methods):
            gs.fatal(
                _(
                    "When result columns are specified, the number of "
                    "aggregate columns ({columns_to_aggregate}) needs to be "
                    "the same as the number of methods ({methods})"
                ).format(
                    columns_to_aggregate=len(columns_to_aggregate),
                    methods=len(methods),
                )
            )
        if len(result_columns) != len(columns_to_aggregate):
            gs.fatal(
                _(
                    "The number of result columns ({result_columns}) needs to be "
                    "the same as the number of aggregate columns "
                    "({columns_to_aggregate})"
                ).format(
                    result_columns=len(result_columns),
                    columns_to_aggregate=len(columns_to_aggregate),
                )
            )
        if len(result_columns) != len(methods):
            gs.fatal(
                _(
                    "The number of result columns ({result_columns}) needs to be "
                    "the same as the number of aggregation methods ({methods})"
                ).format(
                    result_columns=len(result_columns),
                    methods=len(methods),
                )
            )
        return result_columns
    return [
        f"{aggregate_column}_{method}"
        for aggregate_column, method in zip(columns_to_aggregate, methods)
    ]


def aggregate_attributes_sql(
    input_name,
    input_layer,
    column,
    quote_column,
    columns_to_aggregate,
    methods,
    result_columns,
):
    """Aggregate values in selected columns grouped by column using SQL backend"""
    if len(columns_to_aggregate) != len(methods) != len(result_columns):
        raise ValueError(
            "Number of columns_to_aggregate, methods, and result_columns "
            "must be the same"
        )
    select_columns = [
        f"{method}({agg_column})"
        for method, agg_column in zip(methods, columns_to_aggregate)
    ]
    column_types = [
        "INTEGER" if method == "count" else "DOUBLE" for method in methods
    ] * len(columns_to_aggregate)
    records = json.loads(
        gs.read_command(
            "v.db.select",
            map=input_name,
            layer=input_layer,
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
    input_layer,
    column,
    quote_column,
    columns_to_aggregate,
    methods,
    result_columns,
):
    """Aggregate values in selected columns grouped by column using v.db.univar"""
    if len(columns_to_aggregate) != len(methods) != len(result_columns):
        raise ValueError(
            "Number of columns_to_aggregate, methods, and result_columns "
            "must be the same"
        )
    records = json.loads(
        gs.read_command(
            "v.db.select",
            map=input_name,
            layer=input_layer,
            columns=column,
            group=column,
            format="json",
        )
    )["records"]
    columns = defaultdict(list)
    for agg_column, method, result in zip(
        columns_to_aggregate, methods, result_columns
    ):
        columns[agg_column].append((method, result))
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
        # for i, aggregate_column in enumerate(columns_to_aggregate):
        for aggregate_column, methods_results in columns.items():
            stats = json.loads(
                gs.read_command(
                    "v.db.univar",
                    map=input_name,
                    column=aggregate_column,
                    format="json",
                    where=where,
                )
            )["statistics"]
            for method, result_column in methods_results:
                updates.append(
                    {
                        "column": result_column,
                        "value": stats[method],
                        "where": where,
                    }
                )
    return updates, add_columns


def cleanup(name):
    """Remove temporary vector silently"""
    gs.run_command(
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
    options, unused_flags = gs.parser()
    input_vector = options["input"]
    output = options["output"]
    layer = options["layer"]
    column = options["column"]
    aggregate_backend = options["aggregate_backend"]

    columns_to_aggregate = option_as_list(options, "aggregate_column")
    user_aggregate_methods = option_as_list(options, "aggregate_method")
    user_aggregate_methods, aggregate_backend = get_methods_and_backend(
        user_aggregate_methods, aggregate_backend
    )
    result_columns = option_as_list(options, "result_column")
    if not result_columns:
        columns_to_aggregate, user_aggregate_methods = match_columns_and_methods(
            columns_to_aggregate, user_aggregate_methods
        )
    aggregate_methods = modify_methods_for_backend(
        user_aggregate_methods, backend=aggregate_backend
    )
    check_aggregate_methods_or_fatal(aggregate_methods, backend=aggregate_backend)
    result_columns = create_or_check_result_columns_or_fatal(
        result_columns=result_columns,
        columns_to_aggregate=columns_to_aggregate,
        methods=user_aggregate_methods,
    )

    # does map exist?
    if not gs.find_file(input_vector, element="vector")["file"]:
        gs.fatal(_("Vector map <%s> not found") % input_vector)

    if not column:
        gs.warning(
            _(
                "No '%s' option specified. Dissolving based on category values from layer <%s>."
            )
            % ("column", layer)
        )
        gs.run_command(
            "v.extract",
            flags="d",
            input=input_vector,
            output=output,
            type="area",
            layer=layer,
        )
    else:
        if int(layer) == -1:
            gs.warning(
                _(
                    "Invalid layer number (%d). "
                    "Parameter '%s' specified, assuming layer '1'."
                )
                % (int(layer), "column")
            )
            layer = "1"
        try:
            coltype = gs.vector_columns(input_vector, layer)[column]
        except KeyError:
            gs.fatal(_("Column <%s> not found") % column)

        if coltype["type"] not in ("INTEGER", "SMALLINT", "CHARACTER", "TEXT"):
            gs.fatal(_("Key column must be of type integer or string"))
        column_is_str = bool(coltype["type"] in ("CHARACTER", "TEXT"))
        if columns_to_aggregate and not column_is_str:
            gs.fatal(
                _(
                    "Key column type must be string (text) "
                    "for aggregation method to work, not '{column_type}'"
                ).format(column_type=coltype["type"])
            )

        tmpfile = gs.append_node_pid(output)
        atexit.register(cleanup, tmpfile)

        try:
            gs.run_command(
                "v.reclass",
                input=input_vector,
                output=tmpfile,
                layer=layer,
                column=column,
            )
            gs.run_command(
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
                        input_name=input_vector,
                        input_layer=layer,
                        column=column,
                        quote_column=column_is_str,
                        columns_to_aggregate=columns_to_aggregate,
                        methods=aggregate_methods,
                        result_columns=result_columns,
                    )
                else:
                    updates, add_columns = aggregate_attributes_univar(
                        input_name=input_vector,
                        input_layer=layer,
                        column=column,
                        quote_column=column_is_str,
                        columns_to_aggregate=columns_to_aggregate,
                        methods=aggregate_methods,
                        result_columns=result_columns,
                    )
                update_columns(
                    output_name=output,
                    output_layer=layer,
                    updates=updates,
                    add_columns=add_columns,
                )
        except CalledModuleError as error:
            gs.fatal(
                _(
                    "A processing step failed."
                    " Check the above error messages and"
                    " see the following details:\n{error}"
                ).format(error=error)
            )

    # write cmd history:
    gs.vector_history(output)


if __name__ == "__main__":
    main()
