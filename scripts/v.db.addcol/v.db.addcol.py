#!/usr/bin/env python
#
############################################################################
#
# MODULE:       v.db.addcolumn
# AUTHOR(S):   	Moritz Lennert 
#               Converted to Python by Glynn Clements
# PURPOSE:      interface to db.execute to add a column to the attribute table
#               connected to a given vector map
# COPYRIGHT:    (C) 2005 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################


#%Module
#%  description: Adds one or more columns to the attribute table connected to a given vector map.
#%  keywords: vector, database, attribute table
#%End

#%option
#% key: map
#% type: string
#% gisprompt: old,vector,vector
#% key_desc : name
#% description: Vector map for which to edit attribute table
#% required : yes
#%end

#%option
#% key: layer
#% type: integer
#% description: Layer where to add column
#% answer: 1
#% required : no
#%end

#%option
#% key: columns
#% type: string
#% description: Name and type of the new column(s) ('name type [,name type, ...]' - types depend on database backend, but all support VARCHAR(), INT, DOUBLE PRECISION and DATE)
#% required : yes
#%end

import sys
import os
import grass

def main():
    map = options['map']
    layer = options['layer']
    columns = options['columns']
    columns = [col.strip() for col in columns.split(',')]

    # does map exist in CURRENT mapset?
    mapset = grass.gisenv()['MAPSET']
    exists = bool(grass.find_file(map, element = 'vector', mapset = mapset)['file'])

    if not exists:
	grass.fatal("Vector map <%s> not found in current mapset" % map)

    f = grass.vector_db(map, layer)
    if not f:
	grass.fatal("There is no table connected to this map. Run v.db.connect or v.db.addtable first.")
    table = f[1]
    database = f[3]
    driver = f[4]

    colnum = len(columns)

    for col in columns:
	if not col:
	    grass.fatal("There is an empty column. Did you leave a trailing comma?")

	p = grass.feed_command('db.execute', database = database, driver = driver)
	p.stdin.write("ALTER TABLE %s ADD COLUMN %s" % (table, col))
	p.stdin.close()
	if p.wait() != 0:
	    grass.fatal("Unable to add column <%s>." % col)

    # write cmd history:
    grass.vector_history(map)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
