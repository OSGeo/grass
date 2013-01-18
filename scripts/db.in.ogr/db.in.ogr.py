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
#%  keywords: database
#%  keywords: attribute table
#%End

#%option G_OPT_F_INPUT
#% key: dsn
#% gisprompt: old,bin,file
#% description: Table file to be imported or DB connection string
#% required : yes
#%end

#%option
#% key: db_table
#% type: string
#% key_desc : name
#% description: Name of table from given DB to be imported
#% required : no
#%end

#%option 
#% key: output
#% type: string
#% key_desc : name
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
import grass.script as grass

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
		grass.fatal(_("Table <%s> already exists") % output)
    else:
	grass.write_command('db.execute', input = '-', stdin = "DROP TABLE %s" % output)

    # treat DB as real vector map...
    if db_table:
	layer = db_table
    else:
	layer = None

    if grass.run_command('v.in.ogr', flags = 'o', dsn = dsn, output = output,
			 layer = layer, quiet = True) != 0:
	if db_table:
	    grass.fatal(_("Input table <%s> not found or not readable") % input)
	else:
	    grass.fatal(_("Input DSN <%s> not found or not readable") % input)
    
    # rename ID col if requested from cat to new name
    if key:
	grass.write_command('db.execute', quiet = True,
                          input = '-', 
			  stdin = "ALTER TABLE %s ADD COLUMN %s integer" % (output, key) )
	grass.write_command('db.execute', quiet = True,
                          input = '-', 
			  stdin = "UPDATE %s SET %s=cat" % (output, key) )

    # ... and immediately drop the empty geometry
    vectfile = grass.find_file(output, element = 'vector', mapset = mapset)['file']
    if not vectfile:
	grass.fatal(_("Something went wrong. Should not happen"))
    else:
	# remove the vector part
	grass.run_command('v.db.connect', quiet = True, map = output, layer = '1', flags = 'd')
	grass.run_command('g.remove', quiet = True, vect = output)

    # get rid of superfluous auto-added cat column (and cat_ if present)
    nuldev = file(os.devnull, 'w+')
    grass.run_command('db.dropcolumn', quiet = True, flags = 'f', table = output,
		      column = 'cat', stdout = nuldev, stderr = nuldev)
    nuldev.close()
    
    records = grass.db_describe(output)['nrows']
    grass.message(_("Imported table <%s> with %d rows") % (output, records))

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
