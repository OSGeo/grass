#!/usr/bin/env python3

# checks for HTML files missing DESCRIPTION section
# (C) 2003-2009 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements

import sys
import os

from build import message_tmpl, get_files, read_file
from build_html import man_dir

os.chdir(man_dir)

sys.stdout.write(message_tmpl.substitute(man_dir=man_dir))

for cmd in get_files(man_dir, "*"):
    if "DESCRIPTION" not in read_file(cmd):
        sys.stdout.write("%s\n" % cmd[:-5])

sys.stdout.write(
    r"""
----------------------------------------------------------------------
"""
)
