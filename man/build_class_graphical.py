#!/usr/bin/env python3

############################################################################
#
# MODULE:    build_class_graphical
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
# PURPOSE:   Build page with modules per family/class/category with images
# COPYRIGHT: (C) 2015 by Vaclav Petras and the GRASS Development Team
#
#        This program is free software under the GNU General Public
#        License (>=v2). Read the file COPYING that comes with GRASS
#        for details.
#
#############################################################################

import sys
import os
import fnmatch

from build import (
    default_year,
    grass_version,
    to_title,
    get_files,
    check_for_desc_override,
    write_footer,
    replace_file,
)

from build_html import (
    header1_tmpl,
    modclass_intro_tmpl,
    get_desc,
    man_dir,
)

import build_md


graphical_index_style = """\
<style>
.img-list {
    margin: 0;
    padding: 0;
    list-style-type: none;
}

.img-list li {
    padding: 5px;
    overflow: auto;
}

.img-list li:hover {
    background-color: #eee;
}

.img-list li a {
    color: initial;
    text-decoration: none;
    display: block;
}

.img-list li img {
    width: 10%;
    float: left;
    margin: 0 15px 0 0;
    background: white;
    object-fit: scale-down;
}

.img-list li img.default-img {
    max-height: 5ex;
    background-color: var(--gs-primary-color);
    padding: 5px;
}

.img-list li .desc {
    margin: 0px;
}

.img-list li .name {
    margin: 5px;
    display: block;
    color: #409940;
    font-weight: bold;
    font-style: italic;
}
</style>
"""

header_graphical_index_tmpl = f"""\
<link rel="stylesheet" href="grassdocs.css" type="text/css">
{graphical_index_style}
</head>
<body style="width: 99%">
<div id="container">

<a href="index.html"><img src="grass_logo.png" alt="GRASS logo"></a>
<hr class="header">
<h2>Graphical index of GRASS tools</h2>
"""


def file_matches(filename, patterns):
    return any(fnmatch.fnmatch(filename, pattern) for pattern in patterns)


def starts_with_module(string, module) -> bool:
    # not solving:
    # module = module.replace('wxGUI.', 'g.gui.')
    # TODO: matches g.mapsets images for g.mapset and d.rast.num for d.rast
    return bool(
        string.startswith((module.replace(".", "_"), module.replace(".", ""), module))
    )


def get_module_image(module, images):
    candidates = []
    for image in images:
        if starts_with_module(image, module):
            candidates.append(image)
    if len(candidates) == 1:
        # matches g.mapsets images for g.mapset and d.rast.num for d.rast
        return candidates[0]
    if not candidates:
        return None
    for image in candidates:
        basename, unused = image.rsplit(".", 1)
        if basename == module.replace(".", "_"):
            return image
        if basename == module.replace(".", ""):
            return image
        if basename == module:
            return image
    return min(candidates, key=len)


def generate_page_for_category(
    short_family, module_family, imgs, year, skip_no_image=False
):
    filename = module_family + "_graphical.html"
    with open(filename + ".tmp", "w") as output:
        output.write(
            header1_tmpl.substitute(
                title="GRASS %s Reference Manual: Graphical index" % grass_version
            )
        )
        output.write(header_graphical_index_tmpl)

        if module_family.lower() not in {"general", "postscript"}:
            if module_family == "raster3d":
                # covert keyword to nice form
                module_family = "3D raster"
            output.write(
                modclass_intro_tmpl.substitute(
                    modclass=to_title(module_family),
                    modclass_lower=module_family.lower(),
                )
            )
        if module_family == "wxGUI":
            output.write("<h3>wxGUI components:</h3>")
        elif module_family == "guimodules":
            output.write("<h3>g.gui.* tools:</h3>")
        else:
            output.write("<h3>{0} tools:</h3>".format(to_title(module_family)))
        output.write('<ul class="img-list">')

        # for all modules:
        for cmd in get_files(man_dir, short_family, ignore_gui=False):
            basename = os.path.splitext(cmd)[0]
            desc = check_for_desc_override(basename)
            if desc is None:
                desc = get_desc(cmd)
            img = get_module_image(basename, imgs)
            img_class = "linkimg"
            if skip_no_image and not img:
                continue
            if not img:
                img = "grass_logo.png"
                img_class = "default-img"
            if basename.startswith("wxGUI"):
                basename = basename.replace(".", " ")
            output.write(
                "<li>"
                '<a href="{html}">'
                '<img class="{img_class}" src="{img}">'
                '<span class="name">{name}</span> '
                '<span class="desc">{desc}</span>'
                "</a>"
                "</li>".format(
                    html=cmd, img=img, name=basename, desc=desc, img_class=img_class
                )
            )

        output.write("</ul>")

        write_footer(output, "index.html", year, template="html")

    replace_file(filename)


