# Copyright (C) 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Open a vector map in update mode, close it, and check for a leaked sidx handle

Run in a GRASS session by ``grass_pygrass_vector_sidx_test.py`` in a
subprocess, because the vector library state is per process.
"""

import os
import sys
from pathlib import Path

from grass.pygrass.vector import VectorTopo


def sidx_is_open(path):
    """Return True if this process still has the file open"""
    if sys.platform.startswith("linux"):
        # A handle to a replaced file is reported with a " (deleted)" suffix.
        return any(
            os.path.realpath(fd).startswith(path)
            for fd in Path("/proc/self/fd").iterdir()
        )
    # Windows refuses to delete an open file.
    try:
        os.remove(path)
    except PermissionError:
        return True
    return False


def main():
    """Create a map, reopen it in update mode, and check the sidx file"""
    vect = VectorTopo("sidx_map")
    vect.open("w")
    vect.close()
    vect.open("rw")
    vect.close()
    mapset_path = os.environ["MAPSET_PATH"]
    path = os.path.realpath(os.path.join(mapset_path, "vector", "sidx_map", "sidx"))
    if sidx_is_open(path):
        print(f"The file {path} was left open after closing the map")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
