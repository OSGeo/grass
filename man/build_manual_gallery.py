#!/usr/bin/env python3
# -*- coding: utf-8 -*-

############################################################################
#
# MODULE:    build_graphical_index
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
# PURPOSE:   Build index gallery from images from all HTML files
# COPYRIGHT: (C) 2015 by Vaclav Petras and the GRASS Development Team
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


output_name = 'manual_gallery.html'

img_extensions = ['png', 'jpg', 'gif']
img_patterns = ['*.' + extension for extension in img_extensions]

# we don't want some images to show up
# logos
img_blacklist = ['grass_logo.png', 'grass_icon.png']
# circles with numbers from helptext.html (unfortunate we have to list it here)
# perhaps some general name ending would be good, like *_noindex.png
img_blacklist.extend(['circle_{0}.png'.format(num) for num in range(1, 6)])

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
    width: 7em;
    margin: 0;
    padding: 0.5em;
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
    font-weight: normal;
    font-style: italic;
    font-size: 80%;
}
</style>
</head>
<body style="width: 99%">
<div id="container">

<a href="index.html"><img src="grass_logo.png" alt="GRASS logo"></a>
<hr class="header">
<h2>GRASS GIS manual gallery</h2>
"""


def img_in_html(filename, imagename):
    # for some reason, calling search just once is much faster
    # than calling it on every line (time is spent in _compile)
    pattern = re.compile('<img .*src=.{0}.*>'.format(imagename))
    with open(filename) as file:
        if re.search(pattern, file.read()):
            return True
    return False


def file_matches(filename, patterns):
    for pattern in patterns:
        if fnmatch.fnmatch(filename, pattern):
            return True
    return False


def get_files(directory, patterns, exclude_patterns):
    files = []
    for filename in sorted(os.listdir(directory)):
        if file_matches(filename, patterns):
            if not file_matches(filename, exclude_patterns):
                files.append(filename)
    return files


def remove_module_name(string, module):
    string = string.replace(module.replace('wxGUI.', 'g.gui.'), '')
    string = string.replace(module.replace('.', '_'), '')  # using _
    string = string.replace(module.replace('.', ''), '')  # using nothing
    string = string.replace(module, '')  # using original dots
    return string


def title_from_names(module_name, img_name):
    # we ignore the possibility of having extension at the end of image
    # so possibly r.out.png fails but image name should use _ anyway
    # strictly speaking, it could be also, e.g., index name
    for extension in img_extensions:
        img_name = img_name.replace('.' + extension, '')
    img_name = remove_module_name(img_name, module_name)
    img_name = img_name.replace('_', ' ')
    img_name = img_name.strip()
    if img_name:
        return "{name} ({desc})".format(name=module_name, desc=img_name)
    else:
        return "{name}".format(name=module_name)


def get_module_name(filename):
    return filename.replace('.html', '')


def main():
    html_dir = sys.argv[1]

    html_files = get_files(html_dir, ['*.html'],
                           exclude_patterns=[output_name, '*_graphical.html', 'graphical_index.html'])
    img_html_files = {}

    for filename in os.listdir(html_dir):
        if filename in img_blacklist:
            continue
        if file_matches(filename, img_patterns):
            for html_file in html_files:
                if img_in_html(os.path.join(html_dir, html_file), filename):
                    img_html_files[filename] = html_file
                    # for now suppose one image per html

    with open(os.path.join(html_dir, output_name), 'w') as output:
        output.write(header1_tmpl.substitute(title="GRASS GIS %s Reference "
                                               "Manual: Manual gallery" % grass_version))
        output.write(header_graphical_index_tmpl)
        output.write('<ul class="img-list">\n')
        for image, html_file in sorted(img_html_files.items()):
            name = get_module_name(html_file)
            title = title_from_names(name, image)
            output.write(
                '<li>'
                '<a href="{html}" title="{title}">'
                '<img src="{img}">'
                '<span class="name">{name}</span>'
                '</a>'
                '</li>\n'
                .format(html=html_file, img=image, title=title, name=name))
        output.write('</ul>')
        write_html_footer(output, "index.html", year)


if __name__ == '__main__':
    main()
