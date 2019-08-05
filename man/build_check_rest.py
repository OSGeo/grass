#!/usr/bin/env python3

# checks for HTML files missing DESCRIPTION section
# (c) The GRASS Development Team, Markus Neteler, Glynn Clements 2003, 2004, 2005, 2006, 2009

import sys
import os
import string

from build_rest import *

os.chdir(rest_dir)

sys.stdout.write(message_tmpl.substitute(rest_dir = rest_dir))

for cmd in rest_files('*'):
    if "DESCRIPTION" not in read_file(cmd):
        sys.stdout.write("%s\n" % cmd[:-5])

sys.stdout.write(r"""
----------------------------------------------------------------------
""")
