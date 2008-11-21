#!/usr/bin/env python
#
############################################################################
#
# MODULE:	r.grow
# AUTHOR(S):	Glynn Clements
# PURPOSE:	Fast replacement for r.grow using r.grow.distance
#
# COPYRIGHT:	(C) 2008 by Glynn Clements
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%Module
#% description: Generates a raster map layer with contiguous areas grown by one cell.
#% keywords: raster
#%End
#%Flag
#% key: m
#% description: radius is in map units rather than cells
#%End
#%Option
#% key: input
#% type: string
#% required: yes
#% multiple: no
#% key_desc: name
#% description: Name of input raster map
#% gisprompt: old,cell,raster
#%End
#%Option
#% key: output
#% type: string
#% required: yes
#% multiple: no
#% key_desc: name
#% description: Name for output raster map
#% gisprompt: new,cell,raster
#%End
#%Option
#% key: radius
#% type: double
#% required: no
#% multiple: no
#% description: Radius of buffer in raster cells
#% answer: 1.01
#%End
#%Option
#% key: metric
#% type: string
#% required: no
#% multiple: no
#% options: euclidean,maximum,manhattan
#% description: Metric
#% answer: euclidean
#%End
#%Option
#% key: old
#% type: integer
#% required: no
#% multiple: no
#% description: Value to write for input cells which are non-NULL (-1 => NULL)
#%End
#%Option
#% key: new
#% type: integer
#% required: no
#% multiple: no
#% description: Value to write for "grown" cells
#%End

import sys
import os
import atexit
import math
import grass

# what to do in case of user break:
def cleanup():
    for map in [temp_dist, temp_val]:
	if map:
	    grass.run_command('g.remove', quiet = True, flags = 'f', rast = map)

def main():
    global temp_dist, temp_val

    input = options['input']
    output = options['output']
    radius = float(options['radius'])
    metric = options['metric']
    old = options['old']
    new = options['new']
    mapunits = flags['m']

    tmp = str(os.getpid())

    temp_dist = "r.grow.tmp.%s.dist" % tmp

    if new == '':
	temp_val = "r.grow.tmp.%s.val" % tmp
	new = temp_val
    else:
	temp_val = None

    if old == '':
	old = input

    if not mapunits:
	kv = grass.region()
	scale = math.sqrt(float(kv['nsres']) * float(kv['ewres']))
	radius *= scale

    if metric == 'euclidean':
	metric = 'squared'
	radius = radius * radius

    #check if input file exists
    if not grass.find_file(input)['file']:
	grass.fatal("<%s> does not exist." % input)

    grass.run_command('r.grow.distance',  input = input, metric = metric,
		      distance = temp_dist, value = temp_val)

    grass.mapcalc(
	"$output = if(!isnull($input),$old,if($dist < $radius,$new,null()))",
	output = output, input = input, radius = radius,
	old = old, new = new, dist = temp_dist)

    grass.run_command('r.colors', map = output, raster = input)

    # write cmd history:
    grass.raster_history(output)

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
