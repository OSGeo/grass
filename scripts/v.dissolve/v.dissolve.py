#!/usr/bin/env python
############################################################################
#
# MODULE:       v.dissolve
# AUTHOR:       M. Hamish Bowman, Dept. Marine Science, Otago Univeristy,
#                 New Zealand
#               Markus Neteler for column support
#               Converted to Python by Glynn Clements
# PURPOSE:      Dissolve common boundaries between areas with common cat
#                 (frontend to v.extract -d)
# COPYRIGHT:    (c) 2006 Hamish Bowman, and the GRASS Development Team
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%Module
#% description: Dissolves boundaries between adjacent areas sharing a common category number or attribute.
#% keywords: vector, area, dissolve
#%End
#%option
#% key: input
#% type: string
#% gisprompt: old,vector,vector
#% key_desc: name
#% description: Name of input vector map 
#% required: yes
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
#% key: layer
#% type: integer
#% label: Layer number. If -1, all layers are extracted.
#% description: A single vector map can be connected to multiple database tables. This number determines which table to use.
#% answer: 1
#% gisprompt: old_layer,layer,layer_all
#% required : no
#%end
#%option
#% key: column
#% type: string
#% description: Name of column used to dissolve common boundaries
#% gisprompt: old_dbcolumn,dbcolumn,dbcolumn
#% required : no
#%end

import sys
import os
import grass
import atexit

def cleanup():
    nuldev = file(os.devnull, 'w')
    grass.run_command('g.remove', vect = '%s_%s' % (output, tmp), quiet = True, stderr = nuldev)

def main():
    global output, tmp

    input = options['input']
    output = options['output']
    layer = options['layer']
    column = options['column']

    #### setup temporary file
    tmp = str(os.getpid())

    # does map exist?
    if not grass.find_file(input, element = 'vector')['file']:
	grass.fatal("Vector map <%s> not found in mapset search path", input)

    if not column:
	grass.run_command('v.extract', flags = 'd', input = input,
			  output = output, type = 'area', layer = layer)
    else:
	coltype = ''
	for f in grass.vector_columns(map, layer):
	    if f[1] == column:
		coltype = f[0]

	if coltype not in ['INTEGER', 'CHARACTER']:
	    grass.fatal("Key column must be of type integer or string")

	f = grass.vector_db(input, layer)
	if not f:
	    grass.fatal("There is no table connected to this map")

	table = f[1]

	tmpfile = '%s_%s' % (output, tmp)

	grass.run_command('v.reclass', input = input, output = tmpfile,
			  layer = layer, column = column)

	grass.run_command('v.extract', flags = 'd', input = tmpfile,
			  output = output, type = 'area', layer = layer)

    # write cmd history:
    grass.vector_history(output)

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
