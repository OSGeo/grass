#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       v.unpack
# AUTHOR(S):    Luca Delucchi
#               
# PURPOSE:      Unpack up a vector map packed with v.pack
# COPYRIGHT:    (C) 2010-2013 by the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

#%module
#% description: Unpacks a vector map packed with v.pack.
#% keywords: vector, import, copying
#%end
#%option G_OPT_F_INPUT
#% gisprompt: old,bin,file
#% description: Name of input pack file
#% required : yes
#%end
#%option G_OPT_V_OUTPUT
#% label: Name for output vector map
#% description: Default: taken from input file internals
#% required : no
#%end
#%flag
#% key: o
#% description: Override projection check (use current location's projection)
#%end

import os
import sys
import shutil
import tarfile
import atexit
import filecmp

from grass.script import core as grass
from grass.script import db as grassdb

def cleanup():
    grass.try_rmdir(tmp_dir)

def main():
    infile = options['input']
    
    # create temporary directory
    global tmp_dir
    tmp_dir = grass.tempdir()
    grass.debug('tmp_dir = %s' % tmp_dir)
    
    # check if the input file exists
    if not os.path.exists(infile):
        grass.fatal(_("File <%s> not found") % infile)
    
    # copy the files to tmp dir
    input_base = os.path.basename(infile)
    shutil.copyfile(infile, os.path.join(tmp_dir, input_base))
    os.chdir(tmp_dir)
    tar = tarfile.TarFile.open(name = input_base, mode = 'r')
    try:
        data_name = tar.getnames()[0]
    except:
        grass.fatal(_("Pack file unreadable"))
    
    # set the output name
    if options['output']:
        map_name = options['output']
    else:
        map_name = data_name
    
    # grass env
    gisenv = grass.gisenv()
    mset_dir = os.path.join(gisenv['GISDBASE'],
                            gisenv['LOCATION_NAME'],
                            gisenv['MAPSET'])
    
    new_dir = os.path.join(mset_dir, 'vector', map_name)
    
    gfile = grass.find_file(name = map_name, element = 'vector',
                            mapset = '.')
    overwrite = os.getenv('GRASS_OVERWRITE')
    if gfile['file'] and overwrite != '1':
        grass.fatal(_("Vector map <%s> already exists") % map_name)
    elif overwrite == '1' and gfile['file']:
        grass.warning(_("Vector map <%s> already exists and will be overwritten") % map_name)
        grass.run_command('g.remove', quiet = True, vect = map_name)
        shutil.rmtree(new_dir,True)
    
    # extract data
    tar.extractall()
    
    # check projection compatibility in a rather crappy way
    loc_proj = os.path.join(mset_dir, '..', 'PERMANENT', 'PROJ_INFO')
    loc_proj_units = os.path.join(mset_dir, '..', 'PERMANENT', 'PROJ_UNITS')
    if not grass.compare_key_value_text_files(os.path.join(tmp_dir,'PROJ_INFO'), loc_proj) or \
       not grass.compare_key_value_text_files(os.path.join(tmp_dir,'PROJ_UNITS'), loc_proj_units):
        if flags['o']:
            grass.warning(_("Projection information does not match. Proceeding..."))
        else:
            grass.fatal(_("Projection information does not match. Aborting."))
    
    # new db
    fromdb = os.path.join(tmp_dir, 'db.sqlite')
    # copy file
    shutil.copytree(data_name, new_dir)
    # exist fromdb
    if os.path.exists(fromdb):
        # the db connection in the output mapset
        dbconn = grassdb.db_connection()
        todb = dbconn['database']
        # return all tables
        list_fromtable = grass.read_command('db.tables', driver = 'sqlite',
                                            database = fromdb).splitlines()
        
        # return the list of old connection for extract layer number and key
        dbln = open(os.path.join(new_dir,'dbln'), 'r')
        dbnlist = dbln.readlines()
        dbln.close()
        # check if dbf or sqlite directory exists
        if dbconn['driver'] == 'dbf' and not os.path.exists(os.path.join(mset_dir, 'dbf')):
	    os.mkdir(os.path.join(mset_dir, 'dbf'))
	elif dbconn['driver'] == 'sqlite' and not os.path.exists(os.path.join(mset_dir, 'sqlite')):
	    os.mkdir(os.path.join(mset_dir, 'sqlite'))
        # for each old connection
        for t in dbnlist:
            # it split the line of each connection, to found layer number and key
            if len(t.split('|')) != 1:
                values = t.split('|')
            else:
                values = t.split(' ')
            
            from_table = values[1]
            layer = values[0].split('/')[0]
            # we need to take care about the table name in case of several layer
            if options["output"]:
                if len(dbnlist) > 1:
                    to_table = "%s_%s" % (map_name, layer)
                else:
                    to_table = map_name
            else:
                to_table = from_table
            
            grass.verbose(_("Coping table <%s> as table <%s>") % (from_table, to_table))
            
            # copy the table in the default database
            if 0 != grass.run_command('db.copy', to_driver = dbconn['driver'], 
                                      to_database = todb, to_table = to_table, 
                                      from_driver = 'sqlite', from_database = fromdb,
                                      from_table = from_table):
                grass.fatal(_("Unable to copy table <%s> as table <%s>") % (from_table, to_table))
                
            grass.verbose(_("Connect table <%s> to vector map <%s> at layer <%s>") % \
                              (to_table, map_name, layer))
            
            # and connect the new tables with the right layer
            if 0 != grass.run_command('v.db.connect', flags = 'o', quiet = True,
                                      driver = dbconn['driver'], database = todb, 
                                      map =  map_name, key = values[2],
                                      layer = layer, table = to_table):
                grass.fatal(_("Unable to connect table <%s> to vector map <%s>") % \
                                (to_table, map_name))
    
    grass.message(_("Vector map <%s> succesfully unpacked") % map_name)

if __name__ == "__main__":
  options, flags = grass.parser()
  atexit.register(cleanup)
  sys.exit(main())
