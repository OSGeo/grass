#!/usr/bin/env python
#
############################################################################
#
# MODULE:       r.fillnulls
# AUTHOR(S):    Markus Neteler
#               Updated to GRASS 5.7 by Michael Barton
#               Updated to GRASS 6.0 by Markus Neteler
#               Ring and zoom improvements by Hamish Bowman
#               Converted to Python by Glynn Clements
#               Add support to v.surf.bspline by Luca Delucchi
#               Per hole filling with RST by Maris Nartiss
# PURPOSE:      fills NULL (no data areas) in raster maps
#               The script respects a user mask (MASK) if present.
#
# COPYRIGHT:    (C) 2001-2012 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################


#%module
#% description: Fills no-data areas in raster maps using spline interpolation.
#% keywords: raster
#% keywords: elevation
#% keywords: interpolation
#%end
#%option G_OPT_R_INPUT
#%end
#%option G_OPT_R_OUTPUT
#%end
#%option
#% key: tension
#% type: double
#% description: Spline tension parameter
#% required : no
#% answer : 40.
#% guisection: RST options
#%end
#%option
#% key: smooth
#% type: double
#% description: Spline smoothing parameter
#% required : no
#% answer : 0.1
#% guisection: RST options
#%end
#%option
#% key: edge
#% type: integer
#% description: Width of hole edge used for interpolation (in cells)
#% required : no
#% answer : 3
#% options : 2-100
#% guisection: RST options
#%end
#%option
#% key: method
#% type: string
#% description: Interpolation method
#% required : yes
#% options : bilinear,bicubic,rst
#% answer : rst
#%end

import sys
import os
import atexit

import grass.script as grass

tmp_rmaps = list()
tmp_vmaps = list()
usermask = None
mapset = None

# what to do in case of user break:
def cleanup():
    #delete internal mask and any TMP files:
    if len(tmp_vmaps) > 0:
        grass.run_command('g.remove', quiet = True, flags = 'f', vect = tmp_vmaps)
    if len(tmp_rmaps) > 0:
        grass.run_command('g.remove', quiet = True, flags = 'f', rast = tmp_rmaps)
    if usermask and mapset:
        if grass.find_file(usermask, mapset = mapset)['file']:
            grass.run_command('g.rename', quiet = True, rast = (usermask, 'MASK'), overwrite = True)

