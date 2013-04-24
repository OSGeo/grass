#!/usr/bin/env python

############################################################################
#
# MODULE:       db.out.ogr
# AUTHOR(S):   	Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      exports attribute tables into various formats
# COPYRIGHT:    (C) 2007 by Markus Neteler and the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Exports attribute tables into various formats.
#% keywords: database
#% keywords: attribute table
#%end

#%option G_OPT_DB_TABLE
#% key: input
#% required: yes
#%end

#%option G_OPT_F_OUTPUT
#% key: dsn
#% description: Table file to be exported or DB connection string
#% required : yes
#%end

#%option G_OPT_V_FIELD
#% required: no
#%end

#%option
#% key: format
#% type: string
#% description: Table format
#% required: yes
#% options: CSV,DBF,GML,MySQL,PostgreSQL,SQLite
#% answer: CSV
#%end

#%option
#% key: table
#% type: string
#% key_desc: name
#% description: Name for output table (defaut: input)
#% required: no
#%end

import sys
import os
from grass.script import core as grass

def main():
    input = options['input']
    layer = options['layer']
    format = options['format']
    dsn = options['dsn']
    table = options['table']

    if format.lower() == 'dbf':
	format = "ESRI_Shapefile"

    if format.lower() == 'csv':
	olayer = grass.basename(dsn, 'csv')
    else:
	olayer = None

    #is there a simpler way of testing for --overwrite?
    dbffile = input + '.dbf'
    if os.path.exists(dbffile) and not grass.overwrite():
	grass.fatal(_("File <%s> already exists") % dbffile)

    if olayer:
	if grass.run_command('v.out.ogr', flags = 'c', quiet = True, input = input, layer = layer,
			     dsn = dsn,
			     format = format, type = 'point,line,area', olayer = olayer) != 0:
	    sys.exit(1)
    else:
	if grass.run_command('v.out.ogr', flags = 'c', quiet = True, input = input, layer = layer,
			     dsn = dsn, format = format, type = 'point,line,area') != 0:
	    sys.exit(1)

    if format == "ESRI_Shapefile":
	exts = ['shp', 'shx', 'prj']
	if dsn.endswith('.dbf'):
	    outname = grass.basename(dsn, 'dbf')
	    for ext in exts:
		grass.try_remove("%s.%s" % (outname, ext))
	    outname += '.dbf'
	else:
	    for ext in exts:
		grass.try_remove(os.path.join(dsn, "%s.%s" % (input, ext)))
	    outname = os.path.join(dsn, input + ".dbf")
    elif format.lower() == 'csv':
	outname = dsn + '.csv'
    else:
	outname = input

    grass.message(_("Exported table <%s>") % outname)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

