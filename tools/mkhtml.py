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
<img src="grass_logo.png" alt="GRASS logo">
<hr class="header">
"""

header_nopgm = """<h2>${PGM}</h2>
"""

header_pgm = """<h2>NAME</h2>
<em><b>${PGM}</b></em>
"""

footer_index = string.Template(\
"""<hr class="header">
<p><a href="index.html">Main index</a> | <a href="${INDEXNAME}.html">${INDEXNAMECAP} index</a> | <a href="topics.html">Topics index</a> | <a href="keywords.html">Keywords Index</a> | <a href="full_index.html">Full index</a></p>
<p>&copy; 2003-${YEAR} <a href="http://grass.osgeo.org">GRASS Development Team</a>, GRASS GIS ${GRASS_VERSION} Reference Manual</p>
</body>
</html>
""")

footer_noindex = string.Template(\
"""<hr class="header">
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
            self.tag_curr = ''
            self.tag_last = ''
            self.process_text = False
            self.data = []
            self.tags_allowed = ('h1', 'h2', 'h3')
            self.tags_ignored = ('img')
            self.text = ''

        def handle_starttag(self, tag, attrs):
            if tag in self.tags_allowed:
                self.process_text = True
            self.tag_last = self.tag_curr
            self.tag_curr = tag

        def handle_endtag(self, tag):
            if tag in self.tags_allowed:
                self.data.append((tag, '%s_%d' % (tag, self.idx),
                                  self.text))
                self.idx += 1
                self.process_text = False
                self.text = ''
            
            self.tag_curr = self.tag_last
                
        def handle_data(self, data):
            if not self.process_text:
                return
            if self.tag_curr in self.tags_allowed or self.tag_curr in self.tags_ignored:
                self.text += data
            else:
                self.text += '<%s>%s</%s>' % (self.tag_curr, data, self.tag_curr)
    
    # instantiate the parser and fed it some HTML
    parser = MyHTMLParser()
    parser.feed(src_data)
    
    return parser.data

def escape_href(label):
    # remove html tags
    label = re.sub('<[^<]+?>', '', label)
    # fix &nbsp;
    label = label.replace('&nbsp;', '')
    # replace space with underscore + lower
    return label.replace(' ', '-').lower()

def write_toc(data):
    if not data:
        return
    
    fd = sys.stdout
    fd.write('<div class="toc">\n')
    fd.write('<ul class="toc">\n')
    first = True
    has_h2 = False
    in_h3 = False
    indent = 4
    for tag, href, text in data:
        if tag == 'h3' and not in_h3 and has_h2:
            fd.write('\n%s<ul class="toc">\n' % (' ' * indent))
            indent += 4
            in_h3 = True
        elif not first:
            fd.write('</li>\n')
            
        if tag == 'h2':
            has_h2 = True
            if in_h3:
                indent -= 4
                fd.write('%s</ul></li>\n' % (' ' * indent))
                in_h3 = False
        
        fd.write('%s<li class="toc"><a href="#%s" class="toc">%s</a>' % \
                     (' ' * indent, escape_href(text), text))
        first = False
    
    fd.write('</li>\n</ul>\n')
    fd.write('</div>\n')

def update_toc(data):
    ret_data = []
    pat = re.compile(r'(<(h[2|3])>)(.+)(</h[2|3]>)')
    idx = 1
    for line in data.splitlines():
        if pat.search(line):
            xline = pat.split(line)
            line = xline[1] + '<a name="%s">' % escape_href(xline[3]) + xline[3] + '</a>' + xline[4]
            idx += 1
        ret_data.append(line)

    return '\n'.join(ret_data)

# process header
src_data = read_file(src_file)
name = re.search('(<!-- meta page name:)(.*)(-->)', src_data, re.IGNORECASE)
if name:
    pgm = name.group(2).strip().split('-', 1)[0].strip()
desc = re.search('(<!-- meta page description:)(.*)(-->)', src_data,
                 re.IGNORECASE)
if desc:
    pgm = desc.group(2).strip()
    header_tmpl = string.Template(header_base + header_nopgm)
else:
    header_tmpl = string.Template(header_base + header_pgm)

if not re.search('<html>', src_data, re.IGNORECASE):
    tmp_data = read_file(tmp_file)
    if not re.search('<html>', tmp_data, re.IGNORECASE):
        sys.stdout.write(header_tmpl.substitute(PGM=pgm))
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
    index_name = index.group(2).strip()
    if '|' in index_name:
        index_name, index_name_cap = index_name.split('|', 1)
    else:
        index_name_cap = index_name
else:
    mod_class = pgm.split('.', 1)[0]
    index_name = index_names.get(mod_class, '')
    index_name_cap = index_name.title()

grass_version = os.getenv("VERSION_NUMBER", "unknown")
year = os.getenv("VERSION_DATE")
if not year:
    year = str(datetime.now().year)

if index_name:
    sys.stdout.write(footer_index.substitute(INDEXNAME=index_name,
                                             INDEXNAMECAP=index_name_cap,
                                             YEAR=year,
                                             GRASS_VERSION=grass_version))
else:
    sys.stdout.write(footer_noindex.substitute(YEAR=year,
                                               GRASS_VERSION=grass_version))
