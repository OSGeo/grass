#!/usr/bin/env python3
# -*- coding: utf-8 -*-

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

#from build_html import *
from build_html import (
    default_year, header1_tmpl, grass_version,
    modclass_intro_tmpl, to_title, html_files,
    check_for_desc_override, get_desc, write_html_footer, replace_file,
    )


header_graphical_index_tmpl = """\
<link rel="stylesheet" href="grassdocs.css" type="text/css">
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
</head>
<body style="width: 99%">
<div id="container">

<a href="index.html"><img src="grass_logo.png" alt="GRASS logo"></a>
<hr class="header">
<h2>Graphical index of GRASS GIS modules</h2>
"""


def file_matches(filename, patterns):
    for pattern in patterns:
        if fnmatch.fnmatch(filename, pattern):
            return True
    return False


def starts_with_module(string, module):
    # not solving:
    # module = module.replace('wxGUI.', 'g.gui.')
    # TODO: matches g.mapsets images for g.mapset and d.rast.num for d.rast
    if string.startswith(module.replace('.', '_')):
        return True
    if string.startswith(module.replace('.', '')):
        return True
    if string.startswith(module):
        return True
    return False


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
        basename, unused = image.rsplit('.', 1)
        if basename == module.replace('.', '_'):
            return image
        if basename == module.replace('.', ''):
            return image
        if basename == module:
            return image
    return sorted(candidates, key=len)[0]


def generate_page_for_category(short_family, module_family, imgs, year,
                               skip_no_image=False):
    filename = module_family + "_graphical.html"

    output = open(filename + ".tmp", 'w')

    output.write(header1_tmpl.substitute(
        title="GRASS GIS %s Reference "
              "Manual: Graphical index" % grass_version))
    output.write(header_graphical_index_tmpl)

    if module_family.lower() not in ['general', 'postscript']:
        if module_family == 'raster3d':
            # covert keyword to nice form
            module_family = '3D raster'
        output.write(modclass_intro_tmpl.substitute(
            modclass=module_family, modclass_lower=module_family.lower()))
    if module_family == 'wxGUI':
        output.write("<h3>wxGUI components:</h3>")
    elif module_family == 'guimodules':
        output.write("<h3>g.gui.* modules:</h3>")
    else:
        output.write("<h3>{0} modules:</h3>".format(to_title(module_family)))
    output.write('<ul class="img-list">')

    #for all modules:
    for cmd in html_files(short_family, ignore_gui=False):
        basename = os.path.splitext(cmd)[0]
        desc = check_for_desc_override(basename)
        if desc is None:
            desc = get_desc(cmd)
        img = get_module_image(basename, imgs)
        img_class = 'linkimg'
        if skip_no_image and not img:
            continue
        elif not img:
            img = 'grass_logo.png'
            img_class = 'default-img'
        if basename.startswith('wxGUI'):
            basename = basename.replace('.', ' ')
        output.write(
            '<li>'
            '<a href="{html}">'
            '<img class="{img_class}" src="{img}">'
            '<span class="name">{name}</span> '
            '<span class="desc">{desc}</span>'
            '</a>'
            '</li>'
            .format(html=cmd, img=img, name=basename,
                    desc=desc, img_class=img_class))

    output.write('</ul>')

    write_html_footer(output, "index.html", year)

    output.close()
    replace_file(filename)


# TODO: dependencies in makefile for this have to be fixed
# TODO: there is a potential overlap with other scripts (-> refactoring)

def main():
    year = default_year
    html_dir = sys.argv[1]
    os.chdir(html_dir)

    img_extensions = ['png', 'jpg', 'gif']
    img_patterns = ['*.' + extension for extension in img_extensions]
    imgs = []
    for filename in sorted(os.listdir(html_dir)):
        if file_matches(filename, img_patterns):
            imgs.append(filename)

    # using term family
    # category has its meaning in GRASS already
    # class has its meaning in Python, plus it is a synonym for category
    # TODO: what would be user friendly is unclear
    families = [
        ('d', 'display'),
        ('db', 'database'),
        ('g', 'general'),
        ('i', 'imagery'),
        ('m', 'miscellaneous'),
        ('ps', 'postscript'),
        ('r', 'raster'),
        ('r3', 'raster3d'),
        ('t', 'temporal'),
        ('v', 'vector'),
        ('wxGUI', 'wxGUI'),
        ('g.gui', 'guimodules'),
    ]

    # partial compatibility with build_class.py
    # first arg is dist html dir but the 3 other are like first 3 there
    if len(sys.argv) > 2:
        short_family = sys.argv[2]
        module_family = sys.argv[3]
        classes = [(short_family, module_family)]
        if len(sys.argv) > 4:
            year = sys.argv[4]

    for short_family, module_family in families:
        generate_page_for_category(short_family, module_family, imgs,
                                   year=year, skip_no_image=False)


if __name__ == '__main__':
    main()
