#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	i.tasscap
# AUTHOR(S):	Agustin Lobo, Markus Neteler
#               Converted to Python by Glynn Clements
#               Code improvements by Leonardo Perathoner
#               Sentinel-2 support by Veronica Andreo
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
#  MODIS Tasseled Cap coefficients
#  https://gis.stackexchange.com/questions/116107/tasseled-cap-transformation-on-modis-in-grass/116110
#  Ref: Lobser & Cohen (2007). MODIS tasselled cap: land cover characteristics
#                              expressed through transformed MODIS data.
#       International Journal of Remote Sensing, Volume 28(22), Table 3
#
#  Sentinel-2 Tasseled Cap coefficients
#  https://www.researchgate.net/publication/329184434_ORTHOGONAL_TRANSFORMATION_OF_SEGMENTED_IMAGES_FROM_THE_SATELLITE_SENTINEL-2
#  Nedkov, R. (2017). ORTHOGONAL TRANSFORMATION OF SEGMENTED IMAGES FROM THE SATELLITE SENTINEL-2.
#  Comptes rendus de l'Académie bulgare des sciences. 70. 687-692.
#
#  Worldview-2 Tasseled Cap coefficients
#  https://www.researchgate.net/publication/263369074_Presentation_of_the_Kauth-Thomas_transform_for_WorldView-2_reflectance_data
#  Yarbrough, Lance & , Navulur & , Ravi. (2014). Presentation of the Kauth–Thomas transform for WorldView-2 reflectance data.
#  Remote Sensing Letters. 5. DOI: 10.1080/2150704X.2014.885148.
#############################################################################

#%Module
#% description: Performs Tasseled Cap (Kauth Thomas) transformation.
#% keyword: imagery
#% keyword: transformation
#% keyword: Landsat
#% keyword: MODIS
#% keyword: Worldview
#% keyword: Sentinel
#% keyword: Tasseled Cap transformation
#%end

#%option G_OPT_R_INPUTS
#% description: For Landsat4-7: bands 1, 2, 3, 4, 5, 7; for Landsat8: bands 2, 3, 4, 5, 6, 7; for MODIS: bands 1, 2, 3, 4, 5, 6, 7; for Sentinel-2: bands 1 to 12, 8A; for Worldview-2: bands 1, 2, 3, 4, 5, 6, 7, 8
#%end

#%option G_OPT_R_BASENAME_OUTPUT
#% label: basename for output raster map(s)
#%end

#%option
#% key: sensor
#% type: string
#% description: Satellite sensor
#% required: yes
#% multiple: no
#% options: landsat4_tm,landsat5_tm,landsat7_etm,landsat8_oli,modis,sentinel2,worldview2
#%end

import grass.script as grass


# weights for 6 Landsat bands: TM4, TM5, TM7, OLI
# MODIS: Red, NIR1, Blue, Green, NIR2, SWIR1, SWIR2
# Sentinel-2: B1 ... B12, B8A
# Worldview-2: B1 ... B8 (PAN not used)
#
## entries: 1. Brightness, 2. Greeness, 3. Wetness and shadows, 4. Haze
parms = [[(0.3037, 0.2793, 0.4743, 0.5585, 0.5082, 0.1863),  # Landsat TM4
          (-0.2848, -0.2435, -0.5435, 0.7243, 0.0840, -0.1800),
          (0.1509, 0.1973, 0.3279, 0.3406, -0.7112, -0.4572)],
         [(0.2909, 0.2493, 0.4806, 0.5568, 0.4438, 0.1706, 10.3695),  # Landsat TM5
          (-0.2728, -0.2174, -0.5508, 0.7221, 0.0733, -0.1648, -0.7310),
          (0.1446, 0.1761, 0.3322, 0.3396, -0.6210, -0.4186, -3.3828),
          (0.8461, -0.0731, -0.4640, -0.0032, -0.0492, -0.0119, 0.7879)],
         [(0.3561, 0.3972, 0.3904, 0.6966, 0.2286, 0.1596),  # Landsat TM7
          (-0.3344, -0.3544, -0.4556, 0.6966, -0.0242, -0.2630),
          (0.2626, 0.2141, 0.0926, 0.0656, -0.7629, -0.5388),
          (0.0805, -0.0498, 0.1950, -0.1327, 0.5752, -0.7775)],
         [(0.3029, 0.2786, 0.4733, 0.5599, 0.5080, 0.1872),  # Landsat OLI
          (-0.2941, -0.2430, -0.5424, 0.7276, 0.0713, -0.1608),
          (0.1511, 0.1973, 0.3283, 0.3407, -0.7117, -0.4559),
          (-0.8239, 0.0849, 0.4396, -0.0580, 0.2013, -0.2773)],
         [(0.4395, 0.5945, 0.2460, 0.3918, 0.3506, 0.2136, 0.2678),  # MODIS
          (-0.4064, 0.5129, -0.2744, -0.2893, 0.4882, -0.0036, -0.4169),
          (0.1147, 0.2489, 0.2408, 0.3132, -0.3122, -0.6416, -0.5087)],
         [(0.0356, 0.0822, 0.1360, 0.2611, 0.2964, 0.3338, 0.3877, 0.3895, 0.0949, 0.0009, 0.3882, 0.1366, 0.4750), # Sentinel-2
          (-0.0635, -0.1128, -0.1680, -0.3480, -0.3303, 0.0852, 0.3302, 0.3165, 0.0467, -0.0009, -0.4578, -0.4064, 0.3625),
          (0.0649, 0.1363, 0.2802, 0.3072, 0.5288, 0.1379, -0.0001, -0.0807, -0.0302, 0.0003, -0.4064, -0.5602, -0.1389)],
         [(-0.060436,0.012147,0.125846,0.313039,0.412175,0.482758,-0.160654,0.673510), # WV-2
          (-0.140110,-0.206224,-0.215854,-0.314441,-0.410892,0.095786,0.600549,0.503672),
          (-0.270951,-0.317080,-0.317263,-0.242544,-0.256463,-0.096550,-0.742535,0.202430),
          (0.546979,0.392244,0.232894,-0.151027,-0.540102,0.327952,-0.243740,0.106010)]]

