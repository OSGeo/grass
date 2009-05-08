############################################################################
#
# MODULE:       r.in.wms / wms_gdalwarp.py
#
# AUTHOR(S):    Cedric Shock, 2006
#               Update for GRASS 7 by Martin Landa <landa.martin gmail.com>, 2009
#
# PURPOSE:      To import data from web mapping servers
#               (based on Bash script by Cedric Shock)
#
# COPYRIGHT:    (C) 2009 Martin Landa, and GRASS development team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

import os
import subprocess

import grass

class GDALWarp():
    def __init__(self, flags, options):
        self.flags   = flags
        self.options = options
    
        self.tmp = grass.tempfile()
        
        self.suffixes = []
        self.patches  = []
        self.maplist  = []
        self.tiler = 0
    
        if not flags['p']:
            # todo: check if gdalwarp is available
            pass
    
        # flags for r.in.gdal
        self.gdal_flags = ''
        if flags['e']:
            self.gdal_flags += 'e'
        if flags['k']:
            self.gdal_flags += 'k'
    
        # options for gdalwarp
        method_opt = options['method']
        if method_opt == 'nearest':
            self.gdal_method = '-rn'
        elif method_opt == 'bilinear':
            self.gdal_method = '-rb'
        elif method_opt == 'cubic':
            self.gdal_method = '-rc'
        elif method_opt == 'cubicspline':
            self.gdal_method = '-rcs'

    def run(self):
        # import tiles
        tiler = 0
        for input in self.options['input'].split(','):
            tmptilename = self.options['output'] + '_tile_' + str(tiler)
            if not os.path.exists(input):
                grass.warning("Missing input '%s'" % input)
                continue
            grass.info('Importing tile <%s>...' % os.path.basename(input))
            if self.flags['p']:
                self.nowarp_import(input, tmptilename, self.gdal_flags)
            else:
                self.warp_import(input, tmptilename, self.gdal_method)
            
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
                    ffile = self.options['output'] + '_tile_0' + suffix
                    tfile = self.options['output'] + suffix
                    if grass.run_command('g.rename',
                                         quiet = True,
                                         rast = ffile + ',' + tfile) != 0:
                        grass.fatal('g.rename failed')
            else: # single-band, single-tile
                ffile = self.options['output'] + '_tile_0' # + sfx ?
                tfile = self.options['output'] # + sfx ?
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
                                         output = self.options['output'] + suffix) != 0:
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
                                     output = self.options['output']) != 0:
                    grass.fatal('r.composite failed')

                if grass.run_command('g.mremove',
                                     quiet = True,
                                     flags = 'f',
                                     rast = map + '*') != 0:
                    grass.fatal('g.remove failed')

        # there's already a no suffix, can't make colors
        # can only go up from here ;)
        colors = 4 # ???
        for suffix in suffixes:
            if suffix in ('.red', '.green', '.blue'):
                colors += 1

        # make a composite image if asked for and colors exist
        if colors == 3 or self.flags['c']:
            grass.message("Building color image <%s>..." % self.options['output'])
            if grass.run_command('r.composite',
                                 quiet = True,
                                 red = self.options['output'] + '.red',
                                 green = self.options['output'] + '.green',
                                 blue = self.options['output'] + '.blue',
                                 output = option['output']) != 0:
                grass.fatal('r.composite failed')
        
        return 0
    
    def warp_import(self, file, map, method):
        """Wrap raster file using gdalwarp and import wrapped file
        into GRASS"""
        warpfile = self.tmp + 'warped.geotiff'
        tmpmapname = map + '_tmp'

        t_srs = grass.read_command('g.proj',
                                   quiet = True,
                                   flags = 'wf')
        if not t_srs:
            grass.fatal('g.proj failed')
        
        grass.debug('gdalwarp -s_srs "%s" -t_srs "%s" "%s" "%s" %s %s' % \
                        (self.options['srs'], t_srs, file,
                         warpfile, self.options['warpoptions'], method))
        grass.verbose("Warping input file '%s'..." % file)
        ps = subprocess.Popen(['gdalwarp',
                               '-s_srs', self.options['srs'],
                               '-t_srs', t_srs,
                               file, warpfile,
                               self.options['warpoptions'],
                               method])
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
        self.suffixes = self.suffixes + channel_suffixes
        self.suffixes.sort()
        
        # get last suffix
        if len(channel_suffixes) > 0:
            last_suffix = channel_suffixes[-1]
        else:
            last_suffix = ''

        # find the alpha layer
        if self.flags['k']:
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
            if self.tiler == 0:
                self.patches = []
            self.patches = self.patches.append(map + suffix)
    
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
        
    def nowarp_import(self, file, map, gdal_flags):
        """Import raster file into GRASS"""
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
        self.suffixes = self.suffixes + channel.suffixes
        self.suffixes.sort()
    
        for suffix in channel_suffixes:
            # make patch lists
            suffix = suffix.replace('.', '_')
            # this is a hack to make the patch lists empty
            if self.tiler == 0:
                self.patches = []
            self.patches = self.patches.append(map + suffix)
    

