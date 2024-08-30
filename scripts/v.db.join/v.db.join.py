#!/usr/bin/env python3

############################################################################
#
# MODULE:       v.db.join
# AUTHOR(S):    Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      Join a table to a map table
# COPYRIGHT:    (C) 2007-2023 by Markus Neteler and the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

# %module
# % description: Joins a database table to a vector map table.
# % keyword: vector
# % keyword: attribute table
# % keyword: database
# %end

# %option G_OPT_V_MAP
# % description: Vector map to which to join other table
# % guidependency: layer,column
# %end

# %option G_OPT_V_FIELD
# % description: Layer where to join
# % guidependency: column
# %end

# %option G_OPT_DB_COLUMN
# % description: Identifier column (e.g.: cat) in the vector table to be used for join
# % required : yes
# %end

# %option G_OPT_DB_TABLE
# % key: other_table
# % description: Other table name
# % required: yes
# % guidependency: ocolumn,scolumns
# %end

# %option G_OPT_DB_COLUMN
# % key: other_column
# % description: Identifier column (e.g.: id) in the other table used for join
# % required: yes
# %end

# %option G_OPT_DB_COLUMN
# % key: subset_columns
# % multiple: yes
# % required: no
# % description: Subset of columns from the other table
# %end

# %option G_OPT_DB_COLUMN
# % key: exclude_columns
# % multiple: yes
# % required: no
# % description: Columns to exclude from the other table
# %end

import atexit
import sys

from pathlib import Path

import grass.script as gs
from grass.exceptions import CalledModuleError

rm_files = []


def cleanup():
    for file_path in rm_files:
        try:
            file_path.unlink(missing_ok=True)
        except Exception as e:
            gs.warning(
                _("Unable to remove file {file}: {message}").format(
                    file=file_path, message=e
                )
            )


def main():
    global rm_files
    # Include mapset into the name, so we avoid multiple messages about
    # found in more mapsets. The following generates an error message, while the code
    # above does not. However, the above checks that the map exists, so we don't
    # check it here.
    vector_map = gs.find_file(options["map"], element="vector")["fullname"]
    layer = options["layer"]
    column = options["column"]
    otable = options["other_table"]
    ocolumn = options["other_column"]
    scolumns = None
    if options["subset_columns"]:
        scolumns = options["subset_columns"].split(",")
    ecolumns = None
    if options["exclude_columns"]:
        ecolumns = options["exclude_columns"].split(",")

    try:
        f = gs.vector_layer_db(vector_map, layer)
    except CalledModuleError:
        sys.exit(1)

    maptable = f["table"]
    database = f["database"]
    driver = f["driver"]

    if driver == "dbf":
        gs.fatal(_("JOIN is not supported for tables stored in DBF format"))

    if not maptable:
        gs.fatal(
            _("There is no table connected to this map. Unable to join any column.")
        )

    all_cols_tt = gs.vector_columns(vector_map, int(layer)).keys()
    # This is used for testing presence (and potential name conflict) with
    # the newly added columns, but the test needs to case-insensitive since it
    # is SQL, so we lowercase the names here and in the test
    # An alternative is quoting identifiers (as in e.g. #3634)
    all_cols_tt = [name.lower() for name in all_cols_tt]

    # check if column is in map table
    if column.lower() not in all_cols_tt:
        gs.fatal(
            _("Column <{column}> not found in table <{table}>").format(
                column=column, table=maptable
            )
        )

    # describe other table
    all_cols_ot = {
        col_desc[0].lower(): col_desc[1:]
        for col_desc in gs.db_describe(otable, driver=driver, database=database)["cols"]
    }

    # check if ocolumn is on other table
    if ocolumn.lower() not in all_cols_ot:
        gs.fatal(
            _("Column <{column}> not found in table <{table}>").format(
                column=ocolumn, table=otable
            )
        )

    # determine columns subset from other table
    if not scolumns:
        # select all columns from other table
        cols_to_update = all_cols_ot
    else:
        cols_to_update = {}
        # check if scolumns exists in the other table
        for scol in scolumns:
            if scol not in all_cols_ot:
                gs.warning(
                    _("Column <{column}> not found in table <{table}>").format(
                        column=scol, table=otable
                    )
                )
            else:
                cols_to_update[scol] = all_cols_ot[scol]

    # skip the vector column which is used for join
    if column in cols_to_update:
        cols_to_update.pop(column)

    # exclude columns from other table
    if ecolumns:
        for ecol in ecolumns:
            if ecol not in all_cols_ot:
                gs.warning(
                    _("Column <{column}> not found in table <{table}>").format(
                        column=ecol, table=otable
                    )
                )
            else:
                cols_to_update.pop(ecol)

    cols_to_add = []
    for col_name, col_desc in cols_to_update.items():
        use_len = False
        col_type = f"{col_desc[0]}"
        # Sqlite 3 does not support the precision number any more
        if len(col_desc) > 2 and driver != "sqlite":
            use_len = True
            # MySQL - expect format DOUBLE PRECISION(M,D), see #2792
            if driver == "mysql" and col_desc[1] == "DOUBLE PRECISION":
                use_len = False

        if use_len:
            col_type = f"{col_desc[0]}({col_desc[1]})"

        col_spec = f"{col_name.lower()} {col_type}"

        # add only the new column to the table
        if col_name.lower() not in all_cols_tt:
            cols_to_add.append(col_spec)

    if cols_to_add:
        try:
            gs.run_command(
                "v.db.addcolumn",
                map=vector_map,
                columns=",".join(cols_to_add),
                layer=layer,
            )
        except CalledModuleError:
            gs.fatal(
                _("Error creating columns <{}>").format(
                    ", ".join([col.split(" ")[0] for col in cols_to_add])
                )
            )

    update_str = "BEGIN TRANSACTION\n"
    for col in cols_to_update:
        cur_up_str = (
            f"UPDATE {maptable} SET {col} = (SELECT {col} FROM "
            f"{otable} WHERE "
            f"{otable}.{ocolumn}={maptable}.{column});\n"
        )
        update_str += cur_up_str
    update_str += "END TRANSACTION"
    gs.debug(update_str, 1)
    gs.verbose(
        _("Updating columns {columns} of vector map {map_name}...").format(
            columns=", ".join(cols_to_update.keys()), map_name=vector_map
        )
    )
    sql_file = Path(gs.tempfile())
    rm_files.append(sql_file)
    sql_file.write_text(update_str, encoding="UTF8")

    try:
        gs.run_command(
            "db.execute",
            input=str(sql_file),
            database=database,
            driver=driver,
        )
    except CalledModuleError:
        gs.fatal(_("Error filling columns {}").format(cols_to_update))

    # write cmd history
    gs.vector_history(vector_map)

    return 0


if __name__ == "__main__":
    options, flags = gs.parser()
    atexit.register(cleanup)
    sys.exit(main())
