#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.snap
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Temporally snap the maps of a space time dataset.
# SPDX-FileCopyrightText: 2013-2017 Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#
#############################################################################

# %module
# % description: Snaps temporally the maps of a space time dataset.
# % keyword: temporal
# % keyword: time management
# % keyword: snapping
# % keyword: time
# %end

# %option G_OPT_STDS_INPUT
# % description: Name of an existing space time dataset
# %end

# %option G_OPT_STDS_TYPE
# % guidependency: input
# % guisection: Required
# %end

import grass.script as gs

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    name = options["input"]
    type = options["type"]

    # Make sure the temporal database exists
    tgis.init()

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    stds = tgis.open_old_stds(name, type, dbif)
    stds.snap(dbif=dbif)

    stds.update_command_string(dbif=dbif)
    dbif.close()


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
