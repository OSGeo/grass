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
# 8/2012: added fp maps support, cleanup, removed tabs AK
# 3/2007: added label support MN
# 3/2004: added parser support MN
# 11/2001 added mapset support markus
# 2/2001 fixes markus
# 2000: updated to GRASS 5
# 1998 from NRCS, slightly modified for GRASS 4.2.1

#%module
#% description: Reclasses a raster map greater or less than user specified area size (in hectares).
#% keywords: raster
#% keywords: statistics
#% keywords: aggregation
#%end

#%option G_OPT_R_INPUT
#%end

#%option G_OPT_R_OUTPUT
#%end

#%option
#% key: lesser
#% type: double
#% description: Lesser value option that sets the <= area size limit [hectares]
#% guisection: Area
#%end

#%option
#% key: greater
#% type: double
#% description: Greater value option that sets the >= area size limit [hectares]
#% guisection: Area
#%end

import sys
import os
import atexit
import grass.script as grass

TMPRAST = []

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
    TMPRAST.append(clumpfile)

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
    TMPRAST.append(recfile)

    flags = 'aln'
    if grass.raster_info(infile)['datatype'] in ('FCELL', 'DCELL'):
        flags += 'i'
    p1 = grass.pipe_command('r.stats', flags = flags, input = (clumpfile, infile), sep = ';')
    p2 = grass.feed_command('r.reclass', input = clumpfile, output = recfile, rules = '-')
    rules = ''
    for line in p1.stdout:
        f = line.rstrip(os.linesep).split(';')
        if len(f) < 5:
            continue
        hectares = float(f[4]) * 0.0001
        if lesser:
            test = hectares <= limit
        else:
            test = hectares >= limit
        if test:
            rules += "%s = %s %s\n" % (f[0], f[2], f[3])
    if rules:
        p2.stdin.write(rules)
    p1.wait()
    p2.stdin.close()
    p2.wait()
    if p2.returncode != 0:
        if lesser:
            grass.fatal(_("No areas of size less than or equal to %f hectares found.") % limit)
        else:
            grass.fatal(_("No areas of size greater than or equal to %f hectares found.") % limit)

    grass.message(_("Generating output raster map <%s>...") % outfile)

    grass.mapcalc("$outfile = $recfile", outfile = outfile, recfile = recfile)

def cleanup():
    """!Delete temporary maps"""
    TMPRAST.reverse() # reclassed map first
    for rast in TMPRAST:
        grass.run_command("g.remove",
                          rast = rast,
                          quiet = True)

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    sys.exit(main())
