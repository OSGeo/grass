#!/usr/bin/env python
#
############################################################################
#
# MODULE:       v.db.addtable
# AUTHOR(S):   	Markus Neteler 
#               Converted to Python by Glynn Clements
# PURPOSE:      interface to db.execute to creates and add a new table to given vector map
# COPYRIGHT:    (C) 2005, 2007, 2008  by Markus Neteler & the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Creates and connects a new attribute table to a given layer of an existing vector map.
#% keywords: vector
#% keywords: database
#% keywords: attribute table
#%end
#%option G_OPT_V_MAP
#%end
#%option 
#% key: table
#% type: string
#% description: Name of new attribute table (default: vector map name)
#% required : no
#%end
#%option 
#% key: layer
#% type: integer
#% description: Layer number where to add new attribute table
#% answer: 1
#% required: no
#%end
#%option
#% key: columns
#% type: string
#% description: Name and type of the new column(s) (types depend on database backend, but all support VARCHAR(), INT, DOUBLE PRECISION and DATE)
#% answer: cat integer
#% required: no
#% multiple: yes
#% key_desc: name type
#%end

import sys
import os
import grass.script as grass

def main():
    map = options['map']
    table = options['table']
    layer = options['layer']
    columns = options['columns']

    mapset = grass.gisenv()['MAPSET']

    # does map exist in CURRENT mapset?
    if not grass.find_file(map, element = 'vector', mapset = mapset)['file']:
	grass.fatal(_("Vector map <%s> not found in current mapset") % map)

    map_name = map.split('@')[0]

    if not table:
	if layer == '1':
	    grass.message(_("Using vector map name as table name: ") + map_name)
	    table = map_name
	else:
	    # to avoid tables with identical names on higher layers
	    grass.message(_("Using vector map name extended by layer number as table name: %s_%s") % (map_name, layer))
	    table = "%s_%s" % (map_name, layer)
    else:
	grass.message(_("Using user specified table name: ") + table)

    # check if DB parameters are set, and if not set them.
    grass.run_command('db.connect', flags = 'c')

    grass.message(_("Creating new DB connection based on default mapset settings..."))
    kv = grass.db_connection()
    database = kv['database']
    driver = kv['driver']

    # maybe there is already a table linked to the selected layer?
    nuldev = file(os.devnull, 'w')
    try:
        grass.vector_db(map, stderr = nuldev)[int(layer)]
	grass.fatal(_("There is already a table linked to layer <%s>") % layer)
    except KeyError:
        pass
    
    # maybe there is already a table with that name?
    found = False
    p = grass.pipe_command('db.tables', database = database, driver = driver, stderr = nuldev)
    for line in p.stdout:
	if line.rstrip('\r\n') == table:
	    found = True
	    break
    p.wait()

    if not found:
	column_def = [col.strip().lower() for col in columns.split(',')]

	#if not existing, create it:
	if "cat integer" not in column_def:
	    column_def.append("cat integer")
	column_def = ','.join(column_def)

	grass.message(_("Creating table with columns (%s)") % column_def)

	# take care if the DBF directory is missing (heck, the DBF driver should take care!)
	if driver == "dbf":
	    env = grass.gisenv()
	    path = os.path.join(env['GISDBASE'], env['LOCATION_NAME'], env['MAPSET'], "dbf")
	    if not os.path.isdir(path):
		grass.message(_("Creating missing DBF directory in mapset <%s>") % env['MAPSET'])
		os.mkdir(path)
		grass.run_command('db.connect', driver = dbf, database = '$GISDBASE/$LOCATION_NAME/$MAPSET/dbf/')

    sql = "CREATE TABLE %s (%s)" % (table, column_def)

    if grass.write_command('db.execute', input = '-', database = database, driver = driver, stdin = sql) != 0:
	grass.fatal(_("Cannot continue."))

    # connect the map to the DB:
    grass.run_command('v.db.connect', map = map, database = database, driver = driver,
		      layer = layer, table = table, key = 'cat')

    # finally we have to add cats into the attribute DB to make modules such as v.what.rast happy:
    # (creates new row for each vector line):
    grass.run_command('v.to.db', map = map, layer = layer,
                      option = 'cat', column = 'cat', qlayer = layer)

    if grass.verbosity() > 0:
	grass.message(_("Current attribute table links:"))
	grass.run_command('v.db.connect', flags = 'p', map = map)

    # write cmd history:
    grass.vector_history(map)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
