#!/usr/bin/env python3

# generates topics.html and topic_*.html
# (c) 2012-2024 by the GRASS Development Team, Markus Neteler, Luca Delucchi, Martin Landa

import os
import glob

year = os.getenv("VERSION_DATE")

min_num_modules_for_topic = 3


def build_topics(ext):
    keywords = {}

    files = glob.glob1(path, f"*.{ext}")

    for fname in files:
        fil = open(os.path.join(path, fname))
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
        except:
            continue
        try:
            if ext == "html":
                key = lines[index_keys].split(",")[1].strip().replace(" ", "_")
                key = key.split(">")[1].split("<")[0]
            else:
                # expecting markdown
                key = lines[index_keys].split("]")[0].lstrip("[")
        except:
            continue
        try:
            desc = lines[index_desc].split("-", 1)[1].strip()
        except:
            desc.strip()

        if key not in keywords.keys():
            keywords[key] = {}
            keywords[key][fname] = desc
        elif fname not in keywords[key]:
            keywords[key][fname] = desc

    topicsfile = open(os.path.join(path, f"topics.{ext}"), "w")
    if ext == "html":
        topicsfile.write(
            header1_tmpl.substitute(
                title="GRASS GIS " "%s Reference Manual: Topics index" % grass_version
            )
        )
    topicsfile.write(headertopics_tmpl)

    for key, values in sorted(keywords.items(), key=lambda s: s[0].lower()):
        keyfile = open(os.path.join(path, f"topic_%s.{ext}" % key), "w")
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
                [moduletopics_tmpl.substitute(key=key, name=key.replace("_", " "))]
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
                " for additional references.</em>".format(key=key.replace("_", " "))
            )
        else:
            # expecting markdown
            keyfile.write(
                "*See also the corresponding keyword"
                " [{key}](keywords.md#{key})"
                " for additional references.*\n".format(key=key.replace("_", " "))
            )

        write_footer(keyfile, f"index.{ext}", year)
    if ext == "html":
        topicsfile.write("</ul>\n")
    write_footer(topicsfile, f"index.{ext}", year)
    topicsfile.close()


if __name__ == "__main__":
    from build_html import (
        header1_tmpl,
        grass_version,
        headertopics_tmpl,
        headerkey_tmpl,
        desc1_tmpl,
        write_html_footer as write_footer,
        moduletopics_tmpl,
        html_dir as path,
    )

    build_topics("html")

    from build_md import (
        grass_version,
        headertopics_tmpl,
        headerkey_tmpl,
        desc1_tmpl,
        write_md_footer as write_footer,
        moduletopics_tmpl,
        md_dir as path,
    )

    build_topics("md")
