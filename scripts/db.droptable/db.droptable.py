#!/usr/bin/env python

############################################################################
#
# MODULE:       db.droptable
# AUTHOR(S):   	Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      interface to db.execute to drop an attribute table
# COPYRIGHT:    (C) 2007 by Markus Neteler and the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################


#%Module
#%  description: Drops an attribute table.
#%  keywords: database, attribute table
#%End

#%flag
#%  key: f
#%  description: Force removal (required for actual deletion of files)
#%end

#%option
#% key: table
#% type: string
#% key_desc : name
#% description: Table to drop
#% required : yes
#% gisprompt: old,dbtable,dbtable
#%end

import sys
import os
import grass
import subprocess

def main():
    table = options['table']
    force = flags['f']

    # check if DB parameters are set, and if not set them.
    grass.run_command('db.connect', flags = 'c')

    s = grass.read_command('db.connect', flags = 'p')
    kv = grass.parse_key_val(s, sep = ':')
    database = kv['database']
    driver = kv['driver']
    # schema needed for PG?

    if force:
	grass.message("Forcing ...")

    # check if table exists
    nuldev = file(os.devnull, 'w')
    if grass.run_command('db.describe', flags = 'c', table = table,
			 stdout = nuldev, stderr = nuldev):
	grass.fatal("Table <%s> not found in current mapset" % table)

    # check if table is used somewhere (connected to vector map)
    used = []
    vects = grass.list_strings('vect')
    for vect in vects:
	s = grass.read_command('v.db.connect', flags = 'g', map = vect, stderr = nuldev)
	if not s:
	    continue
	for l in s.splitlines():
	    f = l.split()
	    if f[1] == table:
		used.append(vect)
		break
    if used:
	grass.warning("Deleting table <%s> which is attached to following map(s):" % table)
	for vect in used:
	    grass.message(vect)

    if not force:
	grass.message("The table <%s> would be deleted." % table)
	grass.message("")
	grass.message("You must use the force flag to actually remove it. Exiting.")
	sys.exit(0)

    p = grass.start_command('db.execute', database = database, driver = driver,
			    stdin = subprocess.PIPE)
    p.stdin.write("DROP TABLE " + table)
    p.stdin.close()
    p.wait()
    if p.returncode != 0:
  	grass.fatal("Cannot continue (problem deleting table).")

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

