#!/usr/bin/env python3

############################################################################
#
# MODULE:       Builds manual pages
# AUTHOR(S):    Markus Neteler
#               Glynn Clements
#               Martin Landa <landa.martin gmail.com>
# PURPOSE:      Create HTML manual page snippets
# COPYRIGHT:    (C) 2007-2021 by Glynn Clements
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

try:
    # Python 2 import
    from HTMLParser import HTMLParser
except:
    # Python 3 import
    from html.parser import HTMLParser
try:
    import urlparse
except:
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
        encoding = 'UTF-8'
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


html_page_footer_pages_path = os.getenv('HTML_PAGE_FOOTER_PAGES_PATH') if \
    os.getenv('HTML_PAGE_FOOTER_PAGES_PATH') else ''

pgm = sys.argv[1]

src_file = "%s.html" % pgm
tmp_file = "%s.tmp.html" % pgm

trunk_url = "https://github.com/OSGeo/grass/tree/master/"
addons_url = "https://github.com/OSGeo/grass-addons/tree/master/"

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
<p>Available at: <a href="${URL_SOURCE}">${PGM} source code</a> (<a href="${URL_LOG}">history</a>)</p>
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
""")

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
""")

def read_file(name):
    try:
        f = open(name, 'rb')
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
            self.tag_curr = ''
            self.tag_last = ''
            self.process_text = False
            self.data = []
            self.tags_allowed = ('h1', 'h2', 'h3')
            self.tags_ignored = ('img')
            self.text = ''

        def handle_starttag(self, tag, attrs):
            if tag in self.tags_allowed:
                self.process_text = True
            self.tag_last = self.tag_curr
            self.tag_curr = tag

        def handle_endtag(self, tag):
            if tag in self.tags_allowed:
                self.data.append((tag, '%s_%d' % (tag, self.idx),
                                  self.text))
                self.idx += 1
                self.process_text = False
                self.text = ''

            self.tag_curr = self.tag_last

        def handle_data(self, data):
            if not self.process_text:
                return
            if self.tag_curr in self.tags_allowed or self.tag_curr in self.tags_ignored:
                self.text += data
            else:
                self.text += '<%s>%s</%s>' % (self.tag_curr, data, self.tag_curr)

    # instantiate the parser and fed it some HTML
    parser = MyHTMLParser()
    parser.feed(src_data)

    return parser.data

def escape_href(label):
    # remove html tags
    label = re.sub('<[^<]+?>', '', label)
    # fix &nbsp;
    label = label.replace('&nbsp;', '')
    # fix "
    label = label.replace('"', '')
    # replace space with underscore + lower
    return label.replace(' ', '-').lower()

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
        if tag == 'h3' and not in_h3 and has_h2:
            fd.write('\n%s<ul class="toc">\n' % (' ' * indent))
            indent += 4
            in_h3 = True
        elif not first:
            fd.write('</li>\n')

        if tag == 'h2':
            has_h2 = True
            if in_h3:
                indent -= 4
                fd.write('%s</ul></li>\n' % (' ' * indent))
                in_h3 = False

        text = text.replace(u'\xa0', u' ')
        fd.write('%s<li class="toc"><a href="#%s" class="toc">%s</a>' % \
                     (' ' * indent, escape_href(text), text))
        first = False

    fd.write('</li>\n</ul>\n')
    fd.write('</div>\n')

def update_toc(data):
    ret_data = []
    pat = re.compile(r'(<(h[2|3])>)(.+)(</h[2|3]>)')
    idx = 1
    for line in data.splitlines():
        if pat.search(line):
            xline = pat.split(line)
            line = xline[1] + '<a name="%s">' % escape_href(xline[3]) + xline[3] + '</a>' + xline[4]
            idx += 1
        ret_data.append(line)

    return '\n'.join(ret_data)


def get_addon_path(pgm):
    """Check if pgm is in addons list and get addon path

    :param pgm str: pgm

    :return tuple: (True, path) if pgm is addon else (None, None)
    """
    addon_base = os.getenv('GRASS_ADDON_BASE')
    if addon_base:
        """'addons_paths.json' is file created during install extension
        check get_addons_paths() function in the g.extension.py file
        """
        addons_paths = os.path.join(addon_base, 'addons_paths.json')
        if os.path.exists(addons_paths):
            with open(addons_paths, 'r') as f:
                addons_paths = json.load(f)
            for addon in addons_paths['tree']:
                split_path = addon['path'].split('/')
                root_dir, module_dir = split_path[0], split_path[-1]
                if 'grass7' == root_dir and pgm == module_dir:
                    return True, addon['path']
    return None, None


# process header
src_data = read_file(src_file)
name = re.search('(<!-- meta page name:)(.*)(-->)', src_data, re.IGNORECASE)
pgm_desc = "GRASS GIS Reference Manual"
if name:
    pgm = name.group(2).strip().split('-', 1)[0].strip()
    name_desc = re.search('(<!-- meta page name description:)(.*)(-->)', src_data, re.IGNORECASE)
    if name_desc:
        pgm_desc = name_desc.group(2).strip()
