#!/usr/bin/env python
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
#% keywords: vector, centroid, area
#%End

#%option
#% key: input
#% type: string
#% gisprompt: old,vector,vector
#% key_desc: name
#% description: Name of input vector map 
#%required: yes
#%end

#%option
#% key: output
#% type: string
#% gisprompt: new,vector,vector
#% key_desc: name
#% description: Name for output vector map
#% required: yes
#%end

#%option
#% key: option
#% type: string
#% description: Action to be taken
#% options: add
#% answer: add
#% required: no
#%end

#%option
#% key: layer
#% type: integer
#% description: Layer number
#% gisprompt: new_layer,layer,layer
#% answer: 1
#% required: no
#%end

#%option
#% key: cat
#% type: integer
#% description: Category number starting value
#% answer: 1
#% required: no
#%end

#%option
#% key: step
#% type: integer
#% description: Category increment
#% answer: 1
#% required: no
#%end

import sys
import os
import re
import grass

def main():
    if options['option'] == 'add':
	num_bound = 0
	tenv = os.environ.copy()
	tenv['LC_ALL'] = 'C'
        num_bound = grass.vector_info_topo(map = options['input'])['boundaries']
	if num_bound == 0:
	    grass.fatal("Input vector map contains no boundaries.")

	grass.exec_command("v.category", type = 'area', **options)

    sys.exit(0)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
