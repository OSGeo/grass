#!/usr/bin/env python3
#
############################################################################
#
# MODULE:       d.correlate for GRASS 6; based on dcorrelate.sh for GRASS 4,5
# AUTHOR(S):    CERL - Michael Shapiro; updated to GRASS 6 by Markus Neteler 5/2005
#               Converted to Python by Glynn Clements
# PURPOSE:      prints a graph of the correlation between data layers (in pairs)
#               derived from <grass5>/src.local/d.correlate.sh
# COPYRIGHT:    (C) 2005, 2008, 2011 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Prints a graph of the correlation between raster maps (in pairs).
#% keyword: display
#% keyword: statistics
#% keyword: raster
#% keyword: diagram
#% keyword: correlation
#%end
#%option G_OPT_R_MAPS
#%end

import sys
import os
from grass.script.utils import try_remove
from grass.script import core as gcore
from grass.exceptions import CalledModuleError


def main():
    layers = options['map'].split(',')

    if len(layers) < 2:
        gcore.error(_("At least 2 maps are required"))

    tmpfile = gcore.tempfile()

    for map in layers:
        if not gcore.find_file(map, element='cell')['file']:
            gcore.fatal(_("Raster map <%s> not found") % map)

    try:
        gcore.write_command('d.text', color='black', size=4, line=1,
                            stdin="CORRELATION")
    except CalledModuleError:
        return 1
    
    os.environ['GRASS_RENDER_FILE_READ'] = 'TRUE'

    colors = "red black blue green gray violet".split()
    line = 2
    iloop = 0
    jloop = 0
    for iloop, i in enumerate(layers):
        for jloop, j in enumerate(layers):
            if i != j and iloop <= jloop:
                color = colors[0]
                colors = colors[1:]
                colors.append(color)
                gcore.write_command('d.text', color=color, size=4, line=line,
                                    stdin="%s %s" % (i, j))
                line += 1

                ofile = open(tmpfile, 'w')
                gcore.run_command('r.stats', flags='cnA', input=(i, j),
                                  stdout=ofile)
                ofile.close()

                ifile = open(tmpfile, 'r')
                first = True
                for l in ifile:
                    f = l.rstrip('\r\n').split(' ')
                    x = float(f[0])
                    y = float(f[1])
                    if first:
                        minx = maxx = x
                        miny = maxy = y
                        first = False
                    if minx > x:
                        minx = x
                    if maxx < x:
                        maxx = x
                    if miny > y:
                        miny = y
                    if maxy < y:
                        maxy = y
                ifile.close()

                kx = 100.0 / (maxx - minx + 1)
                ky = 100.0 / (maxy - miny + 1)

                p = gcore.feed_command('d.graph', color=color)
                ofile = p.stdin

                ifile = open(tmpfile, 'r')
                for l in ifile:
                    f = l.rstrip('\r\n').split(' ')
                    x = float(f[0])
                    y = float(f[1])
                    ofile.write(b"icon + 0.1 %f %f\n" % ((x - minx + 1) * kx,
                                                        (y - miny + 1) * ky))
                ifile.close()

                ofile.close()
                p.wait()

    try_remove(tmpfile)

    return 0

if __name__ == "__main__":
    options, flags = gcore.parser()
    sys.exit(main())
