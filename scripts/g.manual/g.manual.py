#!/usr/bin/env python

############################################################################
#
# MODULE:	g.manual
# AUTHOR(S):	Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:	Display the HTML/MAN pages
# COPYRIGHT:	(C) 2003, 2008, 2010-2012 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Displays the manual pages of GRASS modules.
#% keywords: general
#% keywords: manual
#% keywords: help
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
#%option
#% key: entry
#% type: string
#% description: Manual entry to be displayed
#% required : yes
#%end

import sys
import os
from grass.script import core as grass

def start_browser(entry):
    if browser != 'xdg-open' and not grass.find_program(browser):
        grass.fatal(_("Browser <%s> not found") % browser)
    
    path = os.path.join(gisbase, 'docs', 'html', entry + '.html')
    if not os.path.exists(path) and os.getenv('GRASS_ADDON_BASE'):
        path = os.path.join(os.getenv('GRASS_ADDON_BASE'), 'docs', 'html', entry + '.html')
    
    if not os.path.exists(path):
        grass.fatal(_("No HTML manual page entry for <%s>") % entry)
    
    grass.verbose(_("Starting browser <%s> for module %s...") % (browser_name, entry))
    
    os.execlp(browser, browser_name, "file://%s" % (path))
    grass.fatal(_("Error starting browser <%s> for HTML file <%s>") % (browser, entry))
    
def start_man(entry):
    path = os.path.join(gisbase, 'docs', 'man', 'man1', entry + '.1')
    if not os.path.exists(path) and os.getenv('GRASS_ADDON_BASE'):
        path = os.path.join(os.getenv('GRASS_ADDON_BASE'), 'docs', 'man', 'man1', entry + '.1')
    
    for ext in ['', '.gz', '.bz2']:
	if os.path.exists(path + ext):
	    os.execlp('man', 'man', path + ext)
	    grass.fatal(_("Error starting 'man' for <%s>") % path)
    grass.fatal(_("No manual page entry for <%s>") % entry)

def main():
    global gisbase, browser, browser_name
    
    if flags['i'] and flags['t']:
        grass.fatal(_("Flags -%c and -%c are mutually exclusive") % ('i', 't'))
    
    special = None
    if flags['i']:
        special = 'index'
    elif flags ['t']:
        special = 'topics'
    
    if flags['m']:
        start = start_man
    else:
        start = start_browser
    
    entry  = options['entry']
    
    gisbase = os.environ['GISBASE']
    
    browser = os.getenv('GRASS_HTML_BROWSER', '')
    
    if sys.platform == 'darwin':
        # hack for MacOSX
	browser_name = os.getenv('GRASS_HTML_BROWSER_MACOSX', '..').split('.')[2]
    elif sys.platform == 'cygwin':
        # hack for Cygwin
	browser_name = grass.basename(browser, 'exe')
    else:
	browser_name = grass.basename(browser)
    
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
