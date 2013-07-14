#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       v.pack
# AUTHOR(S):    Luca Delucchi, Fondazione E. Mach (Italy)
#
# PURPOSE:      Pack up a vector map, collect vector map elements => gzip
# COPYRIGHT:    (C) 2011-2013 by the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

#%module
#% description: Packs up a vector map and support files for copying.
#% keywords: vector, export, copying
#%end
#%option G_OPT_V_INPUT
#% label: Name of vector map to pack up
#% description:
#%end
#%option G_OPT_F_OUTPUT
#% description: Name for output file (default is <input>.pack)
#% required : no
#%end
#%flag
#% key: c
#% description: Switch the compression off
#%end

import os
import sys
import shutil
import tarfile
import atexit

from grass.script import core as grass
from grass.script import vector as vector

def cleanup():
    grass.try_rmdir(basedir)

def main():
    infile = options['input']
    compression_off = flags['c']
    
    # check if vector map exists
    gfile = grass.find_file(infile, element = 'vector')
    if not gfile['name']:
        grass.fatal(_("Vector map <%s> not found") % infile)
    
    # check if input vector map is in the native format
    if vector.vector_info(gfile['fullname'])['format'] != 'native':
        grass.fatal(_("Unable to pack vector map <%s>. Only native format supported.") % \
                        gfile['fullname'])
    
    # split the name if there is the mapset name
    if infile.find('@'):
        infile = infile.split('@')[0]
    
    # output name
    if options['output']:
        outfile = options['output']
    else:
        outfile = infile + '.pack'
    
    # check if exists the output file
    if os.path.exists(outfile):
        if os.getenv('GRASS_OVERWRITE'):
            grass.warning(_("Pack file <%s> already exists and will be overwritten") % outfile)
            grass.try_remove(outfile)
        else:
            grass.fatal(_("option <%s>: <%s> exists.") % ("output", outfile))
    
    # prepare for packing
    grass.verbose(_("Packing <%s>...") % (gfile['fullname']))
    global basedir
    basedir = grass.tempdir()

    # write tar file, optional compression 
    if compression_off:
        tar = tarfile.open(name = outfile, mode = 'w:')
    else:
        tar = tarfile.open(name = outfile, mode = 'w:gz')
    tar.add(gfile['file'], infile)
    
    # check if exist a db connection for the vector 
    db_vect = vector.vector_db(gfile['fullname'])
    if not db_vect:
        grass.verbose(_('There is not database connected with vector map <%s>') % gfile['fullname'])
    else:
        # for each layer connection save a table in sqlite database
        sqlitedb = os.path.join(basedir, 'db.sqlite')
        for i, dbconn in db_vect.iteritems():
            grass.run_command('db.copy', from_driver = dbconn['driver'], 
                              from_database = dbconn['database'],
                              from_table =  dbconn['table'], 
                              to_driver = 'sqlite', to_database = sqlitedb, 
                              to_table = dbconn['table'])
        tar.add(sqlitedb, os.path.join(infile, 'db.sqlite'))
    
    # add to the tar file the PROJ files to check when unpack file    
    gisenv = grass.gisenv()
    for support in ['INFO', 'UNITS']:
        path = os.path.join(gisenv['GISDBASE'], gisenv['LOCATION_NAME'],
                            'PERMANENT', 'PROJ_' + support)
        if os.path.exists(path):
            tar.add(path, os.path.join(infile, 'PROJ_' + support))
    tar.close()
    
    grass.message(_("Pack file <%s> created") % os.path.join(os.getcwd(), outfile))
            
if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    sys.exit(main())
