#!/usr/bin/env python3

# generates docs/html/index.html
# (C) 2003-2009 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements

import sys
import os

<<<<<<< markdown_docs
from build import (
    write_header,
    write_cmd_overview,
    write_footer,
    replace_file,
    grass_version,
)
=======
from build_html import (
    html_dir,
    grass_version,
    write_html_header,
    write_html_cmd_overview,
    write_html_footer,
    replace_file,
)

os.chdir(html_dir)

filename = "index.html"
f = open(filename + ".tmp", "w")
>>>>>>> main

year = None
if len(sys.argv) > 1:
    year = sys.argv[1]


def build_index(ext):
    filename = f"index.{ext}"
    os.chdir(man_dir)
    with open(filename + ".tmp", "w") as f:
        write_header(
            f, f"GRASS GIS {grass_version} Reference Manual", True, template=ext
        )
        write_cmd_overview(f)
        write_footer(f, f"index.{ext}", year, template=ext)
    replace_file(filename)


if __name__ == "__main__":
    from build_html import (
        man_dir,
    )

    build_index("html")

    from build_md import (
        man_dir,
    )

    build_index("md")
