#!/usr/bin/env python3

############################################################################
#
# MODULE:    build_graphical_index
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
# PURPOSE:   Build graphical index
# COPYRIGHT: (C) 2015-2025 by Vaclav Petras and the GRASS Development Team
#
#        This program is free software under the GNU General Public
#        License (>=v2). Read the file COPYING that comes with GRASS
#        for details.
#
#############################################################################

import os

output_name = "graphical_index"

year = os.getenv("VERSION_DATE")


def std_img_name(name):
    return "gi_{0}.jpg".format(name)


index_items = [
    ("raster_graphical.html", std_img_name("raster"), "Raster"),
    ("vector_graphical.html", std_img_name("vector"), "Vector"),
    ("database_graphical.html", std_img_name("database"), "Database"),
    ("general_graphical.html", std_img_name("general"), "General"),
    ("display_graphical.html", std_img_name("display"), "Display"),
    ("imagery_graphical.html", std_img_name("imagery"), "Imagery"),
    ("raster3d_graphical.html", std_img_name("raster3d"), "3D raster"),
    ("temporal_graphical.html", std_img_name("temporal"), "Temporal"),
    ("miscellaneous_graphical.html", std_img_name("miscellaneous"), "Miscellaneous"),
    ("postscript_graphical.html", std_img_name("cartography"), "Cartography"),
    ("wxGUI_graphical.html", std_img_name("gui"), "GUI"),
    ("wxGUI.nviz.html", std_img_name("3dview"), "3D view"),
    (
        "https://grass.osgeo.org/grass-devel/manuals/libpython/index.html",
        std_img_name("python"),
        "Python",
    ),
    ("https://grass.osgeo.org/programming8/", std_img_name("c"), "C library"),
    ("manual_gallery.html", std_img_name("gallery"), "Gallery"),
    (
        "https://grass.osgeo.org/grass8/manuals/addons/",
        std_img_name("addons"),
        "Addons",
    ),
]


def main(ext):
    if ext == "html":
        from build_html import (
            header1_tmpl,
            header_graphical_index_tmpl,
            man_dir,
        )
    else:
        from build_md import (
            header1_tmpl,
            header_graphical_index_tmpl,
            man_dir,
        )

    with open(os.path.join(man_dir, output_name + f".{ext}"), "w") as output:
        output.write(
            header1_tmpl.substitute(
                title=f"GRASS {grass_version} Reference Manual - Graphical index"
            )
        )
        output.write(header_graphical_index_tmpl)
        if ext == "html":
            output.write('<ul class="img-list">\n')
        for html_file, image, label in index_items:
            if ext == "html":
                output.write(
                    "<li>"
                    '<a href="{html}">'
                    '<img src="{img}">'
                    '<span class="name">{name}</span>'
                    "</a>"
                    "</li>\n".format(html=html_file, img=image, name=label)
                )
            else:
                output.write(
                    "[![{name}]({img})]({link}.md)\n".format(
                        link=html_file.removesuffix(".html"), img=image, name=label
                    )
                )

        if ext == "html":
            output.write("</ul>")
        write_footer(output, f"index.{ext}", year, template=ext)


if __name__ == "__main__":
    from build import (
        write_footer,
        grass_version,
    )

    main("html")

    main("md")
