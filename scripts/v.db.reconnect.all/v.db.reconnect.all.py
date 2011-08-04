#!/usr/bin/env python
############################################################################
#
# MODULE:	v.db.reconnect.all
# AUTHOR(S):	Radim Blazek
#               Converted to Python by Glynn Clements
# PURPOSE:	Reconnect vectors
# COPYRIGHT:	(C) 2004 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Reconnects vectors to a new database.
#% keywords: vector
#% keywords: database
#% keywords: attribute table
#%end
#%option G_OPT_DB_DATABASE
#% key: old_database
#% description: Name of old database
#% required: yes
#%end
#%option G_OPT_DB_DATABASE
#% key: new_database
#% description: Name of new database
#% required: yes
#%end
#%option G_OPT_DB_SCHEMA
#% key: old_schema
#% description: Old database schema
#%end
#%option G_OPT_DB_SCHEMA
#% key: new_schema
#% description: New database schema
#%end

import sys
import os
import grass.script as grass

def main():
    old_database = options['old_database']
    new_database = options['new_database']
    old_schema = options['old_schema']
    new_schema = options['new_schema']

    mapset = grass.gisenv()['MAPSET']

    nuldev = file(os.devnull, 'w')

    for vect in grass.list_grouped('vect')[mapset]:
	vect = "%s@%s" % (vect, mapset)
	grass.message(_("Reconnecting vector <%s>") % vect)
	for f in grass.vector_db(map, stderr = nuldev).itervalues():
	    layer = f['layer']
	    schema_table = f['table']
	    key = f['key']
	    database = f['database']
	    driver = f['driver']
	    if '.' in schema_table:
		st = schema_table.split('.', 1)
		schema = st[0]
		table = st[1]
	    else:
		schema = ''
		table = schema_table

	    if new_schema:
		new_schema_table = "%s.%s" % (new_schema, table)
	    else:
		new_schema_table = table

	    grass.message(_("SCHEMA = %s TABLE = %s NEW_SCHEMA_TABLE = %s") % (schema, table, new_schema_table))
	    if database == old_database and schema == old_schema:
		grass.message(_("Reconnecting layer ") + layer)
		grass.message(_("v.db.connect -o map=%s layer=%s driver=%s database=%s table=%s key=%s") %
			      (vect, layer, driver, new_database, new_schema_table, key))
		grass.run_command('v.db.connect', flags = 'o', map = vect,
				  layer = layer, driver = driver, database = new_database,
				  table = new_schema_table, key = key)
	    else:
		grass.message(_("Layer <%s> will not be reconnected, database or schema do not match.") % layer)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

