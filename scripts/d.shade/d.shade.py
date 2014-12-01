#!/usr/bin/env python

############################################################################
#
# MODULE:        d.shade
# AUTHOR(S):        Unknown; updated to GRASS 5.7 by Michael Barton
#                Converted to Python by Glynn Clements
# PURPOSE:        Uses d.his to drape a color raster over a shaded relief map
# COPYRIGHT:     (C) 2004-2013 by the GRASS Development Team
#
#                This program is free software under the GNU General Public
#                License (>=v2). Read the file COPYING that comes with GRASS
#                for details.
#
#############################################################################

#%module
#% description: Drapes a color raster over an shaded relief or aspect map.
#% keywords: display
#% keywords: elevation
#% keywords: relief
#% keywords: hillshade
#% keywords: visualization
#%end
#%option G_OPT_R_INPUT
#% key: reliefmap
#% description: Name of shaded relief or aspect raster map
#%end
#%option G_OPT_R_INPUT
#% key: drapemap
#% label: Name of raster to drape over relief raster map
#% description: Typically, this raster is elevation or other colorful raster
#%end
#%option
#% key: brighten
#% type: integer
#% description: Percent to brighten
#% options: -99-99
#% answer: 0
#%end


from grass.script import core as gcore
from grass.exceptions import CalledModuleError


def main():
    options, unused = gcore.parser()

    drape_map = options['drapemap']
    relief_map = options['reliefmap']
    brighten = options['brighten']

    try:
        gcore.run_command('d.his', hue=drape_map, intensity=relief_map,
                          brighten=brighten)
    except CalledModuleError:
        gcore.fatal(_("Module %s failed. Check the above error messages.") % 'd.his')


if __name__ == "__main__":
    main()
