#!/usr/bin/env python3

"""
Generates keywords.html file for core and optionally for addons modules.

Usage:

Generate core modules keywords HTML page

python man/build_keywords.py <path_to_core_modules_html_man_files>

Generate core modules and optionally inject addons keywords HTML page

python man/build_keywords.py <dir_path_to_core_modules_html_man_files>
    <dir_path_to_addons_modules_html_man_files>

Generate Markdown keywords file:

python man/build_keywords.py md <path_to_core_file> <path_to_addon_files>

If called without parameters, ARCH_DISTDIR environment variable is used
to determine the path to core documentation. If parameters are provided,
the variable is ignored, but it must be set regardless.

@author Luca Delucchi
@author Tomas Zigo <tomas.zigo slovanet.sk> - inject addons modules keywords
@author Martin Landa - Markdown support
"""

import os
import re
import sys
import glob

from build import (
    grass_version,
    write_footer,
)

keywords_to_hide_in_overview = [
    "General",
    "Misc",
]

year = os.getenv("VERSION_DATE")


def get_module_man_file_path(man_dir, addons_path, module, addons_man_files):
    """Get module manual HTML file path

    :param str module: module manual HTML file name e.g. v.surf.rst.html
    :param addons_man_files: list of HTML manual files

    :return str module_path: core/addon module manual HTML file path
    """
    if addons_path and module in ",".join(addons_man_files):
        module_path = os.path.join(addons_path, module)
        module_path = module_path.replace(
            os.path.commonpath([man_dir, module_path]),
            ".",
        )
    else:
        module_path = f"./{module}"
    return module_path


