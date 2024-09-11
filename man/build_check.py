#!/usr/bin/env python3

# checks for HTML files missing DESCRIPTION section
# (C) 2003-2009 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements

import sys
import os

from build_html import html_dir, message_tmpl, html_files, read_file

os.chdir(html_dir)

sys.stdout.write(message_tmpl.substitute(html_dir=html_dir))

for cmd in html_files("*"):
    if "DESCRIPTION" not in read_file(cmd):
        sys.stdout.write("%s\n" % cmd[:-5])

sys.stdout.write(
    r"""
----------------------------------------------------------------------
"""
)
