#!/usr/bin/env python3

# generates docs/rest/index.txt
# (C) 2003-2012 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements
#   Luca Delucchi

import os

from build_rest import (
    rest_dir,
    grass_version,
    write_rest_header,
    write_rest_cmd_overview,
    write_rest_footer,
    replace_file,
)

os.chdir(rest_dir)

filename = "index.txt"
with open(filename + ".tmp", "w") as f:
    write_rest_header(f, "GRASS GIS %s Reference Manual" % grass_version, True)
    write_rest_cmd_overview(f)
    write_rest_footer(f, "index.txt")

replace_file(filename)
