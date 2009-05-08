#!/usr/bin/env python

############################################################################
#
# MODULE:       r.in.gdalwarp
#
# AUTHOR(S):    Cedric Shock, 2006
#               Pythonized by Martin Landa <landa.martin gmail.com>, 2009
#
# PURPOSE:      To warp and import data
#               (based on Bash script by Cedric Shock)
#
# COPYRIGHT:    (C) 2009 Martin Landa, and GRASS development team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

#%module
#% description: Warps and imports GDAL supported raster file complete with correct NULL values
#% keywords: raster, rotate, reproject
#%end
#%flag
#% key: p
#% description: Don't reproject the data, just patch it
#%end
#%flag
#% key: e
#% description: Extend location extents based on new dataset
#%end
#%flag
#% key: c
#% description: Make color composite image if possible
#%end
#%flag
#% key: k
#% description: Keep band numbers instead of using band color names
#%end
#%option
#% key: input
#% type: string
#% label: Raster file or files to be imported
#% description: If multiple files are specified they will be patched together.
#% multiple: yes
#% required : yes
#%end
#%option
#% key: output
#% type: string
#% description: Name for resultant raster map. Each band will be name output.bandname
#% required : yes
#%end
#%option
#% key: s_srs
#% type: string
#% description: Source projection in gdalwarp format
#% required : yes
#%end
#%option
#% key: method
#% type: string
#% description: Reprojection method to use
#% options:nearest,bilinear,cubic,cubicspline
#% answer:nearest
#% required: yes
#%end
#%option
#% key: warpoptions
#% type: string
#% description: Additional options for gdalwarp
#% required : no
#%end

import os
import sys
import atexit

import grass

def cleanup():
    if tmp:
	grass.run_command('g.remove', rast = tmp, quiet = True)

def warp_import(file, map, method,
                suffixes, tiler, patches):
    """Wrap raster file using gdalwarp before importing into GRASS"""
    global tmp
    warpfile = tmp + 'warped.geotiff'
    tmpmapname = map + '_tmp'

    t_srs = grass.read_command('g.proj',
                               quiet = True,
                               flags = 'wf')
    if not t_srs:
        grass.fatal('g.proj failed')
    
    grass.debug('gdalwarp -s_srs "%s" -t_srs "%s" "%s" "%s" %s %s' % \
                    (options['s_srs'], t_srs, file,
                     warpfile, options['warpoptions'], method))

    grass.info("Warping input file '%s'..." % file)
    ps = subprocess.Popen('gdalwarp',
                          '-s_srs', options['s_srs'],
                          '-t_srs', t_srs,
                          file, warpfile, options['warpoptions'], method)
    if ps.wait() != 0:
        grass.fatal('gdalwarp failed')
    
    # import it into a temporary map
    grass.info('Importing temporary raster map...')
    if grass.run_command('r.in.gdal',
                         quiet = True,
                         flags = gdal_flags,
                         input = warpfile,
                         output = tmpmapname) != 0:
        grass.fatal('r.in.gdal failed')
    
    os.remove(warpfile)

    # get list of channels
    pattern = tmpmapfile + '*'
    grass.debug('Pattern: %s' % pattern)
    mapset = grass.gisenv()['MAPSET']
    channel_list = grass.mlist_grouped(type = 'rast', pattern = pattern, mapset = mapset)
    grass.debug('Channel list: %s' % ','.join(channel_list))

    if len(channel_list) < 2: # test for single band data
        channel_suffixes = []
    else:
        channel_suffixes = channel_list # ???

    grass.debug('Channel suffixes: %s', ','.join(channel_suffixes))

    # add to the list of all suffixes
    suffixes = suffixes + channel_suffixes
    suffixes.sort()

    # get last suffix
    if len(channel_suffixes) > 0:
        last_suffix = channel_suffixes[-1]
    else:
        last_suffix = ''

    # find the alpha layer
    if flags['k']:
        alphalayer = tmpmapname + last_suffix
    else:
        alphalayer = tmpmapname + '.alpha'

    # test to see if the alpha map exists
    if not grass.find_file(element = 'cell', file = alphalayer)['name']:
        alphalayer = ''
        
    # calculate the new maps:
    for suffix in channel_suffixes:
        grass.debug("alpha=%s MAPsfx=%s% tmpname=%s%s" % \
                        (alphalayer, map, suffix, tmpmapname, suffix))
        if alphalayer:
            # Use alpha channel for nulls: problem: I've seen a map
            # where alpha was 1-255; 1 being transparent. what to do?
            # (Geosci Australia Gold layer, format=tiff)
            if grass.run_command('r.mapcalc',
                                 quiet = True,
                                 expression = "%s%s = if(%s, %s%s, null())" % \
                                     (map, sfx, alphalayer, tmpmapname, sfx)) != 0:
                grass.fatal('r.mapcalc failed')
        else:
            if grass.run_command('g.copy',
                                 quiet = True,
                                 rast = "%s%s,%s%s" % \
                                     (tmpmapname, suffix, map, suffix)) != 0:
                grass.fatal('g.copy failed')
        
        # copy the color tables
        if grass.run_command('r.colors',
                             quiet = True,
                             map = map + suffix,
                             rast = tmpmapname + suffix) != 0:
            grass.fatal('g.copy failed')

        # make patch lists
        suffix = suffix.replace('.', '_')
        # this is a hack to make the patch lists empty:
        if tiler == 0:
            patches = []
        patches = patches.append(map + suffix)
    
    # if no suffix, processing is simple (e.g. elevation has only 1
    # band)
    if len(channel_list) < 1:
        # run r.mapcalc to crop to region
        if grass.run_command('r.mapcalc',
                             quiet = True,
                             expression = "%s = %s" % \
                                 (map, tmpmapname)) != 0:
            grass.fatal('r.mapcalc failed')
            
        if grass.run_command('r.colors',
                             quiet = True,
                             map = map,
                             rast = tmpmapname) != 0:
            grass.fatal('r.colors failed')
    
    # remove the old channels
    if grass.run_command('g.remove',
                         quiet = True,
                         rast = ','.channel_list) != 0:
        grass.fatal('g.remove failed')
    
