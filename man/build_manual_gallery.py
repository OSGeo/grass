#!/usr/bin/env python3

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
from __future__ import annotations

import os
from pathlib import Path
import fnmatch
import re
from typing import TYPE_CHECKING
import operator

if TYPE_CHECKING:
    from collections.abc import Iterable

img_extensions = ["png", "jpg", "gif"]
img_patterns = ["*." + extension for extension in img_extensions]

# we don't want some images to show up
# logos
img_blacklist = ["grass_logo.png", "grass_icon.png"]
# circles with numbers from helptext.html (unfortunate we have to list it here)
# perhaps some general name ending would be good, like *_noindex.png
img_blacklist.extend(["circle_{0}.png".format(num) for num in range(1, 6)])

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
<h2>GRASS manual gallery</h2>
"""

# The recommeded width from style guide.
MIN_IMAGE_WIDTH = 600


def image_width(filename):
    """Get image size in pixels (width times height)"""
    try:
        from PIL import Image
    except ImportError:
        return None

    return Image.open(filename).size[0]  # First element is width.


def img_in_file(filename: str | os.PathLike[str], imagename: str, ext: str) -> bool:
    # for some reason, calling search just once is much faster
    # than calling it on every line (time is spent in _compile)
    if ext == "html":
        pattern = re.compile("<img .*src=.{0}.*>".format(imagename))
    else:
        # expecting markdown
        pattern = re.compile(r"\]\({0}\)".format(imagename))
    return bool(re.search(pattern, Path(filename).read_text()))


def file_matches(filename: str, patterns: Iterable[str]):
    return any(fnmatch.fnmatch(filename, pattern) for pattern in patterns)


def get_files(directory, patterns, exclude_patterns):
    files = []
    for filename in sorted(os.listdir(directory)):
        if file_matches(filename, patterns):
            if not file_matches(filename, exclude_patterns):
                files.append(filename)
    return files


def remove_module_name(string, module):
    string = string.replace(module.replace("wxGUI.", "g.gui."), "")
    string = string.replace(module.replace(".", "_"), "")  # using _
    string = string.replace(module.replace(".", ""), "")  # using nothing
    return string.replace(module, "")  # using original dots


def title_from_names(module_name, img_name):
    # we ignore the possibility of having extension at the end of image
    # so possibly r.out.png fails but image name should use _ anyway
    # strictly speaking, it could be also, e.g., index name
    for extension in img_extensions:
        img_name = img_name.replace("." + extension, "")
    img_name = remove_module_name(img_name, module_name)
    img_name = img_name.replace("_", " ")
    img_name = img_name.strip()
    if img_name:
        return "{name} ({desc})".format(name=module_name, desc=img_name)
    return "{name}".format(name=module_name)


def get_module_name(filename, ext):
    return filename.replace(f".{ext}", "")


def main(ext):
    if ext == "html":
        from build_html import (
            header1_tmpl,
            man_dir,
        )
    else:
        from build_md import (
            header1_tmpl,
            man_dir,
        )

    output_name = f"manual_gallery.{ext}"

    man_files = get_files(
        man_dir,
        [f"*.{ext}"],
        exclude_patterns=[output_name, f"*_graphical.{ext}", f"graphical_index.{ext}"],
    )
    img_files = {}

    for filename in os.listdir(man_dir):
        if filename in img_blacklist:
            continue
        if file_matches(filename, img_patterns):
            width = image_width(Path(man_dir, filename))
            if width is not None and width < MIN_IMAGE_WIDTH:
                # Skip small images.
                continue
            for man_filename in man_files:
                if img_in_file(Path(man_dir, man_filename), filename, ext):
                    img_files[filename] = man_filename
                    # for now suppose one image per manual filename
                    continue

    with open(Path(man_dir, output_name), "w") as output:
        if ext == "html":
            title = "GRASS %s Reference Manual: Manual gallery" % grass_version
        else:
            title = "Manual gallery"
        output.write(header1_tmpl.substitute(title=title))
        if ext == "html":
            output.write(header_graphical_index_tmpl)
            output.write('<ul class="img-list">\n')
        for image, filename in sorted(img_files.items(), key=operator.itemgetter(1)):
            name = get_module_name(filename, ext)
            title = title_from_names(name, image)
            if ext == "html":
                output.write(
                    "<li>"
                    '<a href="{fn}" title="{title}">'
                    '<img src="{img}">'
                    '<span class="name">{name}</span>'
                    "</a>"
                    "</li>\n".format(fn=filename, img=image, title=title, name=name)
                )
            else:
                output.write(
                    f'[![{name}]({image} "{title}")]({filename})  \n*{title}*\n\n'
                )
        if ext == "html":
            output.write("</ul>")
        write_footer(output, f"index.{ext}", year, template=ext)

    return img_files


if __name__ == "__main__":
    from build import (
        write_footer,
        grass_version,
    )

    img_files_html = main("html")

    img_files_md = main("md")

    # TODO: img_files_html and img_files_md should be the same
    # remove lines when fixed
    for k in img_files_html:
        if k not in img_files_md:
            print(k)
