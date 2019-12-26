#!/usr/bin/env python3
############################################################################
#
# MODULE:       r.colors.stddev
# AUTHOR:       M. Hamish Bowman, Dept. Marine Science, Otago University,
#                 New Zealand
#               Converted to Python by Glynn Clements
# PURPOSE:      Set color rules based on stddev from a map's mean value.
#
# COPYRIGHT:    (c) 2007,2009-2010 Hamish Bowman, and the GRASS Development Team
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Sets color rules based on stddev from a raster map's mean value.
#% keyword: raster
#% keyword: color table
#%end
#% option G_OPT_R_MAP
#%end
#%flag
#% key: b
#% description: Color using standard deviation bands
#%end
#%flag
#% key: z
#% description: Force center at zero
#%end

import os
import atexit

import grass.script as gscript
from grass.script.utils import decode


def z(n):
    return mean + n * stddev


def cleanup():
    if tmpmap:
        gscript.run_command('g.remove', flags='f', type='raster',
                            name=tmpmap, quiet=True)


def main():
    global tmpmap
    tmpmap = None

    map = options['map']
    zero = flags['z']
    bands = flags['b']

    if not zero:
        s = gscript.read_command('r.univar', flags='g', map=map)
        kv = gscript.parse_key_val(decode(s))
        global mean, stddev
        mean = float(kv['mean'])
        stddev = float(kv['stddev'])

        if not bands:
            # smooth free floating blue/white/red
            rules = '\n'.join(["0% blue",
                               "%f blue" % z(-2),
                               "%f white" % mean,
                               "%f red" % z(+2),
                               "100% red"])
        else:
            # banded free floating  black/red/yellow/green/yellow/red/black

            # reclass with labels only works for category (integer) based maps
            # r.reclass input="$GIS_OPT_MAP" output="${GIS_OPT_MAP}.stdevs" <<
            # EOF

            # >3 S.D. outliers colored black so they show up in d.histogram w/ white background
            rules = '\n'.join(["0% black",
                               "%f black" % z(-3),
                               "%f red" % z(-3),
                               "%f red" % z(-2),
                               "%f yellow" % z(-2),
                               "%f yellow" % z(-1),
                               "%f green" % z(-1),
                               "%f green" % z(+1),
                               "%f yellow" % z(+1),
                               "%f yellow" % z(+2),
                               "%f red" % z(+2),
                               "%f red" % z(+3),
                               "%f black" % z(+3),
                               "100% black"])
    else:
        tmpmap = "r_col_stdev_abs_%d" % os.getpid()
        gscript.mapcalc("$tmp = abs($map)", tmp=tmpmap, map=map)

        # data centered on 0  (e.g. map of deviations)
        info = gscript.raster_info(tmpmap)
        maxv = info['max']

        # current r.univar truncates percentage to the base integer
        s = gscript.read_command('r.univar', flags='eg', map=map,
                                 percentile=[95.45,
                                             68.2689,
                                             99.7300])
        kv = gscript.parse_key_val(decode(s))

        stddev1 = float(kv['percentile_68_2689'])
        stddev2 = float(kv['percentile_95_45'])
        stddev3 = float(kv['percentile_99_73'])

        if not bands:
            # zero centered smooth blue/white/red
            rules = '\n'.join(["%f blue" % -maxv,
                               "%f blue" % -stddev2,
                               "0 white",
                               "%f red" % stddev2,
                               "%f red" % maxv])
        else:
            # zero centered banded  black/red/yellow/green/yellow/red/black

            # >3 S.D. outliers colored black so they show up in d.histogram w/ white background
            rules = '\n'.join(["%f black" % -maxv,
                               "%f black" % -stddev3,
                               "%f red" % -stddev3,
                               "%f red" % -stddev2,
                               "%f yellow" % -stddev2,
                               "%f yellow" % -stddev1,
                               "%f green" % -stddev1,
                               "%f green" % stddev1,
                               "%f yellow" % stddev1,
                               "%f yellow" % stddev2,
                               "%f red" % stddev2,
                               "%f red" % stddev3,
                               "%f black" % stddev3,
                               "%f black" % maxv, ])

    gscript.write_command('r.colors', map=map, rules='-', stdin=rules)

if __name__ == "__main__":
    options, flags = gscript.parser()
    atexit.register(cleanup)
    main()