def generate_page_for_category_md(
    short_family, module_family, imgs, year, skip_no_image=False
):
    filename = module_family + "_graphical.md"
    with open(filename + ".tmp", "w") as output:
        output.write(graphical_index_style)

        if module_family.lower() not in {"general", "postscript"}:
            if module_family == "raster3d":
                # covert keyword to nice form
                module_family = "3D raster"
            output.write(
                modclass_intro_tmpl.substitute(
                    modclass=module_family, modclass_lower=module_family.lower()
                )
            )
        if module_family == "wxGUI":
            output.write("# wxGUI components\n")
        elif module_family == "guimodules":
            output.write("# g.gui.* modules\n")
        else:
            output.write("# {0} tools\n".format(to_title(module_family)))
        output.write('<ul class="img-list">')

        # for all modules:
        for cmd in get_files(
            build_md.man_dir, short_family, ignore_gui=False, extension="md"
        ):
            basename = os.path.splitext(cmd)[0]
            desc = check_for_desc_override(basename)
            if desc is None:
                desc = build_md.get_desc(cmd)
            img = get_module_image(basename, imgs)
            img_class = "linkimg"
            if skip_no_image and not img:
                continue
            if not img:
                img = "grass_logo.svg"
                img_class = "default-img"
            if basename.startswith("wxGUI"):
                basename = basename.replace(".", " ")
            output.write(
                "<li>"
                '<a href="{html}.html">'
                '<img class="{img_class}" src="{img}">'
                "</a>"
                '<a href="{html}.html">'
                '<span class="name">{name}</span> '
                '<span class="desc">{desc}</span>'
                "</a>"
                "</li>".format(
                    html=cmd.removesuffix(".md"),
                    img=img,
                    name=basename,
                    desc=desc,
                    img_class=img_class,
                )
            )

        output.write("</ul>")

        write_footer(output, "index.html", year, template="md")

    replace_file(filename)


# TODO: dependencies in makefile for this have to be fixed
# TODO: there is a potential overlap with other scripts (-> refactoring)


def main():
    year = default_year
    output_format = sys.argv[1]
    html_dir = sys.argv[2]
    os.chdir(html_dir)

    img_extensions = ["png", "jpg", "gif"]
    img_patterns = ["*." + extension for extension in img_extensions]
    imgs = []
    for filename in sorted(os.listdir(html_dir)):
        if file_matches(filename, img_patterns):
            imgs.append(filename)

    # using term family
    # category has its meaning in GRASS already
    # class has its meaning in Python, plus it is a synonym for category
    # TODO: what would be user friendly is unclear
    families = [
        ("d", "display"),
        ("db", "database"),
        ("g", "general"),
        ("i", "imagery"),
        ("m", "miscellaneous"),
        ("ps", "postscript"),
        ("r", "raster"),
        ("r3", "raster3d"),
        ("t", "temporal"),
        ("v", "vector"),
        ("wxGUI", "wxGUI"),
        ("g.gui", "guimodules"),
    ]

    # partial compatibility with build_class.py
    # first arg is dist html dir but the 3 other are like first 3 there
    if len(sys.argv) > 3:
        short_family = sys.argv[3]
        module_family = sys.argv[4]
        if len(sys.argv) > 5:
            year = sys.argv[5]

    for short_family, module_family in families:
        if output_format == "html":
            generate_page_for_category(
                short_family, module_family, imgs, year=year, skip_no_image=False
            )
        elif output_format == "md":
            generate_page_for_category_md(
                short_family, module_family, imgs, year=year, skip_no_image=False
            )
        else:
            msg = f"Unknown format: {output_format}"
            raise ValueError(msg)


if __name__ == "__main__":
    main()
