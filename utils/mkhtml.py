#!/usr/bin/env python3

############################################################################
#
# MODULE:       Builds manual pages
# AUTHOR(S):    Markus Neteler
#               Glynn Clements
#               Martin Landa <landa.martin gmail.com>
# PURPOSE:      Create HTML manual page snippets
# COPYRIGHT:    (C) 2007-2022 by Glynn Clements
#                and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

import sys
import os
import string
import re
from datetime import datetime
import locale
import json
import pathlib
from subprocess import check_output

try:
    # Python 2 import
    from HTMLParser import HTMLParser
except ImportError:
    # Python 3 import
    from html.parser import HTMLParser
try:
    import urlparse
except ImportError:
    import urllib.parse as urlparse

if sys.version_info[0] == 2:
    PY2 = True
else:
    PY2 = False


if not PY2:
    unicode = str


def _get_encoding():
    encoding = locale.getdefaultlocale()[1]
    if not encoding:
        encoding = "UTF-8"
    return encoding


def decode(bytes_):
    """Decode bytes with default locale and return (unicode) string

    No-op if parameter is not bytes (assumed unicode string).

    :param bytes bytes_: the bytes to decode
    """
    if isinstance(bytes_, unicode):
        return bytes_
    if isinstance(bytes_, bytes):
        enc = _get_encoding()
        return bytes_.decode(enc)
    return unicode(bytes_)


def get_last_commit_git(src_dir):
    gitlog_dict = {}
    cwd = os.getcwd()
    # If git is not available we might try API but remember requests limit there
    # trunk_api_url = f"https://api.github.com/repos/osgeo/grass/commits?path={{path}}&page=1&per_page=1
    # addons_uapi_url = f"https://api.github.com/repos/osgeo/grass/commits?path={{path}}&page=1&per_page=1
    # commits = urllib.request.urlopen(trunk_api_url)
    # json.loads(commits.read().decode())
    try:
        os.chdir(src_dir)
        gitlog = decode(check_output(["git", "log", "-1"])).split("\n")
        print("src_dir: ", src_dir, "gitlog: ", gitlog)
        gitlog_dict = {
            "commit": gitlog[0].split(" ")[1],
            "date": gitlog[2].lstrip("Date:").strip(),
        }
    except RuntimeError:
        gitlog_dict = {
            "commit": "unknown",
            "date": "unknown",
        }
    os.chdir(cwd)
    return gitlog_dict


html_page_footer_pages_path = (
    os.getenv("HTML_PAGE_FOOTER_PAGES_PATH")
    if os.getenv("HTML_PAGE_FOOTER_PAGES_PATH")
    else ""
)

pgm = sys.argv[1]

src_file = "%s.html" % pgm
tmp_file = "%s.tmp.html" % pgm

grass_version = os.getenv("VERSION_NUMBER", "unknown")
trunk_url = ""
addons_url = ""
if grass_version != "unknown":
    major, minor, patch = grass_version.split(".")
    trunk_url = "https://github.com/OSGeo/grass/tree/main/"
    addons_url = f"https://github.com/OSGeo/grass-addons/tree/grass{major}/"

header_base = """<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
 <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
 <title>${PGM} - GRASS GIS Manual</title>
 <meta name="Author" content="GRASS Development Team">
 <meta name="description" content="${PGM}: ${PGM_DESC}">
 <link rel="stylesheet" href="grassdocs.css" type="text/css">
</head>
<body bgcolor="white">
<div id="container">

<a href="index.html"><img src="grass_logo.png" alt="GRASS logo"></a>
<hr class="header">
"""

header_nopgm = """<h2>${PGM}</h2>
"""

header_pgm = """<h2>NAME</h2>
<em><b>${PGM}</b></em>
"""

header_pgm_desc = """<h2>NAME</h2>
<em><b>${PGM}</b></em> - ${PGM_DESC}
"""

sourcecode = string.Template(
    """<h2>SOURCE CODE</h2>
<p>
  Available at:
  <a href="${URL_SOURCE}">${PGM} source code</a>
  (<a href="${URL_LOG}">history</a>)
</p>
<p>
  Latest change: ${COMMIT_DATE} in commit: ${COMMIT}
</p>
"""
)

