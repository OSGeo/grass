#!/usr/bin/env python

############################################################################
#
# MODULE:       r.in.wms
#
# AUTHOR(S):    Cedric Shock, 2006
#               Pythonized by Martin Landa <landa.martin gmail.com>, 2009
#
# PURPOSE:      To import data from web mapping servers
#               (based on Bash script by Cedric Shock)
#
# COPYRIGHT:    (C) 2009 Martin Landa, and GRASS development team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

import os
import urllib

import grass

class WMSDownload:
    def __init__(self, flags, options):
        self.flags   = flags
        self.options = options
        
    def GetTiles(self, requests):
        grass.message("Downloading tiles...")
        
        i = 0
        for item in requests:
            if os.path.exists(item['output']) and \
                    os.path.getsize(item['output']) > 0:
                grass.message("Tile already downloaded")
            else:
                self.GetData(i, item['server'] + item['string'], item['output'])
            i += 1
        
    def GetData(self, idx, url, output):
        """Download data"""
        grass.message("Downloading data (tile %d)..." % idx)
        grass.verbose("Requesting data: %s" % self.options['mapserver'])
        grass.verbose(url)

        if not self.flags['g']: # -> post
            try:
                urllib.urlretrieve(url, output, data="POST")
            except IOError:
                grass.fatal("Failed while downloading the data")
            
            if not os.path.exists(output):
                grass.fatal("Failed while downloading the data")

            # work-around for brain-dead ArcIMS servers which want POST-data as part of the GET URL
            #   (this is technically allowed by OGC WMS def v1.3.0 Sec6.3.4)
            if os.path.getsize(output) == 0:
                grass.warning("Downloaded image file is empty -- trying another method")
                self.flags['g'] = True
        
        if self.flags['g']: # -> get
            try:
                urllib.urlretrieve(url, output, data="GET")
            except IOError:
                grass.fatal("Failed while downloading the data")
            
            if not os.path.exists(output) or os.path.getsize(output) == 0:
                grass.fatal("Failed while downloading the data")
        
