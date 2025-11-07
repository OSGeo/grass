#!/usr/bin/env python3

############################################################################
#
# MODULE:	t.vect.extract
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Extract a subset of a space time vector dataset
# COPYRIGHT:	(C) 2011-2017 by the GRASS Development Team
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
# % description: Extracts a subset of a space time vector dataset.
# % keyword: temporal
# % keyword: extract
# % keyword: vector
# % keyword: time
# %end

# %option G_OPT_STVDS_INPUT
# %end

# %option G_OPT_T_WHERE
# %end

# %option G_OPT_DB_WHERE
# % key: expression
# %end

# %option G_OPT_STVDS_OUTPUT
# %end

# %option G_OPT_V_FIELD
# %end

# %option G_OPT_V_TYPE
# %end

# %option
# % key: basename
# % type: string
# % label: Basename of the new generated output maps
# % description: A numerical suffix separated by an underscore will be attached to create a unique identifier
# % required: no
# % multiple: no
# %end

# %option
# % key: suffix
# % type: string
# % description: Suffix to add at basename: set 'gran' for granularity, 'time' for the full time format, 'num' for numerical suffix with a specific number of digits (default %05)
# % answer: gran
# % required: no
# % multiple: no
# %end

# %option
# % key: nprocs
# % type: integer
# % description: The number of v.extract processes to run in parallel. Use only if database backend is used which supports concurrent writing
# % required: no
# % multiple: no
# % answer: 1
# %end

# %flag
# % key: n
# % description: Register empty maps
# %end

import grass.script as gs

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    output = options["output"]
    where = options["where"]
    expression = options["expression"]
    layer = options["layer"]
    type = options["type"]
    base = options["basename"]
    nprocs = int(options["nprocs"])
    register_null = flags["n"]
    time_suffix = options["suffix"]

    # Make sure the temporal database exists
    tgis.init()

    tgis.extract_dataset(
        input,
        output,
        "vector",
        where,
        expression,
        base,
        time_suffix,
        nprocs,
        register_null,
        layer,
        type,
    )


###############################################################################

if __name__ == "__main__":
    options, flags = gs.parser()
    main()
