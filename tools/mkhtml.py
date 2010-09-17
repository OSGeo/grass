#!/usr/bin/env python

############################################################################
#
# MODULE:       mkhtml.py
# AUTHOR(S):    Markus Neteler, Glynn Clements
#               TOC by Martin Landa <landa.martin gmail.com>
# PURPOSE:      create HTML manual page snippets
# COPYRIGHT:    (C) 2007,2009-2010 Glynn Clements and the GRASS Development Team
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

def get_toc_item(line, pattern):
    found = pattern.search(line)
    if found:
        return found.group(2).strip()
    
    return None

def filter_toc_item(label):
    return label.replace('"', '&quot;')

def write_toc_level(line, src_list, pattern, pattern_level, level, idx):
    indent = 4
    
    text = get_toc_item(line, pattern)
    if not text:
        return False, level
    
    if pattern_level > level:
        for l in range(level, pattern_level):
            sys.stdout.write('%s<ul>\n' % (' ' * l * indent))
    elif level > pattern_level:
        for l in range(level, pattern_level, -1):
            sys.stdout.write('%s</ul>\n' % (' ' * (l - 1) * indent))
    level = pattern_level
    
    sys.stdout.write('%s<li><a href="#%s">%s</a>\n' % (' ' * level * indent,
                                                       filter_toc_item(text), filter_toc_item(text)))
    src_list[idx] = '<a name="%s"></a>' % filter_toc_item(text) + line
    
    return True, level
    
def write_toc():
    global src_data
    level  = 1
    idx    = 0
    indent = 4
    
    sys.stdout.write('<a name="TOC"></a><h2>TABLE OF CONTENT</h2>\n\n<ul>\n')
    src_list = src_data.splitlines()
    for line in src_list:
        found, level = write_toc_level(line, src_list,
                                       re.compile(r"(<h2>)(.*)(</h2>)", re.IGNORECASE), 1,
                                       level, idx)
        if not found:
            found, level = write_toc_level(line, src_list,
                                           re.compile(r"(<h3>)(.*)(</h3>)", re.IGNORECASE), 2,
                                           level, idx)
        if not found:
            found, level = write_toc_level(line, src_list,
                                           re.compile(r"(<h4>)(.*)(</h4>)", re.IGNORECASE), 3,
                                           level, idx)
                
        idx += 1
    
    for l in range(level, 0, -1):
        sys.stdout.write('%s</ul>\n' % (' ' * (l - 1) * indent))
    src_data = '\n'.join(src_list)

if not re.search('<html>', src_data, re.IGNORECASE):
    tmp_data = read_file(tmp_file)
    if not re.search('<html>', tmp_data, re.IGNORECASE):
	sys.stdout.write(header_tmpl.substitute(PGM = pgm))
    if tmp_data:
	for line in tmp_data.splitlines(True):
	    if not re.search('</body>|</html>', line, re.IGNORECASE):
		sys.stdout.write(line)
    write_toc()

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
    'v' : 'vector'
    }

mod_class = pgm.split('.', 1)[0]
index_name = index_names.get(mod_class, mod_class)

sys.stdout.write(footer_tmpl.substitute(INDEXNAME = index_name))
