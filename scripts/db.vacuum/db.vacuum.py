#!/usr/bin/env python3

############################################################################
#
# MODULE:       db.vacuum
# AUTHOR(S):    Vaclav Petras
# PURPOSE:      Interface to db.execute to drop an attribute table
# COPYRIGHT:    (C) 2022 by Vaclav Petras and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

# %module
# % description: Vacuums (cleans) database from dropped (deleted) data
# % keyword: database
# % keyword: attribute table
# %end

# %option
# % key: operation
# % type: string
# % required: yes
# % multiple: no
# % options: vacuum,enable_auto_vacuum,disable_auto_vacuum,enable_incremental_vacuum,do_incremental_vacuum
# % description: Operation to be performed
# %end

# %option G_OPT_DB_DRIVER
# % label: Name of database driver
# % description: If not given then default driver is used
# % guisection: Connection
# %end

# %option G_OPT_DB_DATABASE
# % label: Name of database
# % description:  If not given then default database is used
# % guisection: Connection
# %end

import sys

import grass.script as gs


def database_vacuum(operation, database, driver):
    if operation == "vacuum":
        sql = "VACUUM;"
    elif operation == "enable_auto_vacuum" and driver == "sqlite":
        sql = "PRAGMA auto_vacuum = 'full'; VACUUM;"
    elif operation == "disable_auto_vacuum" and driver == "sqlite":
        sql = "PRAGMA auto_vacuum = 'none'; VACUUM;"
    elif operation == "enable_incremental_vacuum" and driver == "sqlite":
        sql = "PRAGMA auto_vacuum = 'incremental'; VACUUM;"
    elif operation == "do_incremental_vacuum" and driver == "sqlite":
        sql = "PRAGMA incremental_vacuum;"
    else:
        gs.fatal(_("Unsupported operation <{operation} for database driver <{driver}>").format(operation=operation, driver=driver))
    gs.run_command("db.execute", input=f"{sql}", database=database, driver=driver, error="fatal")


def main():
    options, unused_flags = gs.parser()
    table = options["table"]

    if not options["driver"] or not options["database"]:
        # check if DB parameters are set, and if not set them.
        gs.run_command("db.connect", flags="c", quiet=True)

    kv = gs.db_connection()
    if options["database"]:
        database = options["database"]
    else:
        database = kv["database"]
    if options["driver"]:
        driver = options["driver"]
    else:
        driver = kv["driver"]
    # schema needed for PG?

    database_vacuum(operation=options["operation"], database, driver)

if __name__ == "__main__":
    main()
