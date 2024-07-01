#!/usr/bin/env python3

"""
Generates keywords.html file for core and optionally for addons modules.

Usage:

Generate core modules keywords HTML page

python man/build_keywords.py <path_to_core_modules_html_man_files>

Generate core modules and optionally inject addons keywords HTML page

python man/build_keywords.py <dir_path_to_core_modules_html_man_files>
    <dir_path_to_addons_modules_html_man_files>

@author Luca Delucchi
@author Tomas Zigo <tomas.zigo slovanet.sk> - inject addons modules keywords
"""

import os
import sys
import glob
from build_html import *

blacklist = [
    "Display",
    "Database",
    "General",
    "Imagery",
    "Misc",
    "Postscript",
    "Raster",
    "Raster3D",
    "Temporal",
    "Vector",
]

path = sys.argv[1]
addons_path = None
if len(sys.argv) >= 3:
    addons_path = sys.argv[2]

year = os.getenv("VERSION_DATE")

keywords = {}

htmlfiles = glob.glob(os.path.join(path, "*.html"))
if addons_path:
    addons_man_files = glob.glob(os.path.join(addons_path, "*.html"))
    htmlfiles.extend(addons_man_files)

char_list = {}


def get_module_man_html_file_path(module):
    """Get module manual HTML file path

    :param str module: module manual HTML file name e.g. v.surf.rst.html

    :return str module_path: core/addon module manual HTML file path
    """
    if addons_path and module in ",".join(addons_man_files):
        module_path = os.path.join(addons_path, module)
        module_path = module_path.replace(
            os.path.commonpath([path, module_path]),
            ".",
        )
    else:
        module_path = f"./{module}"
    return module_path


for html_file in htmlfiles:
    fname = os.path.basename(html_file)
    with open(html_file) as f:
        lines = f.readlines()
    # TODO maybe move to Python re (regex)
    # remove empty lines
    lines = [x for x in lines if x != "\n"]
    try:
        index_keys = lines.index("<h2>KEYWORDS</h2>\n") + 1
        index_desc = lines.index("<h2>NAME</h2>\n") + 1
    except:
        continue
    try:
        keys = lines[index_keys].split(",")
    except:
        continue
    for key in keys:
        key = key.strip()
        try:
            key = key.split(">")[1].split("<")[0]
        except:
            pass
        if not key:
            sys.exit("Empty keyword from file %s line: %s" % (fname, lines[index_keys]))
        if key not in keywords.keys():
            keywords[key] = []
            keywords[key].append(fname)
        elif fname not in keywords[key]:
            keywords[key].append(fname)

for black in blacklist:
    try:
        del keywords[black]
    except:
        try:
            del keywords[black.lower()]
        except:
            continue

for key in sorted(keywords.keys()):
    # this list it is useful to create the TOC using only the first
    # character for keyword
    firstchar = key[0].lower()
    if firstchar not in char_list.keys():
        char_list[str(firstchar)] = key
    elif firstchar in char_list.keys():
        if key.lower() < char_list[str(firstchar)].lower():
            char_list[str(firstchar.lower())] = key

keywordsfile = open(os.path.join(path, "keywords.html"), "w")
keywordsfile.write(
    header1_tmpl.substitute(
        title="GRASS GIS %s Reference Manual: Keywords index" % grass_version
    )
)
keywordsfile.write(headerkeywords_tmpl)
keywordsfile.write("<dl>")

sortedKeys = sorted(keywords.keys(), key=lambda s: s.lower())

for key in sortedKeys:
    keyword_line = '<dt><b><a name="%s" class="urlblack">%s</a></b></dt><dd>' % (
        key,
        key,
    )
    for value in sorted(keywords[key]):
        keyword_line += (
            f' <a href="{get_module_man_html_file_path(value)}">'
            f'{value.replace(".html", "")}</a>,'
        )
    keyword_line = keyword_line.rstrip(",")
    keyword_line += "</dd>\n"
    keywordsfile.write(keyword_line)
keywordsfile.write("</dl>\n")
# create toc
toc = '<div class="toc">\n<h4 class="toc">Table of contents</h4><p class="toc">'
test_length = 0
all_keys = len(char_list.keys())
for k in sorted(char_list.keys()):
    test_length += 1
    #    toc += '<li><a href="#%s" class="toc">%s</a></li>' % (char_list[k], k)
    if test_length % 4 == 0 and not test_length == all_keys:
        toc += '\n<a href="#%s" class="toc">%s</a>, ' % (char_list[k], k)
    elif test_length % 4 == 0 and test_length == all_keys:
        toc += '\n<a href="#%s" class="toc">%s</a>' % (char_list[k], k)
    elif test_length == all_keys:
        toc += '<a href="#%s" class="toc">%s</a>' % (char_list[k], k)
    else:
        toc += '<a href="#%s" class="toc">%s</a>, ' % (char_list[k], k)
toc += "</p></div>\n"
keywordsfile.write(toc)

write_html_footer(keywordsfile, "index.html", year)
keywordsfile.close()
