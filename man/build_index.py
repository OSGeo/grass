#!/usr/bin/env python3

# generates docs/html/index.html
# (C) 2003-2009 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements

import sys
import os

from build import (
    write_header,
    write_cmd_overview,
    write_footer,
    replace_file,
    grass_version,
)

year = None
if len(sys.argv) > 1:
    year = sys.argv[1]


def build_index(ext):
    if ext == "html":
        from build_html import (
            man_dir,
        )
    else:
        from build_md import (
            man_dir,
        )

    filename = f"index.{ext}"
    os.chdir(man_dir)
    with open(filename + ".tmp", "w") as f:
        write_header(f, f"GRASS {grass_version} Reference Manual", True, template=ext)
        write_cmd_overview(f)
        write_footer(f, f"index.{ext}", year, template=ext)
    replace_file(filename)


if __name__ == "__main__":
    build_index("html")
