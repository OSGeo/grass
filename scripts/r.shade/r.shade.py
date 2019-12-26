#!/usr/bin/env python3
#
############################################################################
#
# MODULE:        r.shade
# AUTHOR(S):     Hamish Bowman
#                Vaclav Petras <wenzeslaus gmail com>
#                Inspired by d.shade (formerly d.shadedmap)
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
#% keyword: raster
#% keyword: elevation
#% keyword: relief
#% keyword: hillshade
#% keyword: visualization
#%end
#%option G_OPT_R_INPUT
#% key: shade
#% description: Name of shaded relief or aspect raster map
#%end
#%option G_OPT_R_INPUT
#% key: color
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
#%option
#% key: bgcolor
#% type: string
#% key_desc: name
#% label: Color to use instead of NULL values
#% description: Either a standard color name, R:G:B triplet, or "none"
#% gisprompt: old,color_none,color
#%end
#%flag
#% key: c
#% description: Use colors from color tables for NULL values
#%end
#%rules
#% exclusive: bgcolor, -c
#%end

# TODO: bgcolor is not using standard option because it has default white
# using `answer:` will cause `default:` which is not the same as no default


import os
from grass.script import core as gcore
from grass.script import raster as grast
from grass.exceptions import CalledModuleError


def remove(maps):
    """Remove raster maps"""
    gcore.run_command('g.remove', flags='f', quiet=True,
                      type='rast', name=maps)


def main():
    options, flags = gcore.parser()

    drape_map = options['color']
    relief_map = options['shade']
    brighten = int(options['brighten'])
    output_map = options['output']
    bgcolor = options['bgcolor']

    rhis_extra_args = {}
    if bgcolor:
        rhis_extra_args['bgcolor'] = bgcolor
    if flags['c']:
        rhis_extra_args['flags'] = 'c'

    to_remove = []
    try:
        unique_name = 'tmp__rshade_%d' % os.getpid()
        tmp_base = '%s_drape' % unique_name
        tmp_r = tmp_base + '.r'
        tmp_g = tmp_base + '.g'
        tmp_b = tmp_base + '.b'

        if brighten:
            # steps taken from r.his manual page
            # how much they are similar with d.shade/d.his is unknown
            # perhaps even without brightness, there can be some differences
            # comparing to d.shade
            relief_map_tmp = '%s_relief' % unique_name
            # convert [-99, -99] to [0.01, 1.99]
            brighten = 1 + brighten / 100.
            grast.mapcalc('{n} = {c} * #{o}'.format(
                n=relief_map_tmp, o=relief_map, c=brighten))
            gcore.run_command('r.colors', map=relief_map_tmp, color='grey255')
            relief_map = relief_map_tmp
            to_remove.append(relief_map_tmp)
        gcore.run_command('r.his', hue=drape_map, intensity=relief_map,
                          red=tmp_r, green=tmp_g, blue=tmp_b,
                          **rhis_extra_args)
        to_remove.extend([tmp_r, tmp_g, tmp_b])
        gcore.run_command('r.composite', red=tmp_r, green=tmp_g,
                          blue=tmp_b, output=output_map)
        remove(to_remove)  # who knows if finally is called when exit
    except CalledModuleError as error:
        remove(to_remove)
        # TODO: implement module name to CalledModuleError
        gcore.fatal(_("Module %s failed. Check the above error messages.") %
                    error.cmd)


if __name__ == "__main__":
    main()
