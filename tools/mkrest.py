
#!/usr/bin/env python3

############################################################################
#
# MODULE:       mkrest.py
# AUTHOR(S):    Luca Delucchi
# PURPOSE:      Create HTML manual page snippets
# COPYRIGHT:    (C) 2012 by Luca Delucchi
#                and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

import sys
import string
import re
import subprocess
from datetime import datetime

pgm = sys.argv[1]
if len(sys.argv) > 1:
    year = sys.argv[2]
else:
    year = str(datetime.now().year)

src_file = "%s.html" % pgm
tmp_file = "%s.tmp.txt" % pgm

#TODO add copyright

footer_index = string.Template(\
"""

:doc:`Main Page <index>` - :doc:`${INDEXNAMECAP} index <${INDEXNAME}>` - :doc:`Full index <full_index>` 
2003-${YEAR} `GRASS Development Team <http://grass.osgeo.org>`_
""")

footer_noindex = string.Template(\
"""

:doc:`Main Page <index>`  - :doc:`Full index <full_index>` 
2003-${YEAR} `GRASS Development Team <http://grass.osgeo.org>`_
""")


def read_file(name):
    try:
        f = open(name, 'rb')
        s = f.read()
        f.close()
        return s
    except IOError:
        return ""

replacement = {
    '*`' : '`',
    '`* `' : '`',
    '>`_*' : '>`_',
    '>`_,*' : '>`_,',
    '``*\ "' : '``"',
    '***' : '**'
}

src_data = read_file(src_file)

title = re.search('(<!-- meta page description:)(.*)(-->)', src_data, re.IGNORECASE)

if title:
    title_name = title.group(2).strip()
    sys.stdout.write("%s\n" % title_name)
    title_style = "=" * (len(title_name)+2)
    sys.stdout.write("%s\n\n" % title_style)

tmp_data = read_file(tmp_file)
if tmp_data:
    sys.stdout.write(tmp_data)

process = subprocess.Popen('pandoc -s -r html %s -w rst' % src_file, 
                           shell=True, stdout=subprocess.PIPE)
html_text = process.communicate()[0]
if html_text:
    for k, v in replacement.iteritems():
        html_text = html_text.replace(k, v)

#TODO remove with space if string start with it, " vector..." -> "vector..."
#     not if it is a tab: "     vector...." -> "    vector..."

#    for line in html_text.splitlines(True):
#        sys.stdout.write("%s" % line.lstrip())

sys.stdout.write(html_text)

index_names = {
    'd': 'display',
    'db': 'database',
    'g': 'general',
    'i': 'imagery',
    'm': 'miscellaneous',
    'ps': 'postscript',
    'p': 'paint',
    'r': 'raster',
    'r3': 'raster3D',
    's': 'sites',
    't': 'temporal',
    'v': 'vector'
    }

index = re.search('(<!-- meta page index:)(.*)(-->)', src_data, re.IGNORECASE)

if index:
    index_name = index.group(2).strip()
else:
    mod_class = pgm.split('.', 1)[0]
    index_name = index_names.get(mod_class, '')

if index_name:
    sys.stdout.write(footer_index.substitute(INDEXNAME = index_name,
                                             INDEXNAMECAP = index_name.title(),
                                             YEAR = year))
else:
    sys.stdout.write(footer_noindex.substitute(YEAR = year))
    
sys.exit()
