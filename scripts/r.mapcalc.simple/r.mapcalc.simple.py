#!/usr/bin/env python3

"""
MODULE:    r.mapcalc.simple

AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
           R. Brunzema <r.brunzema web de> (original 5.0 version)
           Michael Barton <michael.barton asu edu> (update to GRASS 5.7)
           Huidae Cho <grass4u gmail com> (removed bashism)

PURPOSE:   Provides wrapper friendly wrapper to r.mapcalc

COPYRIGHT: (C) 2018 by Vaclav Petras and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

#%module
#% description: Calculates a new raster map from a simple r.mapcalc expression.
#% keyword: raster
#% keyword: algebra
#% keyword: simple
#%end
#%option
#% key: expression
#% type: string
#% description: Formula (e.g. A-B or A*C+B)
#% required : yes
#%end
#%option G_OPT_R_INPUT
#% key: a
#% description: Name of input A raster map
#% required : no
#% guisection: Input maps
#%end
#%option G_OPT_R_INPUT
#% key: b
#% description: Name of input B raster map
#% required : no
#% guisection: Input maps
#%end
#%option G_OPT_R_INPUT
#% key: c
#% description: Name of input C raster map
#% required : no
#% guisection: Input maps
#%end
#%option G_OPT_R_INPUT
#% key: d
#% description: Name of input D raster map
#% required : no
#% guisection: Input maps
#%end
#%option G_OPT_R_INPUT
#% key: e
#% description: Name of input E raster map
#% required : no
#% guisection: Input maps
#%end
#%option G_OPT_R_INPUT
#% key: f
#% description: Name of input F raster map
#% required : no
#% guisection: Input maps
#%end
#%option G_OPT_R_OUTPUT
#%end
#%option
#% key: seed
#% type: integer
#% required: no
#% multiple: no
#% description: Seed for rand() function
#% guisection: Random
#%end
#%flag
#% key: s
#% description: Generate random seed (result is non-deterministic)
#% guisection: Random
#%end
#%flag
#% key: q
#% description: Quote the map names
#% guisection: Input maps
#%end
#%flag
#% key: c
#% description: Case sensitive variable names
#%end

import sys
import re

import grass.script as gs


def name_quote(name):
    return '"{}"'.format(name)


def main():
    options, flags = gs.parser()
    expr = options['expression']
    if not expr:
        gs.fatal(_("The expression is an empty string"))
    output = options['output']
    quote = flags['q']
    re_flags = 0
    if flags['c']:
        re_flags = re.IGNORECASE

    if quote:
        output = name_quote(output)

    seed = None
    if options['seed']:
        seed = options['seed']
    elif flags['s']:
        seed = 'auto'

    variables = []
    for key in "ABCDEF":
        name = options[key.lower()]
        if name:
            if quote:
                name = name_quote(name)
            variables.append((key, name))

    for key, name in variables:
        find = r'([^a-zA-Z0-9]|^){key}([^a-zA-Z0-9]|$)'.format(key=key)
        replace = r'\1{}\2'.format(name)
        # we need to do the substitution twice because we are matching
        # also the char before and after which fails when there is only
        # one char between the two usages of the same var (e.g. A*A)
        expr = re.sub(find, replace, expr, flags=re_flags)
        expr = re.sub(find, replace, expr, flags=re_flags)

    expr = '{lhs} = {rhs}'.format(lhs=output, rhs=expr)
    gs.verbose(_("Expression: {}").format(expr))
    gs.mapcalc(expr, seed=seed)
    # g.message -e "Calculating $GIS_OPT_OUTFILE. Try expert mode."

    return 0


if __name__ == "__main__":
    sys.exit(main())

