#!/usr/bin/env python3

############################################################################
#
# MODULE:       Builds manual pages (Markdown)
# AUTHOR(S):    Markus Neteler
#               Glynn Clements
#               Martin Landa <landa.martin gmail.com>
# PURPOSE:      Create Markdown manual page snippets
#               Inspired by mkhtml.py
# COPYRIGHT:    (C) 2024 by the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

import os
import sys
import string
import re
import urllib.parse as urlparse

try:
    import grass.script as gs
except ImportError:
    # During compilation GRASS GIS
    gs = None

from mkdocs import (
    read_file,
    get_version_branch,
    get_last_git_commit,
    top_dir,
    get_addon_path,
)


def parse_source(pgm):
    """Parse source code to get source code and log message URLs,
    and date time of the last modification.

    :param str pgm: program name

    :return url_source, url_log, date_time
    """
    grass_version = os.getenv("VERSION_NUMBER", "unknown")
    main_url = ""
    addons_url = ""
    grass_git_branch = "main"
    major, minor, patch = None, None, None
    if grass_version != "unknown":
        major, minor, patch = grass_version.split(".")
        base_url = "https://github.com/OSGeo/"
        main_url = urlparse.urljoin(
            base_url,
            urlparse.urljoin(
                "grass/tree/",
                grass_git_branch + "/",
            ),
        )
        addons_url = urlparse.urljoin(
            base_url,
            urlparse.urljoin(
                "grass-addons/tree/",
                get_version_branch(
                    major,
                    urlparse.urljoin(base_url, "grass-addons/"),
                ),
            ),
        )

    cur_dir = os.path.abspath(os.path.curdir)
    if cur_dir.startswith(top_dir + os.path.sep):
        source_url = main_url
        pgmdir = cur_dir.replace(top_dir, "").lstrip(os.path.sep)
    else:
        # addons
        source_url = addons_url
        pgmdir = os.path.sep.join(cur_dir.split(os.path.sep)[-3:])

    url_source = ""
    addon_path = None
    if os.getenv("SOURCE_URL", ""):
        addon_path = get_addon_path(base_url=base_url, pgm=pgm, major_version=major)
        if addon_path:
            # Addon is installed from the local dir
            if os.path.exists(os.getenv("SOURCE_URL")):
                url_source = urlparse.urljoin(
                    addons_url,
                    addon_path,
                )
            else:
                url_source = urlparse.urljoin(
                    os.environ["SOURCE_URL"].split("src")[0],
                    addon_path,
                )
    else:
        url_source = urlparse.urljoin(source_url, pgmdir)
    if sys.platform == "win32":
        url_source = url_source.replace(os.path.sep, "/")

    # Process Source code section
    branches = "branches"
    tree = "tree"
    commits = "commits"

    if branches in url_source:
        url_log = url_source.replace(branches, commits)
        url_source = url_source.replace(branches, tree)
    else:
        url_log = url_source.replace(tree, commits)

    git_commit = get_last_git_commit(
        src_dir=cur_dir,
        top_dir=top_dir,
        pgm=pgm,
        addon_path=addon_path or None,
        major_version=major,
    )
    if git_commit["commit"] == "unknown":
        date_tag = "Accessed: {date}".format(date=git_commit["date"])
    else:
        commit = git_commit["commit"]
        date_tag = (
            "Latest change: {date} in commit: "
            "[{commit_short}](https://github.com/OSGeo/grass/commit/{commit})".format(
                date=git_commit["date"], commit=commit, commit_short=commit[:7]
            )
        )

    return url_source, url_log, date_tag


def extract_yaml_header(md_content):
    """Extract YAML header and content without the header from an MD file."""
    match = re.match(r"^---\n(.*?)\n---\n(.*)", md_content, re.DOTALL)
    if match:
        yaml_header = match.group(1).strip()
        content = match.group(2).strip()
    else:
        yaml_header = None
        content = md_content.strip()
    return yaml_header, content


def merge_md_files(md1, md2):
    """Merge two markdown files by concatenating their YAML headers and content."""
    yaml1, content1 = extract_yaml_header(md1)
    yaml2, content2 = extract_yaml_header(md2)

    # Contatenate the headers.
    yaml_items = []
    if yaml1:
        yaml_items.append(yaml1)
    if yaml2:
        yaml_items.append(yaml2)
    combined_yaml = "\n".join(yaml_items)

    # Attach the rest of the document.
    return ["---\n", combined_yaml, "\n---\n\n", content1, "\n", content2, "\n"]


def main():
    pgm = sys.argv[1]

    src_file = f"{pgm}.md"
    tmp_file = f"{pgm}.tmp.md"

    sourcecode = string.Template(
        """
## SOURCE CODE

Available at: [${PGM} source code](${URL_SOURCE})
([history](${URL_LOG}))${MD_NEWLINE}
${DATE_TAG}
"""
    )

    # process header/usage generated by --md-description
    tool_generated = read_file(tmp_file)
    # process body
    source_code = read_file(src_file)
    # combine
    for item in merge_md_files(tool_generated, source_code):
        if not item:
            continue
        sys.stdout.write(item)

    # process footer
    url_source, url_log, date_tag = parse_source(pgm)
    sys.stdout.write(
        sourcecode.substitute(
            URL_SOURCE=url_source,
            PGM=pgm,
            URL_LOG=url_log,
            DATE_TAG=date_tag,
            MD_NEWLINE="  ",
        )
    )


if __name__ == "__main__":
    main()
