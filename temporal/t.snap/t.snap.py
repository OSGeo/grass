#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.snap
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Temporally snap the maps of a space time dataset.
# SPDX-FileCopyrightText: 2013-2017 GRASS Development Team
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

    mapset = gs.gisenv()["MAPSET"]

    # Try initializing the temporal database in the current mapset
    tgis.init(skip_db_init=True)
    dbif = tgis.SQLDatabaseInterfaceConnection(mapsets=mapset)
    if not dbif.tgis_mapsets:
        gs.fatal(_("No temporal database found in the current mapset."))
    dbif.connect()

    stds = tgis.open_old_stds(name, type, dbif)
    stds.snap(dbif=dbif)

    stds.update_command_string(dbif=dbif)
    dbif.close()


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
