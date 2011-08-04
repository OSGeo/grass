#!/usr/bin/env python

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
#% keywords: vector
#% keywords: import
#%end
#%option G_OPT_F_INPUT
#% description: Name of input E00 file
#%end
#%option G_OPT_V_TYPE
#% options: point,line,area
#% required: yes
#%end
#%option G_OPT_V_OUTPUT
#% required : no
#%end

import sys
import os
import shutil
import glob
from grass.script import core as grass

def main():
    filename = options['file']
    type = options['type']
    vect = options['vect']

    e00tmp = str(os.getpid())

    #### check for avcimport
    if not grass.find_program('avcimport'):
	grass.fatal(_("'avcimport' program not found, install it first") +
		    "\n" +
		    "http://avce00.maptools.org")

    #### check for e00conv
    if not grass.find_program('e00conv'):
	grass.fatal(_("'e00conv' program not found, install it first") +
		    "\n" +
		    "http://avce00.maptools.org")

    # check that the user didn't use all three, which gets past the parser.
    if type not in ['point','line','area']:
	grass.fatal(_('Must specify one of "point", "line", or "area".'))

    e00name = grass.basename(filename, 'e00')
    # avcimport only accepts 13 chars:
    e00shortname = e00name[:13]

    #check if this is a split E00 file (.e01, .e02 ...):
    merging = False
    if os.path.exists(e00name + '.e01') or os.path.exists(e00name + '.E01'):
	grass.message(_("Found that E00 file is split into pieces (.e01, ...). Merging..."))
	merging = True

    if vect:
	name = vect
    else:
	name = e00name

    ### do import

    #make a temporary directory
    tmpdir = grass.tempfile()
    grass.try_remove(tmpdir)
    os.mkdir(tmpdir)

    files = glob.glob(e00name + '.e[0-9][0-9]') + glob.glob(e00name + '.E[0-9][0-9]')
    for f in files:
	shutil.copy(f, tmpdir)

    #change to temporary directory to later avoid removal problems (rm -r ...)
    os.chdir(tmpdir)

    #check for binay E00 file (we can just check if import fails):
    #avcimport doesn't set exist status :-(

    if merging:
	files.sort()
	filename = "%s.cat.%s.e00" % (e00name, e00tmp)
	outf = file(filename, 'wb')
	for f in files:
	    inf = file(f, 'rb')
	    shutil.copyfileobj(inf, outf)
	    inf.close()
	outf.close()

    nuldev = file(os.devnull, 'w+')

    grass.message(_("An error may appear next which will be ignored..."))
    if grass.call(['avcimport', filename, e00shortname], stdout = nuldev, stderr = nuldev) == 1:
	grass.message(_("E00 ASCII found and converted to Arc Coverage in current directory"))
    else:
	grass.message(_("E00 Compressed ASCII found. Will uncompress first..."))
	grass.try_remove(e00shortname)
	grass.try_remove(info)
	grass.call(['e00conv', filename, e00tmp + '.e00'])
	grass.message(_("...converted to Arc Coverage in current directory"))
	grass.call(['avcimport', e00tmp + '.e00', e00shortname], stderr = nuldev)

    #SQL name fix:
    name = name.replace('-', '_')

    ## let's import...
    grass.message(_("Importing %ss...") % type)

    layer = dict(point = 'LAB', line = 'ARC', area = ['LAB','ARC'])
    itype = dict(point = 'point', line = 'line', area = 'centroid')

    if grass.run_command('v.in.ogr', flags = 'o', dsn = e00shortname,
			 layer = layer[type], type = itype[type],
			 output = name) != 0:
	grass.fatal(_("An error occurred while running v.in.ogr"))

    grass.message(_("Imported <%s> vector map <%s>.") % (type, name))

    #### clean up the mess
    for root, dirs, files in os.walk('.', False):
	for f in files:
	    path = os.path.join(root, f)
	    grass.try_remove(path)
	for d in dirs:
	    path = os.path.join(root, d)
	    grass.try_rmdir(path)

    os.chdir('..')
    os.rmdir(tmpdir)
	
    #### end
    grass.message(_("Done."))

    # write cmd history:
    grass.vector_history(name)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

