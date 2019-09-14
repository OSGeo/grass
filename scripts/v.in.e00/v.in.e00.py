#!/usr/bin/env python3

############################################################################
#
# MODULE:       v.in.e00
#
# AUTHOR(S):    Markus Neteler, Otto Dassau
#               Converted to Python by Glynn Clements
#
# PURPOSE:      Import E00 data into a GRASS vector map
#		Imports single and split E00 files (.e00, .e01, .e02 ...)
#
# COPYRIGHT:    (c) 2004, 2005 GDF Hannover bR, http://www.gdf-hannover.de
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################
#
# REQUIREMENTS:
#      -  avcimport: http://avce00.maptools.org

#%module
#% description: Imports E00 file into a vector map.
#% keyword: vector
#% keyword: import
#% keyword: E00
#%end
#%option G_OPT_F_BIN_INPUT
#% description: Name of input E00 file
#%end
#%option G_OPT_V_TYPE
#% options: point,line,area
#% answer: point
#% required: yes
#%end
#%option G_OPT_V_OUTPUT
#%end

import os
import shutil
import glob
from grass.script.utils import try_rmdir, try_remove, basename
from grass.script import vector as gvect
from grass.script import core as gcore
from grass.exceptions import CalledModuleError


def main():
    filename = options['input']
    type = options['type']
    vect = options['output']

    e00tmp = str(os.getpid())

    # check for avcimport
    if not gcore.find_program('avcimport'):
        gcore.fatal(_("'avcimport' program not found, install it first") +
                    "\n" + "http://avce00.maptools.org")

    # check for e00conv
    if not gcore.find_program('e00conv'):
        gcore.fatal(_("'e00conv' program not found, install it first") +
                    "\n" + "http://avce00.maptools.org")

    # check that the user didn't use all three, which gets past the parser.
    if type not in ['point', 'line', 'area']:
        gcore.fatal(_('Must specify one of "point", "line", or "area".'))

    e00name = basename(filename, 'e00')
    # avcimport only accepts 13 chars:
    e00shortname = e00name[:13]

    # check if this is a split E00 file (.e01, .e02 ...):
    merging = False
    if os.path.exists(e00name + '.e01') or os.path.exists(e00name + '.E01'):
        gcore.message(_("Found that E00 file is split into pieces (.e01, ...)."
                        " Merging..."))
        merging = True

    if vect:
        name = vect
    else:
        name = e00name

    # do import

    # make a temporary directory
    tmpdir = gcore.tempfile()
    try_remove(tmpdir)
    os.mkdir(tmpdir)

    files = glob.glob(
        e00name + '.e[0-9][0-9]') + glob.glob(e00name + '.E[0-9][0-9]')
    for f in files:
        shutil.copy(f, tmpdir)

    # change to temporary directory to later avoid removal problems (rm -r ...)
    os.chdir(tmpdir)

    # check for binay E00 file (we can just check if import fails):
    # avcimport doesn't set exist status :-(

    if merging:
        files.sort()
        filename = "%s.cat.%s.e00" % (e00name, e00tmp)
        outf = open(filename, 'wb')
        for f in files:
            inf = open(f, 'rb')
            shutil.copyfileobj(inf, outf)
            inf.close()
        outf.close()

    nuldev = open(os.devnull, 'w+')

    gcore.message(_("An error may appear next which will be ignored..."))
    if gcore.call(['avcimport', filename, e00shortname], stdout=nuldev,
                  stderr=nuldev) == 1:
        gcore.message(_("E00 ASCII found and converted to Arc Coverage in "
                        "current directory"))
    else:
        gcore.message(
            _("E00 Compressed ASCII found. Will uncompress first..."))
        try_remove(e00shortname)
        gcore.call(['e00conv', filename, e00tmp + '.e00'])
        gcore.message(_("...converted to Arc Coverage in current directory"))
        gcore.call(['avcimport', e00tmp + '.e00', e00shortname], stderr=nuldev)

    # SQL name fix:
    name = name.replace('-', '_')

    # let's import...
    gcore.message(_("Importing %ss...") % type)

    layer = dict(point='LAB', line='ARC', area=['LAB', 'ARC'])
    itype = dict(point='point', line='line', area='centroid')

    try:
        gcore.run_command('v.in.ogr', flags='o', input=e00shortname,
                          layer=layer[type], type=itype[type],
                          output=name)
    except CalledModuleError:
        gcore.fatal(_("An error occurred while running v.in.ogr"))

    gcore.message(_("Imported <%s> vector map <%s>.") % (type, name))

    # clean up the mess
    for root, dirs, files in os.walk('.', False):
        for f in files:
            path = os.path.join(root, f)
            try_remove(path)
        for d in dirs:
            path = os.path.join(root, d)
            try_rmdir(path)

    os.chdir('..')
    os.rmdir(tmpdir)

    # end
    gcore.message(_("Done."))

    # write cmd history:
    gvect.vector_history(name)

if __name__ == "__main__":
    options, flags = gcore.parser()
    main()
