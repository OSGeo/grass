#!/usr/bin/env python

############################################################################
#
# MODULE:	g.manual
# AUTHOR(S):	Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:	Display the HTML/MAN pages
# COPYRIGHT:	(C) 2003,2008 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%Module
#%  description: Display the HTML man pages of GRASS
#%  keywords: general, manual, help
#%End
#%flag
#%  key: i
#%  description: Display index
#%End
#%flag
#%  key: m
#%  description: Display as MAN text page instead of HTML page in browser
#%End
#%option
#% key: entry
#% type: string
#% description: Manual entry to be displayed
#% required : no
#%End

import sys
import os
import grass

def start_browser(entry):
    path = os.path.join(gisbase, 'docs', 'html', entry + '.html')
    if not os.path.exists(path):
	grass.fatal("No HTML manual page entry for <%s>." % entry)
    verbose = os.getenv("GRASS_VERBOSE")
    if not verbose or int(verbose) > 1:
	grass.message("Starting browser <%s> for module %s..." % (browser_name, entry))
    os.execlp(browser, browser_name, "file://%s/docs/html/%s.html" % (gisbase, entry))
    grass.fatal("Error starting browser <%s> for HTML file <%s>" % (browser, entry))

def start_man(entry):
    path = os.path.join(gisbase, 'man', 'man1', entry + '.1')
    for ext in ['', '.gz', '.bz2']:
	if os.path.exists(path + ext):
	    os.execlp('man', 'man', path + ext)
	    grass.fatal("Error starting 'man' for <%s>" % path)
    grass.fatal("No manual page entry for <%s>." % entry)

def main():
    global gisbase, browser, browser_name

    index = flags['i']
    manual = flags['m']
    entry = options['entry']

    gisbase = os.environ['GISBASE']

    browser = os.getenv('GRASS_HTML_BROWSER')

    if "html_browser_mac.sh" in browser:
        #hack for MacOSX:
	browser_name = os.getenv('GRASS_HTML_BROWSER_MACOSX','..').split('.')[2]
    elif os.getenv('OSTYPE') == "cygwin":
        #hack for Cygwin:
	browser_name = grass.basename(browser, 'exe')
    else:
	browser_name = grass.basename(browser)

    #keep order!
    #first test for index...
    if index:
	if manual:
	    start_man('index')
	else:
	    start_browser('index')
	sys.exit(0)

    #... then for help parameter:
    if not entry:
	grass.run_command('g.manual', '--help')
	sys.exit(0)

    if manual:
	start_man(entry)
    else:
	start_browser(entry)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
