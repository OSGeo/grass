#!/usr/bin/env python3

# generates docs/html/index.html
# (C) 2003-2009 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements

import sys
import os

from build_html import (
    html_dir,
    grass_version,
    write_html_header,
    write_html_cmd_overview,
    write_html_footer,
    replace_file,
)

os.chdir(html_dir)

filename = "index.html"

year = None
if len(sys.argv) > 1:
    year = sys.argv[1]

with open(filename + ".tmp", "w") as f:
    write_html_header(f, "GRASS GIS %s Reference Manual" % grass_version, True)
    write_html_cmd_overview(f)
    write_html_footer(f, "index.html", year)

replace_file(filename)
