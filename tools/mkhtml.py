#!/usr/bin/env python

############################################################################
#
# MODULE:       Builds manual pages
# AUTHOR(S):    Markus Neteler
#               Glynn Clements
#               Martin Landa <landa.martin gmail.com>
# PURPOSE:      Create HTML manual page snippets
# COPYRIGHT:    (C) 2007-2014 by Glynn Clements
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
from HTMLParser import HTMLParser

pgm = sys.argv[1]

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
<p><a href="index.html">Main index</a> | <a href="${INDEXNAME}.html">${INDEXNAMECAP} index</a> | <a href="topics.html">Topics index</a> | <a href="keywords.html">Keywords Index</a> | <a href="full_index.html">Full index</a></p>
<p>&copy; 2003-${YEAR} <a href="http://grass.osgeo.org">GRASS Development Team</a>, GRASS GIS ${GRASS_VERSION} Reference Manual</p>
</body>
</html>
""")

footer_noindex = string.Template(\
"""<hr>
<p><a href="index.html">Main index</a> | <a href="topics.html">Topics index</a> | <a href="keywords.html">Keywords Index</a> | <a href="full_index.html">Full index</a></p>
<p>&copy; 2003-${YEAR} <a href="http://grass.osgeo.org">GRASS Development Team</a>, GRASS GIS ${GRASS_VERSION} Reference Manual</p>
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

def create_toc(src_data):
    class MyHTMLParser(HTMLParser):
        def __init__(self):
            self.reset()
            self.idx = 1
            self.tag = ''
            self.data = []
            
        def handle_starttag(self, tag, attrs):
            self.tag = tag

        def handle_endtag(self, tag):
            self.tag = ''
        
        def handle_data(self, data):
            if self.tag in ('h1', 'h2', 'h3'):
                self.data.append((self.tag, '%s_%d' % (self.tag, self.idx), data))
                self.idx += 1

    # instantiate the parser and fed it some HTML
    parser = MyHTMLParser()
    parser.feed(src_data)
    
    return parser.data

def write_toc(data):
    fd = sys.stdout
    fd.write('<table class="toc">\n')
    for tag, href, text in data:
        fd.write('<tr><td>%s <a href="#%s" class="toc">%s</a></td></tr>\n' % \
                     ('&nbsp;' if tag == 'h3' else '', href, text))
    fd.write('</table>\n')

def update_toc(data):
    ret_data = []
    pat = re.compile(r'(<(h\d)>)(.+)(</h\d>)')
    idx = 1
    for line in data.splitlines():
        if pat.search(line):
            xline = pat.split(line)
            line = xline[1] + '<a name="%s_%d">' % (xline[2], idx) + xline[3] + '</a>' + xline[4]
            idx += 1
        ret_data.append(line)
    
    return '\n'.join(ret_data)

# process header
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

# create TOC
write_toc(create_toc(src_data))

# process body
sys.stdout.write(update_toc(src_data))

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

# process footer
index = re.search('(<!-- meta page index:)(.*)(-->)', src_data, re.IGNORECASE)
if index:
    index_name_cap = index_name = index.group(2).strip()
else:
    mod_class = pgm.split('.', 1)[0]
    index_name = index_names.get(mod_class, '')
    index_name_cap = index_name.title()

grass_version = os.getenv("VERSION_NUMBER", "unknown") 
year = os.getenv("VERSION_DATE")
if not year:
    year = str(datetime.now().year)

if index_name:
    sys.stdout.write(footer_index.substitute(INDEXNAME = index_name, INDEXNAMECAP = index_name_cap,
                                             YEAR = year, GRASS_VERSION = grass_version))
else:
    sys.stdout.write(footer_noindex.substitute(YEAR = year, GRASS_VERSION = grass_version))
