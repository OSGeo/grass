#!/usr/bin/env python
#
############################################################################
#
# MODULE:	d.shadedmap
# AUTHOR(S):	Unknown; updated to GRASS 5.7 by Michael Barton
#		Converted to Python by Glynn Clements
# PURPOSE:	Uses d.his to drape a color raster over a shaded relief map
# COPYRIGHT:	(C) 2004,2008 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Drapes a color raster over a shaded relief map.
#% keywords: display
#%end
#%option G_OPT_R_INPUT
#% key: reliefmap
#% description: Name of shaded relief or aspect raster map
#%end
#%option G_OPT_R_INPUT
#% key: drapemap
#% description: Name of raster to drape over relief raster map
#%end
#%option
#% key: brighten
#% type: integer
#% description: Percent to brighten
#% options: -99-99
#% answer: 0
#%end

import sys
from grass.script import core as grass

def main():
    drape_map = options['drapemap']
    relief_map = options['reliefmap']
    brighten = options['brighten']
    ret = grass.run_command("d.his", h_map = drape_map,  i_map = relief_map, brighten = brighten)
    sys.exit(ret)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
