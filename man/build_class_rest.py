#!/usr/bin/env python3

# generates REST man pages docs/rest/<category>.txt
# (C) 2003-2009 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements

import os
import sys

from build_rest import (
    check_for_desc_override,
    desc2_tmpl,
    get_desc,
    grass_version,
    modclass_intro_tmpl,
    modclass_tmpl,
    replace_file,
    rest_dir,
    rest_files,
    write_rest_footer,
    write_rest_header,
)

os.chdir(rest_dir)

# write separate module pages:

# for all module groups:
cls = sys.argv[1]
modclass = sys.argv[2]

filename = modclass + ".txt"
with open(filename + ".tmp", "wb") as f:
    write_rest_header(f, "GRASS %s Reference Manual: %s" % (grass_version, modclass))
    if modclass.lower() not in {"general", "miscellaneous", "postscript"}:
        f.write(
            modclass_intro_tmpl.substitute(
                modclass=modclass, modclass_lower=modclass.lower()
            )
        )
    f.write(modclass_tmpl.substitute(modclass=modclass))

    # for all modules:
    for cmd in rest_files(cls):
        basename = os.path.splitext(cmd)[0]
        desc = check_for_desc_override(basename)
        if desc is None:
            desc = get_desc(cmd)
        f.write(desc2_tmpl.substitute(basename=basename, desc=desc))

    write_rest_footer(f, "index.txt")

replace_file(filename)
