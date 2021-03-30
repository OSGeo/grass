"""
Manage GRASS GIS session.

(C) 2020 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Linda Kladivova <linda.kladivova seznam cz>
"""

import os
import sys

### TODO move SetSessionMapset and GetVersion from guiutils here

def read_gisrc():
    """Read variables from a current GISRC file

    Returns a dictionary representation of the file content.
    """
    grassrc = {}

    gisrc = os.getenv("GISRC")

    try:
        rc = open(gisrc, "r")
    except IOError:
        return grassrc

    if gisrc and os.path.isfile(gisrc):
        try:
            rc = open(gisrc, "r")
            for line in rc.readlines():
                try:
                    key, val = line.split(":", 1)
                except ValueError as e:
                    sys.stderr.write(
                        _('Invalid line in GISRC file (%s):%s\n' % (e, line)))
                grassrc[key.strip()] = val.strip()
        finally:
            rc.close()

    return grassrc