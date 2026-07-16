#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.vect.algebra
# AUTHOR(S):    Thomas Leppelt, Soeren Gebbert
#
# PURPOSE:      Provide temporal vector algebra to perform spatial and temporal operations
#               for space time datasets by topological relationships to other space time
#               datasets.
# SPDX-FileCopyrightText: 2014-2017 Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#############################################################################

# %module
# % description: Apply temporal and spatial operations on space time vector datasets using temporal vector algebra.
# % keyword: temporal
# % keyword: algebra
# % keyword: vector
# % keyword: time
# %end

# %option
# % key: expression
# % type: string
# % description: Spatio-temporal mapcalc expression
# % key_desc: expression
# % required : yes
# %end

# %option
# % key: basename
# % type: string
# % label: Basename of the new generated output maps
# % description: A numerical suffix separated by an underscore will be attached to create a unique identifier
# % key_desc: basename
# % required : yes
# %end

# %flag
# % key: s
# % description: Check the spatial topology of temporally related maps and process only spatially related maps
# %end

import sys

import grass.script as gs


def main():
    # lazy imports
    import grass.temporal as tgis

    expression = options["expression"]
    basename = options["basename"]
    spatial = flags["s"]

    tgis.init(True)
    p = tgis.TemporalVectorAlgebraParser(run=True, debug=False, spatial=spatial)
    p.parse(expression, basename, gs.overwrite())


if __name__ == "__main__":
    options, flags = gs.parser()
    sys.exit(main())

