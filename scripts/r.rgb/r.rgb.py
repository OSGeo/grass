#!/usr/bin/env python

############################################################################
#
# MODULE:       r.rgb
# AUTHOR(S):	Glynn Clements
# PURPOSE:	Split a raster map into red, green and blue maps
# COPYRIGHT:	(C) 2009 Glynn Clements and the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Splits a raster map into red, green and blue maps.
#% keywords: raster
#% keywords: RGB
#%end
#%option G_OPT_R_INPUT
#%end
#%option
#% key: output_prefix
#% type: string
#% description: Prefix for output raster maps (default: input)
#% required: no
#%end

import sys
import os
import string
import grass.script as grass

def main():
    input = options['input']
    output = options['output_prefix']

    if not grass.find_file(input)['file']:
	grass.fatal(_("Raster map <%s> not found") % input)

    if not output:
	output = input

    expr = ';'.join(["${output}.r = r#${input}",
		     "${output}.g = g#${input}",
		     "${output}.b = b#${input}"])
    grass.mapcalc(expr, input = input, output = output)

    for ch in ['r', 'g', 'b']:
	name = "%s.%s" % (output, ch)
	grass.run_command('r.colors', map = name, color = 'grey255')
	grass.raster_history(name)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
