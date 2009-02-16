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

#% Module
#%  description: Split a raster map into red, green and blue maps
#%  keywords: raster
#% End
#% option
#%  key: input
#%  type: string
#%  gisprompt: old,cell,raster
#%  description: Input map
#%  required : yes
#% end
#% option
#%  key: output
#%  type: string
#%  description: Base name for output maps
#%  required : no
#% end

import sys
import os
import string
import grass

def main():
    input = options['input']
    output = options['output']

    if not grass.find_file(input)['file']:
	grass.fatal("Map <%s> not found." % input)

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
