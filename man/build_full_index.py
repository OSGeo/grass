#!/usr/bin/env python3

# generates docs/html/full_index.html
# (C) 2003-2009 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements

import sys
import os

from datetime import date
from operator import itemgetter

from build import (
    get_files,
    write_footer,
    write_header,
    grass_version,
    grass_version_major,
    grass_version_minor,
    check_for_desc_override,
    replace_file,
)

CORE_TEXT = """\
# Tools

GRASS offers a comprehensive set of tools for geospatial processing, modeling,
analysis, and visualization. These tools are categorized by data type,
with prefixes indicating their respective categories. Use the table below to explore
the main tool categories:

| Prefix | Category                          | Description                        |
|--------|-----------------------------------|------------------------------------|
| `g.`   | [General](general.md)             | General GIS management tools       |
| `r.`   | [Raster](raster.md)               | Raster data processing tools       |
| `r3.`  | [3D raster](raster3d.md)          | 3D Raster data processing tools    |
| `v.`   | [Vector](vector.md)               | Vector data processing tools       |
| `i.`   | [Imagery](imagery.md)             | Imagery processing tools           |
| `t.`   | [Temporal](temporal.md)           | Temporal data processing tools     |
| `db.`  | [Database](database.md)           | Database management tools          |
| `d.`   | [Display](display.md)             | Display and visualization tools    |
"""

ADDONS_TEXT = """\
# Addon Tools

GRASS is free and open source software,
anyone may develop their own extensions (addons).
The [GRASS Addons repository](https://github.com/OSGeo/grass-addons)
on GitHub contains a growing list of GRASS
tools, which are currently not part of the core software package, but
can easily be  <b>installed</b> in your local GRASS installation
through the graphical user interface (<i>Menu - Settings - Addons
Extension - Install</i>) or via the <a
href="../g.extension.html">g.extension</a> command.

## How to contribute?
You may propose your Addon to the [GRASS Addons repository](https://github.com/OSGeo/grass-addons).
Please read the [addons contributing file](https://github.com/OSGeo/grass-addons/blob/grass8/CONTRIBUTING.md)
as well as the [GRASS style guide](style_guide.md)</a>.

These manual pages are updated daily. Last run: {date}.
If you don't see an addon you know exists, please check the log files of compilation:
<a href="{linux_logs}">Linux log files</a> |
<a href="{windows_logs}">Windows log files</a>

## Tools
"""


def build_full_index(ext, index_name, source_dir, year, text_type):
    """Generate index with all tools"""
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
            cmd2_tmpl,
            desc1_tmpl,
            get_desc,
        )

    if source_dir is None:
        source_dir = man_dir

    os.chdir(source_dir)

    # TODO: create some master function/dict somewhere
    class_labels = {
        "d": "Display",
        "db": "Database",
        "g": "General",
        "i": "Imagery",
        "m": "Miscellaneous",
        "ps": "PostScript",
        "r": "Raster",
        "r3": "3D raster",
        "t": "Temporal",
        "v": "Vector",
    }

    ignore_classes = ["test"]

    classes = []
    for cmd in get_files(source_dir, "*", extension=ext):
        prefix = cmd.split(".")[0]
        if prefix in ignore_classes:
            continue
        if prefix not in [item[0] for item in classes]:
            classes.append((prefix, class_labels.get(prefix, prefix)))
    # Sort by prefix.
    classes.sort(key=itemgetter(0))

    # begin full index:
    filename = f"{index_name}.{ext}"
    with open(filename + ".tmp", "w") as f:
        if ext == "html":
            text = f"GRASS {grass_version} Reference Manual - Full index"
            write_header(
                f,
                text,
                body_width="80%",
                template=ext,
            )
            f.write(full_index_header)
        elif text_type == "core":
            f.write(CORE_TEXT)
        elif text_type == "addons":
            linux = f"https://grass.osgeo.org/addons/grass{grass_version_major}/logs"
            windows = f"https://wingrass.fsv.cvut.cz/grass{grass_version_major}{grass_version_minor}/addons/grass-{grass_version}/logs/"
            f.write(
                ADDONS_TEXT.format(
                    date=date.today(), linux_logs=linux, windows_logs=windows
                )
            )
        else:
            msg = f"Unknown text type: {text_type}"
            raise ValueError(msg)

        if ext == "html":
            f.write(toc)

        # for all module groups:
        for cls, cls_label in classes:
            f.write(cmd2_tmpl.substitute(cmd_label=cls_label, cmd=cls))
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
    text_type = "core"
    if len(sys.argv) > 1:
        if sys.argv[1] in ["html", "md"]:
            ext = sys.argv[1]
        else:
            year = sys.argv[1]
    if len(sys.argv) > 3:
        index_name = sys.argv[2]
        source_dir = sys.argv[3]
    if len(sys.argv) > 4:
        text_type = sys.argv[4]
    if len(sys.argv) > 5:
        year = sys.argv[5]

    if ext is None:
        for ext in ["html", "md"]:
            build_full_index(
                ext,
                index_name=index_name,
                year=year,
                source_dir=source_dir,
                text_type=text_type,
            )
    elif ext in ["html", "md"]:
        build_full_index(
            ext,
            index_name=index_name,
            year=year,
            source_dir=source_dir,
            text_type=text_type,
        )
    else:
        msg = f"Unknown extension: {ext}"
        raise ValueError(msg)


if __name__ == "__main__":
    main()
