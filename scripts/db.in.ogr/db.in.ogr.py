#!/usr/bin/env python

############################################################################
#
# MODULE:       db.in.ogr
# AUTHOR(S):   	Markus Neteler
# PURPOSE:      imports attribute tables in various formats
#               Converted to Python by Glynn Clements
# COPYRIGHT:    (C) 2007, 2008 by Markus Neteler and the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%Module
#%  description: Imports attribute tables in various formats.
#%  keywords: database, attribute table
#%End

#%option
#% key: dsn
#% type: string
#% key_desc : name
#% gisprompt: old_file,file,input
#% description: Table file to be imported or DB connection string
#% required : yes
#%end

#%option
#% key: db_table
#% type: string
#% key_desc : name
#% description: Table name of SQL DB table
#% required : no
#%end

#%option
#% key: output
#% type: string
#% description: Name for output table
#% required : no
#%end

#%option
#% key: key
#% type: string
#% description: Name for auto-generated unique key column
#% required : no
#%end

import sys
import os
import grass

def main():
    dsn = options['dsn']
    db_table = options['db_table']
    output = options['output']
    key = options['key']

    mapset = grass.gisenv()['MAPSET']

    if db_table:
	input = db_table
    else:
	input = dsn

    if not output:
	tmpname = input.replace('.', '_')
	output = grass.basename(tmpname)

    if not grass.overwrite():
	s = grass.read_command('db.tables', flags = 'p')
	for l in s.splitlines():
	    if l == output:
		grass.fatal("Table <%s> already exists" % output)
    else:
	grass.write_command('db.execute', stdin = "DROP TABLE %s" % output)

    # treat DB as real vector map...
    layer_opt = {}
    if db_table:
	layer_opt['layer'] = db_table

    if grass.run_command('v.in.ogr', flags = 'o', dsn = dsn, output = output,
			 quiet = True, **layer_opt) != 0:
	if db_table:
	    grass.fatal("Input table <%s> not found or not readable" % input)
	else:
	    grass.fatal("Input DSN <%s> not found or not readable" % input)

    nuldev = file(os.devnull, 'w')

    # rename ID col if requested from cat to new name
    if key:
	grass.run_command('v.db.renamecol', quiet = True, map = output, layer = 1,
			  column = (cat, key), stdout = nuldev, stderr = nuldev)

    # ... and immediately drop the empty geometry
    vectfile = grass.find_file(output, element = 'vector', mapset = mapset)['file']
    if not file:
	grass.fatal("Something went wrong. Should not happen")
    else:
	# remove the vector part
	grass.try_remove(file)

    # get rid of superfluous auto-added cat column (and cat_ if present)
    grass.run_command('db.dropcol', quiet = True, flags = 'f', table = output,
		      colum = 'cat', stdout = nuldev, stderr = nuldev)

    s = grass.read_command('db.describe', flags = 'c', table = output)
    kv = grass.parse_key_val(s, sep = ':')
    records = int(kv['nrows'].strip())
    grass.message("Imported table <%s> with %d rows" % (output, records))

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
