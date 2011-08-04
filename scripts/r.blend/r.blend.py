#!/usr/bin/env python

############################################################################
#
# MODULE:	r.blend for GRASS 5.7; based on blend.sh for GRASS 5
# AUTHOR(S):	CERL?; updated to GRASS 5.7 by Michael Barton
#               Converted to Python by Glynn Clements
# PURPOSE:	To redraw current displayed maps to 24 bit PNG output
# COPYRIGHT:	(C) 2004 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Blends color components of two raster maps by a given ratio.
#% keywords: raster
#%end
#%option G_OPT_R_INPUT
#% key: first
#% description: Name of first raster map for blending
#%end
#%option G_OPT_R_INPUT
#% key: second
#% description: Name of second raster map for blending
#%end
#%option 
#% key: output_prefix
#% type: string
#% description: Prefix for red, green and blue output raster maps containing
#% key_desc : name
#% required : yes
#%end
#%option
#% key: percent
#% type: integer
#% answer: 50
#% options : 1-99
#% description: Percentage weight of first map for color blending
#% required : no
#%end

import sys
import os
import string
import grass.script as grass

def main():
    first = options['first']
    second = options['second']
    output = options['output']
    percent = options['percent']

    mapset = grass.gisenv()['MAPSET']

    if not grass.overwrite():
	for ch in ['r','g','b']:
	    map = '%s.%s' % (output, ch)
	    if grass.find_file(map, element = 'cell', mapset = mapset)['file']:
		grass.fatal(_("Raster map <%s> already exists.") % map)

    percent = int(percent)
    perc_inv = 100 - percent

    frac1 = percent / 100.0
    frac2 = perc_inv / 100.0

    grass.message(_("Calculating the three component maps..."))

    template = string.Template("$$output.$ch = $$frac1 * $ch#$$first + $$frac2 * $ch#$$second")
    cmd = [template.substitute(ch = ch) for ch in ['r','g','b']]
    cmd = ';'.join(cmd)

    grass.mapcalc(cmd,
		  output = output,
		  first = first, second = second,
		  frac1 = frac1, frac2 = frac2)

    for ch in ['r','g','b']:
	map = "%s.%s" % (output, ch)
	grass.run_command('r.colors', map = map, color = 'grey255')
	grass.run_command('r.support', map = map,
			  title = "Color blend of %s and %s" % (first, second), history="")
	grass.run_command('r.support', map = map,
			  history = "r.blend %s channel." % ch)
	grass.run_command('r.support', map = map,
			  history = "  %d%% of %s, %d%% of %s" % (percent, first, perc_inv, second))
	grass.run_command('r.support', map = map, history = os.environ['CMDLINE'])

    grass.message(_("Done. Use the following command to visualize the result:"))
    grass.message(_("d.rgb r=%s.r g=%s.g b=%s.b") % (output, output, output))

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
