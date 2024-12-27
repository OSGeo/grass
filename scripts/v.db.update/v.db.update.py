#!/usr/bin/env python3
#
############################################################################
#
# MODULE:       v.db.update
# AUTHOR(S):    Moritz Lennert
#               Extensions by Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      Interface to db.execute to update a column in the attribute table connected to a given map
# COPYRIGHT:    (C) 2005-2014 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

# %module
# % description: Updates a column in the attribute table connected to a vector map.
# % keyword: vector
# % keyword: attribute table
# % keyword: database
# % keyword: attribute update
# % keyword: type casting
# %end
# %option G_OPT_V_MAP
# %end
# %option G_OPT_V_FIELD
# % required: yes
# %end
# %option G_OPT_DB_COLUMN
# % key: column
# % description: Name of attribute column to update
# % required: yes
# %end
# %option
# % key: value
# % type: string
# % description: Literal value to update the column with
# % required: no
# %end
# %option G_OPT_DB_COLUMN
# % key: query_column
# % description: Name of other attribute column to query, can be combination of columns (e.g. co1+col2)
# %end
# %option G_OPT_DB_WHERE
# %end
# %option G_OPT_F_INPUT
# % key: sqliteextra
# % description: Name of SQLite extension file for extra functions (SQLite backend only)
# % gisprompt: old,bin,file
# % required: no
# %end

import sys
import os
import grass.script as gs


def main():
    vector = options["map"]
    layer = options["layer"]
    column = options["column"]
    value = options["value"]
    qcolumn = options["query_column"]
    where = options["where"]
    sqlitefile = options["sqliteextra"]

    mapset = gs.gisenv()["MAPSET"]

    # does map exist in CURRENT mapset?
    if not gs.find_file(vector, element="vector", mapset=mapset)["file"]:
        gs.fatal(_("Vector map <%s> not found in current mapset") % vector)

    try:
        f = gs.vector_db(vector)[int(layer)]
    except KeyError:
        gs.fatal(
            _(
                "There is no table connected to this map. Run v.db.connect or v.db.addtable first."
            )
        )

    table = f["table"]
    database = f["database"]
    driver = f["driver"]

    # check for SQLite backend for extra functions
    if sqlitefile and driver != "sqlite":
        gs.fatal(_("Use of libsqlitefunctions only with SQLite backend"))
    if driver == "sqlite" and sqlitefile:
        if not os.access(sqlitefile, os.R_OK):
            gs.fatal(_("File <%s> not found") % sqlitefile)

    # Check column existence and get its type.
    all_columns = gs.vector_columns(vector, layer)
    coltype = None
    for column_name, column_record in all_columns.items():
        if column.lower() == column_name.lower():
            coltype = column_record["type"]
            break
    if not coltype:
        gs.fatal(_("Column <%s> not found") % column)

    if qcolumn:
        if value:
            gs.fatal(_("<value> and <qcolumn> are mutually exclusive"))
        # special case: we copy from another column
        value = qcolumn
    else:
        if not value:
            gs.fatal(_("Either <value> or <qcolumn> must be given"))
        # we insert a value
        if coltype.upper() not in {"INTEGER", "DOUBLE PRECISION"}:
            value = "'%s'" % value

    cmd = "UPDATE %s SET %s=%s" % (table, column, value)
    if where:
        cmd += " WHERE " + where

    # SQLite: preload extra functions from extension lib if provided by user
    if sqlitefile:
        sqliteload = "SELECT load_extension('%s');\n" % sqlitefile
        cmd = sqliteload + cmd

    gs.verbose('SQL: "%s"' % cmd)
    gs.write_command(
        "db.execute", input="-", database=database, driver=driver, stdin=cmd
    )

    # write cmd history:
    gs.vector_history(vector)

    return 0


if __name__ == "__main__":
    options, flags = gs.parser()
    sys.exit(main())
