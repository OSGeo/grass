#!/usr/bin/env python

############################################################################
#
# MODULE:       mkhtml.py
# AUTHOR(S):    Markus Neteler, Glynn Clements
# PURPOSE:      create HTML manual page snippets
# COPYRIGHT:    (C) 2007,2009 Glynn Clements and the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

import sys
import os
import string
import re

pgm = sys.argv[1]

src_file = "%s.html" % pgm
tmp_file = "%s.tmp.html" % pgm

header_tmpl = string.Template(\
"""<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title>GRASS GIS Manual: ${PGM}</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<link rel="stylesheet" href="grassdocs.css" type="text/css">
</head>
<body bgcolor="white">
<img src="grass_logo.png" alt="GRASS logo"><hr align=center size=6 noshade>
<h2>NAME</h2>
<em><b>${PGM}</b></em>
""")

footer_tmpl = string.Template(\
"""<hr>
<p><a href="index.html">Main index</a> - <a href="$INDEXNAME.html">$INDEXNAME index</a> - <a href="full_index.html">Full index</a></p>
<p>&copy; 2003-2010 <a href="http://grass.osgeo.org">GRASS Development Team</a></p>
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
    'd': 'display',
    'db': 'database',
    'g': 'general',
    'i': 'imagery',
    'm': 'misc',
    'pg': 'postGRASS',
    'ps': 'postscript',
    'p': 'paint',
    'r': 'raster',
    'r3': 'raster3D',
    's': 'sites',
    'v': 'vector'
    }

mod_class = pgm.split('.', 1)[0]
index_name = index_names.get(mod_class, mod_class)

sys.stdout.write(footer_tmpl.substitute(INDEXNAME = index_name))

