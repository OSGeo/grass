#!/usr/bin/env python

############################################################################
#
# MODULE:	v.db.univar (formerly called v.univar.sh)
# AUTHOR(S):	Michael Barton, Arizona State University
#               Converted to Python by Glynn Clements
#               Sync'ed to r.univar by Markus Metz
# PURPOSE:	Calculates univariate statistics from a GRASS vector map attribute column.
#               Based on r.univar.sh by Markus Neteler
# COPYRIGHT:	(C) 2005, 2007, 2008 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Calculates univariate statistics on selected table column for a GRASS vector map.
#% keywords: vector
#% keywords: statistics
#% keywords: attribute table
#%end
#%option G_OPT_V_MAP
#% required: yes
#%end
#%option G_OPT_V_FIELD
#%end
#%option G_OPT_DB_COLUMN
#% description: Name of attribute column on which to calculate statistics (must be numeric)
#% required: yes
#%end
#%option G_OPT_DB_WHERE
#%end
#%option
#% key: percentile
#% type: double
#% description: Percentile to calculate (requires extended statistics flag)
#% required : no
#% answer: 90
#% options: 0-100
#% multiple: yes
#%end
#%flag
#% key: e
#% description: Extended statistics (quartiles and 90th percentile)
#%end
#%flag
#% key: g
#% description: Print stats in shell script style
#%end

import sys
import os
import grass.script as grass

def main():
    global tmp
    tmp = grass.tempfile()

    vector = options['map']
    layer = options['layer']
    column = options['column']
    where = options['where']
    perc = options['percentile']
    extend = flags['e']
    shellstyle = flags['g']

    
    fi = grass.vector_db(vector, stderr = nuldev)[int(layer)]
    table = fi['table']
    database = fi['database']
    driver = fi['driver']
    
    passflags = None
    if flags['e']:
	passflags = 'e'
    if flags['g']:
	if not passflags:
	    passflags = 'g'
	else:
	    passflags = passflags + 'g'
    
    grass.run_command('db.univar', table = table, column = column, 
                      database = database, driver = driver,
		      perc = perc, where = where, flags = passflags)


if __name__ == "__main__":
    options, flags = grass.parser()
    nuldev = file(os.devnull, 'w')
    main()
