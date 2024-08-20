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
    os.chdir(man_dir)

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
    print(man_dir)
    for cmd in get_files(man_dir, "*", extension=ext):
        print(cmd)
        prefix = cmd.split(".")[0]
        if prefix not in [item[0] for item in classes]:
            classes.append((prefix, class_labels.get(prefix, prefix)))
    classes.sort(key=lambda tup: tup[0])

    # begin full index:
    filename = f"full_index.{ext}"
    f = open(filename + ".tmp", "w")

    write_header(
        f, "GRASS GIS {} Reference Manual - Full index".format(grass_version),
        body_width="80%", template=ext
    )

    # generate main index of all modules:
    f.write(full_index_header)

    if ext == "html":
        f.write(toc)

    # for all module groups:
    for cls, cls_label in classes:
        f.write(cmd2_tmpl.substitute(cmd_label=to_title(cls_label), cmd=cls))
        # for all modules:
        for cmd in get_files(man_dir, cls, extension=ext):
            basename = os.path.splitext(cmd)[0]
            desc = check_for_desc_override(basename)
            if desc is None:
                desc = get_desc(cmd)
            f.write(desc1_tmpl.substitute(cmd=cmd, basename=basename, desc=desc))
        f.write("</table>\n")

    write_footer(f, f"index.{ext}", year, template=ext)

    f.close()
    replace_file(filename)


if __name__ == "__main__":
    from build import (
        get_files,
        write_footer,
        write_header,
        to_title,
        grass_version,
        check_for_desc_override,
        replace_file,
    )

    from build_html import (
        man_dir,
        full_index_header,
        cmd2_tmpl,
        desc1_tmpl,
        get_desc,
        toc,
    )
    
    # build_full_index("html")

    from build_md import (
        man_dir,
        full_index_header,
        cmd2_tmpl,
        desc1_tmpl,
        get_desc,
    )

    build_full_index("md")