def build_keywords(ext, main_path, addons_path):
    if ext == "html":
        from build_html import header1_tmpl, headerkeywords_tmpl, man_dir
    else:
        from build_md import (
            header1_tmpl,
            headerkeywords_tmpl,
            man_dir,
        )

    keywords = {}

    main_doc_dir = main_path or man_dir

    files = glob.glob(os.path.join(main_doc_dir, f"*.{ext}"))
    # TODO: add markdown support
    if addons_path:
        addons_man_files = glob.glob(os.path.join(addons_path, f"*.{ext}"))
        files.extend(addons_man_files)
    else:
        addons_man_files = []

    for in_file in files:
        fname = os.path.basename(in_file)
        with open(in_file) as f:
            lines = f.readlines()

        if ext == "html":
            # TODO maybe move to Python re (regex)
            try:
                index_keys = lines.index("<h2>KEYWORDS</h2>\n") + 1
            except Exception:
                continue
            try:
                keys = []
                for k in lines[index_keys].split(","):
                    keys.append(k.strip().split(">")[1].split("<")[0])
            except Exception:
                continue
        else:
            keys = []
            for line in lines:
                # We accept, but don't require, YAML inline list syntax.
                match = re.match(r"keywords:\s*\[\s*(.*)\s*\]\s*", line)
                if not match:
                    match = re.match(r"keywords:\s*(.*)\s*", line)
                if match:
                    text = match.group(1)
                    if not text:
                        print(
                            f"Warning: Empty keyword list in {fname}", file=sys.stderr
                        )
                        break
                    # We accept only non-quoted YAML strings.
                    keys = [item.strip() for item in text.split(",")]
                    break

        # No keywords in file is allowed because some non-tool pages don't have
        # keywords, so we don't check for that specifically (only set but broken
        # keywords are flagged).

        for key in keys:
            if not key:
                print(
                    f"Warning: Empty keyword in {fname}, all keywords: {keys}",
                    file=sys.stderr,
                )
                continue
            if key not in keywords.keys():
                keywords[key] = []
                keywords[key].append(fname)
            elif fname not in keywords[key]:
                keywords[key].append(fname)

    for keyword in keywords_to_hide_in_overview:
        try:
            del keywords[keyword]
        except Exception:
            try:
                del keywords[keyword.lower()]
            except Exception:
                continue

    def create_char_list(keywords):
        """Create a dict with the first letters and corresponding first keywords.

        This list it is useful to create the TOC using only the first character
        for keyword.
        """
        char_list = {}
        for key in sorted(keywords.keys()):
            firstchar = key[0].lower()
            if firstchar not in char_list.keys():
                char_list[str(firstchar)] = key
            elif firstchar in char_list.keys():
                if key.lower() < char_list[str(firstchar)].lower():
                    char_list[str(firstchar.lower())] = key
        return char_list

    with open(os.path.join(main_doc_dir, f"keywords.{ext}"), "w") as keywordsfile:
        keywordsfile.write(
            header1_tmpl.substitute(
                title=f"GRASS {grass_version} Reference Manual - Keywords index"
            )
        )
        keywordsfile.write(headerkeywords_tmpl)
        if ext == "html":
            keywordsfile.write("<dl>")
        sortedKeys = sorted(keywords.keys(), key=lambda s: s.lower())

        for key in sortedKeys:
            if ext == "html":
                keyword_line = '<dt><b><a name="{key}" class="urlblack">{key}</a></b></dt><dd>'.format(  # noqa: E501
                    key=key
                )
            else:
                keyword_line = f"### **{key}**\n"
            for value in sorted(keywords[key]):
                man_file_path = get_module_man_file_path(
                    man_dir=main_doc_dir,
                    addons_path=addons_path,
                    module=value,
                    addons_man_files=addons_man_files,
                )
                if ext == "html":
                    keyword_line += f' <a href="{man_file_path}">{value.replace(f".{ext}", "")}</a>,'  # noqa: E501
                else:
                    keyword_line += f" [{value.rsplit('.', 1)[0]}]({man_file_path}),"
            keyword_line = keyword_line.rstrip(",")
            if ext == "html":
                keyword_line += "</dd>"
            keyword_line += "\n"
            keywordsfile.write(keyword_line)
        if ext == "html":
            keywordsfile.write("</dl>\n")
        if ext == "html":
            # create toc
            toc = '<div class="toc">\n<h4 class="toc">Table of contents</h4><p class="toc">'  # noqa: E501
            test_length = 0
            char_list = create_char_list(keywords)
            all_keys = len(char_list.keys())
            for k in sorted(char_list.keys()):
                test_length += 1
                # toc += '<li><a href="#%s" class="toc">%s</a></li>' % (char_list[k], k)
                if test_length % 4 == 0 and test_length != all_keys:
                    toc += '\n<a href="#%s" class="toc">%s</a>, ' % (char_list[k], k)
                elif test_length % 4 == 0 and test_length == all_keys:
                    toc += '\n<a href="#%s" class="toc">%s</a>' % (char_list[k], k)
                elif test_length == all_keys:
                    toc += '<a href="#%s" class="toc">%s</a>' % (char_list[k], k)
                else:
                    toc += '<a href="#%s" class="toc">%s</a>, ' % (char_list[k], k)
            toc += "</p></div>\n"
            keywordsfile.write(toc)

        write_footer(keywordsfile, f"index.{ext}", year, template=ext)


def main():
    if len(sys.argv) == 1:
        # Build only HTML by default.
        build_keywords("html", main_path=None, addons_path=None)
        return

    if len(sys.argv) >= 2:
        doc_type = sys.argv[1]
    if doc_type in ["html", "md"]:
        offset = 1
    else:
        # The original usage according to the build sever scripts.
        offset = 0
        doc_type = "html"

    # Usage allowing to pick Markdown-only build.
    main_path = None
    if len(sys.argv) >= 2 + offset:
        main_path = sys.argv[1 + offset]
    addons_path = None
    if len(sys.argv) >= 3 + offset:
        addons_path = sys.argv[2 + offset]

    build_keywords(doc_type, main_path=main_path, addons_path=addons_path)


if __name__ == "__main__":
    main()
