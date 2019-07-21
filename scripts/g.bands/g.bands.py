#!/usr/bin/env python

############################################################################
#
# MODULE:       g.bands
# AUTHOR(S):    Martin Landa <landa.martin gmail com>
#
# PURPOSE:      Prints band reference information.
#
# COPYRIGHT:    (C) 2019 by mundialis GmbH & Co.KG, and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

#%module
#% description: Prints band reference information.
#% keyword: general
#% keyword: imagery
#% keyword: image collections
#% keyword: bands
#%end
#%option
#% key: band
#% type: string
#% key_desc: name
#% description: Name of band reference identifier (example: S2, S2A or S2A_1)
#% required: no
#% multiple: no
#%end
#%flag
#% key: e
#% description: Print extended metadata information
#%end

import sys

import grass.script as gs

def main():
    from grass.bands import BandReader, BandReaderError

    band = None
    kwargs = {}
    if ',' in options['band']:
        gs.fatal("Multiple values not supported")
    if '_' in options['band']:
        kwargs['shortcut'], kwargs['band'] = options['band'].split('_')
    else:
        kwargs['shortcut'] = options['band']
    kwargs['extended'] = flags['e']

    try:
        reader = BandReader()
        reader.print_info(**kwargs)
    except BandReaderError as e:
        gs.fatal(e)

    return 0

if __name__ == "__main__":
    options, flags = gs.parser()

    sys.exit(
        main()
    )
