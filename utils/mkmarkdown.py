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
import re
import string
import subprocess
import http
from pathlib import Path
import urllib.parse as urlparse
from html.parser import HTMLParser

try:
    import grass.script as gs
except ImportError:
    # During compilation GRASS GIS
    gs = None

from mkdocs import read_file, get_version_branch, get_last_git_commit, top_dir

HEADERS = {
    "User-Agent": "Mozilla/5.0",
}
HTTP_STATUS_CODES = list(http.HTTPStatus)


def write_toc(data, hamburger_menu_toc=False):
    """Write Table of Contents

    :param tuple data: parsed data from MyHTMLParser class instance
    :param bool hamburger_menu_toc: write hamburger menu TOC for the
                                    mobile, tablet screen
    """

    if not data:
        return

    fd = sys.stdout
    if hamburger_menu_toc:
        fd.write("<script>\n")
        fd.write("// Create hamburger menu TOC HTML elements by the JavaScript\n")
        fd.write("let temp = document.createElement('template');\n")
        fd.write(
            """const toc = '<ul class="toc-mobile-screen" """
            """id="toc-mobile-screen">' + \n"""
        )
    else:
        fd.write('<div class="toc">\n')
        fd.write('<h4 class="toc">Table of contents</h4>\n')
        fd.write('<ul class="toc">\n')
    first = True
    has_h2 = False
    in_h3 = False
    indent = 4
    for tag, href, text in data:
        if tag == "h3" and not in_h3 and has_h2:
            if hamburger_menu_toc:
                fd.write("'<ul>' + \n")
            else:
                fd.write('\n%s<ul class="toc">\n' % (" " * indent))
            indent += 4
            in_h3 = True
        elif not first:
            if hamburger_menu_toc:
                fd.write("'</li>' + \n")
            else:
                fd.write("</li>\n")

        if tag == "h2":
            has_h2 = True
            if in_h3:
                indent -= 4
                if hamburger_menu_toc:
                    fd.write("'</ul></li>' + \n")
                else:
                    fd.write("%s</ul></li>\n" % (" " * indent))
                in_h3 = False

        text = text.replace("\xa0", " ")
        if hamburger_menu_toc:
            fd.write(
                f"""'<li><a class="toc-item" href="#{escape_href(text)}">"""
                f"{text}</a>' + \n"
            )
        else:
            fd.write(
                '%s<li class="toc"><a href="#%s" class="toc">%s</a>'
                % (" " * indent, escape_href(text), text)
            )
        first = False

    if hamburger_menu_toc:
        fd.write(
            """'</li>' +
'<a class="close" href="#">' +
'<img src="./hamburger_menu_close.svg" alt="close">' +
'</a>' +
'</ul>' +
'<a class="hamburger" href="#toc-mobile-screen">' +
'<img src="./hamburger_menu.svg" alt="menu">' +
'</a>';
temp.innerHTML = toc;
const grassLogoLink = document.getElementsByTagName("img")[0];
grassLogoLink.after(temp.content);
</script>
"""
        )
    else:
        fd.write("</li>\n</ul>\n")
        fd.write("</div>\n")


def update_toc(data):
    ret_data = []
    pat = re.compile(r"(<(h[2|3])>)(.+)(</h[2|3]>)")
    idx = 1
    for line in data.splitlines():
        if pat.search(line):
            xline = pat.split(line)
            line = (
                xline[1]
                + '<a name="%s">' % escape_href(xline[3])
                + xline[3]
                + "</a>"
                + xline[4]
            )
            idx += 1
        ret_data.append(line)

    return "\n".join(ret_data)


def escape_href(label):
    # remove html tags
    label = re.sub("<[^<]+?>", "", label)
    # fix &nbsp;
    label = label.replace("&nbsp;", "")
    # fix "
    label = label.replace('"', "")
    # replace space with underscore + lower
    return label.replace(" ", "-").lower()