footer_index = string.Template(
    """<hr class="header">
<p>
<a href="index.html">Main index</a> |
<a href="${HTML_PAGE_FOOTER_PAGES_PATH}${INDEXNAME}.html">${INDEXNAMECAP} index</a> |
<a href="${HTML_PAGE_FOOTER_PAGES_PATH}topics.html">Topics index</a> |
<a href="${HTML_PAGE_FOOTER_PAGES_PATH}keywords.html">Keywords index</a> |
<a href="${HTML_PAGE_FOOTER_PAGES_PATH}graphical_index.html">Graphical index</a> |
<a href="${HTML_PAGE_FOOTER_PAGES_PATH}full_index.html">Full index</a>
</p>
<p>
&copy; 2003-${YEAR}
<a href="https://grass.osgeo.org">GRASS Development Team</a>,
GRASS GIS ${GRASS_VERSION} Reference Manual
</p>

</div>
</body>
</html>
"""
)

footer_noindex = string.Template(
    """<hr class="header">
<p>
<a href="index.html">Main index</a> |
<a href="${HTML_PAGE_FOOTER_PAGES_PATH}topics.html">Topics index</a> |
<a href="${HTML_PAGE_FOOTER_PAGES_PATH}keywords.html">Keywords index</a> |
<a href="${HTML_PAGE_FOOTER_PAGES_PATH}graphical_index.html">Graphical index</a> |
<a href="${HTML_PAGE_FOOTER_PAGES_PATH}full_index.html">Full index</a>
</p>
<p>
&copy; 2003-${YEAR}
<a href="https://grass.osgeo.org">GRASS Development Team</a>,
GRASS GIS ${GRASS_VERSION} Reference Manual
</p>

</div>
</body>
</html>
"""
)


def read_file(name):
    try:
        f = open(name, "rb")
        s = f.read()
        f.close()
        if PY2:
            return s
        else:
            return decode(s)
    except IOError:
        return ""


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


def escape_href(label):
    # remove html tags
    label = re.sub("<[^<]+?>", "", label)
    # fix &nbsp;
    label = label.replace("&nbsp;", "")
    # fix "
    label = label.replace('"', "")
    # replace space with underscore + lower
    return label.replace(" ", "-").lower()


def write_toc(data):
    if not data:
        return

    fd = sys.stdout
    fd.write('<div class="toc">\n')
    fd.write('<h4 class="toc">Table of contents</h4>\n')
    fd.write('<ul class="toc">\n')
    first = True
    has_h2 = False
    in_h3 = False
    indent = 4
    for tag, href, text in data:
        if tag == "h3" and not in_h3 and has_h2:
            fd.write('\n%s<ul class="toc">\n' % (" " * indent))
            indent += 4
            in_h3 = True
        elif not first:
            fd.write("</li>\n")

        if tag == "h2":
            has_h2 = True
            if in_h3:
                indent -= 4
                fd.write("%s</ul></li>\n" % (" " * indent))
                in_h3 = False

        text = text.replace("\xa0", " ")
        fd.write(
            '%s<li class="toc"><a href="#%s" class="toc">%s</a>'
            % (" " * indent, escape_href(text), text)
        )
        first = False

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


def get_addon_path():
    """Check if pgm is in the addons list and get addon path

    return: pgm path if pgm is addon else None
    """
    addon_base = os.getenv("GRASS_ADDON_BASE")
    if addon_base:
        # addons_paths.json is file created during install extension
        # check get_addons_paths() function in the g.extension.py file
        addons_file = "addons_paths.json"
        addons_paths = os.path.join(addon_base, addons_file)
        if not os.path.exists(addons_paths):
            # Compiled addon has own dir e.g. ~/.grass8/addons/db.join/
            # with bin/ docs/ etc/ scripts/ subdir, required for compilation
            # addons on osgeo lxd container server and generation of
            # modules.xml file (build-xml.py script), when addons_paths.json
            # file is stored one level dir up
            addons_paths = os.path.join(
                os.path.abspath(os.path.join(addon_base, "..")),
                addons_file,
            )
            if not os.path.exists(addons_paths):
                return
        with open(addons_paths) as f:
            addons_paths = json.load(f)
        for addon in addons_paths["tree"]:
            if pgm == pathlib.Path(addon["path"]).name:
                return addon["path"]


# process header
src_data = read_file(src_file)
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
else:
    if not pgm_desc:
        header_tmpl = string.Template(header_base + header_pgm)
    else:
        header_tmpl = string.Template(header_base + header_pgm_desc)

