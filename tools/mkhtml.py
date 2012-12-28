#!/usr/bin/env python

############################################################################
#
# MODULE:       mkhtml.py
# AUTHOR(S):    Markus Neteler
#               Glynn Clements
#               Martin Landa <landa.martin gmail.com>
# PURPOSE:      Create HTML manual page snippets
# COPYRIGHT:    (C) 2007, 2009, 2011-2012 by Glynn Clements
#                and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

import sys
import os
import string
import re
from datetime import datetime

pgm = sys.argv[1]
if len(sys.argv) > 1:
    year = sys.argv[2]
else:
    year = str(datetime.now().year)

src_file = "%s.html" % pgm
tmp_file = "%s.tmp.html" % pgm

header_base = """<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>GRASS GIS Manual: ${PGM}</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<link rel="stylesheet" href="grassdocs.css" type="text/css">
</head>
<body bgcolor="white">
<img src="grass_logo.png" alt="GRASS logo"><hr align=center size=6 noshade>
"""

header_nopgm = """<h2>${PGM}</h2>
"""

header_pgm = """<h2>NAME</h2>
<em><b>${PGM}</b></em>
"""

footer_index = string.Template(\
"""<hr>
<p><a href="index.html">Main index</a> - <a href="${INDEXNAME}.html">${INDEXNAMECAP} index</a> - <a href="topics.html">Topics index</a> - <a href="full_index.html">Full index</a></p>
<p>&copy; 2003-${YEAR} <a href="http://grass.osgeo.org">GRASS Development Team</a></p>
</body>
</html>
""")

footer_noindex = string.Template(\
"""<hr>
<p><a href="index.html">Main index</a> - <a href="topics.html">Topics index</a> - <a href="full_index.html">Full index</a></p>
<p>&copy; 2003-${YEAR} <a href="http://grass.osgeo.org">GRASS Development Team</a></p>
</body>
</html>
""")

def read_file(name):
    try:
        f = open(name, 'rb')
        s = f.read()
        f.close()
        return s
    except IOError:
        return ""

src_data = read_file(src_file)

name = re.search('(<!-- meta page name:)(.*)(-->)', src_data, re.IGNORECASE)
if name:
    pgm = name.group(2).strip().split('-', 1)[0].strip()
desc = re.search('(<!-- meta page description:)(.*)(-->)', src_data, re.IGNORECASE)
if desc:
    pgm = desc.group(2).strip()
    header_tmpl = string.Template(header_base + header_nopgm)
else:
    header_tmpl = string.Template(header_base + header_pgm)

if not re.search('<html>', src_data, re.IGNORECASE):
    tmp_data = read_file(tmp_file)
    if not re.search('<html>', tmp_data, re.IGNORECASE):
        sys.stdout.write(header_tmpl.substitute(PGM = pgm))
    if tmp_data:
        for line in tmp_data.splitlines(True):
            if not re.search('</body>|</html>', line, re.IGNORECASE):
                sys.stdout.write(line)

sys.stdout.write(src_data)

# if </html> is found, suppose a complete html is provided.
# otherwise, generate module class reference:
if re.search('</html>', src_data, re.IGNORECASE):
    sys.exit()

index_names = {
    'd' : 'display',
    'db': 'database',
    'g' : 'general',
    'i' : 'imagery',
    'm' : 'misc',
    'ps': 'postscript',
    'p' : 'paint',
    'r' : 'raster',
    'r3': 'raster3D',
    's' : 'sites',
    't' : 'temporal',
    'v' : 'vector'
    }

index = re.search('(<!-- meta page index:)(.*)(-->)', src_data, re.IGNORECASE)
if index:
    index_name_cap = index_name = index.group(2).strip()
else:
    mod_class = pgm.split('.', 1)[0]
    index_name = index_names.get(mod_class, '')
    index_name_cap = index_name.title()

if index_name:
    sys.stdout.write(footer_index.substitute(INDEXNAME = index_name, INDEXNAMECAP = index_name_cap,
                                             YEAR = year))
else:
    sys.stdout.write(footer_noindex.substitute(YEAR = year))
