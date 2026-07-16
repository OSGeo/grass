#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.select
# AUTHOR(S):    Thomas Leppelt, Soeren Gebbert
#
# PURPOSE:      Select maps from space time datasets by topological relationships to
#               other space time datasets using temporal algebra.
# SPDX-FileCopyrightText: 2011-2017 Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#############################################################################

# %module
# % description: Select maps from space time datasets by topological relationships to other space time datasets using temporal algebra.
# % keyword: temporal
# % keyword: metadata
# % keyword: time
# %end

# %option G_OPT_STDS_TYPE
# % guidependency: input
# % guisection: Required
# %end

# %option
# % key: expression
# % type: string
# % description: The temporal mapcalc expression
# % key_desc: expression
# % required : yes
# %end

# %flag
# % key: s
# % description: Check the spatial topology of temporally related maps and select only spatially related maps
# %end

# %flag
# % key: d
# % description: Perform a dry run, compute all dependencies and module calls but don't run them
# %end


import sys

import grass.script as gs

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    expression = options["expression"]
    spatial = flags["s"]
    dry_run = flags["d"]
    stdstype = options["type"]

    tgis.init(True)
    p = tgis.TemporalAlgebraParser(
        run=True, debug=False, spatial=spatial, dry_run=dry_run
    )
    pc = p.parse(expression, stdstype, overwrite=gs.overwrite())

    if dry_run is True:
        import pprint

        pprint.pprint(pc)


if __name__ == "__main__":
    options, flags = gs.parser()
    sys.exit(main())

