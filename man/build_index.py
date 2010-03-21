#!/usr/bin/env python

# generates docs/html/index.html
# (c) The GRASS Development Team, Markus Neteler, Glynn Clements 2003, 2004, 2005, 2006, 2009

import sys
import os
import string

from build_html import *

os.chdir(html_dir)

filename = "index.html"
f = open(filename + ".tmp", 'wb')

write_html_header(f, "GRASS GIS %s Reference Manual" % grass_version, True)
write_html_cmd_overview(f)
write_html_footer(f, "index.html")

f.close()
replace_file(filename)

