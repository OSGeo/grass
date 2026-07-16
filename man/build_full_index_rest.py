#!/usr/bin/env python3

# generates docs/rest/full_index.txt
# (C) 2003-2009 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements

import os

from build_rest import (
    check_for_desc_override,
    cmd2_tmpl,
    desc1_tmpl,
    full_index_header,
    get_desc,
    grass_version,
    replace_file,
    rest_dir,
    rest_files,
    sections,
    write_rest_footer,
    write_rest_header,
)

os.chdir(rest_dir)

classes = []
for cmd in rest_files("*"):
    prefix = cmd.split(".")[0]
    if prefix not in classes:
        classes.append(prefix)
classes.sort()

# begin full index:
filename = "full_index.txt"
with open(filename + ".tmp", "wb") as f:
    write_rest_header(f, "GRASS %s Reference Manual: Full index" % grass_version)

    # generate main index of all modules:
    f.write(full_index_header)
    # "

    # for cls in classes:
    # f.write(cmd1_tmpl.substitute(cmd = cls))
    # if cls != classes[-1]:
    # f.write(" | ")

    f.write(sections)

    # for all module groups:
    for cls in classes:
        f.write(cmd2_tmpl.substitute(cmd=cls))
        # for all modules:
        for cmd in rest_files(cls):
            basename = os.path.splitext(cmd)[0]
            desc = check_for_desc_override(basename)
            if desc is None:
                desc = get_desc(cmd)
            f.write(desc1_tmpl.substitute(basename=basename, desc=desc))
        f.write("\n")

    write_rest_footer(f, "index.txt")

replace_file(filename)
