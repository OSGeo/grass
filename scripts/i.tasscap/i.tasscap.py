#!/usr/bin/env python
#
############################################################################
#
# MODULE:	i.tasscap
# AUTHOR(S):	Agustin Lobo, Markus Neteler
#               Converted to Python by Glynn Clements
#               Code improvements by Leonardo Perathoner
#
# PURPOSE:	At-satellite reflectance based tasseled cap transformation.
# COPYRIGHT:	(C) 1997-2014 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
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
#
#  Landsat8: Baig, M.H.A., Zhang, L., Shuai, T., Tong, Q., 2014. Derivation of a tasselled cap transformation
#              based on Landsat 8 at-satellite reflectance. Remote Sensing Letters 5, 423-431. 
#              doi:10.1080/2150704X.2014.915434
#
#  MODIS Tasselled Cap coefficients
#  https://gis.stackexchange.com/questions/116107/tasseled-cap-transformation-on-modis-in-grass/116110
#  Ref: Lobser & Cohen (2007). MODIS tasselled cap: land cover characteristics 
#                              expressed through transformed MODIS data.
#       International Journal of Remote Sensing, Volume 28(22), Table 3
#
#############################################################################
#
#%Module
#% description: Performs Tasseled Cap (Kauth Thomas) transformation.
#% keywords: imagery
#% keywords: transformation
#% keywords: Landsat
#% keywords: MODIS
#% keywords: Tasseled Cap transformation
#%end
#%option G_OPT_R_INPUTS
#% description: For Landsat4-7: bands 1, 2, 3, 4, 5, 7; for Landsat8: bands 2, 3, 4, 5, 6, 7; for MODIS: bands 1, 2, 3, 4, 5, 6, 7
#%end
#%option G_OPT_R_BASENAME_OUTPUT
#% label: Name for output basename raster map(s)
#%end
#%option
#% key: sensor
#% type: string
#% description: Satellite sensor
#% required: yes
#% multiple: no
#% options: landsat4_tm,landsat5_tm,landsat7_etm,landsat8_oli,modis
#% descriptions: landsat4_tm;Use transformation rules for Landsat 4 TM;landsat5_tm;Use transformation rules for Landsat 5 TM;landsat7_etm;Use transformation rules for Landsat 7 ETM;landsat8_oli;Use transformation rules for Landsat 8 OLI;modis;Use transformation rules for MODIS
#%end

import grass.script as grass

# weights for 6 Landsat bands: TM4, TM5, TM7, OLI
# MODIS: Red, NIR1, Blue, Green, NIR2, SWIR1, SWIR2
parms = [[( 0.3037, 0.2793, 0.4743, 0.5585, 0.5082, 0.1863), # Landsat TM4
	  (-0.2848,-0.2435,-0.5435, 0.7243, 0.0840,-0.1800),
	  ( 0.1509, 0.1973, 0.3279, 0.3406,-0.7112,-0.4572)],
	 [( 0.2909, 0.2493, 0.4806, 0.5568, 0.4438, 0.1706, 10.3695), # Landsat TM5
	  (-0.2728,-0.2174,-0.5508, 0.7221, 0.0733,-0.1648, -0.7310),
	  ( 0.1446, 0.1761, 0.3322, 0.3396,-0.6210,-0.4186, -3.3828),
	  ( 0.8461,-0.0731,-0.4640,-0.0032,-0.0492,-0.0119,  0.7879)],
	 [( 0.3561, 0.3972, 0.3904, 0.6966, 0.2286, 0.1596), # Landsat TM7
	  (-0.3344,-0.3544,-0.4556, 0.6966,-0.0242,-0.2630),
	  ( 0.2626, 0.2141, 0.0926, 0.0656,-0.7629,-0.5388),
	  ( 0.0805,-0.0498, 0.1950,-0.1327, 0.5752,-0.7775)],
	 [( 0.3029, 0.2786, 0.4733, 0.5599, 0.5080, 0.1872), # Landsat TM8
	  (-0.2941,-0.2430,-0.5424, 0.7276, 0.0713,-0.1608),
	  ( 0.1511, 0.1973, 0.3283, 0.3407,-0.7117,-0.4559),
	  (-0.8239, 0.0849, 0.4396, -0.058, 0.2013,-0.2773)],
	 [( 0.4395, 0.5945, 0.2460, 0.3918, 0.3506, 0.2136, 0.2678), # MODIS
	  (-0.4064, 0.5129,-0.2744,-0.2893, 0.4882,-0.0036,-0.4169),
	  ( 0.1147, 0.2489, 0.2408, 0.3132,-0.3122,-0.6416,-0.5087)]]
#satellite information
satellites = ["landsat4_tm", 'landsat5_tm', 'landsat7_etm', 'landsat8_oli', 'modis']
used_bands = [6,6,6,6,7]
#components information
ordinals = ["first", "second", "third", "fourth"]
names = ["Brightness", "Greenness", "Wetness", "Haze"]

def calc1bands6(out, bands, k1, k2, k3, k4, k5, k6, k0 = 0):
    grass.mapcalc("$out = $k1 * $in1band + $k2 * $in2band + $k3 * $in3band + $k4 * $in4band + $k5 * $in5band + $k6 * $in6band + $k0",
    out = out, k1 = k1, k2 = k2, k3 = k3, k4 = k4, k5 = k5, k6 = k6, k0 = k0, **bands)    

def calc1bands7(out, bands, k1, k2, k3, k4, k5, k6, k7):
    grass.mapcalc("$out = $k1 * $in1band + $k2 * $in2band + $k3 * $in3band + $k4 * $in4band + $k5 * $in5band + $k6 * $in6band + $k7 * $in7band",
    out = out, k1 = k1, k2 = k2, k3 = k3, k4 = k4, k5 = k5, k6 = k6, k7 = k7, **bands)

def calcN(outpre, bands, satel):
    i=satellites.index(satel)
    grass.message(_("Satellite %s...") % satel)        
    for j, p in enumerate(parms[i]):
        out = "%s.%d" % (outpre, j + 1)
        ord = ordinals[j]
        name = " (%s)" % names[j]        
        grass.message(_("Calculating %s TC component %s%s ...") % (ord, out, name))
        bands_num=used_bands[i]
        eval("calc1bands%d(out, bands, *p)" % bands_num) #use combination function suitable for used number of bands
        grass.run_command('r.colors', map = out, color = 'grey', quiet=True)


def main():
    options, flags = grass.parser()
    satellite = options['sensor']
    output_basename = options['output']
    inputs = options['input'].split(',')
    num_of_bands = used_bands[satellites.index(satellite)]
    if len(inputs) != num_of_bands:
        grass.fatal(_("The number of input raster maps (bands) should be %s") % num_of_bands)

    bands = {}
    for i, band in enumerate(inputs):        
        band_num = i + 1        
        bands['in' + str(band_num) + 'band'] = band
    grass.debug(1, bands)

    calcN(output_basename, bands, satellite) #core tasseled cap components computation

    #assign "Data Description" field in all four component maps
    for i, comp in enumerate(names):
        grass.run_command('r.support', map = "%s.%d" % (output_basename, i+1), description = "Tasseled Cap %d: %s" % (i+1, comp))

    grass.message(_("Tasseled Cap components calculated"))

if __name__ == "__main__":
    main()
