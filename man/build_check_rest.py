#!/usr/bin/env python3

# checks for HTML files missing DESCRIPTION section
# (C) 2003-2009 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements

import sys
import os

from build_rest import rest_dir, message_tmpl, rest_files, read_file

os.chdir(rest_dir)

sys.stdout.write(message_tmpl.substitute(rest_dir=rest_dir))

for cmd in rest_files("*"):
    if "DESCRIPTION" not in read_file(cmd):
        sys.stdout.write("%s\n" % cmd[:-5])

sys.stdout.write(
    r"""
----------------------------------------------------------------------
"""
)
