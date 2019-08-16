"""Setup, initialization, and clean-up functions

Functions can be used in Python scripts to setup a GRASS environment
and session without using grassXY.

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
    grass7bin = 'grass78'
    if sys.platform.startswith('win'):
        # MS Windows
        grass7bin = r'C:\OSGeo4W\bin\grass78.bat'
        # uncomment when using standalone WinGRASS installer
        # grass7bin = r'C:\Program Files (x86)\GRASS GIS 7.8.0\grass78.bat'
        # this can be avoided if GRASS executable is added to PATH
    elif sys.platform == 'darwin':
        # Mac OS X
        # TODO: this have to be checked, maybe unix way is good enough
        grass7bin = '/Applications/GRASS/GRASS-7.8.app/'

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
    import grass.script.setup as gsetup

    # launch session
    rcfile = gsetup.init(gisbase, gisdb, location, mapset)

    # example calls
    gscript.message('Current GRASS GIS 7 environment:')
    print gscript.gisenv()

    gscript.message('Available raster maps:')
    for rast in gscript.list_strings(type='raster'):
        print rast

    gscript.message('Available vector maps:')
    for vect in gscript.list_strings(type='vector'):
        print vect

    # clean up at the end
    gsetup.cleanup()


(C) 2010-2019 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com>
@author Markus Metz
"""

# TODO: this should share code from lib/init/grass.py
# perhaps grass.py can import without much trouble once GISBASE
# is known, this would allow moving things from there, here
# then this could even do locking

import os
import sys
import tempfile as tmpfile


windows = sys.platform == 'win32'


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


def init(gisbase, dbase='', location='demolocation', mapset='PERMANENT'):
    """Initialize system variables to run GRASS modules

    This function is for running GRASS GIS without starting it with the 
    standard script grassXY. No GRASS modules shall be called before 
    call of this function but any module or user script can be called 
    afterwards because a GRASS session has been set up. GRASS Python 
    libraries are usable as well in general but the ones using C 
    libraries through ``ctypes`` are not (which is caused by library 
    path not being updated for the current process which is a common 
    operating system limitation).

    To create a GRASS session a ``gisrc`` file is created.
    Caller is responsible for deleting the ``gisrc`` file.

    Basic usage::

        # ... setup GISBASE and PYTHON path before import
        import grass.script as gscript
        gisrc = gscript.setup.init("/usr/bin/grass7",
                                   "/home/john/grassdata",
                                   "nc_spm_08", "user1")
        # ... use GRASS modules here
        # end the session
        gscript.setup.finish()

    :param gisbase: path to GRASS installation
    :param dbase: path to GRASS database (default: '')
    :param location: location name (default: 'demolocation')
    :param mapset: mapset within given location (default: 'PERMANENT')
    
    :returns: path to ``gisrc`` file (to be deleted later)
    """
    # Set GISBASE
    os.environ['GISBASE'] = gisbase
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
    os.environ['PATH'] += os.pathsep + os.path.join(addon_base, 'bin')

    # define LD_LIBRARY_PATH
    if '@LD_LIBRARY_PATH_VAR@' not in os.environ:
        os.environ['@LD_LIBRARY_PATH_VAR@'] = ''
    os.environ['@LD_LIBRARY_PATH_VAR@'] += os.pathsep + os.path.join(gisbase, 'lib')

    # TODO: lock the mapset?
    os.environ['GIS_LOCK'] = str(os.getpid())

    # Set GRASS_PYTHON and PYTHONPATH to find GRASS Python modules
    if not os.getenv('GRASS_PYTHON'):
        if sys.platform == 'win32':
            os.environ['GRASS_PYTHON'] = "python3.exe"
        else:
            os.environ['GRASS_PYTHON'] = "python3"
    
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


# clean-up functions when terminating a GRASS session
# these fns can only be called within a valid GRASS session
def clean_default_db():
    # clean the default db if it is sqlite
    from grass.script import db as gdb
    from grass.script import core as gcore

    conn = gdb.db_connection()
    if conn and conn['driver'] == 'sqlite':
        # check if db exists
        gisenv = gcore.gisenv()
        database = conn['database']
        database = database.replace('$GISDBASE', gisenv['GISDBASE'])
        database = database.replace('$LOCATION_NAME', gisenv['LOCATION_NAME'])
        database = database.replace('$MAPSET', gisenv['MAPSET'])
        if os.path.exists(database):
            gcore.message(_("Cleaning up default sqlite database ..."))
            gcore.start_command('db.execute', sql = 'VACUUM')
	    # give it some time to start
            import time
            time.sleep(0.1)


def call(cmd, **kwargs):
    import subprocess
    """Wrapper for subprocess.call to deal with platform-specific issues"""
    if windows:
        kwargs['shell'] = True
    return subprocess.call(cmd, **kwargs)


def clean_temp():
    from grass.script import core as gcore

    gcore.message(_("Cleaning up temporary files..."))
    nul = open(os.devnull, 'w')
    gisbase = os.environ['GISBASE']
    call([os.path.join(gisbase, "etc", "clean_temp")], stdout=nul)
    nul.close()


def finish():
    """Terminate the GRASS session and clean up

    GRASS commands can no longer be used after this function has been
    called
    
    Basic usage::
        import grass.script as gscript

        gscript.setup.cleanup()
    """

    clean_default_db()
    clean_temp()
    # TODO: unlock the mapset?
    # unset the GISRC and delete the file
    from grass.script import utils as gutils
    gutils.try_remove(os.environ['GISRC'])
    os.environ.pop('GISRC')
    # remove gislock env var (not the gislock itself
    os.environ.pop('GIS_LOCK')
