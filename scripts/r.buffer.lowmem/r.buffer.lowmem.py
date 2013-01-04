#!/usr/bin/env python
#
############################################################################
#
# MODULE:	r.buffer.lowmem
# AUTHOR(S):	Glynn Clements
# PURPOSE:	Low-memory replacement for r.buffer using r.grow.distance
#
# COPYRIGHT:	(C) 2008, 2010 by Glynn Clements
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% label: Creates a raster map showing buffer zones surrounding cells that contain non-NULL category values.
#% description: This is the low-memory alternative to the classic r.buffer module.
#% keywords: raster
#% keywords: buffer
#%end
#%flag
#% key: z
#% description: Ignore zero (0) data cells instead of NULL cells
#%end
#%option G_OPT_R_INPUT
#%end
#%option G_OPT_R_OUTPUT
#%end
#%option
#% key: distances
#% type: double
#% required: yes
#% multiple: yes
#% description: Distance zone(s)
#%end
#%option G_OPT_M_UNITS
#% options: meters,kilometers,feet,miles,nautmiles
#% description: Units of distance
#% answer: meters
#%end

import sys
import os
import atexit
import math
import grass.script as grass

scales = {
    'meters': 1.0,
    'kilometers': 1000.0,
    'feet': 0.3048,
    'miles': 1609.344,
    'nautmiles': 1852.0
    }

# what to do in case of user break:
def cleanup():
    if grass.find_file(temp_src)['file']:
        grass.run_command('g.remove', quiet = True, flags = 'f', rast = temp_src)
    if grass.find_file(temp_dist)['file']:
        grass.run_command('g.remove', quiet = True, flags = 'f', rast = temp_dist)

def main():
    global temp_dist, temp_src

    input = options['input']
    output = options['output']
    distances = options['distances']
    units = options['units']
    zero = flags['z']

    tmp = str(os.getpid())
    temp_dist = "r.buffer.tmp.%s.dist" % tmp
    temp_src = "r.buffer.tmp.%s.src" % tmp

    #check if input file exists
    if not grass.find_file(input)['file']:
	grass.fatal(_("<%s> does not exist.") % input)

    scale = scales[units]

    distances  = distances.split(',')
    distances1 = [scale * float(d) for d in distances]
    distances2 = [d * d for d in distances1]

    s = grass.read_command("g.proj", flags='j')
    kv = grass.parse_key_val(s)
    if kv['+proj'] == 'longlat':
	metric = 'geodesic'
    else:
	metric = 'squared'

    grass.run_command('r.grow.distance',  input = input, metric = metric,
		      distance = temp_dist, flags = 'm')

    if zero:
	exp = "$temp_src = if($input == 0,null(),1)"
    else:
	exp = "$temp_src = if(isnull($input),null(),1)"

    grass.message(_("Extracting buffers (1/2)..."))
    grass.mapcalc(exp, temp_src = temp_src, input = input)

    exp = "$output = if(!isnull($input),$input,%s)"
    if metric == 'squared':
	for n, dist2 in enumerate(distances2):
	    exp %= "if($dist <= %f,%d,%%s)" % (dist2,n + 2)
    else:
	for n, dist2 in enumerate(distances1):
	    exp %= "if($dist <= %f,%d,%%s)" % (dist2,n + 2)
    exp %= "null()"

    grass.message(_("Extracting buffers (2/2)..."))
    grass.mapcalc(exp, output = output, input = temp_src, dist = temp_dist)

    p = grass.feed_command('r.category', map = output, rules = '-')
    p.stdin.write("1:distances calculated from these locations\n")
    d0 = "0"
    for n, d in enumerate(distances):
	p.stdin.write("%d:%s-%s %s\n" % (n + 2, d0, d, units))
	d0 = d
    p.stdin.close()
    p.wait()

    grass.run_command('r.colors', map = output, color = 'rainbow')

    # write cmd history:
    grass.raster_history(output)
    
if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
