#!/usr/bin/env python
############################################################################
#
# MODULE:	v.build.all
# AUTHOR(S):	Glynn Clements, Radim Blazek
# PURPOSE:	Build all vectors in current mapset
# COPYRIGHT:	(C) 2004,2008 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%Module
#% description: Rebuilds topology on all vector maps in the current mapset.
#% keywords: vector
#%End

import sys
import grass

def main():
    env = grass.gisenv()
    mapset = env['MAPSET']
    ret = 0
    for vect in grass.list_grouped('vect')[mapset]:
	map = "%s@%s" % (vect, mapset)
	grass.message("Build topology for vector '%s'" % map)
	grass.message("v.build map=%s" % map)
	if grass.run_command("v.build", map = map) != 0:
	    ret = 1
    sys.exit(ret)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
