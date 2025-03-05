#!/usr/bin/env python3

# generates docs/html/full_index.html
# (C) 2003-2009 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements

import sys
import os

from operator import itemgetter

from build import (
    get_files,
    write_footer,
    write_header,
    to_title,
    grass_version,
    check_for_desc_override,
    replace_file,
)


def build_full_index(ext, index_name, source_dir, year):
    if ext == "html":
        from build_html import (
            man_dir,
            full_index_header,
            cmd2_tmpl,
            desc1_tmpl,
            get_desc,
            toc,
        )
    else:
        from build_md import (
            man_dir,
            full_index_header,
            cmd2_tmpl,
            desc1_tmpl,
            get_desc,
        )

    if source_dir is None:
        source_dir = man_dir

    os.chdir(source_dir)

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
    for cmd in get_files(source_dir, "*", extension=ext):
        prefix = cmd.split(".")[0]
        if prefix not in [item[0] for item in classes]:
            classes.append((prefix, class_labels.get(prefix, prefix)))
    classes.sort(key=itemgetter(0))

    # begin full index:
    filename = f"{index_name}.{ext}"
    with open(filename + ".tmp", "w") as f:
        write_header(
            f,
            "GRASS GIS {} Reference Manual - Full index".format(grass_version),
            body_width="80%",
            template=ext,
        )

        # generate main index of all modules:
        f.write(full_index_header)

        if ext == "html":
            f.write(toc)

        # for all module groups:
        for cls, cls_label in classes:
            f.write(cmd2_tmpl.substitute(cmd_label=to_title(cls_label), cmd=cls))
            # for all modules:
            for cmd in get_files(source_dir, cls, extension=ext):
                basename = os.path.splitext(cmd)[0]
                desc = check_for_desc_override(basename)
                if desc is None:
                    desc = get_desc(cmd)
                f.write(desc1_tmpl.substitute(cmd=cmd, basename=basename, desc=desc))
            if ext == "html":
                f.write("</table>\n")

        write_footer(f, f"index.{ext}", year, template=ext)

    replace_file(filename)


def main():
    ext = None
    year = None
    index_name = "full_index"
    source_dir = None
    if len(sys.argv) > 1:
        if sys.argv[1] in ["html", "md"]:
            ext = sys.argv[1]
        else:
            year = sys.argv[1]
    if len(sys.argv) > 3:
        print(sys.argv)
        index_name = sys.argv[2]
        source_dir = sys.argv[3]
    if len(sys.argv) > 4:
        year = sys.argv[4]

    if ext is None:
        build_full_index("html", index_name=index_name, year=year)
        build_full_index("md", index_name=index_name, year=year, source_dir=None)
    elif ext == "html":
        build_full_index(
            "html", index_name=index_name, year=year, source_dir=source_dir
        )
    elif ext == "md":
        build_full_index("md", index_name=index_name, year=year, source_dir=source_dir)
    else:
        raise ValueError("Unknown extension: %s" % ext)


if __name__ == "__main__":
    main()
