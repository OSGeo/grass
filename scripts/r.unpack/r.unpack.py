#!/usr/bin/env python3
############################################################################
#
# MODULE:	r.unpack
# AUTHOR(S):	Hamish Bowman, Otago University, New Zealand
#               Converted to Python by Martin Landa <landa.martin gmail.com>
# PURPOSE:	Unpack up a raster map packed with r.pack
# COPYRIGHT:	(C) 2010-2017 by the GRASS Development Team
#
#		This program is free software under the GNU General
#		Public License (>=v2). Read the file COPYING that
#		comes with GRASS for details.
#
#############################################################################

#%module
#% description: Imports a GRASS GIS specific raster archive file (packed with r.pack) as a raster map
#% keyword: raster
#% keyword: import
#% keyword: copying
#%end
#%option G_OPT_F_BIN_INPUT
#% description: Name of input pack file
#% key_desc: name.pack
#%end
#%option G_OPT_R_OUTPUT
#% description: Name for output raster map (default: taken from input file internals)
#% required: no
#% guisection: Output settings
#%end
#%flag
#% key: o
#% label: Override projection check (use current location's projection)
#% description: Assume that the dataset has same projection as the current location
#% guisection: Output settings
#%end
#%flag
#% key: p
#% label: Print projection information of input pack file and exit
#% guisection: Print
#%end

import os
import sys
import shutil
import tarfile
import atexit

from grass.script.utils import diff_files, try_rmdir
from grass.script import core as grass


def cleanup():
    try_rmdir(tmp_dir)


def main():
    infile = options['input']

    global tmp_dir
    tmp_dir = grass.tempdir()
    grass.debug('tmp_dir = {tmpdir}'.format(tmpdir=tmp_dir))

    if not os.path.exists(infile):
        grass.fatal(_('File {name} not found.'.format(name=infile)))

    gisenv = grass.gisenv()
    mset_dir = os.path.join(gisenv['GISDBASE'],
                            gisenv['LOCATION_NAME'],
                            gisenv['MAPSET'])
    input_base = os.path.basename(infile)
    shutil.copyfile(infile, os.path.join(tmp_dir, input_base))
    os.chdir(tmp_dir)
    tar = tarfile.TarFile.open(name=input_base, mode='r')
    try:
        data_name = tar.getnames()[0]
    except:
        grass.fatal(_("Pack file unreadable"))

    if flags['p']:
        # print proj info and exit
        try:
            for fname in ['PROJ_INFO', 'PROJ_UNITS']:
                f = tar.extractfile('{}/{}'.format(data_name, fname))
                sys.stdout.write(f.read().decode())
        except KeyError:
            grass.fatal(_("Pack file unreadable: file '{}' missing".format(fname)))
        tar.close()

        return 0

    if options['output']:
        map_name = options['output']
    else:
        map_name = data_name.split('@')[0]

    gfile = grass.find_file(name=map_name, element='cell', mapset='.')
    if gfile['file']:
        if os.environ.get('GRASS_OVERWRITE', '0') != '1':
            grass.fatal(_('Raster map <{name}> already exists'.format(name=map_name)))
        else:
            grass.warning(
                _('Raster map <{name}> already exists and will be overwritten'.format(name=map_name)))

    # extract data
    tar.extractall()
    tar.close()
    os.chdir(data_name)

    if os.path.exists('cell'):
        pass
    elif os.path.exists('coor'):
        grass.fatal(_('This GRASS GIS pack file contains vector data. Use '
                      'v.unpack to unpack <{name}>'.format(name=map_name)))
    else:
        grass.fatal(_("Pack file unreadable"))

    # check projection compatibility in a rather crappy way

    if flags['o']:
        grass.warning(_("Overriding projection check (using current location's projection)."))

    else:

        diff_result_1 = diff_result_2 = None

        proj_info_file_1 = 'PROJ_INFO'
        proj_info_file_2 = os.path.join(mset_dir, '..', 'PERMANENT', 'PROJ_INFO')

        skip_projection_check = False
        if not os.path.exists(proj_info_file_1):
            if os.path.exists(proj_info_file_2):
                grass.fatal(
                    _("PROJ_INFO file is missing, unpack raster map in XY (unprojected) location."))
            skip_projection_check = True  # XY location

        if not skip_projection_check:
            if not grass.compare_key_value_text_files(filename_a=proj_info_file_1,
                                                      filename_b=proj_info_file_2,
                                                      proj=True):
                diff_result_1 = diff_files(proj_info_file_1, proj_info_file_2)

            proj_units_file_1 = 'PROJ_UNITS'
            proj_units_file_2 = os.path.join(mset_dir, '..', 'PERMANENT', 'PROJ_UNITS')

            if not grass.compare_key_value_text_files(filename_a=proj_units_file_1,
                                                      filename_b=proj_units_file_2,
                                                      units=True):
                diff_result_2 = diff_files(proj_units_file_1, proj_units_file_2)

            if diff_result_1 or diff_result_2:

                if diff_result_1:
                    grass.warning(_("Difference between PROJ_INFO file of packed map "
                                    "and of current location:\n{diff}").format(diff=''.join(diff_result_1)))
                if diff_result_2:
                    grass.warning(_("Difference between PROJ_UNITS file of packed map "
                                    "and of current location:\n{diff}").format(diff=''.join(diff_result_2)))
                grass.fatal(_("Projection of dataset does not appear to match current location."
                              " In case of no significant differences in the projection definitions,"
                              " use the -o flag to ignore them and use"
                              " current location definition."))

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

    grass.message(_('Raster map <{name}> unpacked'.format(name=map_name)))

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    sys.exit(main())
