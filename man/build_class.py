#!/usr/bin/env python

# generates HTML man pages docs/html/<category>.html
# (c) The GRASS Development Team, Markus Neteler, Glynn Clements 2003, 2004, 2005, 2006, 2009

import sys
import os
import string

from build_html import *

os.chdir(html_dir)

#write separate module pages:

#for all module groups:
cls = sys.argv[1]
modclass = sys.argv[2]

filename = modclass + ".html"

f = open(filename + ".tmp", 'wb')

write_html_header(f, "GRASS GIS %s Reference Manual: %s" % (grass_version, modclass))
f.write(modclass_tmpl.substitute(modclass = modclass))

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

write_html_footer(f, "index.html")

f.close()
replace_file(filename)

