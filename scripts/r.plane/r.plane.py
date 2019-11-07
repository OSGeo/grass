#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	r.plane for GRASS 5.7; based on r.plane for GRASS 5
# AUTHOR(S):	1994, Stefan Jäger, University of Heidelberg/USGS
#               updated to GRASS 5.7 by Michael Barton
#               Dec 2004: Alessandro Frigeri & Ivan Marchesini
#               Modified to produce floating and double values maps
#               Converted to Python by Glynn Clements
# PURPOSE:	Creates a raster plane map from user specified inclination and azimuth
# COPYRIGHT:	(C) 2004-2012 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Creates raster plane map given dip (inclination), aspect (azimuth) and one point.
#% keyword: raster
#% keyword: elevation
#%end
#%option G_OPT_R_OUTPUT
#%end
#%option
#% key: dip
#% type: double
#% gisprompt: -90-90
#% answer: 0.0
#% description: Dip of plane in degrees
#% required : yes
#%end
#%option
#% key: azimuth
#% type: double
#% gisprompt: 0-360
#% answer: 0.0
#% description: Azimuth of the plane in degrees
#% required : yes
#%end
#%option
#% key: easting
#% type: double
#% description: Easting coordinate of a point on the plane
#% required : yes
#%end
#%option
#% key: northing
#% type: double
#% description: Northing coordinate of a point on the plane
#% required : yes
#%end
#%option
#% key: elevation
#% type: double
#% description: Elevation coordinate of a point on the plane
#% required : yes
#%end
#%option G_OPT_R_TYPE
#% answer: FCELL
#% required: no
#%end

import math
import string
import grass.script as gscript


def main():
    name = options['output']
    type = options['type']
    dip = float(options['dip'])
    az = float(options['azimuth'])
    try:
        ea = float(options['easting'])
        no = float(options['northing'])
    except ValueError:
        try:
            ea = float(gscript.utils.float_or_dms(options['easting']))
            no = float(gscript.utils.float_or_dms(options['northing']))
        except:
            gscript.fatal(_("Input coordinates seems to be invalid"))
    el = float(options['elevation'])

    # reg = gscript.region()

    ### test input values ###
    if abs(dip) >= 90:
        gscript.fatal(_("dip must be between -90 and 90."))

    if az < 0 or az >= 360:
        gscript.fatal(_("azimuth must be between 0 and 360"))

    # now the actual algorithm
    az_r = math.radians(-az)
    sinaz = math.sin(az_r)
    cosaz = math.cos(az_r)

    dip_r = math.radians(-dip)
    tandip = math.tan(dip_r)

    kx = sinaz * tandip
    ky = cosaz * tandip
    kz = el - ea * sinaz * tandip - no * cosaz * tandip

    if type == "CELL":
        round = "round"
        dtype = "int"
    elif type == "FCELL":
        round = ""
        dtype = "float"
    else:
        round = ""
        dtype = "double"

    gscript.mapcalc("$name = $type($round(x() * $kx + y() * $ky + $kz))",
                    name=name, type=dtype, round=round, kx=kx, ky=ky, kz=kz)

    gscript.run_command('r.support', map=name, history='')
    gscript.raster_history(name)

    gscript.message(_("Done."))
    t = string.Template("Raster map <$name> generated by r.plane "
                        "at point $ea E, $no N, elevation $el with dip = $dip"
                        " degrees and aspect = $az degrees ccw from north.")
    gscript.message(t.substitute(name=name, ea=ea, no=no, el=el, dip=dip,
                                 az=az))

if __name__ == "__main__":
    options, flags = gscript.parser()
    main()
