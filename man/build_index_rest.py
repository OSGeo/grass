#!/usr/bin/env python3

# generates docs/rest/index.txt
# (C) 2003-2012 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements
#   Luca Delucchi

import os

from build_rest import *

os.chdir(rest_dir)

filename = "index.txt"
f = open(filename + ".tmp", "w")

write_rest_header(f, "GRASS GIS %s Reference Manual" % grass_version, True)
write_rest_cmd_overview(f)
write_rest_footer(f, "index.txt")

f.close()
replace_file(filename)
