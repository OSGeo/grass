#!/usr/bin/env python

############################################################################
#
# MODULE:       r.shaded.relief
# AUTHOR(S):	CERL
#               parameters standardized: Markus Neteler, 2008
#               updates: Michael Barton, 2004
#               updates: Gordon Keith, 2003
#               updates: Andreas Lange, 2001
#               updates: David Finlayson, 2001
#               updates: Markus Neteler, 2001, 1999
#               Converted to Python by Glynn Clements
# PURPOSE:	Creates shaded relief map from raster elevation map (DEM)
# COPYRIGHT:	(C) 1999 - 2008, 2010 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################
#
#   July 2007 - allow input from other mapsets (Brad Douglas)
#
#   May 2005 - fixed wrong units parameter (Markus Neteler)
#
#   September 2004 - Added z exaggeration control (Michael Barton) 
#   April 2004 - updated for GRASS 5.7 by Michael Barton 
#
#   9/2004 Adds scale factor input (as per documentation); units set scale only if specified for lat/long regions
#    Also, adds option of controlling z-exaggeration.
#
#   6/2003 fixes for Lat/Long Gordon Keith <gordon.keith@csiro.au>
#   If n is a number then the ewres and nsres are mulitplied by that scale
#    to calculate the shading.
#   If n is the letter M (either case) the number of metres is degree of
#    latitude is used as the scale.
#   If n is the letter f then the number of feet in a degree is used.
#   It scales latitude and longitude equally, so it's only approximately
#   right, but for shading its close enough. It makes the difference
#   between an unusable and usable shade.
#
#   10/2001 fix for testing for dashes in raster file name
#        by Andreas Lange <andreas.lange@rhein-main.de>
#   10/2001 added parser support - Markus Neteler
#   9/2001 fix to keep NULLs as is (was value 22 before) - Markus Neteler
#   1/2001 fix for NULL by David Finlayson <david_finlayson@yahoo.com>
#   11/99 updated $ewres to ewres() and $nsres to nsres()
#       updated number to FP in r.mapcalc statement Markus Neteler

#%module
#% description: Creates shaded relief map from an elevation map (DEM).
#% keywords: raster
#% keywords: elevation
#% keywords: terrain
#%end
#%option G_OPT_R_INPUT
#% description: Name of input elevation raster map
#%end
#%option G_OPT_R_OUTPUT
#%end
#%option
#% key: altitude
#% type: double
#% description: Altitude of the sun in degrees above the horizon
#% required: no
#% options : 0-90
#% answer: 30
#%end
#%option
#% key: azimuth
#% type: double
#% description: Azimuth of the sun in degrees to the east of north
#% required: no
#% options : 0-360
#% answer: 270
#%end
#%option
#% key: zmult    
#% type: double
#% description: Factor for exaggerating relief
#% required: no
#% answer: 1
#%end
#%option
#% key: scale
#% type: double
#% description: Scale factor for converting horizontal units to elevation units
#% required: no
#% answer: 1
#% guisection: Scaling
#%end
#%option
#% key: units
#% type: string
#% description: Set scaling factor (applies to lat./long. locations only, none: scale=1)
#% required: no
#% options: none,meters,feet
#% answer: none
#% guisection: Scaling
#%end

import sys
import os
import string
import grass.script as grass

def main():
    input = options['input']
    output = options['output']
    altitude = options['altitude']
    azimuth = options['azimuth']
    zmult = options['zmult']
    scale = float(options['scale'])
    units = options['units']
    verbose_level = os.getenv('GRASS_VERBOSE')
    if verbose_level is None:
        verbose_level = 2
    
    if not grass.find_file(input)['file']:
	grass.fatal(_("Raster map <%s> not found") % input)

    if input == output:
	grass.fatal(_("Input elevation map and output relief map must have different names"))
    
    # LatLong locations only:
    if units == 'meters':
        # scale=111120
	scale *= 1852 * 60

    # LatLong locations only:
    if units == 'feet':
        # scale=364567.2
	scale *= 6076.12 * 60

    #correct azimuth to East (GRASS convention):
    #  this seems to be backwards, but in fact it works so leave it.
    az = float(azimuth) - 90

    t = string.Template(
	r'''eval( \
	x=($zmult*$input[-1,-1] + 2*$zmult*$input[0,-1] + $zmult*$input[1,-1] - \
	$zmult*$input[-1,1] - 2*$zmult*$input[0,1] - $zmult*$input[1,1])/(8.*ewres()*$scale), \
	y=($zmult*$input[-1,-1] + 2*$zmult*$input[-1,0] + $zmult*$input[-1,1] - \
	$zmult*$input[1,-1] - 2*$zmult*$input[1,0] - $zmult*$input[1,1])/(8.*nsres()*$scale), \
	slope=90.-atan(sqrt(x*x + y*y)), \
	a=round(atan(x,y)), \
	a=if(isnull(a),1,a), \
	aspect=if(x!=0||y!=0,if(a,a,360.)), \
	cang = sin($altitude)*sin(slope) + cos($altitude)*cos(slope) * cos($az-aspect) \
	)
	$output = if(isnull(cang), null(), 100.*cang)''')
    expr = t.substitute(altitude = altitude, az = az, input = input, output = output, scale = scale, zmult = zmult)
    p = grass.feed_command('r.mapcalc')
    p.stdin.write(expr)
    p.stdin.close()

    if p.wait() != 0:
	grass.fatal(_("In calculation, script aborted."))

    if verbose_level < 3:
        grass.run_command('r.colors', map = output, color = 'grey', quiet = True)
    else:
        grass.run_command('r.colors', map = output, color = 'grey')
    
    # record metadata
    grass.run_command('r.support', map = output, title = 'Shaded relief of "%s"' % input, history = '')
    grass.run_command('r.support', map = output, history = "r.shaded.relief settings:")
    t = string.Template("altitude=$altitude  azimuth=$azimuth zmult=$zmult  scale=$scale")
    parms = dict(altitude = altitude, azimuth = azimuth, zmult = zmult, scale = options['scale'])
    grass.run_command('r.support', map = output, history = t.substitute(parms))
    if units:
	grass.run_command('r.support', map = output, history = "  units=%s (adjusted scale=%s)" % (units, scale))

    # write cmd history:
    grass.raster_history(output)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
