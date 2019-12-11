#!/usr/bin/env python3

# generates HTML man pages docs/html/<category>.html
# (c) The GRASS Development Team, Markus Neteler, Glynn Clements 2003, 2004, 2005, 2006, 2009, 2019

import sys
import os
import string

from build_html import *


no_intro_page_classes = ['display', 'general', 'miscellaneous', 'postscript']

os.chdir(html_dir)

#write separate module pages:

#for all module groups:
cls = sys.argv[1]
modclass = sys.argv[2]
year = None
if len(sys.argv) > 3:
    year = sys.argv[3]

filename = modclass + ".html"

f = open(filename + ".tmp", 'w')

write_html_header(f, "%s modules - GRASS GIS %s Reference Manual" % (modclass.capitalize(), grass_version))
modclass_lower = modclass.lower()
modclass_visible = modclass
if modclass_lower not in no_intro_page_classes:
    if modclass_visible == 'raster3d':
        # covert keyword to nice form
        modclass_visible = '3D raster'
    f.write(modclass_intro_tmpl.substitute(modclass=modclass_visible, modclass_lower=modclass_lower))
f.write(modclass_tmpl.substitute(modclass=to_title(modclass_visible)))

#for all modules:
for cmd in html_files(cls):
    basename = os.path.splitext(cmd)[0]
    desc = check_for_desc_override(basename)
    if desc is None:
        desc = get_desc(cmd)
    f.write(desc2_tmpl.substitute(cmd = cmd,
                                  basename = basename,
                                  desc = desc))
f.write("</table>\n")

write_html_footer(f, "index.html", year)

f.close()
replace_file(filename)

