#!/usr/bin/env python3
#
############################################################################
#
# MODULE:    r.grow
# AUTHOR(S): Glynn Clements
# PURPOSE:   Fast replacement for r.grow using r.grow.distance
#
# COPYRIGHT: (C) 2008 by Glynn Clements
#
#   This program is free software under the GNU General Public
#   License (>=v2). Read the file COPYING that comes with GRASS
#   for details.
#
#############################################################################

#%Module
#% description: Generates a raster map layer with contiguous areas grown by one cell.
#% keyword: raster
#% keyword: distance
#% keyword: proximity
#%end
#%flag
#% key: m
#% description: Radius is in map units rather than cells
#%end
#%option G_OPT_R_INPUT
#%end
#%option G_OPT_R_OUTPUT
#%end
#%option
#% key: radius
#% type: double
#% required: no
#% multiple: no
#% description: Radius of buffer in raster cells
#% answer: 1.01
#%end
#%option
#% key: metric
#% type: string
#% required: no
#% multiple: no
#% options: euclidean,maximum,manhattan
#% description: Metric
#% answer: euclidean
#%end
#%option
#% key: old
#% type: integer
#% required: no
#% multiple: no
#% description: Value to write for input cells which are non-NULL (-1 => NULL)
#%end
#%option
#% key: new
#% type: integer
#% required: no
#% multiple: no
#% description: Value to write for "grown" cells
#%end

import os
import atexit
import math

import grass.script as grass
from grass.exceptions import CalledModuleError


# what to do in case of user break:
def cleanup():
    for map in [temp_dist, temp_val]:
        if map:
            grass.run_command('g.remove', flags='fb', quiet=True,
                              type='rast', name=map)


def main():
    global temp_dist, temp_val

    input = options['input']
    radius = float(options['radius'])
    metric = options['metric']
    old = options['old']
    new = options['new']
    mapunits = flags['m']

    tmp = str(os.getpid())

    temp_dist = "r.grow.tmp.%s.dist" % tmp

    shrink = False
    if radius < 0.0:
        shrink = True
        radius = -radius

    if new == '' and shrink == False:
        temp_val = "r.grow.tmp.%s.val" % tmp
        new = '"%s"' % temp_val
    else:
        temp_val = None

    if old == '':
        old = '"%s"' % input

    if not mapunits:
        kv = grass.region()
        scale = math.sqrt(float(kv['nsres']) * float(kv['ewres']))
        radius *= scale

    if metric == 'euclidean':
        metric = 'squared'
        radius = radius * radius

    # check if input file exists
    if not grass.find_file(input)['file']:
        grass.fatal(_("Raster map <%s> not found") % input)

    # Workaround for r.mapcalc bug #3475
    # Mapcalc will fail if output is a fully qualified map name
    out_name = options['output'].split('@')
    if len(out_name) == 2:
        if out_name[1] != grass.gisenv()['MAPSET']:
            grass.fatal(_("Output can be written only to the current mapset"))
        output = out_name[0]
    else:
        output = out_name[0]

    if shrink is False:
        try:
            grass.run_command('r.grow.distance', input=input, metric=metric,
                              distance=temp_dist, value=temp_val)
        except CalledModuleError:
            grass.fatal(_("Growing failed. Removing temporary maps."))

        grass.mapcalc(
            '$output = if(!isnull("$input"),$old,if($dist < $radius,$new,null()))',
            output=output, input=input, radius=radius,
            old=old, new=new, dist=temp_dist)
    else:
        # shrink
        try:
            grass.run_command('r.grow.distance', input=input, metric=metric,
                              distance=temp_dist, value=temp_val, flags='n')
        except CalledModuleError:
            grass.fatal(_("Shrinking failed. Removing temporary maps."))

        grass.mapcalc(
            "$output = if(isnull($dist), $old, if($dist < $radius,null(),$old))",
            output=output, radius=radius, old=old, dist=temp_dist)

    grass.run_command('r.colors', map=output, raster=input)

    # write cmd history:
    grass.raster_history(output)


if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
