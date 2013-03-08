#!/usr/bin/env python
# -*- coding: utf-8 -*-

# generates topics.html and topic_*.html
# (c) 2012 by the GRASS Development Team, Markus Neteler, Luca Delucchi

import os
import sys
import glob
import string
from build_html import *

path = sys.argv[1]
year = os.getenv("VERSION_DATE")

keywords = {}

htmlfiles = glob.glob1(path,'*.html')

for fname in htmlfiles:
    fil = open(os.path.join(path,fname))
    # TODO maybe move to Python re (regex)
    lines=fil.readlines()
    try:
        index_keys = lines.index('<h2>KEYWORDS</h2>\n')+1
        index_desc = lines.index('<h2>NAME</h2>\n')+1
    except:
        continue 
    try:
        key = lines[index_keys].split(',')[1].strip().capitalize().replace(' ', '_')
    except:
        continue
    try:
        desc = lines[index_desc].split('-',1)[1].strip()
    except:
        desc.strip()
    if key not in keywords.keys():
        keywords[key] = {}
        keywords[key][fname] = desc
    elif fname not in keywords[key]:
        keywords[key][fname] = desc

topicsfile = open(os.path.join(path,'topics.html'),'w')
topicsfile.write(header1_tmpl.substitute(title = "GRASS GIS " \
                        "%s Reference Manual: Topics index" % grass_version))
topicsfile.write(headertopics_tmpl)
for key, values in sorted(keywords.iteritems()):
    topicsfile.writelines([moduletopics_tmpl.substitute(key = key.lower(), 
                                                        name = key.replace('_', ' '))])
    keyfile = open(os.path.join(path,'topic_%s.html' % key.lower()),'w')
    keyfile.write(header1_tmpl.substitute(title = "GRASS GIS " \
                        "%s Reference Manual: Topic %s" % (grass_version, 
                                                    key.replace('_', ' '))))
    keyfile.write(headerkey_tmpl.substitute(keyword = key.replace('_', ' ')))
    for mod, desc in sorted(values.iteritems()):
        keyfile.write(desc1_tmpl.substitute(cmd = mod, desc = desc, 
                                            basename = mod.replace('.html','')))
    keyfile.write("</table>\n")
    write_html_footer(keyfile, "index.html", year)
topicsfile.write("</ul>\n")
write_html_footer(topicsfile, "index.html", year)  
topicsfile.close()
