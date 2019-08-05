#!/usr/bin/env python3

# checks for HTML files missing DESCRIPTION section
# (c) The GRASS Development Team, Markus Neteler, Glynn Clements 2003, 2004, 2005, 2006, 2009

import sys
import os
import string

from build_html import *

os.chdir(html_dir)

sys.stdout.write(message_tmpl.substitute(html_dir = html_dir))

for cmd in html_files('*'):
    if "DESCRIPTION" not in read_file(cmd):
        sys.stdout.write("%s\n" % cmd[:-5])

sys.stdout.write(r"""
----------------------------------------------------------------------
""")
