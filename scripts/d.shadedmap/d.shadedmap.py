#!/usr/bin/env python
#
############################################################################
#
# MODULE:        d.shadedmap
# AUTHOR(S):        Unknown; updated to GRASS 5.7 by Michael Barton
#                Converted to Python by Glynn Clements
# PURPOSE:        Uses d.his to drape a color raster over a shaded relief map
# COPYRIGHT:     (C) 2004-2013 by the GRASS Development Team
#
#                This program is free software under the GNU General Public
#                License (>=v2). Read the file COPYING that comes with GRASS
#                for details.
#
#############################################################################

#%module
#% description: Drapes a color raster over a shaded relief map.
#% keywords: display
#% keywords: elevation
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
#%option G_OPT_R_OUTPUT
#% description: Create raster map from result (optional)
#% required: no
#%end


import os
import sys
from grass.script import core as grass

def main():
    drape_map = options['drapemap']
    relief_map = options['reliefmap']
    brighten = options['brighten']
    output_map = options['output']

    if output_map:
        tmp_base = "tmp_drape.%d" % os.getpid()
        tmp_r = tmp_base + '.r'
        tmp_g = tmp_base + '.g'
        tmp_b = tmp_base + '.b'

        grass.run_command('r.his', h_map = drape_map, i_map = relief_map,
                          r_map = tmp_r, g_map = tmp_g, b_map = tmp_b)
        grass.run_command('r.composite', red = tmp_r, green = tmp_g,
                          blue = tmp_b, output = output_map)
        grass.run_command('g.remove', quiet = True,
                          rast = '%s,%s,%s' % (tmp_r, tmp_g, tmp_b))


    ret = grass.run_command("d.his", h_map = drape_map, i_map = relief_map,
                            brighten = brighten)

    sys.exit(ret)


if __name__ == "__main__":
    options, flags = grass.parser()
    main()
