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

addons_path = None
if len(sys.argv) >= 2:
    addons_path = sys.argv[1]

year = os.getenv("VERSION_DATE")


def get_module_man_file_path(module):
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


def build_keywords(ext):
    keywords = {}

    files = glob.glob(os.path.join(path, f"*.{ext}"))
    # TODO: add markdown support
    if addons_path:
        addons_man_files = glob.glob(os.path.join(addons_path, "*.html"))
        files.extend(addons_man_files)

    char_list = {}

    for in_file in files:
        fname = os.path.basename(in_file)
        with open(in_file) as f:
            lines = f.readlines()

        if ext == "html":
            # TODO maybe move to Python re (regex)
            try:
                index_keys = lines.index("<h2>KEYWORDS</h2>\n") + 1
            except:
                continue
            try:
                keys = []
                for k in lines[index_keys].split(","):
                    keys.append(k.strip().split(">")[1].split("<")[0])
            except:
                continue
        else:
            keys = []
            for line in lines:
                if "meta page module keywords" in line:
                    keys = line.split(":", 1)[1].rstrip("-->\n").strip().split(',')
                    break

        for key in keys:
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

    keywordsfile = open(os.path.join(path, f"keywords.{ext}"), "w")
    if ext == "html":
        keywordsfile.write(
            header1_tmpl.substitute(
                title="GRASS GIS %s Reference " "Manual: Keywords index" % grass_version
            )
        )
    keywordsfile.write(headerkeywords_tmpl)
    if ext == "html":
        keywordsfile.write("<dl>")
    sortedKeys = sorted(keywords.keys(), key=lambda s: s.lower())

    for key in sortedKeys:
        if ext == "html":
            keyword_line = (
                '<dt><b><a name="{key}" class="urlblack">{key}</a></b></dt><dd>'.format(
                    key=key
                )
            )
        else:
            keyword_line = f"### **{key}**\n"
        for value in sorted(keywords[key]):
            if ext == "html":
                keyword_line += (
                    f' <a href="{get_module_man_file_path(value)}">'
                    f'{value.replace(".{ext}", "")}</a>,'
                )
            else:
                keyword_line += (
                    f' [{value.rsplit(".", 1)[0]}]({get_module_man_file_path(value)}),'
                )
        keyword_line = keyword_line.rstrip(",")
        if ext == "html":
            keyword_line += "</dd>"
        keyword_line += "\n"
        keywordsfile.write(keyword_line)
    if ext == "html":
        keywordsfile.write("</dl>\n")
    if ext == "html":
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

    write_footer(keywordsfile, f"index.{ext}", year)
    keywordsfile.close()


if __name__ == "__main__":
    from build_html import (
        header1_tmpl,
        grass_version,
        headerkeywords_tmpl,
        write_html_footer as write_footer,
        html_dir as path,
    )

    build_keywords("html")

    from build_md import (
        grass_version,
        headerkeywords_tmpl,
        write_md_footer as write_footer,
        md_dir as path,
    )

    build_keywords("md")
