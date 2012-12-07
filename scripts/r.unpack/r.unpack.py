#!/usr/bin/env python
############################################################################
#
# MODULE:	r.unpack
# AUTHOR(S):	Hamish Bowman, Otago University, New Zealand
#               Converted to Python by Martin Landa <landa.martin gmail.com>
# PURPOSE:	Unpack up a raster map packed with r.pack
# COPYRIGHT:	(C) 2004-2008, 2010-2012 by the GRASS Development Team
#
#		This program is free software under the GNU General
#		Public License (>=v2). Read the file COPYING that
#		comes with GRASS for details.
#
#############################################################################

#%module
#% description: Unpacks a raster map packed with r.pack.
#% keywords: raster
#% keywords: import
#% keywords: copying
#%end
#%option G_OPT_F_INPUT
#% description: Name of input pack file
#% gisprompt: old,file,bin_input
#% key_desc: name.pack
#%end
#%option G_OPT_R_OUTPUT
#% description: Name for output raster map (default: taken from input file internals)
#% required: no
#% guisection: Output settings
#%end
#%flag
#% key: o
#% description: Override projection check (use current location's projection)
#% guisection: Output settings
#%end

import os
import sys
import shutil
import tarfile
import atexit
import filecmp

from grass.script import core as grass

def cleanup():
    grass.try_rmdir(tmp_dir)

def main():
    infile = options['input']
    
    global tmp_dir
    tmp_dir = grass.tempdir()
    grass.debug('tmp_dir = %s' % tmp_dir)
    
    if not os.path.exists(infile):
        grass.fatal(_("File <%s> not found") % infile)
    
    gisenv = grass.gisenv()
    mset_dir = os.path.join(gisenv['GISDBASE'],
                            gisenv['LOCATION_NAME'],
                            gisenv['MAPSET'])
    input_base = os.path.basename(infile)
    shutil.copyfile(infile, os.path.join(tmp_dir, input_base))
    os.chdir(tmp_dir)
    tar = tarfile.TarFile.open(name = input_base, mode = 'r')
    try:
        data_name = tar.getnames()[0]
    except:
        grass.fatal(_("Pack file unreadable"))
    
    if options['output']:
        map_name = options['output']
    else:
        map_name = data_name.split('@')[0]
    
    gfile = grass.find_file(name = map_name, element = 'cell',
                            mapset = '.')
    if gfile['file']:
        if os.environ.get('GRASS_OVERWRITE', '0') != '1':
            grass.fatal(_("Raster map <%s> already exists") % map_name)
        else:
            grass.warning(_("Raster map <%s> already exists and will be overwritten") % map_name)
    
    # extract data
    tar.extractall()
    os.chdir(data_name)
    
    # check projection compatibility in a rather crappy way
    if not grass.compare_key_value_text_files('PROJ_INFO', os.path.join(mset_dir, '..', 'PERMANENT', 'PROJ_INFO')) or \
            not grass.compare_key_value_text_files('PROJ_UNITS', os.path.join(mset_dir, '..', 'PERMANENT', 'PROJ_UNITS')):
        if flags['o']:
            grass.warning(_("Projection information does not match. Proceeding..."))
        else:
            grass.fatal(_("Projection information does not match. Aborting."))
    
    # install in $MAPSET
    for element in ['cats', 'cell', 'cellhd', 'cell_misc', 'colr', 'fcell', 'hist']:
        if not os.path.exists(element):
            continue
        path = os.path.join(mset_dir, element)
        if not os.path.exists(path):
            os.mkdir(path)
        if element == 'cell_misc':
            path = os.path.join(mset_dir, element, map_name)
            if os.path.exists(path):
                shutil.rmtree(path)
            shutil.copytree('cell_misc', path)
        else:
            shutil.copyfile(element, os.path.join(mset_dir, element, map_name))
    
    grass.message(_("Raster map <%s> unpacked") % map_name)
    
if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    sys.exit(main())
