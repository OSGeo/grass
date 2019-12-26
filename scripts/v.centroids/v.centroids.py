#!/usr/bin/env python3
############################################################################
#
# MODULE:       v.centroids
# AUTHOR:       Hamish Bowman
# PURPOSE:      Add missing centroids  (frontend to v.category opt=add)
# COPYRIGHT:    (c) 2006 Hamish Bowman, and the GRASS Development Team
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%Module
#% description: Adds missing centroids to closed boundaries.
#% keyword: vector
#% keyword: centroid
#% keyword: area
#%End

#%option G_OPT_V_INPUT
#%end

#%option G_OPT_V_OUTPUT
#%end

#%option
#% key: option
#% type: string
#% description: Action to be taken
#% options: add
#% answer: add
#% required: no
#%end

#%option G_OPT_V_FIELD
#%end

#%option G_OPT_V_CAT
#% description: Category number starting value
#% answer: 1
#%end

#%option
#% key: step
#% type: integer
#% description: Category increment
#% answer: 1
#% required: no
#%end

import sys
import grass.script as gscript


def main():
    if options['option'] == 'add':
        num_bound = gscript.vector_info_topo(map=options['input'])['boundaries']
        if num_bound == 0:
            gscript.fatal(_("Input vector map contains no boundaries."))

        gscript.run_command("v.category", type='area', **options)

    sys.exit(0)

if __name__ == "__main__":
    options, flags = gscript.parser()
    main()
