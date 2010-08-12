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

#%Module
#%  description: Uploads vector values at positions of vector points to the table.
#%  keywords: vector
#%  keywords: database
#%  keywords: attribute table
#%End

#%option
#% key: vector
#% type: string
#% key_desc: name
#% gisprompt: old,vector,vector
#% description: Vector map to modify
#% required : yes
#%end
#%option
#% key: layer
#% type: integer
#% description: Layer in the vector to be modified
#% answer: 1
#% required : no
#%end
#%option
#% key: column
#% type: string
#% description: Column to be updated with the query result
#% required : yes
#%end
#%option
#% key: qvector
#% type: string
#% key_desc: name
#% gisprompt: old,vector,vector
#% description: Vector map to be queried
#% required : yes
#%end
#%option
#% key: qlayer
#% type: integer
#% description: Layer of the query vector containing data
#% answer: 1
#% required : no
#%end
#%option
#% key: qcolumn
#% type: string
#% description: Column to be queried
#% required : yes
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
	_from = options['vector'],
	to = options['qvector'],
	column = options['column'],
	to_column = options['qcolumn'],
	upload = "to_attr",
	dmax = options['dmax'],
	from_layer = options['layer'],
	to_layer = options['qlayer'])

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
