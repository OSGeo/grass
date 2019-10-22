#!/usr/bin/env python3

# generates docs/rest/index.txt
# (c) The GRASS Development Team, Markus Neteler, Glynn Clements 2003, 2004, 2005, 2006, 2009, Luca Delucchi 2012

import sys
import os
import string

from build_rest import *

os.chdir(rest_dir)

filename = "index.txt"
f = open(filename + ".tmp", 'w')

write_rest_header(f, "GRASS GIS %s Reference Manual" % grass_version, True)
write_rest_cmd_overview(f)
write_rest_footer(f, "index.txt")

f.close()
replace_file(filename)
