"""!@package grass.script.setup

@brief GRASS Python scripting module (setup)

Setup functions to be used in Python scripts.

Usage:

@code
from grass.script import setup as grass

grass.init()
...
@endcode

(C) 2010-2012 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import tempfile as tmpfile

def init(gisbase, dbase = '', location = 'demolocation', mapset = 'PERMANENT'):
    """!Initialize system variables to run scripts without starting
    GRASS explicitly.

    User is resposible to delete gisrc file.

    @param gisbase path to GRASS installation
    @param dbase   path to GRASS database (default: '')
    @param location location name (default: 'demolocation')
    @param mapset   mapset within given location (default: 'PERMANENT')
    @return path to gisrc file
    """
    # define PATH
    os.environ['PATH'] += os.pathsep + os.path.join(gisbase, 'bin')
    os.environ['PATH'] += os.pathsep + os.path.join(gisbase, 'scripts')
    if sys.platform.startswith('win'): # added for winGRASS
         os.environ['PATH'] += os.pathsep + os.path.join(gisbase, 'extralib')
    # define LD_LIBRARY_PATH
    if '@LD_LIBRARY_PATH_VAR@' not in os.environ:
        os.environ['@LD_LIBRARY_PATH_VAR@'] = ''
    os.environ['@LD_LIBRARY_PATH_VAR@'] += os.pathsep + os.path.join(gisbase, 'lib')
    
    os.environ['GIS_LOCK'] = str(os.getpid())
    
    # Set PYTHONPATH to find GRASS Python modules
    path = os.getenv('PYTHONPATH')
    dir  = os.path.join(gisbase, 'etc', 'python')
    if path:
        path = dir + os.pathsep + path
    else:
        path = dir
    os.environ['PYTHONPATH'] = path
    
    if not dbase:
        dbase = gisbase
    
    fd, gisrc = tmpfile.mkstemp()
    os.environ['GISRC'] = gisrc
    os.write(fd, "GISDBASE: %s\n" % dbase)
    os.write(fd, "LOCATION_NAME: %s\n" % location)
    os.write(fd, "MAPSET: %s\n" % mapset)
    os.close(fd)
    
    return gisrc
