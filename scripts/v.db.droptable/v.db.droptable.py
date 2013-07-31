#!/usr/bin/env python

############################################################################
#
# MODULE:       v.db.droptable
# AUTHOR(S):   	Markus Neteler 
#               Converted to Python by Glynn Clements
# PURPOSE:      interface to db.execute to drop an existing table of given vector map
# COPYRIGHT:    (C) 2005, 2008 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################


#%Module
#% description: Removes existing attribute table of a vector map.
#% keywords: vector
#% keywords: attribute table
#% keywords: database
#%End
#%flag
#% key: f
#% description: Force removal (required for actual deletion of table)
#%end
#%option G_OPT_V_MAP
#%end
#%option G_OPT_DB_TABLE
#% description: Table name (default: vector map name)
#%end
#%option G_OPT_V_FIELD
#% required : no
#%end

import sys
import os
import grass.script as grass

def main():
    force = flags['f']
    map = options['map']
    table = options['table']
    layer = options['layer']

    # do some paranoia tests as well:
    f = grass.vector_layer_db(map, layer)
    
    if not table:
	# Removing table name connected to selected layer
	table = f['table']
	if not table:
	    grass.fatal(_("No table assigned to layer <%s>") % layer)
    else:
	# Removing user specified table
	existingtable = f['table']
	if existingtable != table:
	    grass.fatal(_("User selected table <%s> but the table <%s> is linked to layer <%s>")
			% (table, existingtable, layer))

    # we use the DB settings selected layer 
    database = f['database']
    driver = f['driver']

    grass.message(_("Removing table <%s> linked to layer <%s> of vector map <%s>")
		  % (table, layer, map)) 

    if not force:
	grass.message(_("You must use the -f (force) flag to actually remove the table. Exiting."))
	grass.message(_("Leaving map/table unchanged."))
	sys.exit(0)

    grass.message(_("Dropping table <%s>...") % table)

    if grass.write_command('db.execute', stdin = "DROP TABLE %s" % table, input = '-',
                           database = database, driver = driver) != 0:
	grass.fatal(_("An error occurred while running db.execute"))

    grass.run_command('v.db.connect', flags = 'd', map = map, layer = layer)

    grass.message(_("Current attribute table link(s):")) 
    # silently test first to avoid confusing error messages
    nuldev = file(os.devnull, 'w')
    if grass.run_command('v.db.connect', flags ='p', map = map, quiet = True,
			 stdout = nuldev, stderr = nuldev) != 0:
	grass.message(_("(No database links remaining)"))
    else:
	grass.run_command('v.db.connect', flags ='p', map = map)

    # write cmd history:
    grass.vector_history(map)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
