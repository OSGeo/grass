#!/usr/bin/env python
# -*- coding: utf-8 -*-

# generates keywords.html
# (c) 2013 by the GRASS Development Team, Luca Delucchi

import os
import sys
import glob
import string
from build_html import *

blacklist = ['Display', 'Database', 'General', 'Imagery', 'Misc', 'Postscript',
             'Raster', 'Raster3D', 'Temporal', 'Vector']

path = sys.argv[1]
year = os.getenv("VERSION_DATE")

keywords = {}

htmlfiles = glob.glob1(path, '*.html')

for fname in htmlfiles:
    fil = open(os.path.join(path, fname))
    # TODO maybe move to Python re (regex)
    lines=fil.readlines()
    try:
        index_keys = lines.index('<h2>KEYWORDS</h2>\n') + 1
        index_desc = lines.index('<h2>NAME</h2>\n') + 1
    except:
        continue
    try:
        keys = lines[index_keys].split(',')
    except:
        continue
    for key in keys:
        key = key.strip()
        key = "%s%s" % (key[0].upper(), key[1:])
        if key not in keywords.keys():
            keywords[key] = []
            keywords[key].append(fname)
        elif fname not in keywords[key]:
            keywords[key].append(fname)

for black in blacklist:
    try:
        del keywords[black]
    except:
        continue

keywordsfile = open(os.path.join(path, 'keywords.html'), 'w')
keywordsfile.write(header1_tmpl.substitute(title = "GRASS GIS " \
                        "%s Reference Manual: Keywords index" % grass_version))
keywordsfile.write(headerkeywords_tmpl)
for key, values in sorted(keywords.iteritems()):
    keyword_line = "<li><b>%s</b>:" % key
    for value in sorted(values):
        keyword_line += ' <a href="%s">%s</a>,' % (value, value.replace('.html',
                                                                        ''))
    keyword_line = keyword_line.rstrip(',')
    keyword_line += '</li>\n'
    keywordsfile.write(keyword_line)

keywordsfile.write("</ul>\n")
write_html_footer(keywordsfile, "index.html", year)  
keywordsfile.close()
    
