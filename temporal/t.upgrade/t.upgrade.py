#!/usr/bin/env python3
############################################################################
#
# MODULE:       t.upgrade
# AUTHOR(S):    Martin Landa, Markus Neteler
#
# PURPOSE:      Upgrade of TGRASS DB
# SPDX-FileCopyrightText: 2020-2021 Martin Landa
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later
#
#############################################################################

# %module
# % description: Upgrades the version of the temporal database.
# % keyword: temporal
# % keyword: metadata
# % keyword: time
# %end

import grass.script as gs


def main():
    # lazy imports
    import grass.temporal as tgis

    mapset = gs.gisenv()["MAPSET"]
    tgis.init(skip_db_init=True, skip_db_version_check=True)

    dbif = tgis.SQLDatabaseInterfaceConnection(mapsets=mapset)
    if not dbif.tgis_mapsets:
        gs.message(_("No temporal database found in the current mapset."))
        return
    dbif.connect()

    tgis.upgrade_temporal_database(dbif)


if __name__ == "__main__":
    gs.parser()
    main()
