#!/usr/bin/env python

############################################################################
#
# MODULE:       db.dropcolumn
# AUTHOR(S):   	Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      interface to db.execute to drop a column from an 
#               attribute table
#               - with special trick for SQLite
# COPYRIGHT:    (C) 2007 by Markus Neteler and the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################


#%Module
#%  description: Drops a column from selected attribute table
#%  keywords: database
#%  keywords: attribute table
#%End

#%flag
#%  key: f
#%  description: Force removal (required for actual deletion of files)
#%end

#%option G_OPT_DB_TABLE
#% required : yes
#%end

#%option G_OPT_DB_COLUMN
#% required : yes
#%end

import sys
import os
import string
import grass.script as grass

def main():
    table = options['table']
    column = options['column']
    force = flags['f']

    # check if DB parameters are set, and if not set them.
    grass.run_command('db.connect', flags = 'c')

    kv = grass.db_connection()
    database = kv['database']
    driver = kv['driver']
    # schema needed for PG?

    if force:
	grass.message(_("Forcing ..."))

    if column == "cat":
	grass.warning(_("Deleting <%s> column which may be needed to keep table connected to a vector map") % column)

    cols = [f[0] for f in grass.db_describe(table)['cols']]
    if column not in cols:
	grass.fatal(_("Column <%s> not found in table") % column)

    if not force:
	grass.message(_("Column <%s> would be deleted.") % column)
	grass.message("")
	grass.message(_("You must use the force flag to actually remove it. Exiting."))
	sys.exit(0)

    if driver == "sqlite":
	#echo "Using special trick for SQLite"
	# http://www.sqlite.org/faq.html#q13
	colnames = []
	coltypes = []
	for f in grass.db_describe(table)['cols']:
	    if f[0] == column:
		continue
	    colnames.append(f[0])
	    coltypes.append("%s %s" % (f[0], f[1]))

	colnames = ", ".join(colnames)
	coltypes = ", ".join(coltypes)

	cmds = [
	    "BEGIN TRANSACTION",
	    "CREATE TEMPORARY TABLE ${table}_backup(${coldef})",
	    "INSERT INTO ${table}_backup SELECT ${colnames} FROM ${table}",
	    "DROP TABLE ${table}",
	    "CREATE TABLE ${table}(${coldef})",
	    "INSERT INTO ${table} SELECT ${colnames} FROM ${table}_backup",
	    "DROP TABLE ${table}_backup",
	    "COMMIT"
	    ]
	tmpl = string.Template(';\n'.join(cmds))
	sql = tmpl.substitute(table = table, coldef = coltypes, colnames = colnames)
    else:
	sql = "ALTER TABLE %s DROP COLUMN %s" % (table, column)

    if grass.write_command('db.execute', input = '-', database = database, driver = driver,
			   stdin = sql) != 0:
	grass.fatal(_("Cannot continue (problem deleting column)."))

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
