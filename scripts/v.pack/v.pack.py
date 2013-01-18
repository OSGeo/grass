#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       v.pack
# AUTHOR(S):    Luca Delucchi, Fondazione E. Mach (Italy)
#
# PURPOSE:      Pack up a vector map, collect vector map elements => gzip
# COPYRIGHT:    (C) 2011 by the GRASS Development Team
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
#%option
#% key: input
#% type: string
#% gisprompt: old,vector,vector
#% description: Name of vector map to pack up
#% key_desc: name
#% required : yes
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

from grass.script import core as grass
from grass.script import vector as vector

def main():
    infile = options['input']
    compression_off = flags['c']
    #search if file exist
    gfile = grass.find_file(infile, element = 'vector')
    if not gfile['name']:
        grass.fatal(_("Vector map <%s> not found") % infile)
    #split the name if there is the mapset name
    if infile.find('@'):
        infile = infile.split('@')[0]    
    #output name
    if options['output']:
        outfile = options['output']
    else:
        outfile = infile + '.pack'
    #check if exists the output file
    if os.path.exists(outfile):
        if os.getenv('GRASS_OVERWRITE'):
            grass.warning(_("Pack file <%s> already exists and will be overwritten") % outfile)
            grass.try_remove(outfile)
        else:
            grass.fatal(_("option <output>: <%s> exists.") % outfile)
    grass.message(_("Packing <%s> to <%s>...") % (gfile['fullname'], outfile))
    basedir = os.path.sep.join(gfile['file'].split(os.path.sep)[:-2])
    olddir  = os.getcwd()
    #check if exist a db connection for the vector 
    db_vect = vector.vector_db(gfile['fullname'])

    #db not exist and skip the db copy
    sqlitedb = None
    if not db_vect:
        grass.message(_('There is not database connected with vector %s') % gfile['fullname'])
    else:
        # for each layer connection save a table
        for i, dbconn in db_vect.iteritems():
            sqlitedb = os.path.join(basedir, 'vector', infile, 'db.sqlite') 
            cptable = grass.run_command('db.copy', from_driver = dbconn['driver'], 
                      from_database = dbconn['database'], from_table =  dbconn['table'], 
                      to_driver = 'sqlite', to_database = sqlitedb, 
                      to_table = dbconn['table'])
        
    # write tar file, optional compression 
    if compression_off:
        tar = tarfile.open(name = outfile, mode = 'w:')
    else:
        tar = tarfile.open(name = outfile, mode = 'w:gz')
    tar.add(os.path.join(basedir,'vector',infile),infile)
    gisenv = grass.gisenv()
    #add to the tar file the PROJ files to check when unpack file
    for support in ['INFO', 'UNITS']:
        path = os.path.join(gisenv['GISDBASE'], gisenv['LOCATION_NAME'],
                            'PERMANENT', 'PROJ_' + support)
        if os.path.exists(path):
          tar.add(path,os.path.join(infile,'PROJ_' + support))
    tar.close()
    #remove the db from the vector directory #ONLY THE DB FOR THE COPY NOT DB OF GRASS
    if db_vect and sqlitedb:
        os.remove(sqlitedb)
    grass.verbose(_("Vector map saved to '%s'" % os.path.join(olddir, outfile)))
            
if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