def create_toc(src_data):
    class MyHTMLParser(HTMLParser):
        def __init__(self):
            HTMLParser.__init__(self)
            self.reset()
            self.idx = 1
            self.tag_curr = ""
            self.tag_last = ""
            self.process_text = False
            self.data = []
            self.tags_allowed = ("h1", "h2", "h3")
            self.tags_ignored = "img"
            self.text = ""

        def handle_starttag(self, tag, attrs):
            if tag in self.tags_allowed:
                self.process_text = True
            self.tag_last = self.tag_curr
            self.tag_curr = tag

        def handle_endtag(self, tag):
            if tag in self.tags_allowed:
                self.data.append((tag, "%s_%d" % (tag, self.idx), self.text))
                self.idx += 1
                self.process_text = False
                self.text = ""

            self.tag_curr = self.tag_last

        def handle_data(self, data):
            if not self.process_text:
                return
            if self.tag_curr in self.tags_allowed or self.tag_curr in self.tags_ignored:
                self.text += data
            else:
                self.text += "<%s>%s</%s>" % (self.tag_curr, data, self.tag_curr)

    # instantiate the parser and fed it some HTML
    parser = MyHTMLParser()
    parser.feed(src_data)

    return parser.data


def parse_source():
    """Parse source code to get source code and log message URLs,
    and date time of the last modification.

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
        addon_path = get_addon_path(base_url=base_url, major_version=major)
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


def get_addon_path(base_url, major_version):
    """Check if pgm is in the addons list and get addon path

    Make or update list of the official addons source
    code paths g.extension prefix parameter plus /grass-addons directory
    using Git repository

    :param str base_url: base URL
    :param str major_version: GRASS major version

    :return str|None: pgm path if pgm is addon else None
    """
    addons_base_dir = os.getenv("GRASS_ADDON_BASE")
    if addons_base_dir and major_version:
        grass_addons_dir = Path(addons_base_dir) / "grass-addons"
        if gs:
            call = gs.call
            popen = gs.Popen
            fatal = gs.fatal
        else:
            call = subprocess.call
            popen = subprocess.Popen
            fatal = sys.stderr.write
        addons_branch = get_version_branch(
            major_version=major_version,
            addons_git_repo_url=urlparse.urljoin(base_url, "grass-addons/"),
        )
        if not Path(addons_base_dir).exists():
            Path(addons_base_dir).mkdir(parents=True, exist_ok=True)
        if not grass_addons_dir.exists():
            call(
                [
                    "git",
                    "clone",
                    "-q",
                    "--no-checkout",
                    f"--branch={addons_branch}",
                    "--filter=blob:none",
                    urlparse.urljoin(base_url, "grass-addons/"),
                ],
                cwd=addons_base_dir,
            )
        addons_file_list = popen(
            ["git", "ls-tree", "--name-only", "-r", addons_branch],
            cwd=grass_addons_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        addons_file_list, stderr = addons_file_list.communicate()
        if stderr:
            message = (
                "Failed to get addons files list from the"
                " Git repository <{repo_path}>.\n{error}"
            )
            if gs:
                fatal(
                    _(
                        message,
                    ).format(
                        repo_path=grass_addons_dir,
                        error=gs.decode(stderr),
                    )
                )
            else:
                message += "\n"
                fatal(
                    message.format(
                        repo_path=grass_addons_dir,
                        error=stderr.decode(),
                    )
                )
        addon_paths = re.findall(
            rf".*{pgm}*.",
            gs.decode(addons_file_list) if gs else addons_file_list.decode(),
        )
        for addon_path in addon_paths:
            if pgm == Path(addon_path).name:
                return addon_path


def header(pgm):
    name = re.search("(<!-- meta page name:)(.*)(-->)", src_data, re.IGNORECASE)
    pgm_desc = "GRASS GIS Reference Manual"
    if name:
        pgm = name.group(2).strip().split("-", 1)[0].strip()
        name_desc = re.search(
            "(<!-- meta page name description:)(.*)(-->)", src_data, re.IGNORECASE
        )
        if name_desc:
            pgm_desc = name_desc.group(2).strip()
    desc = re.search("(<!-- meta page description:)(.*)(-->)", src_data, re.IGNORECASE)

    if desc:
        pgm = desc.group(2).strip()
        header_tmpl = string.Template(header_base + header_nopgm)
    elif not pgm_desc:
        header_tmpl = string.Template(header_base + header_pgm)
    else:
        header_tmpl = string.Template(header_base + header_pgm_desc)

    return header_tmpl.substitute(PGM=pgm, PGM_DESC=pgm_desc)


if __name__ == "__main__":
    pgm = sys.argv[1]

    src_file = f"{pgm}.md"
    tmp_file = f"{pgm}.tmp.md"

    header_base = """
