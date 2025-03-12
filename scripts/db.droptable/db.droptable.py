#!/usr/bin/env python3

############################################################################
#
# MODULE:       db.droptable
# AUTHOR(S):    Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      Interface to db.execute to drop an attribute table
# COPYRIGHT:    (C) 2007, 2012 by Markus Neteler and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

# %module
# % description: Drops an attribute table.
# % keyword: database
# % keyword: attribute table
# %end

# %flag
# % key: f
# % description: Force removal (required for actual deletion of files)
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

# %option G_OPT_DB_TABLE
# % description: Name of table to drop
# % required: yes
# %end

import sys
import grass.script as gs
from grass.script.utils import encode


def main():
    table = options["table"]
    force = flags["f"]

    if not options["driver"] or not options["database"]:
        # check if DB parameters are set, and if not set them.
        gs.run_command("db.connect", flags="c", quiet=True)

    kv = gs.db_connection()
    database = options["database"] or kv["database"]
    driver = options["driver"] or kv["driver"]
    # schema needed for PG?

    if force:
        gs.message(_("Forcing ..."))

    # check if table exists
    if not gs.db_table_exist(table):
        gs.warning(_("Table <%s> not found in database <%s>") % (table, database))
        sys.exit(0)

    # check if table is used somewhere (connected to vector map)
    used = gs.db.db_table_in_vector(table)
    if used:
        gs.warning(
            _("Deleting table <%s> which is attached to following map(s):") % table
        )
        for vect in used:
            gs.warning("%s" % vect)

    if not force:
        gs.message(_("The table <%s> would be deleted.") % table)
        gs.message(_("You must use the force flag to actually remove it. Exiting."))
        sys.exit(0)

    p = gs.feed_command("db.execute", input="-", database=database, driver=driver)
    p.stdin.write(encode("DROP TABLE " + table))
    p.stdin.close()
    p.wait()
    if p.returncode != 0:
        gs.fatal(_("Cannot continue (problem deleting table)."))


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
