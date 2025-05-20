#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.rast.algebra
# AUTHOR(S):    Thomas Leppelt, Soeren Gebbert
#
# PURPOSE:      Provide temporal raster algebra to perform spatial an temporal operations
#               for space time datasets by topological relationships to other space time
#               datasets.
# COPYRIGHT:    (C) 2014-2017 by the GRASS Development Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#############################################################################

# %module
# % description: Apply temporal and spatial operations on space time raster datasets using temporal raster algebra.
# % keyword: temporal
# % keyword: algebra
# % keyword: raster
# % keyword: time
# %end

# %option
# % key: expression
# % type: string
# % description: r.mapcalc expression for temporal and spatial analysis of space time raster datasets
# % required : yes
# %end

# %option
# % key: basename
# % type: string
# % label: Basename of the new generated output maps
# % description: A numerical suffix separated by an underscore will be attached to create a unique identifier
# % required: yes
# %end

# %option
# % key: suffix
# % type: string
# % description: Suffix to add at basename: set 'gran' for granularity, 'time' for the full time format, 'num' for numerical suffix with a specific number of digits (default %05)
# % answer: num
# % required: no
# % multiple: no
# %end

# %option
# % key: nprocs
# % type: integer
# % description: Number of r.mapcalc processes to run in parallel
# % required: no
# % multiple: no
# % answer: 1
# %end

# %flag
# % key: s
# % description: Check the spatial topology of temporally related maps and process only spatially related maps
# %end

# %flag
# % key: n
# % description: Register Null maps
# %end

# %flag
# % key: g
# % description: Use granularity sampling instead of the temporal topology approach
# %end

# %flag
# % key: d
# % description: Perform a dry run, compute all dependencies and module calls but don't run them
# %end

import sys

import grass.script as gs


def main():
    # lazy imports
    import grass.temporal as tgis

    expression = options["expression"]
    basename = options["basename"]
    nprocs = options["nprocs"]
    time_suffix = options["suffix"]
    spatial = flags["s"]
    register_null = flags["n"]
    granularity = flags["g"]
    dry_run = flags["d"]

    # Check for PLY installation
    try:
        # Intentionally unused imports
        from ply import lex  # noqa: F401
        from ply import yacc  # noqa: F401
    except ImportError:
        gs.fatal(
            _(
                "Please install PLY (Lex and Yacc Python implementation) to use the "
                "temporal algebra modules. You can use t.rast.mapcalc that provides a "
                "limited but useful alternative to t.rast.algebra without PLY "
                "requirement."
            )
        )

    tgis.init(True)
    p = tgis.TemporalRasterAlgebraParser(
        run=True,
        debug=False,
        spatial=spatial,
        nprocs=nprocs,
        register_null=register_null,
        dry_run=dry_run,
        time_suffix=time_suffix,
    )

    if granularity:
        if not p.setup_common_granularity(
            expression=expression, lexer=tgis.TemporalRasterAlgebraLexer()
        ):
            gs.fatal(_("Unable to process the expression in granularity algebra mode"))

    pc = p.parse(expression, basename, gs.overwrite())

    if dry_run is True:
        import pprint

        pprint.pprint(pc)


if __name__ == "__main__":
    options, flags = gs.parser()
    sys.exit(main())
