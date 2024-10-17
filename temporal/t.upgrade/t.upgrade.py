#!/usr/bin/env python3
############################################################################
#
# MODULE:       t.upgrade
# AUTHOR(S):    Martin Landa, Markus Neteler
#
# PURPOSE:      Upgrade of TGRASS DB
# COPYRIGHT:    (C) 2020-2021 by Martin Landa, and the GRASS Development Team
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
