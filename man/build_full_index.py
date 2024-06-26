#!/usr/bin/env python3

# generates docs/html/full_index.html
# (C) 2003-2009 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements

import sys
import os

year = None
if len(sys.argv) > 1:
    year = sys.argv[1]


def build_full_index(ext):
    os.chdir(path)

    # TODO: create some master function/dict somewhere
    class_labels = {
        "d": "display",
        "db": "database",
        "g": "general",
        "i": "imagery",
        "m": "miscellaneous",
        "ps": "PostScript",
        "r": "raster",
        "r3": "3D raster",
        "t": "temporal",
        "v": "vector",
    }

    classes = []
    for cmd in get_files("*"):
        prefix = cmd.split(".")[0]
        if prefix not in [item[0] for item in classes]:
            classes.append((prefix, class_labels.get(prefix, prefix)))
    classes.sort(key=lambda tup: tup[0])

    # begin full index:
    filename = f"full_index.{ext}"
    f = open(filename + ".tmp", "w")

    write_header(
        f, "GRASS GIS %s Reference Manual: Full index" % grass_version, body_width="80%"
    )

    # generate main index of all modules:
    f.write(full_index_header)

    if ext == "html":
        f.write(toc)

    # for all module groups:
    for cls, cls_label in classes:
        f.write(cmd2_tmpl.substitute(cmd_label=to_title(cls_label), cmd=cls))
        # for all modules:
        for cmd in get_files(cls):
            basename = os.path.splitext(cmd)[0]
            desc = check_for_desc_override(basename)
            if desc is None:
                desc = get_desc(cmd)
            f.write(desc1_tmpl.substitute(cmd=cmd, basename=basename, desc=desc))
        f.write("</table>\n")

    write_footer(f, f"index.{ext}", year)

    f.close()
    replace_file(filename)

if __name__ == "__main__":
    from build_html import html_dir as path, html_files as get_files, write_html_footer as write_footer, write_html_header as write_header, grass_version, full_index_header, toc, cmd2_tmpl, to_title, check_for_desc_override, get_desc, desc1_tmpl, replace_file

if __name__ == "__main__":
    from build_html import (
        html_dir as path,
        html_files as get_files,
        write_html_footer as write_footer,
        write_html_header as write_header,
        grass_version,
        full_index_header,
        toc,
        cmd2_tmpl,
        to_title,
        check_for_desc_override,
        get_desc,
        desc1_tmpl,
        replace_file,
    )

    build_full_index("html")

    from build_md import (
        md_dir as path,
        md_files as get_files,
        write_md_footer as write_footer,
        write_md_header as write_header,
        full_index_header,
        cmd2_tmpl,
        to_title,
        desc1_tmpl,
        get_desc,
    )

    build_full_index("md")