def nowarp_import(file, map, gdal_flags,
                  suffixes, tiler, patches):
    if grass.run_command('r.in.gdal',
                         quiet = True,
                         flags = 'o' + gdal_flags,
                         input = file,
                         output = map) != 0:
        grass.fatal('r.in.gdal failed')

    # get a list of channels:
    pattern = map + '*'
    grass.debug("pattern: %s" % ','.join(pattern))
    mapset = grass.gisenv()['MAPSET']
    channel_list = grass.mlist_grouped(type = 'rast', pattern = pattern, mapset = mapset)
    grass.debug("channel list: %s" % ','.join(channel_list))

    if len(channel_list) < 2:
        # test for single band data
        channel_suffixes = []
    else:
        channel_suffixes = channel_list # ???
    
    # add to the list of all suffixes:
    suffixes = suffixes + channel_suffixes
    suffixes.sort()
    
    for suffix in channel_suffixes:
        # make patch lists
        suffix = suffix.replace('.', '_')
        # this is a hack to make the patch lists empty
        if tiler == 0:
            patches = []
        patches = patches.append(map + suffix)
    
def main():
    if not flags['p']:
        # todo: check if gdalwarp is available
        pass
    
    # flags for r.in.gdal
    gdal_flags = ''
    if flags['e']:
        gdal_flags += 'e'
    if flags['k']:
        gdal_flags += 'k'
    
    # options for gdalwarp
    method_opt = options['method']
    if method_opt == 'nearest':
        gdal_method = '-rn'
    elif method_opt == 'bilinear':
        gdal_method = '-rb'
    elif method_opt == 'cubic':
        gdal_method = '-rc'
    elif method_opt == 'cubicspline':
        gdal_method = '-rcs'

    global tmp
    tmp = grass.tempfile()

    # list of all suffixes
    suffixes = []
    # we need a way to make sure patches are intialized correctly
    tiler = 0
    # list of maps
    maplist = []
    # list of maps to be patched
    patches = []

    # show progress infromation and grass.info() by default
    os.environ['GRASS_VERBOSE'] = '1'
    
    # import tiles
    for input in options['input'].split(','):
        tmptilename = options['output'] + '_tile_' + str(tiler)
        if not os.path.exists(input):
            grass.warning("Missing input '%s'" % input)
            continue
        grass.info('Importing tile <%s>...' % input)
        if flags['p']:
            channel_suffixes = nowarp_import(input, tmptilename, gdal_flags,
                                             suffixes, tiler, patches)
        else:
            warp_import(input, tmptilename, gdal_method,
                        suffixes, tiler, patches)
            
        maplist.append(tmptilename)
        tiler += 1

    if tiler < 1:
        grass.message("Nothing imported")
        return 0
    
    # if there's more than one tile patch them together, otherwise
    # just rename that tile.
    if tiler == 1:
        if len(channel_suffixes) > 0:
            # multi-band
            for suffix in channel_suffixes:
                # rename tile 0 to be the output
                ffile = options['output'] + '_tile_0' + suffix
                tfile = options['output'] + suffix
                if grass.run_command('g.rename',
                                     quiet = True,
                                     rast = ffile + ',' + tfile) != 0:
                    grass.fatal('g.rename failed')
        else: # single-band, single-tile
            ffile = options['output'] + '_tile_0' # + sfx ?
            tfile = options['output'] # + sfx ?
            if grass.run_command('g.rename',
                                 quiet = True,
                                 rast = ffile + ',' + tfile) != 0:
                grass.fatal('g.rename failed')
    else:
        # patch together each channel
        grass.debug('suffixes: %s' % ','.join(suffixes))
        if len(suffixes) > 0:
            # multi-band data
            for suffix in suffixes:
                suffix = suffix.replace('.', '_')
                # patch these together (using nulls only)
		grass.message("Patching '%s' channel..." % suffix)
		if grass.run_command('r.patch',
                                     quiet = True,
                                     input = patches, # ???
                                     output = options['output'] + suffix) != 0:
                    grass.fatal('r.patch failed')
                    
		# remove the temporary patches we made
                if grass.run_command('g.remove',
                                     quiet = True,
                                     rast = patches) != 0:
                    grass.fatal('g.remove failed')
        else:
            # single-band data
	    grass.info("Patching tiles (this may take some time)...")
	    grass.debug("patch list = %s" % ','.join(maplist))

            # HACK: for 8bit PNG, GIF all the different tiles can have
	    #   different color tables so patches output end up all freaky.
	    #	r.mapcalc r#,g#,b# manual patching + r.composite?
	    #	or d.out.file + r.in.png + r.region?
	    # *** there is a GDAL utility to do this: gdal_merge.py
	    #       http://www.gdal.org/gdal_merge.html
            for color in ('r', 'g', 'b'):
                maplist_color = []
                for map in maplist:
                    outmap = map + '_' + color
                    maplist_color.append(outmap)
                    if grass.run_command('r.mapcalc',
                                         quiet = True,
                                         expression = '%s = %s#%s' % (outmap, color, map)) != 0:
                        grass.fatal('r.mapcalc failed')
                    if grass.run_command('r.colors',
                                         quiet = True,
                                         map = outmap,
                                         color = 'grey255') != 0:
                        grass.fatal('r.colors failed')
                    if grass.run_command('r.null',
                                         quiet = True,
                                         map = outmap,
                                         setnull = 255) != 0:
                        grass.fatal('r.null failed')
                
                if grass.run_command('r.patch',
                                     input = ','.join(maplist_color),
                                     output = outmap + '_all') != 0:
                    grass.fatal('r.patch failed')
                
            if grass.run_command('r.composite',
                                 quiet = True,
                                 red = map + '_r_all',
                                 green = map + '_g_all',
                                 blue = map + '_b_all',
                                 output = options['output']) != 0:
                grass.fatal('r.composite failed')

            if grass.run_command('g.mremove',
                                 quiet = True,
                                 flags = 'f',
                                 rast = map + '*') != 0:
                grass.fatal('g.remove failed')

    # there's already a no suffix, can't make colors
    # can only go up from here ;)
    colors = 4
    for suffix in suffixes:
        if suffix in ('.red', '.green', '.blue'):
            colors += 1

    # make a composite image if asked for and colors exist
    if colors == 3 or flags['c']:
        grass.message("Building color image <%s>..." % options['output'])
        if grass.run_command('r.composite',
                             quiet = True,
                             red = options['output'] + '.red',
                             green = options['output'] + '.green',
                             blue = options['output'] + '.blue',
                             output = option['output']) != 0:
            grass.fatal('r.composite failed')
    
if __name__ == "__main__":
    options, flags = grass.parser()
    tmp = None
    atexit.register(cleanup)

    sys.exit(main())
