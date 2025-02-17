#!/usr/bin/env python3

# generates topics.html and topic_*.html
# (c) 2012-2025 by the GRASS Development Team

import os
import glob
from pathlib import Path

year = os.getenv("VERSION_DATE")

min_num_modules_for_topic = 3


def build_topics(ext):
    if ext == "html":
        from build_html import (
            header1_tmpl,
            headertopics_tmpl,
            headerkey_tmpl,
            desc1_tmpl,
            moduletopics_tmpl,
            man_dir,
        )
    else:
        from build_md import (
            header1_tmpl,
            headertopics_tmpl,
            headerkey_tmpl,
            desc1_tmpl,
            moduletopics_tmpl,
            man_dir,
        )

    keywords = {}

    files = glob.glob1(man_dir, f"*.{ext}")
    for fname in files:
        with Path(man_dir, fname).open() as fil:
            # TODO maybe move to Python re (regex)
            lines = fil.readlines()
        try:
            if ext == "html":
                index_keys = lines.index("<h2>KEYWORDS</h2>\n") + 1
                index_desc = lines.index("<h2>NAME</h2>\n") + 1
            else:
                # expecting markdown
                index_keys = lines.index("### KEYWORDS\n") + 3
                index_desc = lines.index("## NAME\n") + 2
        except Exception:
            continue
        try:
            if ext == "html":
                key = lines[index_keys].split(",")[1].strip().replace(" ", "_")
                key = key.split(">")[1].split("<")[0]
            else:
                # expecting markdown
                key = lines[index_keys].split("]")[0].lstrip("[")
        except Exception:
            continue
        try:
            desc = lines[index_desc].split("-", 1)[1].strip()
        except Exception:
            desc = desc.strip()

        # Line ending can appear here on Windows.
        key = key.strip()
        if not key:
            continue

        if key not in keywords.keys():
            keywords[key] = {}
            keywords[key][fname] = desc
        elif fname not in keywords[key]:
            keywords[key][fname] = desc

    with Path(man_dir, f"topics.{ext}").open("w") as topicsfile:
        topicsfile.write(
            header1_tmpl.substitute(
                title="GRASS GIS %s Reference Manual - Topics index" % grass_version
            )
        )
        topicsfile.write(headertopics_tmpl)

        for key, values in sorted(keywords.items(), key=lambda s: s[0].lower()):
            with Path(man_dir, f"topic_%s.{ext}" % key.replace(" ", "_")).open(
                "w"
            ) as keyfile:
                if ext == "html":
                    keyfile.write(
                        header1_tmpl.substitute(
                            title="GRASS GIS "
                            "%s Reference Manual: Topic %s"
                            % (grass_version, key.replace("_", " "))
                        )
                    )
                keyfile.write(headerkey_tmpl.substitute(keyword=key.replace("_", " ")))
                num_modules = 0
                for mod, desc in sorted(values.items()):
                    num_modules += 1
                    keyfile.write(
                        desc1_tmpl.substitute(
                            cmd=mod, desc=desc, basename=mod.replace(f".{ext}", "")
                        )
                    )
                if num_modules >= min_num_modules_for_topic:
                    topicsfile.writelines(
                        [
                            moduletopics_tmpl.substitute(
                                key=key.replace(" ", "_"), name=key.replace("_", " ")
                            )
                        ]
                    )
                if ext == "html":
                    keyfile.write("</table>\n")
                else:
                    keyfile.write("\n")
                # link to the keywords index
                # TODO: the labels in keywords index are with spaces and capitals
                # this should be probably changed to lowercase with underscores
                if ext == "html":
                    keyfile.write(
                        "<p><em>See also the corresponding keyword"
                        ' <a href="keywords.html#{key}">{key}</a>'
                        " for additional references.</em>".format(
                            key=key.replace("_", " ")
                        )
                    )
                else:
                    # expecting markdown
                    keyfile.write(
                        "*See also the corresponding keyword"
                        " [{name}](keywords.md#{key})"
                        " for additional references.*\n".format(
                            key=key.replace(" ", "-").replace("_", "-").lower(),
                            name=key.replace("_", " "),
                        )
                    )

                write_footer(keyfile, f"index.{ext}", year, template=ext)

        if ext == "html":
            topicsfile.write("</ul>\n")
        write_footer(topicsfile, f"index.{ext}", year, template=ext)


if __name__ == "__main__":
    from build import (
        grass_version,
        write_footer,
    )

    build_topics("html")

    build_topics("md")
