#!/usr/bin/env python3

# generates docs/html/full_index.html
# (c) The GRASS Development Team, Markus Neteler, Glynn Clements 2003, 2004, 2005, 2006, 2009

import sys
import os
import string

from build_html import *

year = None
if len(sys.argv) > 1:
    year = sys.argv[1]

os.chdir(html_dir)

# TODO: create some master function/dict somewhere
class_labels = {
    'd' : 'display',
    'db' : 'database',
    'g' : 'general',
    'i' : 'imagery',
    'm' : 'miscellaneous',
    'ps' : 'PostScript',
    'r' : 'raster',
    'r3' : '3D raster',
    't' : 'temporal',
    'v' : 'vector'
}

classes = []
for cmd in html_files('*'):
    prefix = cmd.split('.')[0]
    if prefix not in [item[0] for item in classes]:
        classes.append((prefix, class_labels.get(prefix, prefix)))
classes.sort(key=lambda tup: tup[0])

#begin full index:
filename = "full_index.html"
f = open(filename + ".tmp", 'w')

write_html_header(f, "GRASS GIS %s Reference Manual: Full index" % grass_version, body_width="80%")

#generate main index of all modules:
f.write(full_index_header)

f.write(toc)

#for all module groups:
for cls, cls_label in classes:
    f.write(cmd2_tmpl.substitute(cmd_label=to_title(cls_label), cmd=cls))
    #for all modules:  
    for cmd in html_files(cls):
        basename = os.path.splitext(cmd)[0]
        desc = check_for_desc_override(basename)
        if desc is None:
            desc = get_desc(cmd)
        f.write(desc1_tmpl.substitute(cmd = cmd,
                                      basename = basename,
                                      desc = desc))
    f.write("</table>\n")

write_html_footer(f, "index.html", year)

f.close()
replace_file(filename)

# done full index

