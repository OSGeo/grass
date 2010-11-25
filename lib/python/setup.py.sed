"""!@package grass.script.setup

@brief GRASS Python scripting module (setup)

Setup functions to be used in Python scripts.

Usage:

@code
from grass.script import setup as grass

grass.init()
...
@endcode

(C) 2010 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import tempfile as tmpfile

def init(gisbase, dbase, location, mapset):
    """!Initialize system variables to run scripts without starting
    GRASS explicitly.

    User is resposible to delete gisrc file.

    @param gisbase path to GRASS installation
    @param dbase   path to GRASS database
    @param location location name
    @param mapset   mapset within given location
    @return path to gisrc file
    """
    os.environ['PATH'] += os.pathsep + os.path.join(gisbase, 'bin') + \
        os.pathsep + os.path.join(gisbase, 'scripts')
    if not os.environ.has_key('@LD_LIBRARY_PATH_VAR@'):
        os.environ['@LD_LIBRARY_PATH_VAR@'] = ''
    os.environ['@LD_LIBRARY_PATH_VAR@'] += os.path.join(gisbase, 'lib')
    
    os.environ['GIS_LOCK'] = str(os.getpid())
    
    fd, gisrc = tmpfile.mkstemp()
    os.environ['GISRC'] = gisrc
    os.write(fd, "GISDBASE: %s\n" % dbase)
    os.write(fd, "LOCATION_NAME: %s\n" % location)
    os.write(fd, "MAPSET: %s\n" % mapset)
    os.close(fd)
    
    return gisrc

