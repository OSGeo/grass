#!/usr/bin/env python
#
############################################################################
#
# MODULE:       r.mask
# AUTHOR(S):	Michael Barton, Arizona State University
#               Markus Neteler
#               Converted to Python by Glynn Clements
#               Markus Metz
# PURPOSE:      Facilitates creation of raster MASK
# COPYRIGHT:	(C) 2005-2012 by the GRASS Development Team
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
#% overwrite: yes
#%end
#%option G_OPT_R_INPUT
#% key: raster
#% description: Name of raster map to use as mask
#% required: NO
#% guisection: Raster
#%end
#%option
#% key: maskcats
#% type: string
#% description: Raster values to use for mask (format: 1 2 3 thru 7 *)
#% answer: *
#% guisection: Raster
#%end
#%option G_OPT_V_INPUT
#% key: vector
#% label: Name of vector map to use as mask
#% required: NO
#% guisection: Vector
#%end
#%option G_OPT_V_FIELD
#% label: Layer number or name (vector)
#% required: NO
#% guisection: Vector
#%end
#%option G_OPT_V_CATS
#% label: Category values (vector)
#% guisection: Vector
#%end
#%option G_OPT_DB_WHERE
#% label: WHERE conditions of SQL statement without 'where' keyword (vector)
#% guisection: Vector
#%end
#%flag
#% key: i
#% description: Create inverse mask
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
    if tmp_hull:
        grass.run_command('g.remove', vect = tmp_hull, quiet = True)

def main():
    raster = options['raster']
    maskcats = options['maskcats']
    vector = options['vector']
    layer = options['layer']
    cats = options['cats']
    where = options['where']
    remove = flags['r']
    invert = flags['i']

    if not remove and not raster and not vector:
        grass.fatal(_("Either parameter <raster> ot parameter <vector> is required"))

    mapset = grass.gisenv()['MAPSET']
    exists = bool(grass.find_file('MASK', element = 'cell', mapset = mapset)['file'])

    if remove:
        # -> remove
        if exists:
            grass.run_command('g.remove', quiet = True, rast = 'MASK')
            grass.message(_("Raster MASK removed"))
        else:
            grass.fatal(_("No existing MASK to remove"))
    else:
        # -> create
        if exists:
            if not grass.overwrite():
                grass.fatal(_("MASK already found in current mapset. Delete first or overwrite."))
            else:
                grass.warning(_("MASK already exists and will be overwritten"))
                grass.run_command('g.remove', quiet = True, rast = 'MASK')
        
        if raster:
            # check if input raster exists
            if not grass.find_file(raster)['file']:
                grass.fatal(_("<%s> does not exist.") % raster)

            if maskcats != '*' and not remove:
                if grass.raster_info(raster)['datatype'] != "CELL":
                    grass.fatal(_("Raster map %s must be integer for maskcats parameter") % raster)

            p = grass.feed_command('r.reclass', input = raster, output = 'MASK', overwrite = True, rules = '-')
            p.stdin.write("%s = 1" % maskcats)
            p.stdin.close()
            p.wait()
        elif vector:
            vector_name = grass.find_file(vector, 'vector')['fullname']
            if not vector_name:
                grass.fatal(_("<%s> does not exist.") % vector)
            
            # parser bug?
            if len(cats) == 0:
                cats = None
            if len(where) == 0:
                where = None
            
            if grass.vector_info_topo(vector_name)['areas'] < 1:
                grass.warning(_("No area found in vector map <%s>. "
                                "Creating a convex hull for MASK.") % vector_name)
                global tmp_hull
                tmp_hull = "tmp_hull_%d" % os.getpid()
                to_rast_input = tmp_hull
                # force 'flat' convex hull for 3D vector maps
                if 0 != grass.run_command('v.hull', flags = 'f', quiet = True,
                                          input = vector_name, output = tmp_hull,
                                          layer = layer, cats = cats, where = where):
                    grass.fatal(_("Unable to create a convex hull for vector map <%s>") % vector_name)
            else:
                to_rast_input = vector_name
            
            env = os.environ.copy()
            if grass.verbosity() > 1:
                env['GRASS_VERBOSE'] = '1'
            grass.run_command('v.to.rast', input = to_rast_input, layer = layer,
                              output = 'MASK', use = 'val', val = '1',
                              type = 'area', cats = cats, where = where, env = env)
        
	if invert:
	    global tmp
	    tmp = "r_mask_%d" % os.getpid()
	    grass.run_command('g.rename', rast = ('MASK', tmp), quiet = True)
            grass.message(_("Creating inverted raster MASK..."))
	    grass.mapcalc("MASK=if(isnull($tmp),1,null())", tmp = tmp)
	    grass.verbose(_("Inverted raster MASK created"))
	else:
	    grass.verbose(_("Raster MASK created"))

        grass.message(_("All subsequent raster operations will be limited to MASK area. ") +
                      "Removing or renaming raster map named MASK will " +
                      "restore raster operations to normal.")

if __name__ == "__main__":
    options, flags = grass.parser()
    tmp = tmp_hull = None
    atexit.register(cleanup)
    main()

