#!/usr/bin/env python3

############################################################################
#
# MODULE:       i.oif
# AUTHOR(S):    Markus Neteler 21.July 1998
#               Updated for GRASS 5.7 by Michael Barton 2004/04/05
#               Converted to Python by Glynn Clements
#               Customised by Nikos Alexandris 04. August 2013
#               Customised by Luca Delucchi Vienna Code Sprint 2014
# PURPOSE:      calculates the Optimum Index factor of all band combinations
#               for LANDSAT TM 1,2,3,4,5,7
# COPYRIGHT:    (C) 1999,2008 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
# Ref.: Jensen: Introductory digital image processing 1996, p.98
#############################################################################

#% Module
#% description: Calculates Optimum-Index-Factor table for spectral bands
#% keyword: imagery
#% keyword: multispectral
#% keyword: statistics
#% End
#% option G_OPT_R_INPUTS
#% end
#% option G_OPT_F_OUTPUT
#% description: Name for output file (if omitted or "-" output to stdout)
#% required: no
#% end
#% Flag
#% key: g
#% description: Print in shell script style
#% End
#% Flag
#% key: s
#% description: Process bands serially (default: run in parallel)
#% End

import sys
import os
from grass.script.utils import parse_key_val
from grass.script import core as grass


def oifcalc(sdev, corr, k1, k2, k3):
    grass.debug(_("Calculating OIF for combination: %s, %s, %s" % (k1, k2,
                                                                   k3)), 1)
    # calculate SUM of Stddeviations:
    ssdev = [sdev[k1], sdev[k2], sdev[k3]]
    numer = sum(ssdev)

    # calculate SUM of absolute(Correlation values):
    scorr = [corr[k1, k2], corr[k1, k3], corr[k2, k3]]
    denom = sum(map(abs, scorr))

    # Calculate OIF index:
    #     Divide (SUM of Stddeviations) and (SUM of Correlation)
    return numer / denom


def perms(bands):
    n = len(bands)
    for i in range(0, n - 2):
        for j in range(i + 1, n - 1):
            for k in range(j + 1, n):
                yield (bands[i], bands[j], bands[k])


def main():
    shell = flags['g']
    serial = flags['s']
    bands = options['input'].split(',')

    if len(bands) < 4:
        grass.fatal(_("At least four input maps required"))

    output = options['output']
    # calculate the Stddev for TM bands
    grass.message(_("Calculating standard deviations for all bands..."))
    stddev = {}

    if serial:
        for band in bands:
            grass.verbose("band %d" % band)
            s = grass.read_command('r.univar', flags='g', map=band)
            kv = parse_key_val(s)
            stddev[band] = float(kv['stddev'])
    else:
        # run all bands in parallel
        if "WORKERS" in os.environ:
            workers = int(os.environ["WORKERS"])
        else:
            workers = len(bands)
        proc = {}
        pout = {}

        # spawn jobs in the background
        n = 0
        for band in bands:
            proc[band] = grass.pipe_command('r.univar', flags='g', map=band)
            if n % workers is 0:
                # wait for the ones launched so far to finish
                for bandp in bands[:n]:
                    if not proc[bandp].stdout.closed:
                        pout[bandp] = proc[bandp].communicate()[0]
                    proc[bandp].wait()
            n = n + 1

        # wait for jobs to finish, collect the output
        for band in bands:
            if not proc[band].stdout.closed:
                pout[band] = proc[band].communicate()[0]
            proc[band].wait()

    # parse the results
        for band in bands:
            kv = parse_key_val(pout[band])
            stddev[band] = float(kv['stddev'])

    grass.message(_("Calculating Correlation Matrix..."))
    correlation = {}
    s = grass.read_command('r.covar', flags='r', map=[band for band in bands],
                           quiet=True)

    # We need to skip the first line, since r.covar prints the number of values
    lines = s.splitlines()
    for i, row in zip(bands, lines[1:]):
        for j, cell in zip(bands, row.split(' ')):
            correlation[i, j] = float(cell)

    # Calculate all combinations
    grass.message(_("Calculating OIF for all band combinations..."))

    oif = []
    for p in perms(bands):
        oif.append((oifcalc(stddev, correlation, *p), p))
    oif.sort(reverse=True)

    grass.verbose(_("The Optimum Index Factor analysis result "
                    "(best combination shown first):"))

    if shell:
        fmt = "%s,%s,%s:%.4f\n"
    else:
        fmt = "%s, %s, %s:  %.4f\n"

    if not output or output == '-':
        for v, p in oif:
            sys.stdout.write(fmt % (p + (v,)))
    else:
        outf = open(output, 'w')
        for v, p in oif:
            outf.write(fmt % (p + (v,)))
        outf.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
