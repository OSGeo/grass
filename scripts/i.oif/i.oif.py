#!/usr/bin/env python

############################################################################
#
# MODULE:       i.oif
# AUTHOR(S):    Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      calculates the Optimum Index factor of all band combinations
#               for LANDSAT TM 1,2,3,4,5,7
# COPYRIGHT:    (C) 1999,2008 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################
# Ref.: Jensen: Introductory digital image processing 1996, p.98
#
# Input: tm1 - tm5, tm7 (not tm6)
#
# written by Markus Neteler 21.July 1998
#            neteler geog.uni-hannover.de
# updated for GRASS 5.7 by Michael Barton 2004/04/05


#% Module
#% description: Calculates Optimum-Index-Factor table for LANDSAT TM bands 1-5, & 7
#% keywords: imagery
#% keywords: landsat
#% keywords: statistics
#% End
#% option G_OPT_R_INPUT
#% key: image1
#% description: LANDSAT TM band 1.
#% end
#% option G_OPT_R_INPUT
#% key: image2
#% description: LANDSAT TM band 2.
#% end
#% option G_OPT_R_INPUT
#% key: image3
#% description: LANDSAT TM band 3.
#% end
#% option G_OPT_R_INPUT
#% key: image4
#% description: LANDSAT TM band 4.
#% end
#% option G_OPT_R_INPUT
#% key: image5
#% description: LANDSAT TM band 5.
#% end
#% option G_OPT_R_INPUT
#% key: image7
#% description: LANDSAT TM band 7.
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
from grass.script import core as grass

bands = [1,2,3,4,5,7]

def oifcalc(sdev, corr, k1, k2, k3):

    # calculate SUM of Stddeviations:
    ssdev = [sdev[k1], sdev[k2], sdev[k3]]
    numer = sum(ssdev)

    # calculate SUM of absolute(Correlation values):
    scorr = [corr[k1,k2], corr[k1,k3], corr[k2,k3]]
    denom = sum(map(abs, scorr))

    # Calculate OIF index:
    #     Divide (SUM of Stddeviations) and (SUM of Correlation)
    return numer / denom

def perms():
    for i in range(0,4):
	for j in range(i+1,5):
	    for k in range(j+1,6):
		yield (bands[i], bands[j], bands[k])

def main():
    shell = flags['g']
    serial = flags['s']
    image = {}
    for band in bands:
	image[band] = options['image%d' % band]

    # calculate the Stddev for TM bands
    grass.message(_("Calculating Standard deviations for all bands..."))
    stddev = {}

    if serial:
        for band in bands:
	    grass.verbose("band %d" % band)
	    s = grass.read_command('r.univar', flags = 'g', map = image[band])
	    kv = grass.parse_key_val(s)
	    stddev[band] = float(kv['stddev'])
    else:
	# run all bands in parallel
	if "WORKERS" in os.environ:
            workers = int(os.environ["WORKERS"])
	else:
	    workers = 6

	proc = {}
	pout = {}

	# spawn jobs in the background
	for band in bands:
	    grass.debug("band %d, <%s>  %% %d" % (band, image[band], band % workers))
	    proc[band] = grass.pipe_command('r.univar', flags = 'g', map = image[band])
	    if band % workers is 0:
		# wait for the ones launched so far to finish
		for bandp in bands[:band]:
		    if not proc[bandp].stdout.closed:
			pout[bandp] = proc[bandp].communicate()[0]
		    proc[bandp].wait()

	# wait for jobs to finish, collect the output
	for band in bands:
	    if not proc[band].stdout.closed:
		pout[band] = proc[band].communicate()[0]
	    proc[band].wait()

	# parse the results
        for band in bands:
	    kv = grass.parse_key_val(pout[band])
	    stddev[band] = float(kv['stddev'])


    grass.message(_("Calculating Correlation Matrix..."))
    correlation = {}
    s = grass.read_command('r.covar', flags = 'r', map = [image[band] for band in bands])
    for i, row in zip(bands, s.splitlines()):
	for j, cell in zip(bands, row.split(' ')):
	    correlation[i,j] = float(cell)

    # Calculate all combinations
    grass.message(_("Calculating OIF for the 20 band combinations..."))

    oif = []
    for p in perms():
	oif.append((oifcalc(stddev, correlation, *p), p))
    oif.sort(reverse = True)

    grass.verbose(_("The Optimum Index Factor analysis result "
                    "(Best combination comes first):"))
    
    if shell:
	fmt = "%d%d%d:%.4f\n"
    else:
	fmt = "%d%d%d:  %.4f\n"

    outf = file('i.oif.result', 'w')
    for v, p in oif:
	sys.stdout.write(fmt % (p + (v,)))
	outf.write(fmt % (p + (v,)))
    outf.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
