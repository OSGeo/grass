"""

(C) 2025 by Nicklas Larsson and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.


This is not a stable part of the API. Use at your own risk.

The "@...@" variables are being substituted during build process

"""

import os

GRASS_PREFIX = None
GISBASE = None


def set_resource_paths():
    global GRASS_PREFIX, GISBASE

    GRASS_PREFIX = "@GRASS_PREFIX@"

    if "GISBASE" in os.environ and len(os.getenv("GISBASE")) > 0:
        GISBASE = os.path.normpath(os.environ["GISBASE"])
    else:
        GISBASE = os.path.normpath(os.path.join(GRASS_PREFIX, "@GISBASE_INSTALL_PATH@"))
        os.environ["GISBASE"] = GISBASE