# satellite information
satellites = ['landsat4_tm', 'landsat5_tm', 'landsat7_etm', 'landsat8_oli',
              'modis', 'sentinel2', 'worldview2']
used_bands = [6, 6, 6, 6, 7, 13, 8]

# components information
ordinals = ["first", "second", "third", "fourth"]
names = ["Brightness", "Greenness", "Wetness", "Haze"]


def calc1bands6(out, bands, k1, k2, k3, k4, k5, k6, k0=0):
    """
    Tasseled cap transformation equation for Landsat bands
    """
    equation = ('$out = $k1 * $in1band + $k2 * $in2band + $k3 * $in3band + '
                '$k4 * $in4band + $k5 * $in5band + $k6 * $in6band + $k0')
    grass.mapcalc(equation, out=out, k1=k1, k2=k2, k3=k3, k4=k4, k5=k5,
                  k6=k6, k0=k0, **bands)


def calc1bands7(out, bands, k1, k2, k3, k4, k5, k6, k7):
    """
    Tasseled cap transformation equation for MODIS bands
    """
    equation = ('$out = $k1 * $in1band + $k2 * $in2band + $k3 * $in3band + '
                '$k4 * $in4band + $k5 * $in5band + $k6 * $in6band + $k7 * '
                '$in7band')
    grass.mapcalc(equation, out=out, k1=k1, k2=k2, k3=k3, k4=k4, k5=k5, k6=k6,
                  k7=k7, **bands)


def calc1bands8(out, bands, k1, k2, k3, k4, k5, k6, k7, k8):
    """
    Tasseled cap transformation equation for Worldview-2 bands
    """
    equation = ('$out = $k1 * $in1band + $k2 * $in2band + $k3 * $in3band + '
                '$k4 * $in4band + $k5 * $in5band + $k6 * $in6band + $k7 * '
                '$in7band + $k8 * $in8band')
    grass.mapcalc(equation, out=out, k1=k1, k2=k2, k3=k3, k4=k4, k5=k5, k6=k6,
                  k7=k7, k8=k8, **bands)


def calc1bands13(out, bands, k1, k2, k3, k4, k5, k6, k7, k8, k9, k10, k11, k12, k13):
    """
    Tasseled cap transformation equation for Sentinel-2 bands
    """
    equation = ('$out = $k1 * $in1band + $k2 * $in2band + $k3 * $in3band + '
                '$k4 * $in4band + $k5 * $in5band + $k6 * $in6band + $k7 * '
                '$in7band + $k8 * $in8band + $k9 * $in9band + $k10 * $in10band + '
                '$k11 * $in11band + $k12 * $in12band + $k13 * $in13band')
    grass.mapcalc(equation, out=out, k1=k1, k2=k2, k3=k3, k4=k4, k5=k5, k6=k6,
                  k7=k7, k8=k8, k9=k9, k10=k10, k11=k11, k12=k12, k13=k13,
                  **bands)


def calcN(outpre, bands, satel):
    """
    Calculating Tasseled Cap components
    """
    i = satellites.index(satel)
    grass.message(_("Satellite %s...") % satel)

    for j, p in enumerate(parms[i]):
        out = "%s.%d" % (outpre, j + 1)
        ord = ordinals[j]
        name = " (%s)" % names[j]
        message = "Calculating {ordinal} TC component {outprefix}{outname} ..."
        message = message.format(ordinal=ord, outprefix=out, outname=name)
        grass.message(_(message))
        bands_num = used_bands[i]

        # use combination function suitable for used number of bands
        eval("calc1bands%d(out, bands, *p)" % bands_num)
        grass.run_command('r.colors', map=out, color='grey', quiet=True)


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
    grass.debug(bands, 1)

    # core tasseled cap components computation
    calcN(output_basename, bands, satellite)

    # assign "Data Description" field in all four component maps
    num_comp=len(parms[satellites.index(satellite)])
    for i in range(0,num_comp):
        comp=names[i]
        grass.run_command('r.support', map="%s.%d" % (output_basename, i + 1),
                          description="Tasseled Cap %d: %s" % (i + 1, comp))

    grass.message(_("Tasseled Cap components calculated"))

if __name__ == "__main__":
    main()