def main():
    global usermask, mapset, tmp_rmaps, tmp_vmaps

    input = options['input']
    output = options['output']
    tension = options['tension']
    smooth = options['smooth']
    method = options['method']
    edge = int(options['edge'])
    quiet = True # FIXME 
    
    mapset = grass.gisenv()['MAPSET']
    unique = str(os.getpid()) # Shouldn't we use temp name?
    prefix = 'r_fillnulls_%s_' % unique
    failed_list = list() # a list of failed holes. Caused by issues with v.surf.rst. Connected with #1813

    #check if input file exists
    if not grass.find_file(input)['file']:
        grass.fatal(_("Map <%s> does not exist.") % input)

    # save original region
    reg_org = grass.region()
    
    # check if a MASK is already present
    # and remove it to not interfere with NULL lookup part
    # as we don't fill MASKed parts!
    if grass.find_file('MASK', mapset = mapset)['file']:
        usermask = "usermask_mask." + unique
        grass.message(_("A user raster mask (MASK) is present. Saving it..."))
        grass.run_command('g.rename', quiet = quiet, rast = ('MASK',usermask))

    #check if method is rst to use v.surf.rst
    if method == 'rst':
        # idea: filter all NULLS and grow that area(s) by 3 pixel, then
        # interpolate from these surrounding 3 pixel edge
        filling = prefix + 'filled'
        
        grass.use_temp_region()
        grass.run_command('g.region', align = input, quiet = quiet)
        region = grass.region()
        ns_res = region['nsres']
        ew_res = region['ewres']
        
        grass.message(_("Using RST interpolation..."))
        grass.message(_("Locating and isolating NULL areas..."))
        
        # creating binary (0/1) map
        if usermask:
            grass.message(_("Skipping masked raster parts"))
            grass.mapcalc("$tmp1 = if(isnull($input) && !($mask == 0 || isnull($mask)),1,null())",
                tmp1 = prefix + 'nulls', input = input, mask = usermask)
        else:
            grass.mapcalc("$tmp1 = if(isnull($input),1,null())",
                tmp1 = prefix + 'nulls', input = input)
        tmp_rmaps.append(prefix + 'nulls')
            
        # restoring user's mask, if present
        # to ignore MASKed original values
        if usermask:
            grass.message(_("Restoring user mask (MASK)..."))
            if grass.run_command('g.rename', quiet = quiet, rast = (usermask, 'MASK')) != 0:
                grass.warning(_("Failed to restore user MASK!"))
            usermask = None
    
        # grow identified holes by X pixels
        grass.message(_("Growing NULL areas"))
        tmp_rmaps.append(prefix + 'grown')
        if grass.run_command('r.grow', input = prefix + 'nulls', radius = edge + 0.01,
                             old = 1, new = 1, out = prefix + 'grown', quiet = quiet) != 0:
            grass.fatal(_("abandoned. Removing temporary map, restoring user mask if needed:"))
        
        # assign unique IDs to each hole or hole system (holes closer than edge distance)
        grass.message(_("Assigning IDs to NULL areas"))
        tmp_rmaps.append(prefix + 'clumped')
        if grass.run_command('r.clump', input = prefix + 'grown', output = prefix + 'clumped', quiet = quiet) != 0:
            grass.fatal(_("abandoned. Removing temporary map, restoring user mask if needed:"))
            
        # use new IDs to identify holes
        grass.mapcalc("$out = if(isnull($inp), null(), $clumped)",
                        out = prefix + 'holes', inp = prefix + 'nulls', clumped = prefix + 'clumped')
        tmp_rmaps.append(prefix + 'holes')
        
        # get a list of unique hole cat's
        cats_file_name = grass.tempfile(True)
        cats_file = file(cats_file_name, 'w')
        grass.run_command('r.stats', flags = 'n', input = prefix + 'holes', stdout = cats_file, quiet = quiet)
        cats_file.close()
        cat_list = list()
        cats_file = file(cats_file_name)
        for line in cats_file:
            cat_list.append(line.rstrip('\n'))
        cats_file.close()
        os.remove(cats_file_name)
        
        if len(cat_list) < 1:
            grass.fatal(_("Input map has no holes. Check region settings."))
        
        # GTC Hole is NULL area in a raster map
        grass.message(_("Processing %d map holes") % len(cat_list))
        first = True
        for cat in cat_list:
            holename = prefix + 'hole_' + cat
            # GTC Hole is a NULL area in a raster map
            grass.message(_("Filling hole %s of %s") % (cat, len(cat_list)))
            
            # cut out only CAT hole for processing
            if grass.run_command('g.region', rast = prefix + 'holes', align = input, quiet = quiet) != 0:
                grass.fatal(_("abandoned. Removing temporary maps, restoring user mask if needed:"))
            grass.mapcalc("$out = if($inp == $catn, $inp, null())",
                            out = prefix + 'tmp_hole', inp = prefix + 'holes', catn = cat)
            
            # zoom to specific hole to remove rest of data
            if grass.run_command('g.region', zoom = prefix + 'tmp_hole', align = input, quiet = quiet) != 0:
                grass.fatal(_("abandoned. Removing temporary maps, restoring user mask if needed:"))
            
            # increase region a bit to fit edge zone
            if grass.run_command('g.region', align = input, 
                                w = 'w-%d' % (edge * 2 * ew_res), e = 'e+%d' % (edge * 2 * ew_res), 
                                n = 'n+%d' % (edge * 2 * ns_res), s = 's-%d' % (edge * 2 * ns_res),
                                quiet = quiet) != 0:
                grass.fatal(_("abandoned. Removing temporary maps, restoring user mask if needed:"))
            
            # copy only data around hole
            grass.mapcalc("$out = $inp", out = holename, inp = prefix + 'tmp_hole')
            
            # remove temporary map to not overfill disk
            if grass.run_command('g.remove', flags = 'f', rast = prefix + 'tmp_hole', quiet = quiet) != 0:
                grass.fatal(_("abandoned. Removing temporary maps, restoring user mask if needed:"))
            tmp_rmaps.append(holename)
            
            # If here loop is split into two, next part of loop can be run in parallel 
            # (except final result patching)
            # Downside - on large maps such approach causes large disk usage
            
            # grow hole border to get it's edge area
            tmp_rmaps.append(holename + '_grown')
            if grass.run_command('r.grow', input = holename, radius = edge + 0.01,
                             old = -1, out = holename + '_grown', quiet = quiet) != 0:
                grass.fatal(_("abandoned. Removing temporary map, restoring user mask if needed:"))
            
            # no idea why r.grow old=-1 doesn't replace existing values with NULL
            grass.mapcalc("$out = if($inp == -1, null(), $dem)", 
                            out = holename + '_edges', inp = holename + '_grown', dem = input)
            tmp_rmaps.append(holename + '_edges')
            
            # convert to points for interpolation
            tmp_vmaps.append(holename)
            if grass.run_command('r.to.vect', input = holename + '_edges', output = holename,
                                type = 'point', flags = 'z', quiet = quiet) != 0:
                grass.fatal(_("abandoned. Removing temporary maps, restoring user mask if needed:"))
            
            # count number of points to control segmax parameter for interpolation:
            pointsnumber = grass.vector_info_topo(map = holename)['points']
            grass.verbose(_("Interpolating %d points") % pointsnumber)

            if pointsnumber < 2:
                grass.verbose(_("No points to interpolate"))
                failed_list.append(holename)
                continue
            
            grass.message(_("Note: The following warnings about number of points for interpolation may be ignored."))
            
            # set the max number before segmentation
            # npmin and segmax values were choosen by fair guess and thus are superior to all other values ;)
            segmax = 100
            npmin = 300
            if pointsnumber < 600:
                npmin = pointsnumber
                segmax = pointsnumber
            
            # launch v.surf.rst
            tmp_rmaps.append(holename + '_dem')
            if grass.run_command('v.surf.rst', quiet = quiet, input = holename, elev = holename + '_dem',
                                 tension = tension, smooth = smooth, 
                                 segmax = segmax, npmin = npmin) != 0:
                # GTC Hole is NULL area in a raster map
                grass.fatal(_("Failed to fill hole %s") % cat)
            
            # v.surf.rst sometimes fails with exit code 0
            # related bug #1813
            if not grass.find_file(holename + '_dem')['file']:
                try:
                    tmp_rmaps.remove(holename)
                    tmp_rmaps.remove(holename + '_grown')
                    tmp_rmaps.remove(holename + '_edges')
                    tmp_rmaps.remove(holename + '_dem')
                    tmp_vmaps.remove(holename)
                except:
                    pass
                grass.warning(_("Filling has failed silently. Leaving temporary maps with prefix <%s> for debugging.") % holename)
                failed_list.append(holename)
                continue
            
            # append hole result to interpolated version later used to patch into original DEM
            if first:
                tmp_rmaps.append(filling)
                grass.run_command('g.region', align = input, rast = holename + '_dem', quiet = quiet)
                grass.mapcalc("$out = if(isnull($inp), null(), $dem)", 
                                out = filling, inp = holename, dem = holename + '_dem')
                first = False
            else:
                tmp_rmaps.append(filling + '_tmp')
                grass.run_command('g.region', align = input, rast = (filling, holename + '_dem'), quiet = quiet)
                grass.mapcalc("$out = if(isnull($inp), if(isnull($fill), null(), $fill), $dem)", 
                                out = filling + '_tmp', inp = holename, dem = holename + '_dem', fill = filling)
                if grass.run_command('g.rename', rast = (filling + '_tmp', filling), 
                                    overwrite = True, quiet = quiet) != 0:
                    grass.fatal(_("abandoned. Removing temporary maps, restoring user mask if needed:"))
                tmp_rmaps.remove(filling + '_tmp') # this map has been removed. No need for later cleanup.
            
            # remove temporary maps to not overfill disk
            try:
                tmp_rmaps.remove(holename)
                tmp_rmaps.remove(holename + '_grown')
                tmp_rmaps.remove(holename + '_edges')
                tmp_rmaps.remove(holename + '_dem')
            except:
                pass
            if grass.run_command('g.remove', quiet = quiet, flags = 'f', rast = 
                (holename, holename + '_grown', holename + '_edges', holename + '_dem')) != 0:
                grass.fatal(_("abandoned. Removing temporary maps, restoring user mask if needed:"))
            try:
                tmp_vmaps.remove(holename)
            except:
                pass
            if grass.run_command('g.remove', quiet = quiet, flags = 'f', vect = holename) != 0:
                grass.fatal(_("abandoned. Removing temporary maps, restoring user mask if needed:"))
    
    #check if method is different from rst to use r.resamp.bspline
    if method != 'rst':
        grass.message(_("Using %s bspline interpolation") % method)

        # clone current region
        grass.use_temp_region()
        grass.run_command('g.region', align = input)

        reg = grass.region()
        # launch r.resamp.bspline
        tmp_rmaps.append(prefix + 'filled')
        if usermask:
            grass.run_command('r.resamp.bspline', input = input, mask = usermask,
                output = prefix + 'filled', method = method, 
                se = 3 * reg['ewres'], sn = 3 * reg['nsres'], 
                _lambda = 0.01, flags = 'n')
        else:
            grass.run_command('r.resamp.bspline', input = input,
                output = prefix + 'filled', method = method, 
                se = 3 * reg['ewres'], sn = 3 * reg['nsres'], 
                _lambda = 0.01, flags = 'n')

    # restoring user's mask, if present:
    if usermask:
        grass.message(_("Restoring user mask (MASK)..."))
        if grass.run_command('g.rename', quiet = quiet, rast = (usermask, 'MASK')) != 0:
            grass.warning(_("Failed to restore user MASK!"))
        usermask = None

    # set region to original extents, align to input
    grass.run_command('g.region', n = reg_org['n'], s = reg_org['s'], 
                    e = reg_org['e'], w = reg_org['w'], align = input)

    # patch orig and fill map
    grass.message(_("Patching fill data into NULL areas..."))
    # we can use --o here as g.parser already checks on startup
    grass.run_command('r.patch', input = (input,prefix + 'filled'), output = output, overwrite = True)

    # restore the real region
    grass.del_temp_region()

    grass.message(_("Filled raster map is: %s") % output)

    # write cmd history:
    grass.raster_history(output)
    
    if len(failed_list) > 0:
        grass.warning(_("Following holes where not filled. Temporary maps with are left in place to allow examination of unfilled holes"))
        outlist = failed_list[0]
        for hole in failed_list[1:]:
            outlist = ', ' + outlist
        grass.message(outlist)

    grass.message(_("Done."))

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
