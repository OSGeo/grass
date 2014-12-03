#!/usr/bin/env python

############################################################################
#
# MODULE:       db.out.ogr
# AUTHOR(S):   	Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      exports attribute tables into various formats
# COPYRIGHT:    (C) 2007-2014 by Markus Neteler and the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Exports attribute tables into various formats.
#% keywords: database
#% keywords: export
#% keywords: attribute table
#%end

#%option G_OPT_V_INPUT
#% key: input
#% label: GRASS table name
#% required: yes
#%end

#%option G_OPT_F_OUTPUT
#% key: output
#% description: Output table file name or DB connection string
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
#% description: Name for output table (default: input name)
#% required: no
#%end

import sys
import os
from grass.script.utils import try_remove, basename
from grass.script import core as grass
from grass.exceptions import CalledModuleError


def main():
    input = options['input']
    layer = options['layer']
    format = options['format']
    output = options['output']
    table = options['table']

    if format.lower() == 'dbf':
	format = "ESRI_Shapefile"

    if format.lower() == 'csv':
	olayer = basename(output, 'csv')
    else:
	olayer = None

    #is there a simpler way of testing for --overwrite?
    dbffile = input + '.dbf'
    if os.path.exists(dbffile) and not grass.overwrite():
	grass.fatal(_("File <%s> already exists") % dbffile)

    if olayer:
        try:
            grass.run_command('v.out.ogr', quiet=True, input=input, layer=layer,
                              output=output,
                              format=format, type='point,line,area',
                              olayer=olayer)
        except CalledModuleError:
            grass.fatal(_("Module <%s> failed") % 'v.out.ogr')

    else:
        try:
            grass.run_command('v.out.ogr', quiet=True, input=input,
                              layer=layer, output=output,
                              format=format, type='point,line,area')
        except CalledModuleError:
            grass.fatal(_("Module <%s> failed") % 'v.out.ogr')

    if format == "ESRI_Shapefile":
	exts = ['shp', 'shx', 'prj']
	if output.endswith('.dbf'):
	    outname = basename(output, 'dbf')
	    for ext in exts:
		try_remove("%s.%s" % (outname, ext))
	    outname += '.dbf'
	else:
	    for ext in exts:
		try_remove(os.path.join(output, "%s.%s" % (input, ext)))
	    outname = os.path.join(output, input + ".dbf")
    elif format.lower() == 'csv':
	outname = output + '.csv'
    else:
	outname = input

    grass.message(_("Exported table <%s>") % outname)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

