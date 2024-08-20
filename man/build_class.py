#!/usr/bin/env python3

# generates HTML man pages docs/html/<category>.html
# (C) 2003-2019 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements

import sys
import os

no_intro_page_classes = ["display", "general", "miscellaneous", "postscript"]

def build_class(ext):
    os.chdir(path)

    filename = modclass + f".{ext}"
    f = open(filename + ".tmp", "w")

    write_header(
        f,
        "{} modules - GRASS GIS {} Reference Manual".format(
            modclass.capitalize(), grass_version)
    )
    modclass_lower = modclass.lower()
    modclass_visible = modclass
    if modclass_lower not in no_intro_page_classes:
        if modclass_visible == "raster3d":
            # covert keyword to nice form
            modclass_visible = "3D raster"
        f.write(
            modclass_intro_tmpl.substitute(
                modclass=modclass_visible, modclass_lower=modclass_lower
            )
        )
    f.write(modclass_tmpl.substitute(modclass=to_title(modclass_visible)))

    # for all modules:
    for cmd in get_files(cls):
        basename = os.path.splitext(cmd)[0]
        desc = check_for_desc_override(basename)
        if desc is None:
            desc = get_desc(cmd)
        f.write(desc2_tmpl.substitute(cmd=cmd, basename=basename, desc=desc))
    if ext == "html":
        f.write("</table>\n")

    write_footer(f, f"index.{ext}", year)

    f.close()
    replace_file(filename)

if __name__ == "__main__":
    # for all module groups:
    cls = sys.argv[1]
    modclass = sys.argv[2]
    year = None
    if len(sys.argv) > 3:
        year = sys.argv[3]

    from build_html import (
        grass_version,
        modclass_tmpl,
        to_title,
        check_for_desc_override,
        get_desc,
        desc2_tmpl,
        replace_file,
        modclass_intro_tmpl,
        html_files as get_files,
        write_html_header as write_header,
        write_html_footer as write_footer,
        html_dir as path,
    )

    build_class("html")

    from build_md import (
        grass_version,
        modclass_tmpl,
        to_title,
        check_for_desc_override,
        get_desc,
        desc2_tmpl,
        modclass_intro_tmpl,
        md_files as get_files,
        write_md_header as write_header,
        write_md_footer as write_footer,
        md_dir as path,
    )

    build_class("md")
