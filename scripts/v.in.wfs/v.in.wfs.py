#!/usr/bin/env python

############################################################################
#
# MODULE:	v.in.wfs
# AUTHOR(S):	Markus Neteler. neteler itc it
#               Hamish Bowman (fixes)
#               Converted to Python by Glynn Clements
# PURPOSE:	WFS support
# COPYRIGHT:	(C) 2006, 2007, 2008 Markus Neteler and GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
# GetFeature example:
# http://mapserver.gdf-hannover.de/cgi-bin/grassuserwfs?REQUEST=GetFeature&SERVICE=WFS&VERSION=1.0.0
#############################################################################

#%Module
#% description: import GetFeature from WFS
#%End
#%option
#% key: wfs
#% type: string
#% description: GetFeature URL starting with http
#%End
#%option
#% key: output
#% type: string
#% gisprompt: new,vector,vector
#% description: Vector output map
#% required : yes
#%End

import os
import grass
import urllib

def main():
    out = options['output']
    wfs_url = options['wfs']

    tmp = grass.tempfile()
    tmpxml = tmp + '.xml'

    grass.message("Retrieving data ...")
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

    grass.message("Vector points map <%s> imported from WFS." % out)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