# ${PGM} - GRASS GIS Manual
[![GRASS GIS](grass_logo.png 'GRASS GIS')](index.html)
"""

    header_nopgm = """## ${PGM}
"""

    header_pgm = """## NAME
    ***${PGM}***
"""

    header_pgm_desc = """## NAME
***${PGM}*** - ${PGM_DESC}
"""

    sourcecode = string.Template(
        """
## SOURCE CODE

Available at: [${PGM} source code](${URL_SOURCE})</a>
([history](${URL_LOG}"))${MD_NEWLINE}
${DATE_TAG}
"""
    )

    footer_index = string.Template(
        """___
[Main index](index.html) |
[${INDEXNAMECAP} index](${HTML_PAGE_FOOTER_PAGES_PATH}${INDEXNAME}.html) |
[Topics index](${HTML_PAGE_FOOTER_PAGES_PATH}topics.html) |
[Keywords index](${HTML_PAGE_FOOTER_PAGES_PATH}keywords.html) |
[Graphical index](${HTML_PAGE_FOOTER_PAGES_PATH}graphical_index.html) |
[Full index](${HTML_PAGE_FOOTER_PAGES_PATH}full_index.html)\
&copy; 2003-${YEAR}
[GRASS Development Team](https://grass.osgeo.org"),
GRASS GIS ${GRASS_VERSION} Reference Manual
"""
    )

    footer_noindex = string.Template(
        """___
[Main index[(index.html) |
[Topics index](${HTML_PAGE_FOOTER_PAGES_PATH}topics.html) |
[Keywords index](${HTML_PAGE_FOOTER_PAGES_PATH}keywords.html) |
[Graphical index](${HTML_PAGE_FOOTER_PAGES_PATH}graphical_index.html) |
[Full index](${HTML_PAGE_FOOTER_PAGES_PATH}full_index.html)\
&copy; 2003-${YEAR}
[GRASS Development Team](https://grass.osgeo.org),
GRASS GIS ${GRASS_VERSION} Reference Manual
"""
    )

    # process header
    src_data = read_file(src_file)

    # TODO:
    # if not re.search("<html>", src_data, re.IGNORECASE):
    tmp_data = read_file(tmp_file)
    # TODO: if not re.search("<html>", tmp_data, re.IGNORECASE):
    # sys.stdout.write(header(pgm))
    if tmp_data:
        header_logo_img_el = "![GRASS logo](grass_logo.png)"
        for line in tmp_data.splitlines(True):
            # The cleanup happens on Makefile level too.
            # TODO: if not re.search(
            #     "</body>|</html>|</div> <!-- end container -->", line, re.IGNORECASE
            # ):
            if header_logo_img_el in line:
                sys.stdout.write(line)
                # create hamburger menu TOC
                write_toc(create_toc(src_data), hamburger_menu_toc=True)
            else:
                sys.stdout.write(line)

    # process body
    # TODO: sys.stdout.write(update_toc(src_data))
    sys.stdout.write(src_data)

    # if </html> is found, suppose a complete html is provided.
    # otherwise, generate module class reference:
    # TODO:

    # process footer
    url_source, url_log, date_tag = parse_source()
    sys.stdout.write(
        sourcecode.substitute(
            URL_SOURCE=url_source,
            PGM=pgm,
            URL_LOG=url_log,
            DATE_TAG=date_tag,
            MD_NEWLINE="  ",
        )
    )
