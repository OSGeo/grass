#!/usr/bin/env python

############################################################################
#
# MODULE:       v.db.renamecol
# AUTHOR(S):    Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      interface to db.execute to drop a column from the 
#               attribute table connected to a given vector map
#               - Based on v.db.dropcol
#               - with special trick for SQLite and DBF (here the new col is 
#                 added/values copied/old col deleted)
# COPYRIGHT:    (C) 2007 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
# TODO: MySQL untested
#############################################################################


#%Module
#%  description: Renames a column in the attribute table connected to a given vector map.
#%  keywords: vector, database, attribute table
#%End
#%option
#% key: map
#% type: string
#% gisprompt: old,vector,vector
#% key_desc : name
#% description: Vector map for which to rename attribute column
#% required : yes
#%end
#%option
#% key: layer
#% type: integer
#% description: Layer where to rename column
#% answer: 1
#% required : no
#%end
#%option
#% key: column
#% type: string
#% description: Old and new name of the column (old,new)
#% required : yes
#% multiple: yes
#% key_desc: oldcol,newcol
#%end

import sys
import os
import grass

def main():
    map = options['map']
    layer = options['layer']
    column = options['column']

    mapset = grass.gisenv()['MAPSET']

    if not grass.find_file(map, element = 'vector', mapset = mapset):
	grass.fatal("Vector map <%s> not found in current mapset" % map)

    s = grass.read_command('v.db.connect', flags = 'g', map = map, layer = layer);
    if not s:
	grass.fatal("An error occured while running v.db.connect")
    f = s.split()
    table = f[1]
    keycol = f[2]
    database = f[3]
    driver = f[4]

    if not table:
	grass.fatal("There is no table connected to the input vector map. Cannot rename any column")

    cols = column.split(',')
    oldcol = cols[0]
    newcol = cols[1]

    if driver == "dbf":
	if len(newcol) > 10:
	    grass.fatal("Column name <%s> too long. The DBF driver supports column names not longer than 10 characters" % newcol)

    if oldcol == keycol:
	grass.fatal("Cannot rename column <%s> as it is needed to keep table <%s> connected to the input vector map" % (oldcol, table))

    # describe old col
    oldcoltype = None
    s = grass.read_command('db.describe', flags = 'c', table = table)
    for l in s.splitlines():
	if not l.startswith('Column '):
	    continue
	f = l.split(':')
	if f[1].lstrip() != oldcol:
	    continue
	oldcoltype = f[2]
	oldcollength = f[3]

    # old col there?
    if not oldcol:
	grass.fatal("Column <%s> not found in table <%s>" % (coldcol, table))

    # some tricks
    if driver in ['sqlite', 'dbf']:
	if oldcoltype.upper() == "CHARACTER":
	    colspec = "%s varchar(%s)" % (newcol, oldcollength)
	else:
	    colspec = "%s %s" % (newcol, oldcoltype)

	grass.run_command('v.db.addcol', map = map, layer = layer, column = colspec)
	sql = "UPDATE %s SET %s=%s" % (table, newcol, oldcol)
	grass.write_command('db.execute', database = database, driver = driver, stdin = sql)
	grass.run_command('v.db.dropcol', map = map, layer = layer, column = oldcol)
    else:
	sql = "ALTER TABLE %s RENAME %s TO %s" % (table, oldcol, newcol)
	grass.write_command('db.execute', database = database, driver = driver, stdin = sql)

    # write cmd history:
    grass.run_command('v.support', map = map, cmdhist = os.environ['CMDLINE'])

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
