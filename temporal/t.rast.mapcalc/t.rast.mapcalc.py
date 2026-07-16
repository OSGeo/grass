#!/usr/bin/env python3

############################################################################
#
# MODULE:	t.rast.mapcalc
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Perform spatio-temporal mapcalc expressions on temporal sampled maps of space time raster datasets
# SPDX-FileCopyrightText: 2012-2017 Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#############################################################################

# %module
# % description: Performs spatio-temporal mapcalc expressions on temporally sampled maps of space time raster datasets.
# % keyword: temporal
# % keyword: algebra
# % keyword: raster
# % keyword: time
# %end

# %option G_OPT_STRDS_INPUTS
# %end

# %option
# % key: expression
# % type: string
# % description: Spatio-temporal mapcalc expression
# % required: yes
# % multiple: no
# %end

# %option G_OPT_T_SAMPLE
# % key: method
# % answer: equal
# %end

# %option G_OPT_T_WHERE
# %end

# %option G_OPT_STRDS_OUTPUT
# %end

# %option G_OPT_R_BASENAME_OUTPUT
# % key: basename
# % label: Basename for output raster maps
# % description: A numerical suffix separated by an underscore will be attached to create a unique identifier
# % required: yes
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
# % key: n
# % description: Register Null maps
# %end

# %flag
# % key: s
# % description: Check the spatial topology of temporally related maps and process only spatially related maps
# %end

import grass.script as gs

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    inputs = options["inputs"]
    output = options["output"]
    expression = options["expression"]
    base = options["basename"]
    method = options["method"]
    nprocs = int(options["nprocs"])
    register_null = flags["n"]
    spatial = flags["s"]
    where = options.get("where")

    # Create the method list
    method = method.split(",")

    # Make sure the temporal database exists
    tgis.init()

    tgis.dataset_mapcalculator(
        inputs,
        output,
        "raster",
        expression,
        base,
        method,
        nprocs,
        register_null,
        spatial,
        where,
    )


###############################################################################

if __name__ == "__main__":
    options, flags = gs.parser()
    main()

