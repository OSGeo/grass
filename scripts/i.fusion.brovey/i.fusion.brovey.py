#!/usr/bin/env python

############################################################################
#
# MODULE:	i.fusion.brovey
# AUTHOR(S):	Markus Neteler. <neteler itc it>
#               Converted to Python by Glynn Clements
# PURPOSE:	Brovey transform to merge
#                 - LANDSAT-7 MS (2, 4, 5) and pan (high res)
#                 - SPOT MS and pan (high res)
#                 - QuickBird MS and pan (high res)
#
# COPYRIGHT:	(C) 2002-2008 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
# REFERENCES:
#             (?) Roller, N.E.G. and Cox, S., 1980. Comparison of Landsat MSS
#                and merged MSS/RBV data for analysis of natural vegetation.
#                Proc. of the 14th International Symposium on Remote Sensing
#                of Environment, San Jose, Costa Rica, 23-30 April, pp. 1001-1007.
#
#               for LANDSAT 5: see Pohl, C 1996 and others
#
# TODO:         add overwrite test at beginning of the script
#############################################################################

#%Module
#%  description: Brovey transform to merge multispectral and high-res panchromatic channels
#%  keywords: imagery
#%  keywords: fusion
#%  keywords: Brovey
#%End
#%Flag
#%  key: l
#%  description: LANDSAT sensor
#%  guisection: Sensor
#%END
#%Flag
#%  key: q
#%  description: QuickBird sensor
#%  guisection: Sensor
#%END
#%Flag
#%  key: s
#%  description: SPOT sensor
#%  guisection: Sensor
#%END
#%option G_OPT_R_INPUT
#% key: ms1
#% description: Name of input raster map (green: tm2 | qbird_green | spot1)
#%end
#%option G_OPT_R_INPUT
#% key: ms2
#% description: Name of input raster map (NIR: tm4 | qbird_nir | spot2
#%end
#%option G_OPT_R_INPUT
#% key: ms3
#% description: Name of input raster map (MIR; tm5 | qbird_red | spot3
#%end
#%option G_OPT_R_INPUT
#% key: pan
#% description: Name of input raster map (etmpan | qbird_pan | spotpan)
#%end
#%option
#% key: output_prefix
#% type: string
#% description: Prefix for output raster maps
#% required : yes
#%end

import sys
import os
import grass.script as grass

def main():
    global tmp

    landsat = flags['l']
    quickbird = flags['q']
    spot = flags['s']

    ms1 = options['ms1']
    ms2 = options['ms2']
    ms3 = options['ms3']
    pan = options['pan']
    out = options['outputprefix']

    tmp = str(os.getpid())

    if not landsat and not quickbird and not spot:
	grass.fatal(_("Please select a flag to specify the satellite sensor"))

    #get PAN resolution:
    kv = grass.raster_info(map = pan)
    nsres = kv['nsres']
    ewres = kv['ewres']
    panres = (nsres + ewres) / 2

    # clone current region
    grass.use_temp_region()

    grass.verbose("Using resolution from PAN: %f" % panres)
    grass.run_command('g.region', flags = 'a', res = panres)

    grass.verbose("Performing Brovey transformation...")

    # The formula was originally developed for LANDSAT-TM5 and SPOT, 
    # but it also works well with LANDSAT-TM7
    # LANDSAT formula:
    #  r.mapcalc "brov.red=1. *  tm.5 / (tm.2 + tm.4 + tm.5) * etmpan"
    #  r.mapcalc "brov.green=1. * tm.4 /(tm.2 + tm.4 + tm.5) * etmpan"
    #  r.mapcalc "brov.blue=1. * tm.2 / (tm.2 + tm.4 + tm.5) * etmpan"
    #
    # SPOT formula:
    # r.mapcalc "brov.red= 1.  * spot.ms.3 / (spot.ms.1 + spot.ms.2 + spot.ms.3) * spot.p"
    # r.mapcalc "brov.green=1. * spot.ms.2 / (spot.ms.1 + spot.ms.2 + spot.ms.3) * spot.p"
    # r.mapcalc "brov.blue= 1. * spot.ms.1 / (spot.ms.1 + spot.ms.2 + spot.ms.3) * spot.p"
    # note: for RGB composite then revert brov.red and brov.green!

    grass.message(_("Calculating %s.{red,green,blue}: ...") % out)
    e = '''eval(k = float("$pan") / ("$ms1" + "$ms2" + "$ms3"))
	   "$out.red"   = "$ms3" * k
	   "$out.green" = "$ms2" * k
	   "$out.blue"  = "$ms1" * k'''
    grass.mapcalc(e, out = out, pan = pan, ms1 = ms1, ms2 = ms2, ms3 = ms3)

    # Maybe?
    #r.colors   $GIS_OPT_OUTPUTPREFIX.red col=grey
    #r.colors   $GIS_OPT_OUTPUTPREFIX.green col=grey
    #r.colors   $GIS_OPT_OUTPUTPREFIX.blue col=grey
    #to blue-ish, therefore we modify
    #r.colors $GIS_OPT_OUTPUTPREFIX.blue col=rules << EOF
    #5 0 0 0
    #20 200 200 200
    #40 230 230 230
    #67 255 255 255
    #EOF

    if spot:
        #apect table is nice for SPOT:
	grass.message(_("Assigning color tables for SPOT..."))
	for ch in ['red', 'green', 'blue']:
	    grass.run_command('r.colors', map = "%s.%s" % (out, ch), col = 'aspect')
	grass.message(_("Fixing output names..."))
	for s, d in [('green','tmp'),('red','green'),('tmp','red')]:
	    src = "%s.%s" % (out, s)
	    dst = "%s.%s" % (out, d)
	    grass.run_command('g.rename', rast = (src, dst), quiet = True)
    else:
	#aspect table is nice for LANDSAT and QuickBird:
	grass.message(_("Assigning color tables for LANDSAT or QuickBird..."))
	for ch in ['red', 'green', 'blue']:
	    grass.run_command('r.colors', map = "%s.%s" % (out, ch), col = 'aspect')

    grass.message(_("Following pan-sharpened output maps have been generated:"))
    for ch in ['red', 'green', 'blue']:
	grass.message(_("%s.%s") % (out, ch))

    grass.verbose("To visualize output, run:")
    grass.verbose("g.region -p rast=%s.red" % out)
    grass.verbose("d.rgb r=%s.red g=%s.green b=%s.blue" % (out, out, out))
    grass.verbose("If desired, combine channels with 'r.composite' to a single map.")

    # write cmd history:
    for ch in ['red', 'green', 'blue']:
	grass.raster_history("%s.%s" % (out, ch))

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
