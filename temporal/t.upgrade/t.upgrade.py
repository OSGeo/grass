#!/usr/bin/env python3
############################################################################
#
# MODULE:       t.upgrade
# AUTHOR(S):    Martin Landa, Markus Neteler
#
# PURPOSE:      Upgrade of TGRASS DB
# SPDX-FileCopyrightText: 2020-2021 Martin Landa
# SPDX-FileCopyrightText: Other GRASS authors
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

    tgis.init(skip_db_version_check=True)

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    tgis.upgrade_temporal_database(dbif)


if __name__ == "__main__":
    gs.parser()
    main()
