#!/usr/bin/env python3
#
############################################################################
#
# MODULE:       v.what.vect
# AUTHOR(S):    Markus Neteler, converted to Python by Glynn Clements
# PURPOSE:      Uploads attributes at the location of vector points to the table.
# COPYRIGHT:    (C) 2005, 2008, 2011 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Uploads vector values at positions of vector points to the table.
#% keyword: vector
#% keyword: sampling
#% keyword: database
#% keyword: position
#% keyword: querying
#% keyword: attribute table
#%end

#%option G_OPT_V_MAP
#% label: Name of vector points map for which to edit attributes
#% guidependency: layer,column
#%end
#%option G_OPT_V_FIELD
#% guidependency: column
#%end
#%option G_OPT_DB_COLUMN
#% description: Name of attribute column to be updated with the query result
#% required: yes
#%end
#%option G_OPT_V_MAP
#% key: query_map
#% label: Name of vector map to be queried
#% required : yes
#% guidependency: query_layer,query_column
#%end
#%option G_OPT_V_FIELD
#% key: query_layer
#% guidependency: query_column
#%end
#%option G_OPT_DB_COLUMN
#% key: query_column
#% description: Name of attribute column to be queried
#% required: yes
#%end
#%option
#% key: dmax
#% type: double
#% description: Maximum query distance in map units (meters for ll)
#% answer: 0.0
#% required: no
#%end

import sys
from grass.script import core as grass
from grass.exceptions import CalledModuleError


def main():
    try:
        grass.run_command('v.distance',
                          from_=options['map'],
                          to=options['query_map'],
                          column=options['column'],
                          to_column=options['query_column'],
                          upload='to_attr',
                          dmax=options['dmax'],
                          from_layer=options['layer'],
                          to_layer=options['query_layer'])
    except CalledModuleError:
        return 1
    else:
        return 0


if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
