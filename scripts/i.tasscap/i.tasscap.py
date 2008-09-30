#!/usr/bin/env python
#
############################################################################
#
# MODULE:	i.tasscap
# AUTHOR(S):	Markus Neteler. neteler itc.it
#               Converted to Python by Glynn Clements
# PURPOSE:	At-satellite reflectance based tasseled cap transformation.
# COPYRIGHT:	(C) 1997-2004,2008 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
# TODO: Check if MODIS Tasseled Cap makes sense to be added
#       http://ieeexplore.ieee.org/xpls/abs_all.jsp?arnumber=1025776
#############################################################################
# References:
# LANDSAT-4/LANDSAT-5:
#  script based on i.tasscap.tm4 from Dr. Agustin Lobo - alobo@ija.csic.es
#  TC-factor changed to CRIST et al. 1986, p.1467 (Markus Neteler 1/99)
#                       Proc. IGARSS 1986
#
# LANDSAT-7:
#  TASSCAP factors cited from:
#   DERIVATION OF A TASSELED CAP TRANSFORMATION BASED ON LANDSAT 7 AT-SATELLITE REFLECTANCE
#   Chengquan Huang, Bruce Wylie, Limin Yang, Collin Homer and Gregory Zylstra Raytheon ITSS, 
#   USGS EROS Data Center Sioux Falls, SD 57198, USA
#   http://landcover.usgs.gov/pdf/tasseled.pdf
#
#  This is published as well in INT. J. OF RS, 2002, VOL 23, NO. 8, 1741-1748.
#  Compare discussion:
#  http://adis.cesnet.cz/cgi-bin/lwgate/IMAGRS-L/archives/imagrs-l.log0211/date/article-14.html
#############################################################################
#
#%Module
#%  description: Tasseled Cap (Kauth Thomas) transformation for LANDSAT-TM data
#%  keywords: raster, imagery
#%End
#%flag
#%  key: 4
#%  description: use transformation rules for LANDSAT-4
#%END
#%flag
#%  key: 5
#%  description: use transformation rules for LANDSAT-5
#%END
#%flag
#%  key: 7
#%  description: use transformation rules for LANDSAT-7
#%END
#%option
#% key: band1
#% type: string
#% gisprompt: old,cell,raster
#% description: raster input map (LANDSAT channel 1)
#% required : yes
#%end
#%option
#% key: band2
#% type: string
#% gisprompt: old,cell,raster
#% description: raster input map (LANDSAT channel 2)
#% required : yes
#%end
#%option
#% key: band3
#% type: string
#% gisprompt: old,cell,raster
#% description: raster input map (LANDSAT channel 3)
#% required : yes
#%end
#%option
#% key: band4
#% type: string
#% gisprompt: old,cell,raster
#% description: raster input map (LANDSAT channel 4)
#% required : yes
#%end
#%option
#% key: band5
#% type: string
#% gisprompt: old,cell,raster
#% description: raster input map (LANDSAT channel 5)
#% required : yes
#%end
#%option
#% key: band7
#% type: string
#% gisprompt: old,cell,raster
#% description: raster input map (LANDSAT channel 7)
#% required : yes
#%end
#%option
#% key: outprefix
#% type: string
#% gisprompt: new,cell,raster
#% description: raster output TC maps prefix
#% required : yes
#%end

import sys
import os
import string
import grass

parms = [[( 0.3037, 0.2793, 0.4743, 0.5585, 0.5082, 0.1863),
	  (-0.2848,-0.2435,-0.5435, 0.7243, 0.0840,-0.1800),
	  ( 0.1509, 0.1973, 0.3279, 0.3406,-0.7112,-0.4572)],
	 [( 0.2909, 0.2493, 0.4806, 0.5568, 0.4438, 0.1706, 10.3695),
	  (-0.2728,-0.2174,-0.5508, 0.7221, 0.0733,-0.1648, -0.7310),
	  ( 0.1446, 0.1761, 0.3322, 0.3396,-0.6210,-0.4186, -3.3828),
	  ( 0.8461,-0.0731,-0.4640,-0.0032,-0.0492,-0.0119,  0.7879)],
	 [( 0.3561, 0.3972, 0.3904, 0.6966, 0.2286, 0.1596),
	  (-0.3344,-0.3544,-0.4556, 0.6966,-0.0242,-0.2630),
	  ( 0.2626, 0.2141, 0.0926, 0.0656,-0.7629,-0.5388),
	  ( 0.0805,-0.0498, 0.1950,-0.1327, 0.5752,-0.7775)]]
ordinals = ["first", "second", "third", "fourth"]
names = ["Brightness", "Greenness", "Wetness", "Haze"]

def calc1(out, bands, k1, k2, k3, k4, k5, k7, k0 = 0):
    t = string.Template("$out = $k1 * $band1 + $k2 * $band2 + $k3 * $band3 + $k4 * $band4 + $k5 * $band5 + $k7 * $band7 + $k0")
    e = t.substitute(out = out, k1 = k1, k2 = k2, k3 = k3, k4 = k4, k5 = k5, k7 = k7, k0 = k0, **bands)
    grass.run_command('r.mapcalc', expression = e)
    grass.run_command('r.colors', map = out, color = 'grey')

def calcN(options, i, n):
    outpre = options['outprefix']
    grass.message("LANDSAT-%d..." % n)
    for j, p in enumerate(parms[i]):
	out = "%s.%d" % (outpre, j + 1)
	ord = ordinals[j]
	if n == 4:
	    name = ''
	else:
	    name = " (%s)" % names[j]
	grass.message("Calculating %s TC component %s%s ..." % (ord, out, name))
	calc1(out, options, *p)

def main():
    flag4 = flags['4']
    flag5 = flags['5']
    flag7 = flags['7']

    if (flag4 and flag5) or (flag4 and flag7) or (flag5 and flag7):
	grass.fatal("Select only one flag")

    if flag4:
	calcN(options, 0, 4)
    elif flag5:
	calcN(options, 1, 5)
    elif flag7:
	calcN(options, 2, 7)
    else:
	grass.fatal("Select LANDSAT satellite by flag!")

    grass.message("Tasseled Cap components calculated.")

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
