#!/usr/bin/env python3

############################################################################
#
# MODULE:       g.bands
# AUTHOR(S):    Martin Landa <landa.martin gmail com>
#
# PURPOSE:      Prints available band reference information used for multispectral data.
#
# COPYRIGHT:    (C) 2019 by mundialis GmbH & Co.KG, and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

# %module
# % description: Prints available band reference information used for multispectral data.
# % keyword: general
# % keyword: imagery
# % keyword: band reference
# % keyword: image collections
# %end
# %option
# % key: pattern
# % type: string
# % description: Band reference search pattern (examples: L, S2, .*_2, S2_1)
# % required: no
# % multiple: no
# %end
# %option
# % key: operation
# % type: string
# % required: no
# % multiple: no
# % options: print
# % description: Operation to be performed
# % answer: print
# %end
# %flag
# % key: e
# % description: Print extended metadata information
# %end

import sys

import grass.script as gs


def main():
    from grass.bandref import BandReferenceReader, BandReferenceReaderError

    kwargs = {}
    if "," in options["pattern"]:
        gs.fatal("Multiple values not supported")
    if "_" in options["pattern"]:
        # full band identifier specified
        kwargs["shortcut"], kwargs["band"] = options["pattern"].split("_")
    else:
        # pattern
        kwargs["shortcut"] = options["pattern"]
    kwargs["extended"] = flags["e"]

    if options["operation"] == "print":
        try:
            reader = BandReferenceReader()
            reader.print_info(**kwargs)
        except BandReferenceReaderError as e:
            gs.fatal(e)

    return 0


if __name__ == "__main__":
    options, flags = gs.parser()

    sys.exit(main())
