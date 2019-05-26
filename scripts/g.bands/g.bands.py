#!/usr/bin/env python

############################################################################
#
# MODULE:       g.bands
# AUTHOR(S):    Martin Landa <landa.martin gmail com>
#
# PURPOSE:      Print bands references.
#
# COPYRIGHT:    (C) 2019 by mundialis GmbH & Co.KG, and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

#%module
#% description: Prints bands references.
#% keyword: general
#% keyword: imagery
#% keyword: image collections
#% keyword: bands
#%end
#%option
#% key: bands
#% type: string
#% key_desc: name
#% description: Name of band identifier (example: S2A or S2A_1)
#% required: no
#% multiple: no
#%end

import sys

import grass.script as gs

def main():
    gs.utils.set_path('g.bands')
    from reader import BandReader

    band = None
    kwargs = {}
    if ',' in options['bands']:
        gs.fatal("Multiple values not supported")
    if '_' in options['bands']:
        kwargs['shortcut'], kwargs['band'] = options['bands'].split('_')
    else:
        kwargs['shortcut'] = options['bands']

    reader = BandReader()
    reader.print_info(**kwargs)

if __name__ == "__main__":
    options, flags = gs.parser()

    sys.exit(
        main()
    )
