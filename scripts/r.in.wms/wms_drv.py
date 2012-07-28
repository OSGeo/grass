try:
    from osgeo import gdal
    from osgeo import gdalconst 
except:
    grass.fatal(_("Unable to load GDAL python bindings"))

from urllib2 import urlopen

import numpy as Numeric
Numeric.arrayrange = Numeric.arange

import grass.script as grass

from wms_base import WMSBase

class WMSDrv(WMSBase):
    def _download(self):
        """!Downloads data from WMS server using own driver
        
        @return temp_map with stored downloaded data
        """ 
        grass.message(_("Downloading data from WMS server..."))
        

        proj = self.projection_name + "=EPSG:"+ str(self.o_srs)
        url = self.o_mapserver_url + "REQUEST=GetMap&VERSION=%s&LAYERS=%s&WIDTH=%s&HEIGHT=%s&STYLES=%s&BGCOLOR=%s&TRANSPARENT=%s" %\
                  (self.o_wms_version, self.o_layers, self.tile_cols, self.tile_rows, self.o_styles, self.o_bgcolor, self.transparent)
        url += "&" +proj+ "&" + "FORMAT=" + self.mime_format
        
        if self.o_urlparams != "":
            url +="&" + self.o_urlparams
        
        cols = int(self.region['cols'])
        rows = int(self.region['rows'])
        
        # computes parameters of tiles 
        num_tiles_x = cols / self.tile_cols 
        last_tile_x_size = cols % self.tile_cols
        tile_x_length =  float(self.tile_cols) / float(cols ) * (self.bbox['maxx'] - self.bbox['minx']) 
        
        last_tile_x = False
        if last_tile_x_size != 0:
            last_tile_x = True
            num_tiles_x = num_tiles_x + 1
        
        num_tiles_y = rows / self.tile_rows 
        last_tile_y_size = rows % self.tile_rows
        tile_y_length =  float(self.tile_rows) / float(rows) * (self.bbox['maxy'] - self.bbox['miny']) 
        
        last_tile_y = False
        if last_tile_y_size != 0:
            last_tile_y = True
            num_tiles_y = num_tiles_y + 1
        
        # each tile is downloaded and written into temp_map 
        tile_bbox = dict(self.bbox)
        tile_bbox['maxx'] = self.bbox['minx']  + tile_x_length
        
        tile_to_temp_map_size_x = self.tile_cols
        for i_x in range(num_tiles_x):
            # set bbox for tile i_x,i_y (E, W)
            if i_x != 0:
                tile_bbox['maxx'] += tile_x_length 
                tile_bbox['minx'] += tile_x_length            
            
            if i_x == num_tiles_x - 1 and last_tile_x:
                tile_to_temp_map_size_x = last_tile_x_size 
            
            tile_bbox['maxy'] = self.bbox['maxy']                    
            tile_bbox['miny'] = self.bbox['maxy'] - tile_y_length 
            tile_to_temp_map_size_y = self.tile_rows       
            
            for i_y in range(num_tiles_y):
                # set bbox for tile i_x,i_y (N, S)
                if i_y != 0:
                    tile_bbox['miny'] -= tile_y_length 
                    tile_bbox['maxy'] -= tile_y_length
                
                if i_y == num_tiles_y - 1 and last_tile_y:
                    tile_to_temp_map_size_y = last_tile_y_size 
                
                if self.flip_coords:
                    # flips coordinates if WMS strandard is 1.3.0 and 
                    # projection is geographic (see:wms_base.py _computeBbox)
                    query_bbox = dict(self._flipBbox(tile_bbox))
                else:
                    query_bbox = tile_bbox

                query_url = url + "&" + "BBOX=%s,%s,%s,%s" % ( query_bbox['minx'],  query_bbox['miny'],  query_bbox['maxx'],  query_bbox['maxy'])
                grass.debug(query_url)
                try: 
                    wms_data = urlopen(query_url)
                except IOError:
                    grass.fatal(_("Unable to fetch data from mapserver"))
                
                temp_tile = self._tempfile()
                
                # download data into temporary file
                try:
                    temp_tile_opened = open(temp_tile, 'w')
                    temp_tile_opened.write(wms_data.read())
                except IOError:
                    grass.fatal(_("Unable to write data into tempfile"))
                finally:
                    temp_tile_opened.close()
                
                tile_dataset_info = gdal.Open(temp_tile, gdal.GA_ReadOnly) 
                if tile_dataset_info is None:
                    # print error xml returned from server
                    try:
                        error_xml_opened = open(temp_tile, 'r')
                        err_str = error_xml_opened.read()     
                    except IOError:
                        grass.fatal(_("Unable to read data from tempfile"))
                    finally:
                        error_xml_opened.close()

                    if  err_str is not None:
                        grass.fatal(_("WMS server error: %s") %  err_str)
                    else:
                        grass.fatal(_("WMS server unknown error") )
                
                band = tile_dataset_info.GetRasterBand(1) 
                cell_type_func = band.__swig_getmethods__["DataType"]#??
                bands_number_func = tile_dataset_info.__swig_getmethods__["RasterCount"]
                
                ##### see original r.in.wms - file gdalwarp.py line 117 ####
                temp_tile_pct2rgb = None
                if bands_number_func(tile_dataset_info) == 1 and band.GetRasterColorTable() is not None:
                    # expansion of color table into bands 
                    temp_tile_pct2rgb = self._tempfile()
                    tile_dataset = self._pct2rgb(temp_tile, temp_tile_pct2rgb)
                else: 
                    tile_dataset = tile_dataset_info
                
                # initialization of temp_map_dataset, where all tiles are merged
                if i_x == 0 and i_y == 0:
                    temp_map = self._tempfile()
                    
                    driver = gdal.GetDriverByName(self.gdal_drv_format)
                    metadata = driver.GetMetadata()
                    if not metadata.has_key(gdal.DCAP_CREATE) or \
                           metadata[gdal.DCAP_CREATE] == 'NO':
                        grass.fatal(_('Driver %s does not supports Create() method') % drv_format)  
                    
                    self.temp_map_bands_num = bands_number_func(tile_dataset)
                    temp_map_dataset = driver.Create(temp_map, int(cols), int(rows),
                                                     self.temp_map_bands_num, cell_type_func(band));
                
                # tile written into temp_map
                tile_to_temp_map = tile_dataset.ReadRaster(0, 0, tile_to_temp_map_size_x, tile_to_temp_map_size_y,
                                                                 tile_to_temp_map_size_x, tile_to_temp_map_size_y)
                
                temp_map_dataset.WriteRaster(self.tile_cols * i_x, self.tile_rows * i_y,
                                             tile_to_temp_map_size_x,  tile_to_temp_map_size_y, tile_to_temp_map) 
                
                tile_dataset = None
                tile_dataset_info = None
                grass.try_remove(temp_tile)
                grass.try_remove(temp_tile_pct2rgb)    
        
        # georeferencing and setting projection of temp_map
        projection = grass.read_command('g.proj', 
                                        flags = 'wf',
                                        epsg =self.o_srs).rstrip('\n')
        temp_map_dataset.SetProjection(projection)
        


        pixel_x_length = (self.bbox['maxx'] - self.bbox['minx']) / int(cols)
        pixel_y_length = (self.bbox['miny'] - self.bbox['maxy']) / int(rows)
        geo_transform = [ self.bbox['minx'] , pixel_x_length  , 0.0 ,  self.bbox['maxy'] , 0.0 , pixel_y_length ] 
        temp_map_dataset.SetGeoTransform(geo_transform )
        temp_map_dataset = None
        
        return temp_map
    
    def _pct2rgb(self, src_filename, dst_filename):
        """!Create new dataset with data in dst_filename with bands according to src_filename 
        raster color table - modified code from gdal utility pct2rgb
        
        @return new dataset
        """  
        out_bands = 4
        band_number = 1
        
        # open source file
        src_ds = gdal.Open(src_filename)
        if src_ds is None:
            grass.fatal(_('Unable to open %s ' % src_filename))
            
        src_band = src_ds.GetRasterBand(band_number)
        
        # Build color table
        lookup = [ Numeric.arrayrange(256), 
                   Numeric.arrayrange(256), 
                   Numeric.arrayrange(256), 
                   Numeric.ones(256)*255 ]
        
        ct = src_band.GetRasterColorTable()	
        if ct is not None:
            for i in range(min(256,ct.GetCount())):
                entry = ct.GetColorEntry(i)
                for c in range(4):
                    lookup[c][i] = entry[c]
        
        # create the working file
        gtiff_driver = gdal.GetDriverByName(self.gdal_drv_format)
        tif_ds = gtiff_driver.Create(dst_filename,
                                     src_ds.RasterXSize, src_ds.RasterYSize, out_bands)
        
        # do the processing one scanline at a time
        for iY in range(src_ds.RasterYSize):
            src_data = src_band.ReadAsArray(0,iY,src_ds.RasterXSize,1)
            
            for iBand in range(out_bands):
                band_lookup = lookup[iBand]
                
                dst_data = Numeric.take(band_lookup,src_data)
                tif_ds.GetRasterBand(iBand+1).WriteArray(dst_data,0,iY)
        
        return tif_ds       
