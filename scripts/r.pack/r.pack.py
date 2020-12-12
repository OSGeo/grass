#!/usr/bin/env python3
############################################################################
#
# MODULE:	r.pack
# AUTHOR(S):	Hamish Bowman, Otago University, New Zealand
#               Converted to Python by Martin Landa <landa.martin gmail.com>
# PURPOSE:	Pack up a raster map, collect raster map elements => gzip
# COPYRIGHT:	(C) 2004-2013 by the GRASS Development Team
#
#		This program is free software under the GNU General
#		Public License (>=v2). Read the file COPYING that
#		comes with GRASS for details.
#
#############################################################################

#%module
#% description: Exports a raster map as GRASS GIS specific archive file
#% keyword: raster
#% keyword: export
#% keyword: copying
#%end
#%option G_OPT_R_INPUT
#% description: Name of raster map to pack up
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
import atexit
import tarfile

from grass.script.utils import try_rmdir, try_remove
from grass.script import core as grass


def cleanup():
    try_rmdir(tmp)


def main():
    infile = options['input']
    compression_off = flags['c']
    mapset = None
    if '@' in infile:
        infile, mapset = infile.split('@')

    if options['output']:
        outfile_path, outfile_base = os.path.split(os.path.abspath(options['output']))
    else:
        outfile_path, outfile_base = os.path.split(os.path.abspath(infile + ".pack"))

    outfile = os.path.join(outfile_path, outfile_base)

    global tmp
    tmp = grass.tempdir()
    tmp_dir = os.path.join(tmp, infile)
    os.mkdir(tmp_dir)
    grass.debug('tmp_dir = %s' % tmp_dir)

    gfile = grass.find_file(name=infile, element='cell', mapset=mapset)
    if not gfile['name']:
        grass.fatal(_("Raster map <%s> not found") % infile)

    if os.path.exists(outfile):
        if os.getenv('GRASS_OVERWRITE'):
            grass.warning(_("Pack file <%s> already exists and will be overwritten") % outfile)
            try_remove(outfile)
        else:
            grass.fatal(_("option <output>: <%s> exists.") % outfile)

    grass.message(_("Packing <%s> to <%s>...") % (gfile['fullname'], outfile))
    basedir = os.path.sep.join(os.path.normpath(gfile['file']).split(os.path.sep)[:-2])
    olddir = os.getcwd()

    # copy elements
    info = grass.parse_command('r.info', flags='e', map=infile)
    vrt_files = {}
    if info['maptype'] == 'virtual':
        map_file = grass.find_file(
            name=infile, element='cell_misc',
        )
        if map_file['file']:
            vrt = os.path.join(map_file['file'], 'vrt')
            if os.path.exists(vrt):
                with open(vrt, 'r') as f:
                    for r in f.readlines():
                        map, mapset = r.split('@')
                        map_basedir = os.path.sep.join(
                            os.path.normpath(
                                map_file['file'],
                            ).split(os.path.sep)[:-2],
                        )
                        vrt_files[map] = map_basedir

    for element in [
            'cats', 'cell', 'cellhd', 'cell_misc', 'colr', 'fcell',
            'hist',
    ]:
        path = os.path.join(basedir, element, infile)
        if os.path.exists(path):
            grass.debug('copying %s' % path)
            if os.path.isfile(path):
                shutil.copyfile(
                    path, os.path.join(tmp_dir, element),
                )
            else:
                shutil.copytree(
                    path, os.path.join(tmp_dir, element),
                )

        # Copy vrt files
        if vrt_files:
            for f in vrt_files.keys():
                f_tmp_dir = os.path.join(tmp, f)
                if not os.path.exists(f_tmp_dir):
                    os.mkdir(f_tmp_dir)
                path = os.path.join(vrt_files[f], element, f)
                if os.path.exists(path):
                    grass.debug("copying vrt file {}".format(path))
                    if os.path.isfile(path):
                        shutil.copyfile(
                            path, os.path.join(f_tmp_dir, element),
                        )
                    else:
                        shutil.copytree(
                            path, os.path.join(f_tmp_dir, element),
                        )

    if not os.listdir(tmp_dir):
        grass.fatal(_("No raster map components found"))

    # copy projection info
    # (would prefer to use g.proj*, but this way is 5.3 and 5.7 compat)
    gisenv = grass.gisenv()
    for support in ['INFO', 'UNITS', 'EPSG']:
        path = os.path.join(gisenv['GISDBASE'], gisenv['LOCATION_NAME'],
                            'PERMANENT', 'PROJ_' + support)
        if os.path.exists(path):
            shutil.copyfile(path, os.path.join(tmp_dir, 'PROJ_' + support))

    # pack it all up
    os.chdir(tmp)
    if compression_off:
        tar = tarfile.TarFile.open(name=outfile_base, mode='w:')
    else:
        tar = tarfile.TarFile.open(name=outfile_base, mode='w:gz')
    tar.add(infile, recursive=True)
    if vrt_files:
        for f in vrt_files.keys():
            tar.add(f, recursive=True)

    tar.close()
    try:
        shutil.move(outfile_base, outfile)
    except shutil.Error as e:
        grass.fatal(e)

    os.chdir(olddir)

    grass.verbose(_("Raster map saved to '%s'" % outfile))


if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    sys.exit(main())
