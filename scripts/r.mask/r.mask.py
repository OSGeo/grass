#!/usr/bin/env python
#
############################################################################
#
# MODULE:       r.mask
# AUTHOR(S):	Michael Barton, Arizona State University
#               Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      Facilitates creation of raster MASK using r.reclass
# COPYRIGHT:	(C) 2005, 2007, 2008 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%Module
#%  description: Create a MASK for limiting raster operation
#%  keywords: raster, mask
#%End
#%option
#% key: input
#% type: string
#% gisprompt: old,cell,raster
#% description: Raster map to use as MASK
#% required: no
#%END
#%option
#% key: maskcats
#% type: string
#% description: Category values to use for MASK (format: 1 2 3 thru 7 *)
#% answer: *
#%END
#%flag
#% key: i
#% description: Create inverse MASK from specified 'maskcats' list
#%END
#%flag
#% key: r
#% description: Remove existing MASK (overrides other options)
#%END

import sys
import os
import grass
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
	grass.fatal("Required parameter <input> not set")

    mapset = grass.gisenv()['MAPSET']
    exists = bool(grass.find_file('MASK', element = 'cell', mapset = mapset)['file'])

    if remove:
	if exists:
	    grass.run_command('g.remove', rast = 'MASK')
	    grass.message("Raster MASK removed")
 	else:
	    grass.warning("No existing MASK to remove")
    else:
	if exists and not grass.overwrite():
	    grass.fatal("MASK already found in current mapset. Delete first or overwrite")

	p = grass.feed_command('r.reclass', input = input, output = 'MASK', overwrite = True)
	p.stdin.write("%s = 1" % maskcats)
	p.stdin.close()
	p.wait()

	if invert:
	    global tmp
	    tmp = "r_mask_%d" % os.getpid()
	    grass.run_command('g.rename', rast = ('MASK',tmp), quiet = True)
	    grass.run_command('r.mapcalc', expr = "MASK=if(isnull(%s),1,null())" % tmp)
	    grass.run_command('g.remove', rast = tmp, quiet = True)
	    grass.message("Inverted MASK created.")
	else:
	    grass.message("MASK created.")

        grass.message("All subsequent raster operations will be limited to MASK area" +
		      "Removing or renaming raster file named MASK will" +
		      "restore raster operations to normal")

if __name__ == "__main__":
    options, flags = grass.parser()
    tmp = None
    atexit.register(cleanup)
    main()

