"""Setup and initialization functions

Function can be used in Python scripts to setup a GRASS environment
without starting an actual GRASS session.

Usage::

    import os
    import sys
    import subprocess

    # define GRASS Database
    # add your path to grassdata (GRASS GIS database) directory
    gisdb = os.path.join(os.path.expanduser("~"), "grassdata")
    # the following path is the default path on MS Windows
    # gisdb = os.path.join(os.path.expanduser("~"), "Documents/grassdata")

    # specify (existing) Location and Mapset
    location = "nc_spm_08"
    mapset = "user1"

    # path to the GRASS GIS launch script
    # we assume that the GRASS GIS start script is available and on PATH
    # query GRASS itself for its GISBASE
    # (with fixes for specific platforms)
    # needs to be edited by the user
    grass7bin = 'grass70'
    if sys.platform.startswith('win'):
        # MS Windows
        grass7bin = r'C:\OSGeo4W\bin\grass70.bat'
        # uncomment when using standalone WinGRASS installer
        # grass7bin = r'C:\Program Files (x86)\GRASS GIS 7.0.0\grass70.bat'
        # this can be avoided if GRASS executable is added to PATH
    elif sys.platform == 'darwin':
        # Mac OS X
        # TODO: this have to be checked, maybe unix way is good enough
        grass7bin = '/Applications/GRASS/GRASS-7.0.app/'

    # query GRASS GIS itself for its GISBASE
    startcmd = [grass7bin, '--config', 'path']
    try:
        p = subprocess.Popen(startcmd, shell=False,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = p.communicate()
    except OSError as error:
        sys.exit("ERROR: Cannot find GRASS GIS start script"
                 " {cmd}: {error}".format(cmd=startcmd[0], error=error))
    if p.returncode != 0:
        sys.exit("ERROR: Issues running GRASS GIS start script"
                 " {cmd}: {error}"
                 .format(cmd=' '.join(startcmd), error=err))
    gisbase = out.strip(os.linesep)

    # set GISBASE environment variable
    os.environ['GISBASE'] = gisbase

    # define GRASS-Python environment
    grass_pydir = os.path.join(gisbase, "etc", "python")
    sys.path.append(grass_pydir)

    # import (some) GRASS Python bindings
    import grass.script as gscript

    # launch session
    rcfile = gscript.setup.init(gisbase, gisdb, location, mapset)

    # example calls
    gscript.message('Current GRASS GIS 7 environment:')
    print gscript.gisenv()

    gscript.message('Available raster maps:')
    for rast in gscript.list_strings(type='raster'):
        print rast

    gscript.message('Available vector maps:')
    for vect in gscript.list_strings(type='vector'):
        print vect

    # delete the rcfile
    os.remove(rcfile)


(C) 2010-2012 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com>
"""

# TODO: this should share code from lib/init/grass.py
# perhaps grass.py can import without much trouble once GISBASE
# is known, this would allow moving things from there, here
# then this could even do locking

import os
import sys
import tempfile as tmpfile


def write_gisrc(dbase, location, mapset):
    """Write the ``gisrc`` file and return its path."""
    gisrc = tmpfile.mktemp()
    with open(gisrc, 'w') as rc:
        rc.write("GISDBASE: %s\n" % dbase)
        rc.write("LOCATION_NAME: %s\n" % location)
        rc.write("MAPSET: %s\n" % mapset)
    return gisrc


def set_gui_path():
    """Insert wxPython GRASS path to sys.path."""
    gui_path = os.path.join(os.environ['GISBASE'], 'gui', 'wxpython')
    if gui_path and gui_path not in sys.path:
        sys.path.insert(0, gui_path)


# TODO: there should be a function to do the clean up
# (unset the GISRC and delete the file)
def init(gisbase, dbase='', location='demolocation', mapset='PERMANENT'):
    """Initialize system variables to run GRASS modules

    This function is for running GRASS GIS without starting it
    explicitly. No GRASS modules shall be called before call of this
    function but any module or user script can be called afterwards
    as if it would be called in an actual GRASS session. GRASS Python
    libraries are usable as well in general but the ones using
    C libraries through ``ctypes`` are not (which is caused by
    library path not being updated for the current process
    which is a common operating system limitation).

    To create a (fake) GRASS session a ``gisrc`` file is created.
    Caller is resposible for deleting the ``gisrc`` file.

    Basic usage::

        # ... setup GISBASE and PYTHON path before import
        import grass.script as gscript
        gisrc = gscript.setup.init("/usr/bin/grass7",
                                   "/home/john/grassdata",
                                   "nc_spm_08", "user1")
        # ... use GRASS modules here
        # remove the session's gisrc file to end the session
        os.remove(gisrc)

    :param gisbase: path to GRASS installation
    :param dbase: path to GRASS database (default: '')
    :param location: location name (default: 'demolocation')
    :param mapset: mapset within given location (default: 'PERMANENT')
    
    :returns: path to ``gisrc`` file (to be deleted later)
    """
    # TODO: why we don't set GISBASE?
    mswin = sys.platform.startswith('win')
    # define PATH
    os.environ['PATH'] += os.pathsep + os.path.join(gisbase, 'bin')
    os.environ['PATH'] += os.pathsep + os.path.join(gisbase, 'scripts')
    if mswin:  # added for winGRASS
        os.environ['PATH'] += os.pathsep + os.path.join(gisbase, 'extrabin')

    # add addons to the PATH
    # copied and simplified from lib/init/grass.py
    if mswin:
        config_dirname = "GRASS7"
        config_dir = os.path.join(os.getenv('APPDATA'), config_dirname)
    else:
        config_dirname = ".grass7"
        config_dir = os.path.join(os.getenv('HOME'), config_dirname)
    addon_base = os.path.join(config_dir, 'addons')
    os.environ['GRASS_ADDON_BASE'] = addon_base
    if not mswin:
        os.environ['PATH'] += os.pathsep + os.path.join(addon_base, 'scripts')

    # define LD_LIBRARY_PATH
    if '@LD_LIBRARY_PATH_VAR@' not in os.environ:
        os.environ['@LD_LIBRARY_PATH_VAR@'] = ''
    os.environ['@LD_LIBRARY_PATH_VAR@'] += os.pathsep + os.path.join(gisbase, 'lib')

    os.environ['GIS_LOCK'] = str(os.getpid())

    # Set GRASS_PYTHON and PYTHONPATH to find GRASS Python modules
    if not os.getenv('GRASS_PYTHON'):
        if sys.platform == 'win32':
            os.environ['GRASS_PYTHON'] = "python.exe"
        else:
            os.environ['GRASS_PYTHON'] = "python"
    
    path = os.getenv('PYTHONPATH')
    etcpy = os.path.join(gisbase, 'etc', 'python')
    if path:
        path = etcpy + os.pathsep + path
    else:
        path = etcpy
    os.environ['PYTHONPATH'] = path

    # TODO: isn't this contra-productive? may fail soon since we cannot
    # write to the installation (applies also to defaults for Location
    # and mapset) I don't see what would be the use case here.
    if not dbase:
        dbase = gisbase

    os.environ['GISRC'] = write_gisrc(dbase, location, mapset)
    return os.environ['GISRC']
