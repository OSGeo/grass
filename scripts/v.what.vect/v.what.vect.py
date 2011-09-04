#!/usr/bin/env python
#
############################################################################
#
# MODULE:       v.what.vect
# AUTHOR(S):    Markus Neteler, converted to Python by Glynn Clements
# PURPOSE:      Uploads attributes at the location of vector points to the table.
# COPYRIGHT:    (C) 2005, 2008 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Uploads vector values at positions of vector points to the table.
#% keywords: vector
#% keywords: database
#% keywords: attribute table
#%end

#%option G_OPT_V_MAP
#%end
#%option G_OPT_V_FIELD
#%end
#%option G_OPT_DB_COLUMN
#% description: Name of attribute column to be updated with the query result
#% required: yes
#%end
#%option G_OPT_V_MAP
#% key: qmap
#% description: Name of vector map to be queried
#% required : yes
#%end
#%option G_OPT_V_FIELD
#% key: qlayer
#%end
#%option G_OPT_DB_COLUMN
#% key: qcolumn
#% description: Name of attribute column to be queried
#% required: yes
#%end
#%option
#% key: dmax
#% type: double
#% description: Maximum query distance in map units
#% answer: 0.0
#% required: no
#%end

import sys
from grass.script import core as grass

def main():
    grass.exec_command(
	"v.distance",
	_from = options['map'],
	to = options['qmap'],
	column = options['column'],
	to_column = options['qcolumn'],
	upload = "to_attr",
	dmax = options['dmax'],
	from_layer = options['layer'],
	to_layer = options['qlayer'])

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
