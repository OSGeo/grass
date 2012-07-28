import os
from   math import ceil

import xml.etree.ElementTree as etree
from urllib2 import urlopen, HTTPError, URLError

import grass.script as grass

class WMSBase:
    def __init__(self):
        # these variables are information for destructor
        self.temp_files_to_cleanup = []
        self.cleanup_mask   = False
        self.cleanup_layers = False
        
        self.bbox     = None
        self.temp_map = None
        
    def __del__(self):
        # removes temporary mask, used for import transparent or warped temp_map
        if self.cleanup_mask:
            # clear temporary mask, which was set by module      
            if grass.run_command('r.mask',
                                 quiet = True,
                                 flags = 'r') != 0:  
                grass.fatal(_('%s failed') % 'r.mask')
            
            # restore original mask, if exists 
            if grass.find_file(self.o_output + self.original_mask_suffix, element = 'cell', mapset = '.' )['name']:
                if grass.run_command('g.copy',
                                     quiet = True,
                                     rast =  self.o_output + self.original_mask_suffix + ',MASK') != 0:
                    grass.fatal(_('%s failed') % 'g.copy')
        
        # tries to remove temporary files, all files should be
        # removoved before, implemented just in case of unexpected
        # stop of module
        for temp_file in self.temp_files_to_cleanup:
            grass.try_remove(temp_file)
        
        # remove temporary created rasters
        if self.cleanup_layers: 
            maps = []
            for suffix in ('.red', '.green', '.blue', '.alpha', self.original_mask_suffix):
                rast = self.o_output + suffix
                if grass.find_file(rast, element = 'cell', mapset = '.')['file']:
                    maps.append(rast)
            
            if maps:
                grass.run_command('g.remove',
                                  quiet = True,
                                  flags = 'f',
                                  rast  = ','.join(maps))
        
        # deletes enviromental variable which overrides region 
        if 'GRASS_REGION' in os.environ.keys():
            os.environ.pop('GRASS_REGION')
        
    def _debug(self, fn, msg):
        grass.debug("%s.%s: %s" %
                    (self.__class__.__name__, fn, msg))
        
    def _initializeParameters(self, options, flags):
        self._debug("_initialize_parameters", "started")
        
        # inicialization of module parameters (options, flags)
        self.flags = flags 
        if self.flags['o']:
            self.transparent = 'FALSE'
        else:
            self.transparent = 'TRUE'   
        
        self.o_mapserver_url = options['mapserver'].strip() + "?" 
        self.o_layers = options['layers'].strip()
        self.o_styles = options['styles'].strip()
        self.o_output = options['output']
        self.o_method = options['method']
        
        self.o_bgcolor = options['bgcolor'].strip()
        if self.o_bgcolor != "" and not flags["d"]:
            grass.warning(_("Parameter bgcolor ignored, use -d flag"))
        
        self.o_urlparams = options['urlparams'].strip()
        if self.o_urlparams != "" and not flags["d"]:
            grass.warning(_("Parameter urlparams ignored, use -d flag"))
        
        self.o_wms_version = options['wms_version']        
        if self.o_wms_version == "1.3.0":
            self.projection_name = "CRS"
        else:
            self.projection_name = "SRS" 
        
        self.o_format = options['format']
        if self.o_format == "geotiff":
            self.mime_format = "image/geotiff"
        elif self.o_format == "tiff":
            self.mime_format = "image/tiff"
        elif self.o_format == "png":
            self.mime_format = "image/png"
        elif self.o_format == "jpeg":
            self.mime_format = "image/jpeg"
            if flags['o']:
                grass.warning(_("JPEG format does not support transparency"))
        elif self.o_format == "gif":
            self.mime_format = "image/gif"
        else:
            self.mime_format = self.o_format
        
        self.o_srs = int(options['srs'])
        if self.o_srs <= 0:
            grass.fatal(_("Invalid EPSG code %d") % self.o_srs)
        
        # read projection info
        self.proj_location = grass.read_command('g.proj', 
                                                flags ='jf').rstrip('\n')
        
        self.proj_srs = grass.read_command('g.proj', 
                                           flags = 'jf', 
                                           epsg = str(self.o_srs) ).rstrip('\n')
        
        if not self.proj_srs or not self.proj_location:
            grass.fatal(_("Unable to get projection info"))
        
        # set region 
        self.o_region = options['region']
	if self.o_region:                 
            if not grass.find_file(name = self.o_region, element = 'windows', mapset = '.' )['name']:
                grass.fatal(_("Region <%s> not found") % self.o_region)
        
        if self.o_region:
            s = grass.read_command('g.region',
                                   quiet = True,
                                   flags = 'ug',
                                   region = self.o_region)
            self.region = grass.parse_key_val(s, val_type = float)
        else:
            self.region = grass.region()
        
        min_tile_size = 100
        self.o_maxcols = int(options['maxcols'])
        if self.o_maxcols <= min_tile_size:
            grass.fatal(_("Maxcols must be greater than 100"))
        
        self.o_maxrows = int(options['maxrows'])
        if self.o_maxrows <= min_tile_size:
            grass.fatal(_("Maxrows must be greater than 100"))
        
        # setting optimal tile size according to maxcols and maxrows constraint and region cols and rows      
        self.tile_cols = int(self.region['cols'] / ceil(self.region['cols'] / float(self.o_maxcols)))
        self.tile_rows = int(self.region['rows'] / ceil(self.region['rows'] / float(self.o_maxrows)))
        
        # suffix for existing mask (during overriding will be saved
        # into raster named:self.o_output + this suffix)
        self.original_mask_suffix = "_temp_MASK"
        
        # check names of temporary rasters, which module may create 
        maps = []
        for suffix in ('.red', '.green', '.blue', '.alpha', self.original_mask_suffix ):
            rast = self.o_output + suffix
            if grass.find_file(rast, element = 'cell', mapset = '.')['file']:
                maps.append(rast)
        
        if len(maps) != 0:
            grass.fatal(_("Please change output name, or change names of these rasters: %s, "
                          "module needs to create this temporary maps during runing") % ",".join(maps))
        
        # default format for GDAL library
        self.gdal_drv_format = "GTiff"
        
        self._debug("_initialize_parameters", "finished")

    def GetMap(self, options, flags):
        """!Download data from WMS server and import data
        (using GDAL library) into GRASS as a raster map."""

        self._initializeParameters(options, flags)  

        self.bbox     = self._computeBbox()
        
        self.temp_map = self._download()  
        
        self._createOutputMap() 
    
    def GetCapabilities(self, options): 
        """!Get capabilities from WMS server
        """
        # download capabilities file
        cap_url = options['mapserver'] + "?service=WMS&request=GetCapabilities&version=" + options['wms_version'] 
        try:
            cap = urlopen(cap_url)
        except IOError:
            grass.fatal(_("Unable to get capabilities from '%s'") % options['mapserver'])
        
        cap_lines = cap.readlines()
        for line in cap_lines: 
            print line 
        
    def _computeBbox(self):
        """!Get region extent for WMS query (bbox)
        """
        self._debug("_computeBbox", "started")
        
        bbox_region_items = {'maxy' : 'n', 'miny' : 's', 'maxx' : 'e', 'minx' : 'w'}  
        bbox = {}

        if self.proj_srs == self.proj_location: # TODO: do it better
            for bbox_item, region_item in bbox_region_items.iteritems():
                bbox[bbox_item] = self.region[region_item]
        
        # if location projection and wms query projection are
        # different, corner points of region are transformed into wms
        # projection and then bbox is created from extreme coordinates
        # of the transformed points
        else:
            for bbox_item, region_item  in bbox_region_items.iteritems():
                bbox[bbox_item] = None

            temp_region = self._tempfile()
            
            try:
                temp_region_opened = open(temp_region, 'w')
                temp_region_opened.write("%f %f\n%f %f\n%f %f\n%f %f\n"  %\
                                       (self.region['e'], self.region['n'],\
                                        self.region['w'], self.region['n'],\
                                        self.region['w'], self.region['s'],\
                                        self.region['e'], self.region['s'] ))
            except IOError:
                 grass.fatal(_("Unable to write data into tempfile"))
            finally:           
                temp_region_opened.close()            
            
            points = grass.read_command('m.proj', flags = 'd',
                                        proj_output = self.proj_srs,
                                        proj_input = self.proj_location,
                                        input = temp_region) # TODO: stdin
            grass.try_remove(temp_region)
            if not points:
                grass.fatal(_("Unable to determine region, %s failed") % 'm.proj')
            
            points = points.splitlines()
            if len(points) != 4:
                grass.fatal(_("Region defintion: 4 points required"))
            
            for point in points:
                point = map(float, point.split("|"))
                if not bbox['maxy']:
                    bbox['maxy'] = point[1]
                    bbox['miny'] = point[1]
                    bbox['maxx'] = point[0]
                    bbox['minx'] = point[0]
                    continue
                
                if   bbox['maxy'] < point[1]:
                    bbox['maxy'] = point[1]
                elif bbox['miny'] > point[1]:
                    bbox['miny'] = point[1]
                
                if   bbox['maxx'] < point[0]:
                    bbox['maxx'] = point[0]
                elif bbox['minx'] > point[0]:
                    bbox['minx'] = point[0]  
        
        self._debug("_computeBbox", "finished -> %s" % bbox)

        # Ordering of coordinates axis of geographic coordinate
        # systems in WMS 1.3.0 is fliped. If self.flip_coords is 
        # True, coords in bbox need to be flipped in WMS query.

        self.flip_coords = False  
        hasLongLat = self.proj_srs.find("+proj=longlat")   
        hasLatLong = self.proj_srs.find("+proj=latlong")   

        if (hasLongLat != -1 or hasLatLong != -1) and self.o_wms_version == "1.3.0":
            self.flip_coords = True

        return bbox

    def _createOutputMap(self): 
        """!Import downloaded data into GRASS, reproject data if needed
        using gdalwarp
        """
        # reprojection of raster
        if self.proj_srs != self.proj_location: # TODO: do it better
            grass.message(_("Reprojecting raster..."))
            temp_warpmap = self._tempfile()
            
            if int(os.getenv('GRASS_VERBOSE', '2')) <= 2:
                nuldev = file(os.devnull, 'w+')
            else:
                nuldev = None
            
            # RGB rasters - alpha layer is added for cropping edges of projected raster
            if self.temp_map_bands_num == 3:
                ps = grass.Popen(['gdalwarp',
                                  '-s_srs', '%s' % self.proj_srs,
                                  '-t_srs', '%s' % self.proj_location,
                                  '-r', self.o_method, '-dstalpha',
                                  self.temp_map, temp_warpmap], stdout = nuldev)
            # RGBA rasters
            else:
                ps = grass.Popen(['gdalwarp',
                                  '-s_srs', '%s' % self.proj_srs,
                                  '-t_srs', '%s' % self.proj_location,
                                  '-r', self.o_method,
                                  self.temp_map, temp_warpmap], stdout = nuldev)
            ps.wait()
            
            if nuldev:
                nuldev.close()
            
            if ps.returncode != 0:
                grass.fatal(_('%s failed') % 'gdalwarp')
        # raster projection is same as projection of location
        else:
            temp_warpmap = self.temp_map
        
        grass.message(_("Importing raster map into GRASS..."))
        # importing temp_map into GRASS
        if grass.run_command('r.in.gdal',
                             quiet = True,
                             input = temp_warpmap,
                             output = self.o_output) != 0:
            grass.fatal(_('%s failed') % 'r.in.gdal')
        
        # information for destructor to cleanup temp_layers, created
        # with r.in.gdal
        self.cleanup_layers = True
        
        # setting region for full extend of imported raster
        os.environ['GRASS_REGION'] = grass.region_env(rast = self.o_output + '.red')
        
        # mask created from alpha layer, which describes real extend
        # of warped layer (may not be a rectangle), also mask contains
        # transparent parts of raster
        if grass.find_file( self.o_output + '.alpha', element = 'cell', mapset = '.' )['name']:
            # saving current mask (if exists) into temp raster
            if grass.find_file('MASK', element = 'cell', mapset = '.' )['name']:
                if grass.run_command('g.copy',
                                     quiet = True,
                                     rast = 'MASK,' + self.o_output + self.original_mask_suffix) != 0:    
                    grass.fatal(_('%s failed') % 'g.copy')
            
            # info for destructor
            self.cleanup_mask = True
            if grass.run_command('r.mask',
                                 quiet = True,
                                 overwrite = True,
                                 maskcats = "0",
                                 flags = 'i',
                                 input = self.o_output + '.alpha') != 0: 
                grass.fatal(_('%s failed') % 'r.mask')
        
        if grass.run_command('r.composite',
                             quiet = True,
                             red = self.o_output + '.red',
                             green = self.o_output +  '.green',
                             blue = self.o_output + '.blue',
                             output = self.o_output ) != 0:
                grass.fatal(_('%s failed') % 'r.composite')
        
        grass.try_remove(temp_warpmap)
        grass.try_remove(self.temp_map) 

    def _flipBbox(self, bbox):
        """ 
        flips items in dictionary 
        value flips between this keys:
        maxy -> maxx
        maxx -> maxy
        miny -> minx
        minx -> miny
        @return copy of bbox with fliped cordinates
        """  
        temp_bbox = dict(bbox)
        new_bbox = {}
        new_bbox['maxy'] = temp_bbox['maxx']
        new_bbox['miny'] = temp_bbox['minx']
        new_bbox['maxx'] = temp_bbox['maxy']
        new_bbox['minx'] = temp_bbox['miny']

        return new_bbox

    def _tempfile(self):
        """!Create temp_file and append list self.temp_files_to_cleanup 
            with path of file 
     
        @return string path to temp_file
        """
        temp_file = grass.tempfile()
        if temp_file is None:
            grass.fatal(_("Unable to create temporary files"))
        
        # list of created tempfiles for destructor
        self.temp_files_to_cleanup.append(temp_file)
        
        return temp_file
