#!/usr/bin/env python3
############################################################################
#
# MODULE:    g.download.location
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
# PURPOSE:   Download and extract project (location) from web
# COPYRIGHT: (C) 2017-2024 by the GRASS Development Team
#
#    This program is free software under the GNU General
#    Public License (>=v2). Read the file COPYING that
#    comes with GRASS for details.
#
#############################################################################

"""Download GRASS projects"""

# %module
# % label: Download GRASS project (location) from the web
# % description: Get GRASS project from an URL or file path
# % keyword: general
# % keyword: data
# % keyword: download
# % keyword: import
# %end
# %option
# % key: url
# % multiple: no
# % type: string
# % label: URL of the archive with a project to be downloaded
# % description: URL of ZIP, TAR.GZ, or other similar archive
# % required: yes
# %end
# %option G_OPT_M_LOCATION
# % key: name
# % required: no
# % multiple: no
# % key_desc: name
# %end
# %option G_OPT_M_DBASE
# % key: path
# % required: no
# % multiple: no
# %end

import grass.script as gs


def main(options, unused_flags):
    """Download and copy project to destination"""
    gs.run_command("g.download.project", **options)


if __name__ == "__main__":
    main(*gs.parser())
