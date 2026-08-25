#!/usr/bin/env python3

############################################################################
#
# MODULE:       mkrest.py
# AUTHOR(S):    Luca Delucchi
# PURPOSE:      Create HTML manual page snippets
# COPYRIGHT:    (C) 2012 by Luca Delucchi
#                and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

import re
import string
import subprocess
import sys
from datetime import datetime
from pathlib import Path

pgm = sys.argv[1]
year = sys.argv[2] if len(sys.argv) > 1 else str(datetime.now().year)

src_file = "%s.html" % pgm
tmp_file = "%s.tmp.txt" % pgm

# TODO add copyright

footer_index = string.Template(
    """

:doc:`Main Page <index>` - :doc:`${INDEXNAMECAP} index <${INDEXNAME}>` - :doc:`Full index <full_index>`
2003-${YEAR} `GRASS Development Team <https://grass.osgeo.org>`_
"""  # noqa: E501
)

footer_noindex = string.Template(
    """

:doc:`Main Page <index>`  - :doc:`Full index <full_index>`
2003-${YEAR} `GRASS Development Team <https://grass.osgeo.org>`_
"""
)


def read_file(name):
    try:
        return Path(name).read_bytes()
    except OSError:
        return ""


replacement = {
    "*`": "`",
    "`* `": "`",
    ">`_*": ">`_",
    ">`_,*": ">`_,",
    r'``*\ "': '``"',
    "***": "**",
}

src_data = read_file(src_file)

title = re.search(r"(<!-- meta page description:)(.*)(-->)", src_data, re.IGNORECASE)

if title:
    title_name = title.group(2).strip()
    sys.stdout.write("%s\n" % title_name)
    title_style = "=" * (len(title_name) + 2)
    sys.stdout.write("%s\n\n" % title_style)

tmp_data = read_file(tmp_file)
if tmp_data:
    sys.stdout.write(tmp_data)

arguments = ["pandoc", "-s", "-r", "html", src_file, "-w", "rst"]
with subprocess.Popen(arguments, stdout=subprocess.PIPE) as process:
    html_text = process.communicate()[0]

if html_text:
    for k, v in replacement.iteritems():
        html_text = html_text.replace(k, v)

# TODO remove with space if string start with it, " vector..." -> "vector..."
#     not if it is a tab: "     vector...." -> "    vector..."

#    for line in html_text.splitlines(True):
#        sys.stdout.write("%s" % line.lstrip())

sys.stdout.write(html_text)

index_names = {
    "d": "display",
    "db": "database",
    "g": "general",
    "i": "imagery",
    "m": "miscellaneous",
    "ps": "postscript",
    "p": "paint",
    "r": "raster",
    "r3": "raster3D",
    "s": "sites",
    "t": "temporal",
    "v": "vector",
}

index = re.search(r"(<!-- meta page index:)(.*)(-->)", src_data, re.IGNORECASE)

if index:
    index_name = index.group(2).strip()
else:
    mod_class = pgm.split(".", 1)[0]
    index_name = index_names.get(mod_class, "")

if index_name:
    sys.stdout.write(
        footer_index.substitute(
            INDEXNAME=index_name, INDEXNAMECAP=index_name.title(), YEAR=year
        )
    )
else:
    sys.stdout.write(footer_noindex.substitute(YEAR=year))

sys.exit()