desc = re.search('(<!-- meta page description:)(.*)(-->)', src_data,
                 re.IGNORECASE)
if desc:
    pgm = desc.group(2).strip()
    header_tmpl = string.Template(header_base + header_nopgm)
else:
    if not pgm_desc:
        header_tmpl = string.Template(header_base + header_pgm)
    else:
        header_tmpl = string.Template(header_base + header_pgm_desc)

if not re.search('<html>', src_data, re.IGNORECASE):
    tmp_data = read_file(tmp_file)
    """
    Adjusting keywords html pages paths if add-on html man page
    stored on the server
    """
    if html_page_footer_pages_path:
        new_keywords_paths = []
        orig_keywords_paths = re.search(
            r'<h[1-9]>KEYWORDS</h[1-9]>(.*?)<h[1-9]>',
            tmp_data, re.DOTALL,
        )
        if orig_keywords_paths:
            search_txt = 'href="'
            for i in orig_keywords_paths.group(1).split(','):
                if search_txt in i:
                    index = i.index(search_txt) + len(search_txt)
                    new_keywords_paths.append(
                        i[:index] + html_page_footer_pages_path + i[index:],
                    )
        if new_keywords_paths:
            tmp_data = tmp_data.replace(
                orig_keywords_paths.group(1),
                ','.join(new_keywords_paths),
            )
    if not re.search('<html>', tmp_data, re.IGNORECASE):
        sys.stdout.write(header_tmpl.substitute(PGM=pgm, PGM_DESC=pgm_desc))
    if tmp_data:
        for line in tmp_data.splitlines(True):
            if not re.search('</body>|</html>', line, re.IGNORECASE):
                sys.stdout.write(line)

# create TOC
write_toc(create_toc(src_data))

# process body
sys.stdout.write(update_toc(src_data))

# if </html> is found, suppose a complete html is provided.
# otherwise, generate module class reference:
if re.search('</html>', src_data, re.IGNORECASE):
    sys.exit()

index_names = {
    'd' : 'display',
    'db': 'database',
    'g' : 'general',
    'i' : 'imagery',
    'm' : 'miscellaneous',
    'ps': 'postscript',
    'p' : 'paint',
    'r' : 'raster',
    'r3': 'raster3d',
    's' : 'sites',
    't' : 'temporal',
    'v' : 'vector'
    }


def to_title(name):
    """Convert name of command class/family to form suitable for title"""
    if name == 'raster3d':
        return '3D raster'
    elif name == 'postscript':
        return 'PostScript'
    else:
        return name.capitalize()


index_titles = {}
for key, name in index_names.items():
    index_titles[key] = to_title(name)

# process footer
index = re.search('(<!-- meta page index:)(.*)(-->)', src_data, re.IGNORECASE)
if index:
    index_name = index.group(2).strip()
    if '|' in index_name:
        index_name, index_name_cap = index_name.split('|', 1)
    else:
        index_name_cap = to_title(index_name)
else:
    mod_class = pgm.split('.', 1)[0]
    index_name = index_names.get(mod_class, '')
    index_name_cap = index_titles.get(mod_class, '')

grass_version = os.getenv("VERSION_NUMBER", "unknown")
year = os.getenv("VERSION_DATE")
if not year:
    year = str(datetime.now().year)

# check the names of scripts to assign the right folder
topdir = os.path.abspath(os.getenv("MODULE_TOPDIR"))
curdir = os.path.abspath(os.path.curdir)
if curdir.startswith(topdir):
    source_url = trunk_url
    pgmdir = curdir.replace(topdir, '').lstrip(os.path.sep)
else:
    # addons
    source_url = addons_url
    pgmdir = os.path.sep.join(curdir.split(os.path.sep)[-3:])
url_source = ''
if os.getenv('SOURCE_URL', ''):
    # addons
    for prefix in index_names.keys():
        cwd = os.getcwd()
        idx = cwd.find('{0}{1}.'.format(os.path.sep, prefix))
        if idx > -1:
            pgmname = cwd[idx+1:]
            classname = index_names[prefix]
            url_source = urlparse.urljoin('{0}{1}/'.format(
                    os.environ['SOURCE_URL'], classname),
                    pgmname
            )
            break
else:
    url_source = urlparse.urljoin(source_url, pgmdir)
if sys.platform == 'win32':
    url_source = url_source.replace(os.path.sep, '/')

if index_name:
    tree = 'grass/tree'
    commits = 'grass/commits'
    is_addon, addon_path = get_addon_path(pgm=pgm)
    if is_addon:
        # Fix gui/wxpython addon url path
        url_source = urlparse.urljoin(
            os.environ['SOURCE_URL'], addon_path.split('/', 1)[1],
        )
        tree = 'grass-addons/tree'
        commits = 'grass-addons/commits'

    sys.stdout.write(sourcecode.substitute(
        URL_SOURCE=url_source, PGM=pgm, URL_LOG=url_source.replace(
            tree,  commits)))
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
