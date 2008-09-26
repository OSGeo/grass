#!/usr/bin/env python

############################################################################
#
# MODULE:       v.db.join
# AUTHOR(S):    Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      Join a table to a map table
# COPYRIGHT:    (C) 2007 by Markus Neteler and the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%Module
#% description: Allows to join a table to a vector map table.
#% keywords: vector, database, attribute table
#%End

#%option
#% key: map
#% type: string
#% key_desc : name
#% gisprompt: old,vector,vector
#% description: Vector map to which to join other table
#% required : yes
#%end

#%option
#% key: layer
#% type: integer
#% description: Layer where to join
#% answer: 1
#% required : no
#%end

#%option
#% key: column
#% type: string
#% description: Join column in map table
#% required : yes
#%end

#%option
#% key: otable
#% type: string
#% description: Other table name
#% required : yes
#%end

#%option
#% key: ocolumn
#% type: string
#% description: Join column in other table
#% required : yes
#%end

import sys
import os
import subprocess
import grass

def main():
    map = options['map']
    layer = options['layer']
    column = options['column']
    otable = options['otable']
    ocolumn = options['ocolumn']

    s = grass.read_command('v.db.connect', flags = 'g', map = map, layer = layer)
    f = s.split()
    maptable = f[1]
    database = f[3]
    driver = f[4]

    if driver == 'dbf':
	grass.fatal("JOIN is not supported for tables stored in DBF format.")

    if not maptable:
	grass.fatal("There is no table connected to this map. Cannot join any column.")

    found = False
    s = grass.read_command('v.info', flags = 'c', map = map, layer = layer, quiet = True)
    for line in s.splitlines():
	f = line.split('|')
	if len(f) > 1 and f[1] == column:
	    found = True
    if not found:
	grass.fatal("Column <%> not found in table <%s> at layer <%s>" % (column, map, layer))

    s = grass.read_command('db.describe', flags = 'c', driver = driver,
			   database = database, table = otable)
    cols = [l.split(':') for l in s.splitlines() if l.startswith('Column ')]

    select = "SELECT $colname FROM $otable WHERE $otable.$ocolumn=$table.$column"
    template = Template("UPDATE $table SET $colname=(%s);" % select)

    for col in cols:
	colname = col[0]
	if len(col) > 2:
	    coltype = "%s(%s)" % col[1:3]
	else:
	    coltype = "%s" % col[1]
	colspec = "%s %s" % (colname, coltype)
	if grass.run_command('v.db.addcol', map = map, col = colspec) != 0:
	    grass.fatal("Error creating column <%s>." % colname)

	stmt = template.substitute(table = maptable, column = column,
				   otable = otable, ocolumn = ocolumn,
				   colname = colname)

	p = grass.start_command('db.execute', stdin = subprocess.PIPE)
	p.stdin.write(stmt)
	p.stdin.close()
	p.wait()
	if p.returncode != 0:
	    grass.fatal("Error filling column <%s>." % colname)

    # write cmd history:
    grass.run_command('v.support', map = map, cmdhist = cmdline)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
