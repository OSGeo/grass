#!/usr/bin/env python
#
############################################################################
#
# MODULE:       v.db.update
# AUTHOR(S):    Moritz Lennert
#               Extensions by Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      Interface to db.execute to update a column in the attribute table connected to a given map
# COPYRIGHT:    (C) 2005,2007-2008,2011 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Updates a column in the attribute table connected to a vector map.
#% keywords: vector
#% keywords: attribute table
#% keywords: database
#%end
#%option G_OPT_V_MAP
#%end
#%option G_OPT_V_FIELD
#%end
#%option G_OPT_DB_COLUMN
#% key: column
#% description: Name of attribute column to update
#% required: yes
#%end
#%option
#% key: value
#% type: string
#% description: Literal value to update the column with
#% required: no
#%end
#%option G_OPT_DB_COLUMN
#% key: qcolumn
#% description: Name of other attribute column to query, can be combination of columns (e.g. co1+col2)
#%end
#%option G_OPT_DB_WHERE
#%end

import sys
import os
import grass.script as grass

def main():
    vector = options['map']
    layer = options['layer']
    column = options['column']
    value = options['value']
    qcolumn = options['qcolumn']
    where = options['where']

    mapset = grass.gisenv()['MAPSET']

    # does map exist in CURRENT mapset?
    if not grass.find_file(vector, element = 'vector', mapset = mapset)['file']:
	grass.fatal(_("Vector map <%s> not found in current mapset") % vector)

    try:
        f = grass.vector_db(vector)[int(layer)]
    except KeyError:
	grass.fatal(_('There is no table connected to this map. Run v.db.connect or v.db.addtable first.'))

    table = f['table']
    database = f['database']
    driver = f['driver']

    # checking column types
    try:
        coltype = grass.vector_columns(vector, layer)[column]['type']
    except KeyError:
	grass.fatal(_('Column <%s> not found') % column)

    if qcolumn:
	if value:
	    grass.fatal(_('<value> and <qcolumn> are mutually exclusive'))
	# special case: we copy from another column
	value = qcolumn
    else:
	if not value:
	    grass.fatal(_('Either <value> or <qcolumn> must be given'))
	# we insert a value
	if coltype.upper() not in ["INTEGER", "DOUBLE PRECISION"]:
	    value = "'%s'" % value

    cmd = "UPDATE %s SET %s=%s" % (table, column, value)
    if where:
	cmd += " WHERE " + where

    grass.verbose("SQL: \"%s\"" % cmd)

    grass.write_command('db.execute', input = '-', database = database, driver = driver, stdin = cmd)

    # write cmd history:
    grass.vector_history(vector)

    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
