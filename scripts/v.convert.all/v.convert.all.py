#!/usr/bin/env python
#
############################################################################
#
# MODULE:	v.convert.all
# AUTHOR(S):	Markus Neteler, converted to Python by Glynn Clements
# PURPOSE:	converts all old GRASS < V5.7 vector maps to current format
#		in current mapset
# COPYRIGHT:	(C) 2004, 2008 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%Module
#%  description: Convert all old GRASS < Ver5.7 vector maps in current mapset to current format
#%  keywords: vector, import
#%end

import sys
import grass

def main():
    env = grass.gisenv()
    mapset = env['MAPSET']
    converted = 0
    ret = 0
    for vect in grass.list_grouped('oldvect')[mapset]:
	inmap = "%s@%s" % (vect, mapset)
	outmap = vect.replace(".", "_")
	if grass.run_command("v.convert", input = inmap, output = outmap) == 0:
	    converted += 1
	else:
	    grass.warning("Error converting map %s to %s" % (inmap, outmap))
	    ret = 1

	if converted < 1:
	    grass.warning("No vector maps converted as no old vector maps present in current mapset.")
	else:
	    grass.message("Total %u vector maps in current mapset converted." % converted)
	    grass.message("Please verify new vector map(s) before deleting old vector map(s).")

	sys.exit(ret)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
