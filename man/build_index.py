#!/usr/bin/env python3

# generates docs/html/index.html
# (C) 2003-2009 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements

import sys
import os

from build_html import (
    write_html_header,
    write_html_cmd_overview,
    write_html_footer,
    replace_file,
    html_dir,
    grass_version,
)
from build_md import (
    write_md_header,
    write_md_cmd_overview,
    write_md_footer,
    md_dir,
)

year = None
if len(sys.argv) > 1:
    year = sys.argv[1]


# HTML - to be removed
filename = "index.html"
os.chdir(html_dir)
with open(filename + ".tmp", "w") as f:
    write_html_header(f, "GRASS GIS %s Reference Manual" % grass_version, True)
    write_html_cmd_overview(f)
    write_html_footer(f, "index.html", year)
replace_file(filename)

filename = "index.md"
os.chdir(md_dir)
with open(filename + ".tmp", "w") as f:
    write_md_header(f, "GRASS GIS %s Reference Manual" % grass_version, True)
    write_md_cmd_overview(f)
    write_md_footer(f, "index.md", year)
replace_file(filename)
