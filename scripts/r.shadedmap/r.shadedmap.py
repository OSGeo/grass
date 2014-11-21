#!/usr/bin/env python
#
############################################################################
#
# MODULE:        r.shadedmap
# AUTHOR(S):     Hamish Bowman
#                Vaclav Petras <wenzeslaus gmail com>
#                Inspired by d.shadedmap
# PURPOSE:       Uses r.his to drape a color raster over a shaded relief map
# COPYRIGHT:     (C) 2014 by Hamish Bowman, and the GRASS Development Team
#
#                This program is free software under the GNU General Public
#                License (>=v2). Read the file COPYING that comes with GRASS
#                for details.
#
#############################################################################

#%module
#% description: Drapes a color raster over an shaded relief or aspect map.
#% keywords: raster
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
#%option G_OPT_R_OUTPUT
#% description: Name of shaded raster map
#%end
#%option
#% key: brighten
#% type: integer
#% description: Percent to brighten
#% options: -99-99
#% answer: 0
#%end


import os
from grass.script import core as gcore
from grass.script import raster as grast
from grass.exceptions import CalledModuleError


def remove(maps):
    """Remove raster maps"""
    gcore.run_command('g.remove', flags='f', quiet=True,
                      type='rast', name=maps)


def main():
    options, unused = gcore.parser()

    drape_map = options['drapemap']
    relief_map = options['reliefmap']
    brighten = int(options['brighten'])
    output_map = options['output']

    to_remove = []
    try:
        unique_name = 'tmp__rshadedmap_%d' % os.getpid()
        tmp_base = '%s_drape' % unique_name
        tmp_r = tmp_base + '.r'
        tmp_g = tmp_base + '.g'
        tmp_b = tmp_base + '.b'

        if brighten:
            # steps taken from r.his manual page
            # how much they are similar with d.shadedmap/d.his is unknown
            # perhaps even without brightness, there can be some differences
            # comparing to d.shadedmap
            relief_map_tmp = '%s_relief' % unique_name
            # convert [-99, -99] to [0.01, 1.99]
            brighten = 1 + brighten / 100.
            grast.mapcalc('{n} = {c} * #{o}'.format(
                n=relief_map_tmp, o=relief_map, c=brighten))
            gcore.run_command('r.colors', map=relief_map_tmp, color='grey255')
            relief_map = relief_map_tmp
            to_remove.append(relief_map_tmp)
        gcore.run_command('r.his', hue=drape_map, intensity=relief_map,
                          red=tmp_r, green=tmp_g, blue=tmp_b)
        to_remove.extend([tmp_r, tmp_g, tmp_b])
        gcore.run_command('r.composite', red=tmp_r, green=tmp_g,
                          blue=tmp_b, output=output_map)
        remove(to_remove)  # who knows if finally is called when exit
    except CalledModuleError, error:
        remove(to_remove)
        # TODO: implement module name to CalledModuleError
        gcore.fatal(_("Module %s failed. Check the above error messages.") % error.args)


if __name__ == "__main__":
    main()
