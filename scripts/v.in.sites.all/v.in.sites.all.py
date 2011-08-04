#!/usr/bin/env python
############################################################################
#
# MODULE:	v.in.sites.all
# AUTHOR(S):	Markus Neteler, converted to Python by Glynn Clements
# PURPOSE:	converts all old GRASS < V5.7 sites maps to vector maps
#		in current mapset
# COPYRIGHT:	(C) 2004, 2008 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Converts all old GRASS < Ver5.7 sites maps in current mapset to vector maps.
#% keywords: sites
#% keywords: vector
#% keywords: import
#%end

import sys
from grass.script import core as grass

def main():
    env = grass.gisenv()
    mapset = env['MAPSET']
    converted = 0
    ret = 0
    for site in grass.list_grouped('sites')[mapset]:
	inmap = "%s@%s" % (site, mapset)
	outmap = site.replace(".", "_") + "_points"
	grass.message(_("Processing %s -> %s") % (inmap, outmap))
	if grass.run_command("v.in.sites", input = inmap, output = outmap) == 0:
	    converted += 1
	else:
	    grass.warning(_("Error converting map %s to %s") % (inmap, outmap))
	    ret = 1

	if converted < 1:
	    grass.warning(_("No sites maps converted as no old sites maps present in current mapset."))
	else:
	    grass.message(_("Total %u sites maps in current mapset converted to vector maps (original names extended by '_points')") % converted)
	    grass.message(_("Please verify new vector map(s) before deleting old sites map(s)."))

	sys.exit(ret)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
