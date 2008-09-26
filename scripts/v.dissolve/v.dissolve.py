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
#% description: Layer number. If -1, all layers are extracted
#% answer: 1
#% required : no
#%end
#%option
#% key: column
#% type: string
#% description: Name of column used to dissolve common boundaries
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

    # save command line
    cmdline = os.path.basename(sys.argv[0])
    cmdline += ' input=' + input
    cmdline += ' output=' + output
    if layer:
	cmdline += ' layer=' + layer
    if column:
	cmdline += ' column=' + column

    #### setup temporary file
    tmp = str(os.getpid())

    # does map exist?
    if not grass.find_file(input, element = 'vector')['file']:
	grass.fatal("Vector map '$GIS_OPT_INPUT' not found in mapset search path")

    if not column:
	grass.run_command('v.extract', flags = 'd', input = input,
			  output = output, type = 'area', layer = layer)
    else:
	coltype = ''
	s = grass.read_command('v.info', flags = 'c', map = input, quiet = True)
	for line in s.splitlines():
	    f = line.split('|')
	    if len(f) > 1 and f[1] == column:
		coltype = f[0]

	if coltype not in ['INTEGER', 'CHARACTER']:
	    grass.fatal("Key column must be of type integer or string")

	s = grass.read_command('v.db.connect', flags = 'g', map = input, layer = layer)
	table = s.split()[1]
	if not table:
	    grass.fatal("There is no table connected to this map")

	tmpfile = '%s_%s' % (output, tmp)

	grass.run_command('v.reclass', input = input, output = tmpfile,
			  layer = layer, column = column)

	grass.run_command('v.extract', flags = 'd', input = tmpfile,
			  output = output, type = 'area', layer = layer)

    # write cmd history:
    grass.run_command('v.support', map = output, cmdhist = cmdline)

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
