#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.downgrade
# AUTHOR(S):    Martin Landa, Markus Neteler, Markus Metz
# PURPOSE:      Downgrade of TGRASS DB
# COPYRIGHT:    (C) 2019-2021 by Martin Landa, and the GRASS Development Team
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
############################################################################

#%module
#% description: Downgrades the version of a space time dataset (TGRASS DB).
#% keyword: temporal
#% keyword: metadata
#% keyword: time
#%end

import grass.script as gs


def main():
    # lazy imports
    import grass.temporal as tgis

    tgis.init(skip_db_version_check=True)

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    tgis.downgrade_temporal_database(dbif)

    return 0

if __name__ == "__main__":
    gs.parser()
    main()
