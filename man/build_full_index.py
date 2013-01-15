#!/usr/bin/env python

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

classes = []
for cmd in html_files('*'):
    prefix = cmd.split('.')[0]
    if prefix not in classes:
	classes.append(prefix)
classes.sort()

#begin full index:
filename = "full_index.html"
f = open(filename + ".tmp", 'wb')

write_html_header(f, "GRASS GIS %s Reference Manual: Full index" % grass_version)

#generate main index of all modules:
f.write(full_index_header)
#"

for cls in classes:
    f.write(cmd1_tmpl.substitute(cmd = cls))
    if cls != classes[-1]:
	f.write(" | ")

f.write(sections)

#for all module groups:
for cls in classes:
    f.write(cmd2_tmpl.substitute(cmd = cls))
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

