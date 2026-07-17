#!/usr/bin/env python3

############################################################################
#
# MODULE:	t.sample
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Sample the input space time dataset(s) with a sample space time dataset and print the result to stdout
# SPDX-FileCopyrightText: 2011-2017 Soeren Gebbert
# SPDX-FileCopyrightText: Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#
#############################################################################

# %module
# % description: Samples the input space time dataset(s) with a sample space time dataset and print the result to stdout.
# % keyword: temporal
# % keyword: sampling
# % keyword: time
# %end

# %option G_OPT_STDS_INPUTS
# %end

# %option G_OPT_STDS_INPUT
# % key: sample
# % description: Name of the sample space time dataset
# %end

# %option G_OPT_STDS_TYPE
# % key: intype
# % guisection: Required
# %end

# %option G_OPT_STDS_TYPE
# % key: samtype
# % guisection: Required
# % description: Type of the sample space time dataset
# %end

# %option G_OPT_T_SAMPLE
# % key: method
# % answer: during,overlap,contain,equal
# %end

# %option G_OPT_F_SEP
# % description: Field separator between output columns, default is tabular " | "
# % label: Do not use "," as this char is reserved to list several map ids in a sample granule
# %end

# %flag
# % key: c
# % description: Print the column names as first row
# %end

# %flag
# % key: s
# % description: Check for spatial topological overlap
# %end

import grass.script as gs

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    inputs = options["inputs"]
    sampler = options["sample"]
    samtype = options["samtype"]
    intype = options["intype"]
    separator = gs.separator(options["separator"])
    method = options["method"]
    header = flags["c"]
    spatial = flags["s"]

    # Make sure the temporal database exists
    tgis.init()

    tgis.sample_stds_by_stds_topology(
        intype, samtype, inputs, sampler, header, separator, method, spatial, True
    )


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
