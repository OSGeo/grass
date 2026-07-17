#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.create
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Create a space time dataset
# SPDX-FileCopyrightText: 2011-2017 Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#
#############################################################################

# %module
# % description: Creates a space time dataset.
# % keyword: temporal
# % keyword: map management
# % keyword: create
# % keyword: time
# %end

# %option G_OPT_STDS_OUTPUT
# %end

# %option G_OPT_STDS_TYPE
# % description: Type of the output space time dataset
# %end

# %option G_OPT_T_TYPE
# %end

# %option
# % key: semantictype
# % type: string
# % description: Semantic type of the space time dataset
# % required: yes
# % multiple: no
# % options: min,max,sum,mean
# % answer: mean
# %end

# %option
# % key: title
# % type: string
# % description: Title of the new space time dataset
# % required: yes
# % multiple: no
# %end

# %option
# % key: description
# % type: string
# % description: Description of the new space time dataset
# % required: yes
# % multiple: no
# %end

import grass.script as gs

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    name = options["output"]
    type = options["type"]
    temporaltype = options["temporaltype"]
    title = options["title"]
    descr = options["description"]
    semantic = options["semantictype"]

    # Make sure the temporal database exists
    tgis.init()

    tgis.open_new_stds(
        name, type, temporaltype, title, descr, semantic, None, gs.overwrite()
    )


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
