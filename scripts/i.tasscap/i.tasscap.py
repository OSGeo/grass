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
#% description: Performs Tasseled Cap (Kauth Thomas) transformation for LANDSAT-TM data.
#% keywords: imagery
#% keywords: transformation
#% keywords: Landsat
#% keywords: Tasseled Cap transformation
#%end
#%flag
#% key: 4
#% description: Use transformation rules for LANDSAT-4
#%end
#%flag
#% key: 5
#% description: Use transformation rules for LANDSAT-5
#%end
#%flag
#% key: 7
#% description: Use transformation rules for LANDSAT-7
#%end
#%option G_OPT_R_INPUT
#% key: band1
#% description: Name of input raster map (LANDSAT channel 1)
#%end
#%option G_OPT_R_INPUT
#% key: band2
#% description: Name of input raster map (LANDSAT channel 2)
#%end
#%option G_OPT_R_INPUT
#% key: band3
#% description: Name of input raster map (LANDSAT channel 3)
#%end
#%option G_OPT_R_INPUT
#% key: band4
#% description: Name of input raster map (LANDSAT channel 4)
#%end
#%option G_OPT_R_INPUT
#% key: band5
#% description: Name of input raster map (LANDSAT channel 5)
#%end
#%option G_OPT_R_INPUT
#% key: band7
#% description: Name of input raster map (LANDSAT channel 7)
#%end
#%option
#% key: output_prefix
#% type: string
#% description: Prefix for output raster maps
#% required: yes
#%end

import sys
import os
import grass.script as grass

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
    grass.mapcalc(
	"$out = $k1 * $band1 + $k2 * $band2 + $k3 * $band3 + $k4 * $band4 + $k5 * $band5 + $k7 * $band7 + $k0",
	out = out, k1 = k1, k2 = k2, k3 = k3, k4 = k4, k5 = k5, k7 = k7, k0 = k0, **bands)
    grass.run_command('r.colors', map = out, color = 'grey')

def calcN(options, i, n):
    outpre = options['output_prefix']
    grass.message(_("LANDSAT-%d...") % n)
    for j, p in enumerate(parms[i]):
	out = "%s.%d" % (outpre, j + 1)
	ord = ordinals[j]
	if n == 4:
	    name = ''
	else:
	    name = " (%s)" % names[j]
	grass.message(_("Calculating %s TC component %s%s ...") % (ord, out, name))
	calc1(out, options, *p)

def main():
    flag4 = flags['4']
    flag5 = flags['5']
    flag7 = flags['7']

    if (flag4 and flag5) or (flag4 and flag7) or (flag5 and flag7):
	grass.fatal(_("Select only one flag"))

    if flag4:
	calcN(options, 0, 4)
    elif flag5:
	calcN(options, 1, 5)
    elif flag7:
	calcN(options, 2, 7)
    else:
	grass.fatal(_("Select LANDSAT satellite by flag!"))

    grass.message(_("Tasseled Cap components calculated."))

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