if not re.search("<html>", src_data, re.IGNORECASE):
    tmp_data = read_file(tmp_file)
    """
    Adjusting keywords html pages paths if add-on html man page
    stored on the server
    """
    if html_page_footer_pages_path:
        new_keywords_paths = []
        orig_keywords_paths = re.search(
            r"<h[1-9]>KEYWORDS</h[1-9]>(.*?)<h[1-9]>",
            tmp_data,
            re.DOTALL,
        )
        if orig_keywords_paths:
            search_txt = 'href="'
            for i in orig_keywords_paths.group(1).split(","):
                if search_txt in i:
                    index = i.index(search_txt) + len(search_txt)
                    new_keywords_paths.append(
                        i[:index] + html_page_footer_pages_path + i[index:],
                    )
        if new_keywords_paths:
            tmp_data = tmp_data.replace(
                orig_keywords_paths.group(1),
                ",".join(new_keywords_paths),
            )
    if not re.search("<html>", tmp_data, re.IGNORECASE):
        sys.stdout.write(header_tmpl.substitute(PGM=pgm, PGM_DESC=pgm_desc))
    if tmp_data:
        for line in tmp_data.splitlines(True):
            if not re.search("</body>|</html>", line, re.IGNORECASE):
                sys.stdout.write(line)

# create TOC
write_toc(create_toc(src_data))

# process body
sys.stdout.write(update_toc(src_data))

# if </html> is found, suppose a complete html is provided.
# otherwise, generate module class reference:
if re.search("</html>", src_data, re.IGNORECASE):
    sys.exit()

index_names = {
    "d": "display",
    "db": "database",
    "g": "general",
    "i": "imagery",
    "m": "miscellaneous",
    "ps": "postscript",
    "p": "paint",
    "r": "raster",
    "r3": "raster3d",
    "s": "sites",
    "t": "temporal",
    "v": "vector",
}


def to_title(name):
    """Convert name of command class/family to form suitable for title"""
    if name == "raster3d":
        return "3D raster"
    elif name == "postscript":
        return "PostScript"
    else:
        return name.capitalize()


index_titles = {}
for key, name in index_names.items():
    index_titles[key] = to_title(name)

# process footer
index = re.search("(<!-- meta page index:)(.*)(-->)", src_data, re.IGNORECASE)
if index:
    index_name = index.group(2).strip()
    if "|" in index_name:
        index_name, index_name_cap = index_name.split("|", 1)
    else:
        index_name_cap = to_title(index_name)
else:
    mod_class = pgm.split(".", 1)[0]
    index_name = index_names.get(mod_class, "")
    index_name_cap = index_titles.get(mod_class, "")

year = os.getenv("VERSION_DATE")
if not year:
    year = str(datetime.now().year)

# check the names of scripts to assign the right folder
topdir = os.path.abspath(os.getenv("MODULE_TOPDIR"))
curdir = os.path.abspath(os.path.curdir)
if curdir.startswith(topdir + os.path.sep):
    source_url = trunk_url
    pgmdir = curdir.replace(topdir, "").lstrip(os.path.sep)
else:
    # addons
    source_url = addons_url
    pgmdir = os.path.sep.join(curdir.split(os.path.sep)[-3:])
url_source = ""
if os.getenv("SOURCE_URL", ""):
    addon_path = get_addon_path()
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

if index_name:
    branches = "branches"
    tree = "tree"
    commits = "commits"

    if branches in url_source:
        url_log = url_source.replace(branches, commits)
        url_source = url_source.replace(branches, tree)
    else:
        url_log = url_source.replace(tree, commits)

    git_commit_log = get_last_commit_git(curdir)

    sys.stdout.write(
        sourcecode.substitute(
            URL_SOURCE=url_source,
            PGM=pgm,
            URL_LOG=url_log,
            COMMIT_DATE=git_commit_log["date"],
            COMMIT=git_commit_log["commit"],
        )
    )
    sys.stdout.write(
        footer_index.substitute(
            INDEXNAME=index_name,
            INDEXNAMECAP=index_name_cap,
            YEAR=year,
            GRASS_VERSION=grass_version,
            HTML_PAGE_FOOTER_PAGES_PATH=html_page_footer_pages_path,
        ),
    )
else:
    sys.stdout.write(
        footer_noindex.substitute(
            YEAR=year,
            GRASS_VERSION=grass_version,
            HTML_PAGE_FOOTER_PAGES_PATH=html_page_footer_pages_path,
        ),
    )
