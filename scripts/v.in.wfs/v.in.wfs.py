#!/usr/bin/env python3

############################################################################
#
# MODULE:	v.in.wfs
# AUTHOR(S):	Markus Neteler. neteler itc it
#               Hamish Bowman
#               Converted to Python by Glynn Clements
# PURPOSE:	WFS support
# COPYRIGHT:	(C) 2006-2012 Markus Neteler and the GRASS Development Team
#
#		This program is free software under the GNU General
#		Public License (>=v2). Read the file COPYING that
#		comes with GRASS for details.
#
# GetFeature example:
# http://mapserver.gdf-hannover.de/cgi-bin/grassuserwfs?REQUEST=GetFeature&SERVICE=WFS&VERSION=1.0.0
#############################################################################

#
# TODO: suggest to depend on the OWSLib for OGC web service needs
#       http://pypi.python.org/pypi/OWSLib
#

#%Module
#% description: Imports GetFeature from a WFS server.
#% keyword: vector
#% keyword: import
#% keyword: OGC web services
#% keyword: OGC WFS
#%end
#%option
#% key: url
#% type: string
#% description: Base URL starting with 'http' and ending in '?'
#% required: yes
#%end
#%option G_OPT_V_OUTPUT
#%end
#%option
#% key: name
#% type: string
#% description: Comma separated names of data layers to download
#% multiple: yes
#% required: no
#%end
#%option
#% key: srs
#% type: string
#% label: Specify alternate spatial reference system (example: EPSG:4326)
#% description: The given code must be supported by the server, consult the capabilities file
#% required: no
#%end
#%option
#% key: maximum_features
#% type: integer
#% label: Maximum number of features to download
#% description: (default: unlimited)
#%end
#%option
#% key: start_index
#% type: integer
#% label: Skip earlier feature IDs and start downloading at this one
#% description: (default: start with the first feature)
#%end
#%option
#% key: username
#% type: string
#% required: no
#% multiple: no
#% label: Username or file with username or environment variable name with username
#%end
#%option
#% key: password
#% type: string
#% required: no
#% multiple: no
#% label: Password or file with password or environment variable name with password
#%end
#%flag
#% key: l
# todo #% description: List available layers and exit
#% description: Download server capabilities to 'wms_capabilities.xml' in the current directory and exit
#% suppress_required: yes
#%end
#%flag
#% key: r
#% description: Restrict fetch to features which touch the current region
#%end


import os
import sys
from grass.script.utils import try_remove
from grass.script import core as grass
try:
    from urllib2 import urlopen, URLError, HTTPError
    from urllib2 import build_opener, install_opener
    from urllib2 import HTTPPasswordMgrWithDefaultRealm, HTTPBasicAuthHandler
except ImportError:
    from urllib.request import urlopen
    from urllib.request import build_opener, install_opener
    from urllib.request import HTTPPasswordMgrWithDefaultRealm, HTTPBasicAuthHandler
    from urllib.error import URLError, HTTPError


def main():
    out = options['output']
    wfs_url = options['url']

    request_base = 'REQUEST=GetFeature&SERVICE=WFS&VERSION=1.0.0'
    wfs_url += request_base

    if options['name']:
        wfs_url += '&TYPENAME=' + options['name']

    if options['srs']:
        wfs_url += '&SRS=' + options['srs']

    if options['maximum_features']:
        wfs_url += '&MAXFEATURES=' + options['maximum_features']
        if int(options['maximum_features']) < 1:
            # GTC Invalid WFS maximum features parameter
            grass.fatal(_("Invalid maximum number of features"))

    if options['start_index']:
        wfs_url += '&STARTINDEX=' + options['start_index']
        if int(options['start_index']) < 1:
            # GTC Invalid WFS start index parameter
            grass.fatal(_('Features begin with index "1"'))

    if flags['r']:
        bbox = grass.read_command("g.region", flags='w').split('=')[1]
        wfs_url += '&BBOX=' + bbox

    if flags['l']:
        wfs_url = options['url'] + 'REQUEST=GetCapabilities&SERVICE=WFS&VERSION=1.0.0'

    tmp = grass.tempfile()
    tmpxml = tmp + '.xml'

    grass.debug(wfs_url)

    # Set user and password if given
    if options['username'] and options['password']:
        grass.message(_("Setting username and password..."))
        if os.path.isfile(options['username']):
            with open(options['username']) as f:
                filecontent = f.read()
                user = filecontent.strip()
        elif options['username'] in os.environ:
            user = os.environ[options['username']]
        else:
            user = options['username']
        if os.path.isfile(options['password']):
            with open(options['password']) as f:
                filecontent = f.read()
                pw = filecontent.strip()
        elif options['password'] in os.environ:
            pw = os.environ[options['password']]
        else:
            pw = options['password']

        passmgr = HTTPPasswordMgrWithDefaultRealm()
        passmgr.add_password(None, wfs_url,user, pw)
        authhandler = HTTPBasicAuthHandler(passmgr)
        opener = build_opener(authhandler)
        install_opener(opener)

    # GTC Downloading WFS features
    grass.message(_("Retrieving data..."))
    try:
        inf = urlopen(wfs_url)
    except HTTPError as e:
        # GTC WFS request HTTP failure
        grass.fatal(_("The server couldn't fulfill the request.\nError code: %s") % e.code)
    except URLError as e:
        # GTC WFS request network failure
        grass.fatal(_("Failed to reach the server.\nReason: %s") % e.reason)

    outf = open(tmpxml, 'wb')
    while True:
        s = inf.read()
        if not s:
            break
        outf.write(s)
    inf.close()
    outf.close()

    if flags['l']:
        import shutil
        if os.path.exists('wms_capabilities.xml'):
            grass.fatal(_('A file called "wms_capabilities.xml" already exists here'))
        # os.move() might fail if the temp file is on another volume, so we copy instead
        shutil.copy(tmpxml, 'wms_capabilities.xml')
        try_remove(tmpxml)
        sys.exit(0)

    grass.message(_("Importing data..."))
    try:
        grass.run_command('v.in.ogr', flags='o', input=tmpxml, output=out)
        grass.message(_("Vector map <%s> imported from WFS.") % out)
    except:
        grass.message(_("WFS import failed"))
    finally:
        try_remove(tmpxml)


if __name__ == "__main__":
    options, flags = grass.parser()
    main()
