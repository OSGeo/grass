#!/usr/bin/env python
#
############################################################################
#
# MODULE:       v.db.addtable
# AUTHOR(S):   	Markus Neteler 
#               Converted to Python by Glynn Clements
#               Key column added by Martin Landa <landa.martin gmail.com>
# PURPOSE:      interface to db.execute to creates and add a new table to given vector map
# COPYRIGHT:    (C) 2005, 2007, 2008, 2011  by Markus Neteler & the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Creates and connects a new attribute table to a given layer of an existing vector map.
#% keywords: vector
#% keywords: attribute table
#% keywords: database
#%end
#%option G_OPT_V_MAP
#%end
#%option 
#% key: table
#% type: string
#% description: Name of new attribute table (default: vector map name)
#% required: no
#% guisection: Definition
#%end
#%option 
#% key: layer
#% type: integer
#% description: Layer number where to add new attribute table
#% answer: 1
#% required: no
#% guisection: Definition
#%end
#%option G_OPT_DB_KEYCOLUMN
#% guisection: Definition
#%end
#%option
#% key: columns
#% type: string
#% label: Name and type of the new column(s)
#% description: Types depend on database backend, but all support VARCHAR(), INT, DOUBLE PRECISION and DATE. Example: "label varchar(250), type integer"
#% required: no
#% multiple: yes
#% key_desc: name type
#% guisection: Definition
#%end

import sys
import os
import grass.script as grass

def main():
    vector = options['map']
    table = options['table']
    layer = options['layer']
    columns = options['columns']
    key = options['key']
    
    # does map exist in CURRENT mapset?
    mapset = grass.gisenv()['MAPSET']
    if not grass.find_file(vector, element = 'vector', mapset = mapset)['file']:
	grass.fatal(_("Vector map <%s> not found in current mapset") % vector)
    
    map_name = vector.split('@')[0]
    
    if not table:
	if layer == '1':
	    grass.verbose(_("Using vector map name as table name: <%s>") % map_name)
	    table = map_name
	else:
	    # to avoid tables with identical names on higher layers
            table = "%s_%s" % (map_name, layer)
	    grass.verbose(_("Using vector map name extended by layer number as table name: <%s>") % table)
    else:
	grass.verbose(_("Using user specified table name: %s") % table)
    
    # check if DB parameters are set, and if not set them.
    grass.run_command('db.connect', flags = 'c')
    grass.verbose(_("Creating new DB connection based on default mapset settings..."))
    kv = grass.db_connection()
    database = kv['database']
    driver = kv['driver']
    
    # maybe there is already a table linked to the selected layer?
    nuldev = file(os.devnull, 'w')
    try:
        grass.vector_db(map_name, stderr = nuldev)[int(layer)]
	grass.fatal(_("There is already a table linked to layer <%s>") % layer)
    except KeyError:
        pass
    
    # maybe there is already a table with that name?
    tables = grass.read_command('db.tables', flags = 'p', database = database, driver = driver,
                                stderr = nuldev)
    
    if not table in tables.splitlines():
        if columns:
            column_def = map(lambda x: x.strip().lower(), columns.strip().split(','))
        else:
            column_def = []
        
	# if not existing, create it:
        column_def_key = "%s integer" % key
	if column_def_key not in column_def:
	    column_def.insert(0, column_def_key)
        column_def = ','.join(column_def)
        
	grass.verbose(_("Creating table with columns (%s)...") % column_def)
        
        sql = "CREATE TABLE %s (%s)" % (table, column_def)
        if grass.run_command('db.execute', database = database, driver = driver, sql = sql) != 0:
            grass.fatal(_("Unable to create table <%s>") % table)
    
    # connect the map to the DB:
    grass.run_command('v.db.connect', quiet = True,
                      map = map_name, database = database, driver = driver,
		      layer = layer, table = table, key = key)
    
    # finally we have to add cats into the attribute DB to make modules such as v.what.rast happy:
    # (creates new row for each vector line):
    grass.run_command('v.to.db', map = map_name, layer = layer,
                      option = 'cat', column = key, qlayer = layer)
    
    grass.verbose(_("Current attribute table links:"))
    if grass.verbosity() > 2:
	grass.run_command('v.db.connect', flags = 'p', map = map_name)
    
    # write cmd history:
    grass.vector_history(map_name)
    
    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
