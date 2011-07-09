#!/usr/bin/env python
############################################################################
#
# MODULE:	r.pack
# AUTHOR(S):	Hamish Bowman, Otago University, New Zealand
#               Converted to Python by Martin Landa <landa.martin gmail.com>
# PURPOSE:	Pack up a raster map, collect raster map elements => gzip
# COPYRIGHT:	(C) 2004-2008, 2010 by the GRASS Development Team
#
#		This program is free software under the GNU General
#		Public License (>=v2). Read the file COPYING that
#		comes with GRASS for details.
#
#############################################################################

#%module
#% description: Packs up a raster map and support files for copying.
#% keywords: raster
#% keywords: export
#% keywords: copying
#%end
#%option
#% key: input
#% type: string
#% gisprompt: old,cell,raster
#% description: Name of raster map to pack up
#% key_desc: name
#% required : yes
#%end
#%option
#% key: output
#% type: string
#% gisprompt: new_file,file,output
#% description: Name for output file (default is <input>.pack)
#% key_desc: path
#% required : no
#%end

import os
import sys
import shutil
import atexit
import tarfile

from grass.script import core as grass

def cleanup():
    grass.try_rmdir(tmp)

def main():
    infile = options['input']
    if options['output']:
        outfile = options['output']
    else:
        outfile = infile + '.pack'
    
    global tmp
    tmp = grass.tempdir()
    tmp_dir = os.path.join(tmp, infile)
    os.mkdir(tmp_dir)
    grass.debug('tmp_dir = %s' % tmp_dir)
    
    gfile = grass.find_file(name = infile, element = 'cell')
    if not gfile['name']:
        grass.fatal(_("Raster map <%s> not found") % infile)
    
    if os.path.exists(outfile):
        if os.getenv('GRASS_OVERWRITE'):
            grass.warning(_("Pack file <%s> already exists and will be overwritten") % outfile)
            grass.try_remove(outfile)
        else:
            grass.fatal(_("option <output>: <%s> exists.") % outfile)
    
    grass.message(_("Packing <%s> to <%s>...") % (gfile['fullname'], outfile))
    basedir = os.path.sep.join(gfile['file'].split(os.path.sep)[:-2])
    olddir  = os.getcwd()
    
    # copy elements
    for element in ['cats', 'cell', 'cellhd', 'colr', 'fcell', 'hist']:
        path = os.path.join(basedir, element, infile)
        if os.path.exists(path):
            grass.debug('copying %s' % path)
            shutil.copyfile(path,
                            os.path.join(tmp_dir, element))
            
    if os.path.exists(os.path.join(basedir, 'cell_misc', infile)):
        shutil.copytree(os.path.join(basedir, 'cell_misc', infile),
                        os.path.join(tmp_dir, 'cell_misc'))
        
    if not os.listdir(tmp_dir):
        grass.fatal(_("No raster map components found"))
                    
    # copy projection info
    # (would prefer to use g.proj*, but this way is 5.3 and 5.7 compat)
    gisenv = grass.gisenv()
    for support in ['INFO', 'UNITS']:
        path = os.path.join(gisenv['GISDBASE'], gisenv['LOCATION_NAME'],
                            'PERMANENT', 'PROJ_' + support)
        if os.path.exists(path):
            shutil.copyfile(path, os.path.join(tmp_dir, 'PROJ_' + support))
    
    # pack it all up
    os.chdir(tmp)
    tar = tarfile.TarFile.open(name = outfile, mode = 'w:gz')
    tar.add(infile, recursive = True)
    tar.close()
    try:
        shutil.move(outfile, olddir)
    except shutil.Error, e:
        grass.fatal(e)
        
    os.chdir(olddir)
    
    grass.verbose(_("Raster map saved to '%s'" % \
                        os.path.join(olddir, outfile)))
    
if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    sys.exit(main())
