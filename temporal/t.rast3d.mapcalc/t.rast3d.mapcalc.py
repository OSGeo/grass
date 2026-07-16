#!/usr/bin/env python3

############################################################################
#
# MODULE:	t.rast3d.mapcalc
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Perform r3.mapcalc expressions on maps of sampled space time raster3d datasets
# SPDX-FileCopyrightText: 2012-2017 Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#############################################################################

# %module
# % description: Performs r3.mapcalc expressions on maps of sampled space time 3D raster datasets.
# % keyword: temporal
# % keyword: algebra
# % keyword: raster3d
# % keyword: voxel
# % keyword: time
# %end

# %option G_OPT_STR3DS_INPUTS
# %end

# %option
# % key: expression
# % type: string
# % description: r3.mapcalc expression applied to each time step of the sampled data
# % required: yes
# % multiple: no
# %end

# %option G_OPT_T_SAMPLE
# % key: method
# % answer: during,overlap,contain,equal
# %end

# %option G_OPT_STR3DS_OUTPUT
# %end

# %option
# % key: basename
# % type: string
# % label: Basename of the new generated output maps
# % description: A numerical suffix separated by an underscore will be attached to create a unique identifier
# % required: yes
# % multiple: no
# %end

# %option
# % key: nprocs
# % type: integer
# % description: Number of r3.mapcalc processes to run in parallel
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

    # Create the method list
    method = method.split(",")

    # Make sure the temporal database exists
    tgis.init()

    tgis.dataset_mapcalculator(
        inputs,
        output,
        "raster3d",
        expression,
        base,
        method,
        nprocs,
        register_null,
        spatial,
    )


###############################################################################

if __name__ == "__main__":
    options, flags = gs.parser()
    main()

