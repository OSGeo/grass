#!/usr/bin/env python3

# checks for HTML files missing DESCRIPTION section
# SPDX-FileCopyrightText: 2003-2009 Markus Neteler
# SPDX-FileCopyrightText: Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
# Authors:
#   Markus Neteler
#   Glynn Clements

import os
import sys

from build_rest import message_tmpl, read_file, rest_dir, rest_files

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
