#!/usr/bin/env python

############################################################################
#
# MODULE:       r.reclass.area
# AUTHOR(S):    NRCS
#               Converted to Python by Glynn Clements
# PURPOSE:      Reclasses a raster map greater or less than user specified area size (in hectares)
# COPYRIGHT:    (C) 1999,2008 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################
# 3/2007: added label support MN
# 3/2004: added parser support MN
# 11/2001 added mapset support markus
# 2/2001 fixes markus
# 2000: updated to GRASS 5
# 1998 from NRCS, slightly modified for GRASS 4.2.1

#%Module
#%  description: Reclasses a raster map greater or less than user specified area size (in hectares).
#%  keywords: raster
#%  keywords: statistics
#%  keywords: aggregation
#%End

#%option
#% key: input
#% type: string
#% gisprompt: old,cell,raster
#% description: Name of input raster map
#% required : yes
#%END

#%option
#% key: output
#% type: string
#% gisprompt: new,cell,raster
#% description: Name for output raster map
#% required : yes
#%END

#%option
#% key: lesser
#% type: double
#% description: Lesser value option that sets the <= area size limit [hectares]
#%END
#%option
#% key: greater
#% type: double
#% description: Greater value option that sets the >= area size limit [hectares]
#%END

import sys
import os
import grass.script as grass

def main():
    infile = options['input']
    lesser = options['lesser']
    greater = options['greater']
    outfile = options['output']

    s = grass.read_command("g.region", flags = 'p')
    kv = grass.parse_key_val(s, sep = ':')
    s = kv['projection'].strip().split()
    if s == '0':
	grass.fatal(_("xy-locations are not supported"))
	grass.fatal(_("Need projected data with grids in meters"))

    if not lesser and not greater:
	grass.fatal(_("You have to specify either lesser= or greater="))
    if lesser and greater:
	grass.fatal(_("lesser= and greater= are mutually exclusive"))
    if lesser:
	limit = float(lesser)
    if greater:
	limit = float(greater)

    if not grass.find_file(infile)['name']:
	grass.fatal(_("Raster map <%s> not found") % infile)

    clumpfile = "%s.clump.%s" % (infile.split('@')[0], outfile)

    if not grass.overwrite():
	if grass.find_file(clumpfile)['name']:
	    grass.fatal(_("Temporary raster map <%s> exists") % clumpfile)

    grass.message(_("Generating a clumped raster file ..."))
    grass.run_command('r.clump', input = infile, output = clumpfile)

    if lesser:
	grass.message(_("Generating a reclass map with area size less than or equal to %f hectares...") % limit)
    else:
	grass.message(_("Generating a reclass map with area size greater than or equal to %f hectares...") % limit)

    recfile = outfile + '.recl'

    p1 = grass.pipe_command('r.stats', flags = 'aln', input = (clumpfile, infile), fs = '|')
    p2 = grass.feed_command('r.reclass', input = clumpfile, output = recfile, rules = '-')
    for line in p1.stdout:
	f = line.rstrip('\r\n').split('|')
	if len(f) < 5:
	    continue
	hectares = float(f[4]) * 0.0001
	if lesser:
	    test = hectares <= limit
	else:
	    test = hectares >= limit
	if test:
	    p2.stdin.write("%s = %s %s\n" % (f[0], f[2], f[3]))
    p1.wait()
    p2.stdin.close()
    p2.wait()

    grass.message(_("Generating output raster map <%s>...") % outfile)

    grass.mapcalc("$outfile = $recfile", outfile = outfile, recfile = recfile)
    grass.run_command('g.remove', rast = [recfile, clumpfile], quiet = True)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
