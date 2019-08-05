#!/usr/bin/env python3
# -*- coding: utf-8 -*-

############################################################################
#
# MODULE:    build_graphical_index
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
# PURPOSE:   Build graphical index
# COPYRIGHT: (C) 2015-2019 by Vaclav Petras and the GRASS Development Team
#
#        This program is free software under the GNU General Public
#        License (>=v2). Read the file COPYING that comes with GRASS
#        for details.
#
#############################################################################

import os
import sys
import fnmatch
import re

from build_html import write_html_footer, grass_version, header1_tmpl


output_name = 'graphical_index.html'

year = os.getenv("VERSION_DATE")

# other similar strings are in a different file
# TODO: all HTML manual building needs refactoring (perhaps grass.tools?)
header_graphical_index_tmpl = """\
<link rel="stylesheet" href="grassdocs.css" type="text/css">
<style>
.img-list {
    list-style-type: none;
    margin: 0;
    padding: 0;
    text-align: center;
}

.img-list li {
    display: inline-block;
    position: relative;
    width: 8em;
    margin: 0;
    padding: 0.5em;
    margin-bottom: 1em;
}

.img-list li:hover {
    background-color: #eee;
}

.img-list li img {
    float: left;
    max-width: 100%;
    background: white;
}

.img-list li span {
    text-align: center;
}

.img-list li a {
    color: initial;
    text-decoration: none;
}

.img-list li .name {
    margin: 0.1em;
    display: block;
    color: #409940;
    font-weight: bold;
    font-style: normal;
    font-size: 120%;
}
</style>
</head>
<body style="width: 99%">
<div id="container">

<a href="index.html"><img src="grass_logo.png" alt="GRASS logo"></a>
<hr class="header">
<h2>Graphical index of GRASS GIS modules</h2>
"""


def std_img_name(name):
    return "gi_{0}.jpg".format(name)


index_items = [
    ('raster_graphical.html', std_img_name('raster'), 'Raster'),
    ('vector_graphical.html', std_img_name('vector'), 'Vector'),
    ('database_graphical.html', std_img_name('database'), 'Database'),
    ('general_graphical.html', std_img_name('general'), 'General'),
    ('display_graphical.html', std_img_name('display'), 'Display'),
    ('imagery_graphical.html', std_img_name('imagery'), 'Imagery'),
    ('raster3d_graphical.html', std_img_name('raster3d'), '3D raster'),
    ('temporal_graphical.html', std_img_name('temporal'), 'Temporal'),
    ('miscellaneous_graphical.html', std_img_name('miscellaneous'), 'Miscellaneous'),
    ('postscript_graphical.html', std_img_name('cartography'), 'Cartography'),
    ('wxGUI_graphical.html', std_img_name('gui'), 'GUI'),
    ('wxGUI.nviz.html', std_img_name('3dview'), '3D view'),
    ('https://grass.osgeo.org/grass78/manuals/libpython/index.html', std_img_name('python'), 'Python'),
    ('https://grass.osgeo.org/programming7/', std_img_name('c'), 'C library'),
    ('manual_gallery.html', std_img_name('gallery'), 'Gallery'),
    ('https://grass.osgeo.org/grass7/manuals/addons/', std_img_name('addons'), 'Addons'),
]


def main():
    html_dir = sys.argv[1]

    with open(os.path.join(html_dir, output_name), 'w') as output:
        output.write(header1_tmpl.substitute(title="GRASS GIS %s Reference "
                                               "Manual: Graphical index" % grass_version))
        output.write(header_graphical_index_tmpl)
        output.write('<ul class="img-list">\n')
        for html_file, image, label in index_items:
            output.write(
                '<li>'
                '<a href="{html}">'
                '<img src="{img}">'
                '<span class="name">{name}</span>'
                '</a>'
                '</li>\n'
                .format(html=html_file, img=image, name=label))
        output.write('</ul>')
        write_html_footer(output, "index.html", year)


if __name__ == '__main__':
    main()
