#!/usr/bin/env python

############################################################################
#
# MODULE:	v.in.wfs
# AUTHOR(S):	Markus Neteler. neteler itc it
#               Hamish Bowman (fixes)
#               Converted to Python by Glynn Clements
# PURPOSE:	WFS support
# COPYRIGHT:	(C) 2006, 2007, 2008, 2010 Markus Neteler and GRASS Development Team
#
#		This program is free software under the GNU General
#		Public License (>=v2). Read the file COPYING that
#		comes with GRASS for details.
#
# GetFeature example:
# http://mapserver.gdf-hannover.de/cgi-bin/grassuserwfs?REQUEST=GetFeature&SERVICE=WFS&VERSION=1.0.0
#############################################################################

#%Module
#% description: Import GetFeature from WFS.
#% keywords: vector, import, wfs
#%end
#%option
#% key: url
#% type: string
#% description: GetFeature URL starting with 'http'
#%end
#%option
#% key: output
#% type: string
#% gisprompt: new,vector,vector
#% description: Name for output vector map
#% required : yes
#%end

import os
from grass.script import core as grass
import urllib

def main():
    out = options['output']
    wfs_url = options['wfs']

    tmp = grass.tempfile()
    tmpxml = tmp + '.xml'

    grass.message(_("Retrieving data ..."))
    inf = urllib.urlopen(wfs_url)
    outf = file(tmpxml, 'wb')
    while True:
	s = inf.read()
	if not s:
	    break
	outf.write(s)
    inf.close()
    outf.close()

    grass.run_command('v.in.ogr', flags = 'o', dsn = tmpxml, out = out)
    grass.try_remove(tmpxml)

    grass.message(_("Vector points map <%s> imported from WFS.") % out)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
