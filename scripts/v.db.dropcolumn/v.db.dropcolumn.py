#!/usr/bin/env python3

############################################################################
#
# MODULE:       v.db.dropcolumn
# AUTHOR(S):    Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      interface to db.execute to drop a column from the
#               attribute table connected to a given vector map
#               - Based on v.db.addcolumn
#               - with special trick for SQLite
# COPYRIGHT:    (C) 2007 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################


# %module
# % description: Drops a column from the attribute table connected to a given vector map.
# % keyword: vector
# % keyword: attribute table
# % keyword: database
# %end

# %option G_OPT_V_MAP
# % key: map
# %end

# %option G_OPT_V_FIELD
# %end

# %option G_OPT_DB_COLUMNS
# % description: Name of attribute column(s) to drop
# % required: yes
# %end

import grass.script as gs
from grass.exceptions import CalledModuleError
from grass.script.db import DBHandler


def main():
    map = options["map"]
    layer = options["layer"]
    columns = options["columns"].split(",")

    mapset = gs.gisenv()["MAPSET"]

    # does map exist in CURRENT mapset?
    if not gs.find_file(map, element="vector", mapset=mapset)["file"]:
        gs.fatal(_("Vector map <%s> not found in current mapset") % map)

    f = gs.vector_layer_db(map, layer)

    table = f["table"]
    keycol = f["key"]
    database = f["database"]
    driver = f["driver"]

    db_handler = DBHandler(driver_name=driver, database=database)

    if not table:
        gs.fatal(
            _(
                "There is no table connected to the input vector map. "
                "Unable to delete any column. Exiting."
            )
        )

    if keycol in columns:
        gs.fatal(
            _(
                "Unable to delete <%s> column as it is needed to keep table <%s> "
                "connected to the input vector map <%s>"
            )
            % (keycol, table, map)
        )

    for column in columns:
        if column not in gs.vector_columns(map, layer):
            gs.warning(
                _("Column <%s> not found in table <%s>. Skipped") % (column, table)
            )
            continue

        if driver == "sqlite":
            # echo "Using special trick for SQLite"
            # https://www.sqlite.org/faq.html#q11
            colnames = []
            coltypes = []
            for f in gs.db_describe(table, database=database, driver=driver)["cols"]:
                if f[0] == column:
                    continue
                colnames.append(f[0])
                # see db_sqltype_name() for type names
                if f[1] == "CHARACTER":
                    # preserve field length for sql type "CHARACTER"
                    coltypes.append(f'"{f[0]}" {f[1]}({f[2]})')
                else:
                    coltypes.append(f'"{f[0]}" {f[1]}')

            colnames = ", ".join([f'"{col}"' for col in colnames])
            coltypes = ", ".join(coltypes)

            sql = [
                f"CREATE TEMPORARY TABLE {table}_backup ({coltypes})",
                f"INSERT INTO {table}_backup SELECT {colnames} FROM {table}",
                f"DROP TABLE {table}",
                f"CREATE TABLE {table}({coltypes})",
                f"INSERT INTO {table} SELECT {colnames} FROM {table}_backup",
                f"CREATE UNIQUE INDEX {table}_cat ON {table} ({keycol} )",
                f"DROP TABLE {table}_backup",
            ]
        else:
            sql = f'ALTER TABLE {table} DROP COLUMN "{column}"'

        try:
            db_handler.execute(sql=sql)
        except CalledModuleError:
            gs.fatal(_("Deleting column failed"))

    # write cmd history:
    gs.vector_history(map)


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
