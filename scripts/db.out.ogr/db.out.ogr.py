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

#%Module
#%  description: Exports attribute tables into various formats.
#%  keywords: database, attribute table
#%End

#%option
#% key: input
#% type: string
#% key_desc : name
#% description: GRASS table name
#% required : yes
#%end

#%option
#% key: dsn
#% type: string
#% key_desc : name
#% gisprompt: new_file,file,input
#% description: Table file to be exported or DB connection string
#% required : yes
#%end

#%option
#% key: format
#% type: string
#% description: Table format
#% required : yes
#% options: CSV,DBF,GML,MySQL,PostgreSQL,SQLite
#% answer: DBF
#%end

#%option
#% key: db_table
#% type: string
#% key_desc : name
#% description: Name for output table
#% required : no
#%end

import sys
import os
import grass

def main():
    input = options['input']
    format = options['format']
    dsn = options['dsn']
    table = options['db_table']

    if format.lower() == 'dbf':
	format = "ESRI_Shapefile"

    if format.lower() == 'csv':
	olayer = grass.basename(dsn, 'csv')
    else:
	olayer = None

    #is there a simpler way of testing for --overwrite?
    dbffile = input + '.dbf'
    if os.path.exists(dbffile) and not grass.overwrite():
	grass.fatal("File <%s> already exists" % dbffile)

    if grass.run_command('v.out.ogr', quiet = True, input = input, dsn = dsn,
			 format = format, type = 'point', olayer = olayer) != 0:
	sys.exit(1)

    if format == "ESRI_Shapefile":
	exts = ['shp', 'shx', 'prj']
	if dsn.endswith('.dbf'):
	    outname = grass.basename(dsn, 'dbf')
	    for ext in exts:
		try_remove("%s.%s" % (outname, ext))
	    outname += '.dbf'
	else:
	    for ext in exts:
		try_remove(os.path.join(dsn, "%s.%s" % (input, ext)))
	    outname = os.path.join(dsn, input + ".dbf")
    elif format.lower() == 'csv':
	outname = dsn + '.csv'
    else:
	outname = input

    grass.message("Exported table <%s>" % outname)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

