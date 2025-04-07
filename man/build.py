#!/usr/bin/env python3

# utilities for generating HTML indices
# (C) 2003-2025 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements
#   Luca Delucchi

import os
import string
from datetime import datetime
from pathlib import Path

# TODO: better fix this in include/Make/Html.make, see bug RT #5361

# exclude following list of modules from help index:

exclude_mods = [
    "i.find",
    "r.watershed.ram",
    "r.watershed.seg",
    "v.topo.check",
    "helptext.html",
]

# these modules don't use G_parser()

desc_override = {
    "g.parser": "Provides automated parser, GUI, and help support for GRASS scripts.",
    "r.li.daemon": "Support module for r.li landscape index calculations.",
}

# File template pieces follow

message_tmpl = string.Template(
    r"""Generated HTML docs in ${man_dir}/index.html
----------------------------------------------------------------------
Following modules are missing the 'modulename.html' file in src code:
"""
)

############################################################################


def check_for_desc_override(basename):
    return desc_override.get(basename)


def read_file(name):
    return Path(name).read_text()


def write_file(name, contents):
    Path(name).write_text(contents)


def try_mkdir(path):
    try:
        os.mkdir(path)
    except OSError:
        pass


def replace_file(name):
    temp = name + ".tmp"
    if (
        os.path.exists(name)
        and os.path.exists(temp)
        and read_file(name) == read_file(temp)
    ):
        os.remove(temp)
    else:
        try:
            os.remove(name)
        except OSError:
            pass
        os.rename(temp, name)


def copy_file(src, dst):
    write_file(dst, read_file(src))


def get_files(man_dir, cls=None, ignore_gui=True, extension="html"):
    for cmd in sorted(os.listdir(man_dir)):
        if (
            cmd.endswith(f".{extension}")
            and (cls in (None, "*") or cmd.startswith(cls + "."))
            and (cls != "*" or len(cmd.split(".")) >= 3)
            and cmd not in [f"full_index.{extension}", f"index.{extension}"]
            and cmd not in exclude_mods
            and ((ignore_gui and not cmd.startswith("wxGUI.")) or not ignore_gui)
        ):
            yield cmd


def write_header(f, title, ismain=False, body_width="99%", template="html"):
    if template == "html":
        from build_html import header1_tmpl, macosx_tmpl, header2_tmpl
    else:
        from build_md import header1_tmpl, header2_tmpl
    f.write(header1_tmpl.substitute(title=title))
    if ismain and macosx and template == "html":
        f.write(
            macosx_tmpl.substitute(grass_version=grass_version, grass_mmver=grass_mmver)
        )
    f.write(header2_tmpl.substitute(grass_version=grass_version, body_width=body_width))


def write_cmd_overview(f, template="html"):
    from build_html import overview_tmpl

    if template == "html":
        f.write(
            overview_tmpl.substitute(
                grass_version_major=grass_version_major,
                grass_version_minor=grass_version_minor,
            )
        )


def write_footer(f, index_url, year=None, template="html"):
    if template == "html":
        from build_html import footer_tmpl

        cur_year = default_year if year is None else year
        f.write(
            footer_tmpl.substitute(
                grass_version=grass_version, index_url=index_url, year=cur_year
            )
        )


def to_title(name):
    """Convert name of command class/family to form suitable for title"""
    if name.lower() == "postscript":
        return "PostScript"
    if name.lower() == "3d raster":
        return "3D raster"
    return name.capitalize()


############################################################################

arch_dist_dir = os.environ["ARCH_DISTDIR"]
gisbase = os.environ["GISBASE"]
grass_version = os.getenv("VERSION_NUMBER", "unknown")
grass_version_major = grass_version.split(".")[0]
grass_version_minor = grass_version.split(".")[1]
grass_mmver = ".".join(grass_version.split(".")[0:2])
macosx = "darwin" in os.environ["ARCH"].lower()
default_year = os.getenv("VERSION_DATE")
if not default_year:
    default_year = str(datetime.now().year)

############################################################################
