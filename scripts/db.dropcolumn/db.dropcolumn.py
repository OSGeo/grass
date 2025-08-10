#!/usr/bin/env python3

############################################################################
#
# MODULE:       db.dropcolumn
# AUTHOR(S):   	Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      Interface to db.execute to drop a column from an
#               attribute table
#               - with special trick for SQLite
# COPYRIGHT:    (C) 2007, 2012 by Markus Neteler and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

# %module
# % description: Drops a column from selected attribute table.
# % keyword: database
# % keyword: attribute table
# %End

# %flag
# % key: f
# % description: Force removal (required for actual deletion of files)
# %end

# %option G_OPT_DB_TABLE
# % required : yes
# %end

# %option G_OPT_DB_COLUMN
# % required : yes
# %end

# %option G_OPT_DB_DATABASE
# %end

# %option G_OPT_DB_DRIVER
# % options: dbf,odbc,ogr,sqlite,pg
# %end

import sys
import string

from grass.exceptions import CalledModuleError
import grass.script as gs


def main():
    table = options["table"]
    column = options["column"]
    database = options["database"]
    driver = options["driver"]
    force = flags["f"]

    # check if DB parameters are set, and if not set them.
    gs.run_command("db.connect", flags="c")

    if not database or not driver:
        kv = gs.db_connection()
        if not database:
            database = kv["database"]
        if not driver:
            driver = kv["driver"]
    # schema needed for PG?

    if force:
        gs.message(_("Forcing ..."))

    if column == "cat":
        gs.warning(
            _(
                "Deleting <%s> column which may be needed to keep "
                "table connected to a vector map"
            )
            % column
        )

    cols = [
        f[0] for f in gs.db_describe(table, database=database, driver=driver)["cols"]
    ]
    if column not in cols:
        gs.fatal(_("Column <%s> not found in table") % column)

    if not force:
        gs.message(_("Column <%s> would be deleted.") % column)
        gs.message(
            _("You must use the force flag (-f) to actually remove it. Exiting.")
        )
        return 0

    if driver == "sqlite":
        sqlite3_version = gs.read_command(
            "db.select",
            sql="SELECT sqlite_version();",
            flags="c",
            database=database,
            driver=driver,
        ).split(".")[0:2]

        # sqlite version 3.35 compared here
        if [int(i) for i in sqlite3_version] >= [int(i) for i in ["3", "35"]]:
            sql = "ALTER TABLE %s DROP COLUMN %s" % (table, column)
            if column == "cat":
                sql = "DROP INDEX %s_%s; %s" % (table, column, sql)
        else:
            # for older sqlite3 versions, use old way to remove column
            colnames = []
            coltypes = []
            for f in gs.db_describe(table)["cols"]:
                if f[0] != column:
                    colnames.append(f[0])
                    coltypes.append("%s %s" % (f[0], f[1]))

            colnames = ", ".join(colnames)
            coltypes = ", ".join(coltypes)

            cmds = [
                "BEGIN TRANSACTION",
                "CREATE TEMPORARY TABLE ${table}_backup(${coldef})",
                "INSERT INTO ${table}_backup SELECT ${colnames} FROM ${table}",  # noqa: RUF027
                "DROP TABLE ${table}",  # noqa: RUF027
                "CREATE TABLE ${table}(${coldef})",
                "INSERT INTO ${table} SELECT ${colnames} FROM ${table}_backup",  # noqa: RUF027
                "DROP TABLE ${table}_backup",  # noqa: RUF027
                "COMMIT",
            ]
            tmpl = string.Template(";\n".join(cmds))
            sql = tmpl.substitute(table=table, coldef=coltypes, colnames=colnames)
    else:
        sql = "ALTER TABLE %s DROP COLUMN %s" % (table, column)

    try:
        gs.write_command(
            "db.execute", input="-", database=database, driver=driver, stdin=sql
        )
    except CalledModuleError:
        gs.fatal(_("Cannot continue (problem deleting column)"))

    return 0


if __name__ == "__main__":
    options, flags = gs.parser()
    sys.exit(main())
