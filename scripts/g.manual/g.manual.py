#!/usr/bin/env python3

############################################################################
#
# MODULE:       g.manual
# AUTHOR(S):    Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      Display the HTML/MAN pages
# COPYRIGHT:    (C) 2003-2015 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Displays the manual pages of GRASS modules.
#% keyword: general
#% keyword: manual
#% keyword: help
#%end
#%flag
#% key: i
#% description: Display index
#% suppress_required: yes
#%end
#%flag
#% key: t
#% description: Display topics
#% suppress_required: yes
#%end
#%flag
#% key: m
#% description: Display as MAN text page instead of HTML page in browser
#%end
#%flag
#% key: o
#% label: Display online manuals instead of locally installed
#% description: Use online manuals available at https://grass.osgeo.org website. This flag has no effect when displaying MAN text pages.
#%end
#%option
#% key: entry
#% type: string
#% description: Manual entry to be displayed
#% required : yes
#%end

import sys
import os

try:
    from urllib2 import urlopen
except ImportError:
    # python3
    from urllib.request import urlopen

import webbrowser

from grass.script.utils import basename
from grass.script import core as grass


def start_browser(entry):
    if browser and \
       browser not in ('xdg-open', 'start') and \
       not grass.find_program(browser):
        grass.fatal(_("Browser '%s' not found") % browser)

    if flags['o']:
        major, minor, patch = grass.version()['version'].split('.')
        url_path = 'https://grass.osgeo.org/grass%s%s/manuals/%s.html' % (major, minor, entry)
        if urlopen(url_path).getcode() != 200:
            url_path = 'https://grass.osgeo.org/grass%s%s/manuals/addons/%s.html' % (
                major, minor, entry)
    else:
        path = os.path.join(gisbase, 'docs', 'html', entry + '.html')
        if not os.path.exists(path) and os.getenv('GRASS_ADDON_BASE'):
            path = os.path.join(os.getenv('GRASS_ADDON_BASE'), 'docs', 'html', entry + '.html')

        if not os.path.exists(path):
            grass.fatal(_("No HTML manual page entry for '%s'") % entry)

        url_path = 'file://' + path

    if browser and browser not in ('xdg-open', 'start'):
        webbrowser.register(browser_name, None)

    grass.verbose(_("Starting browser '%(browser)s' for manual"
                    " entry '%(entry)s'...") %
                  dict(browser=browser_name, entry=entry))

    try:
        webbrowser.open(url_path)
    except:
        grass.fatal(_("Error starting browser '%(browser)s' for HTML file"
                      " '%(path)s'") % dict(browser=browser, path=path))


def start_man(entry):
    path = os.path.join(gisbase, 'docs', 'man', 'man1', entry + '.1')
    if not os.path.exists(path) and os.getenv('GRASS_ADDON_BASE'):
        path = os.path.join(os.getenv('GRASS_ADDON_BASE'), 'docs', 'man', 'man1', entry + '.1')

    for ext in ['', '.gz', '.bz2']:
        if os.path.exists(path + ext):
            os.execlp('man', 'man', path + ext)
            grass.fatal(_("Error starting 'man' for '%s'") % path)
    grass.fatal(_("No manual page entry for '%s'") % entry)


def main():
    global gisbase, browser, browser_name

    if flags['i'] and flags['t']:
        grass.fatal(_("Flags -%c and -%c are mutually exclusive") % ('i', 't'))

    special = None
    if flags['i']:
        special = 'index'
    elif flags['t']:
        special = 'topics'

    if flags['m']:
        start = start_man
    else:
        start = start_browser

    entry = options['entry']
    gisbase = os.environ['GISBASE']
    browser = os.getenv('GRASS_HTML_BROWSER', '')

    if sys.platform == 'darwin':
        # hack for MacOSX
        browser_name = os.getenv('GRASS_HTML_BROWSER_MACOSX', '..').split('.')[2]
    elif sys.platform == 'cygwin':
        # hack for Cygwin
        browser_name = basename(browser, 'exe')
    else:
        browser_name = basename(browser)

    # keep order!
    # first test for index...
    if special:
        start(special)
    else:
        start(entry)

    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
