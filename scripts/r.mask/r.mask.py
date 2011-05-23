#!/usr/bin/env python
#
############################################################################
#
# MODULE:       r.mask
# AUTHOR(S):	Michael Barton, Arizona State University
#               Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      Facilitates creation of raster MASK using r.reclass
# COPYRIGHT:	(C) 2005, 2007-2009 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Creates a MASK for limiting raster operation.
#% keywords: raster
#% keywords: mask
#%end
#%option
#% key: input
#% type: string
#% gisprompt: old,cell,raster
#% description: Raster map to use as mask
#% required: no
#% guisection: Create
#%end
#%option
#% key: maskcats
#% type: string
#% description: Category values to use for mask (format: 1 2 3 thru 7 *)
#% answer: *
#% guisection: Create
#%end
#%flag
#% key: i
#% description: Create inverse mask from specified 'maskcats' list
#% guisection: Create
#%end
#%flag
#% key: r
#% description: Remove existing mask (overrides other options)
#% guisection: Remove
#%end

import sys
import os
import grass.script as grass
import atexit

def cleanup():
    if tmp:
	grass.run_command('g.remove', rast = tmp, quiet = True)

def main():
    input = options['input']
    maskcats = options['maskcats']
    remove = flags['r']
    invert = flags['i']

    if not remove and not input:
	grass.fatal(_("Required parameter <input> not set"))

    #check if input file exists
    if not grass.find_file(input)['file'] and not remove:
        grass.fatal(_("<%s> does not exist.") % input)

    if not 'MASKCATS' in grass.gisenv() and not remove:
        ## beware: next check is made with != , not with 'is', otherwise:
        #>>> grass.raster_info("basin_50K")['datatype'] is "CELL"
        #False
        # even if:
        #>>> "CELL" is "CELL"
        #True 
        if grass.raster_info(input)['datatype'] != "CELL":
            grass.fatal(_("Raster map %s must be integer for maskcats parameter") % input)

    mapset = grass.gisenv()['MAPSET']
    exists = bool(grass.find_file('MASK', element = 'cell', mapset = mapset)['file'])

    if remove:
	if exists:
	    grass.run_command('g.remove', rast = 'MASK')
	    grass.message(_("Raster MASK removed"))
 	else:
	    grass.fatal(_("No existing MASK to remove"))
    else:
	if exists:
            if not grass.overwrite():
                grass.fatal(_("MASK already found in current mapset. Delete first or overwrite."))
            else:
                grass.warning(_("MASK already exists and will be overwritten"))

	p = grass.feed_command('r.reclass', input = input, output = 'MASK', overwrite = True, rules = '-')
	p.stdin.write("%s = 1" % maskcats)
	p.stdin.close()
	p.wait()

	if invert:
	    global tmp
	    tmp = "r_mask_%d" % os.getpid()
	    grass.run_command('g.rename', rast = ('MASK',tmp), quiet = True)
	    grass.mapcalc("MASK=if(isnull($tmp),1,null())", tmp = tmp)
	    grass.run_command('g.remove', rast = tmp, quiet = True)
	    grass.message(_("Inverted MASK created."))
	else:
	    grass.message(_("MASK created."))

        grass.message(_("All subsequent raster operations will be limited to MASK area. ") +
		      "Removing or renaming raster file named MASK will " +
		      "restore raster operations to normal")

if __name__ == "__main__":
    options, flags = grass.parser()
    tmp = None
    atexit.register(cleanup)
    main()

