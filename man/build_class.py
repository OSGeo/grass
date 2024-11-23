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
    if ext == "html":
        from build_html import (
            modclass_tmpl,
            get_desc,
            desc2_tmpl,
            modclass_intro_tmpl,
            man_dir,
        )
    else:
        from build_md import (
            modclass_tmpl,
            get_desc,
            desc2_tmpl,
            modclass_intro_tmpl,
            man_dir,
        )

    os.chdir(man_dir)

    filename = modclass + f".{ext}"
    f = open(filename + ".tmp", "w")

    write_header(
        f,
        "{} modules - GRASS GIS {} Reference Manual".format(
            modclass.capitalize(), grass_version
        ),
        template=ext,
    )
    modclass_lower = modclass.lower()
    modclass_visible = modclass
    if modclass_lower not in no_intro_page_classes:
        if modclass_visible == "raster3d":
            # convert keyword to nice form
            modclass_visible = "3D raster"
        f.write(
            modclass_intro_tmpl.substitute(
                modclass=modclass_visible, modclass_lower=modclass_lower
            )
        )
    f.write(modclass_tmpl.substitute(modclass=to_title(modclass_visible)))

    # for all modules:
    for cmd in get_files(man_dir, cls, extension=ext):
        basename = os.path.splitext(cmd)[0]
        desc = check_for_desc_override(basename)
        if desc is None:
            desc = get_desc(cmd)
        f.write(desc2_tmpl.substitute(cmd=cmd, basename=basename, desc=desc))
    if ext == "html":
        f.write("</table>\n")

    write_footer(f, f"index.{ext}", year, template=ext)

    f.close()
    replace_file(filename)


if __name__ == "__main__":
    # for all module groups:
    cls = sys.argv[1]
    modclass = sys.argv[2]
    year = None
    if len(sys.argv) > 3:
        year = sys.argv[3]

    from build import (
        grass_version,
        to_title,
        check_for_desc_override,
        replace_file,
        get_files,
        write_header,
        write_footer,
    )

    build_class("html")

    build_class("md")
