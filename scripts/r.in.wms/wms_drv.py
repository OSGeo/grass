"""!
@brief WMS, WMTS and NASA OnEarth drivers implemented in GRASS using GDAL Python bindings. 

List of classes:
 - wms_drv::WMSDrv
 - wms_drv::BaseRequestMgr
 - wms_drv::WMSRequestMgr
 - wms_drv::WMTSRequestMgr
 - wms_drv::OnEarthRequestMgr

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (Mentor: Martin Landa)
"""

import socket
import grass.script as grass 

from time      import sleep

try:
    from osgeo import gdal
    from osgeo import gdalconst 
except:
    grass.fatal(_("Unable to load GDAL python bindings"))

import numpy as Numeric
Numeric.arrayrange = Numeric.arange

from math import pi, floor
from urllib2 import HTTPError
from httplib import HTTPException
try:
    from xml.etree.ElementTree import ParseError
except ImportError: # < Python 2.7
    from xml.parsers.expat import ExpatError as ParseError
    
from wms_base import WMSBase

from wms_cap_parsers import WMTSCapabilitiesTree, OnEarthCapabilitiesTree

class WMSDrv(WMSBase):
    def _download(self):
        """!Downloads data from WMS server using own driver
        
        @return temp_map with downloaded data
        """ 
        grass.message(_("Downloading data from WMS server..."))

        if "?" in self.params["url"]:
            self.params["url"] += "&"
        else:
            self.params["url"] += "?"

        if not self.params['capfile']:
            self.cap_file = self._fetchCapabilities(self.params)
        else:
            self.cap_file = self.params['capfile']

        # initialize correct manager according to chosen OGC service
        if self.params['driver'] == 'WMTS_GRASS':
            req_mgr = WMTSRequestMgr(self.params, self.bbox, self.region, self.proj_srs, self.cap_file)
        elif self.params['driver'] == 'WMS_GRASS':
            req_mgr = WMSRequestMgr(self.params, self.bbox, self.region, self.tile_size, self.proj_srs)
        elif self.params['driver'] == 'OnEarth_GRASS':
            req_mgr = OnEarthRequestMgr(self.params, self.bbox, self.region, self.proj_srs, self.cap_file)

        # get information about size in pixels and bounding box of raster, where all tiles will be joined
        map_region = req_mgr.GetMapRegion()

        init = True
        temp_map = None

        fetch_try = 0

        # iterate through all tiles and download them
        while True:

            if fetch_try == 0:
                # get url for request the tile and information for placing the tile into raster with other tiles
                tile = req_mgr.GetNextTile()

            # if last tile has been already downloaded 
            if not tile:
                break

            # url for request the tile
            query_url = tile[0]

            # the tile size and offset in pixels for placing it into raster where tiles are joined
            tile_ref = tile[1]
            grass.debug(query_url, 2)
            try: 
                wms_data = self._fetchDataFromServer(query_url, self.params['username'], self.params['password'])
            except (IOError, HTTPException), e:
                if HTTPError == type(e) and e.code == 401:
                    grass.fatal(_("Authorization failed to '%s' when fetching data.") % self.params['url'])
                else:
                    grass.fatal(_("Unable to fetch data from: '%s'") % self.params['url'])

            temp_tile = self._tempfile()
                
            # download data into temporary file
            try:
                temp_tile_opened = open(temp_tile, 'w')
                temp_tile_opened.write(wms_data.read())
            except IOError, e:
                # some servers are not happy with many subsequent requests for tiles done immediately,
                # if immediate request was unsuccessful, try to repeat the request after 5s and 30s breaks
                # TODO probably servers can return more kinds of errors related to this problem (not only 104)
                if socket.error == type(e) and e[0] == 104 and fetch_try < 2:
                    fetch_try += 1

                    if fetch_try == 1:
                        sleep_time = 5
                    elif fetch_try == 2:
                        sleep_time = 30

                    grass.warning(_("Server refused to send data for a tile.\nRequest will be repeated after %d s.") % sleep_time)

                    sleep(sleep_time)
                    continue
                else:
                    grass.fatal(_("Unable to write data into tempfile.\n%s") % str(e))
            finally:
                temp_tile_opened.close()

            fetch_try = 0
                
            tile_dataset_info = gdal.Open(temp_tile, gdal.GA_ReadOnly) 
            if tile_dataset_info is None:
                # print error xml returned from server
                try:
                    error_xml_opened = open(temp_tile, 'r')
                    err_str = error_xml_opened.read()     
                except IOError, e:
                    grass.fatal(_("Unable to read data from tempfile.\n%s") % str(e))
                finally:
                    error_xml_opened.close()

                if  err_str is not None:
                    grass.fatal(_("WMS server error: %s") %  err_str)
                else:
                    grass.fatal(_("WMS server unknown error") )
                
            temp_tile_pct2rgb = None
            if tile_dataset_info.RasterCount == 1 and \
               tile_dataset_info.GetRasterBand(1).GetRasterColorTable() is not None:
                # expansion of color table into bands 
                temp_tile_pct2rgb = self._tempfile()
                tile_dataset = self._pct2rgb(temp_tile, temp_tile_pct2rgb)
            else: 
                tile_dataset = tile_dataset_info
                
            # initialization of temp_map_dataset, where all tiles are merged
            if init:
                temp_map = self._tempfile()
                    
                driver = gdal.GetDriverByName(self.gdal_drv_format)
                metadata = driver.GetMetadata()
                if not metadata.has_key(gdal.DCAP_CREATE) or \
                       metadata[gdal.DCAP_CREATE] == 'NO':
                    grass.fatal(_('Driver %s does not supports Create() method') % drv_format)  
                self.temp_map_bands_num = tile_dataset.RasterCount
                temp_map_dataset = driver.Create(temp_map, map_region['cols'], map_region['rows'],
                                                 self.temp_map_bands_num, 
                                                 tile_dataset.GetRasterBand(1).DataType)
                init = False
                
            # tile is written into temp_map
            tile_to_temp_map = tile_dataset.ReadRaster(0, 0, tile_ref['sizeX'], tile_ref['sizeY'],
                                                             tile_ref['sizeX'], tile_ref['sizeY'])
                
            temp_map_dataset.WriteRaster(tile_ref['t_cols_offset'], tile_ref['t_rows_offset'],
                                         tile_ref['sizeX'],  tile_ref['sizeY'], tile_to_temp_map) 
                
            tile_dataset = None
            tile_dataset_info = None
            grass.try_remove(temp_tile)
            grass.try_remove(temp_tile_pct2rgb)    

        if not temp_map:
            return temp_map
        # georeferencing and setting projection of temp_map
        projection = grass.read_command('g.proj', 
                                        flags = 'wf',
                                        epsg =self.params['srs']).rstrip('\n')
        temp_map_dataset.SetProjection(projection)

        pixel_x_length = (map_region['maxx'] - map_region['minx']) / int(map_region['cols'])
        pixel_y_length = (map_region['miny'] - map_region['maxy']) / int(map_region['rows'])

        geo_transform = [map_region['minx'] , pixel_x_length  , 0.0 , map_region['maxy'] , 0.0 , pixel_y_length] 
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

class BaseRequestMgr:
    """!Base class for request managers. 
    """
    def _computeRequestData(self, bbox, tl_corner, tile_span, tile_size, mat_num_bbox):
        """!Initialize data needed for iteration through tiles. Used by WMTS_GRASS and OnEarth_GRASS drivers.
        """ 
        epsilon = 1e-15

        # request data bbox specified in row and col number 
        self.t_num_bbox = {}

        self.t_num_bbox['min_col'] = int(floor((bbox['minx'] - tl_corner['minx']) / tile_span['x'] + epsilon))
        self.t_num_bbox['max_col'] = int(floor((bbox['maxx'] - tl_corner['minx']) / tile_span['x'] - epsilon))

        self.t_num_bbox['min_row'] = int(floor((tl_corner['maxy'] - bbox['maxy']) / tile_span['y'] + epsilon))
        self.t_num_bbox['max_row'] = int(floor((tl_corner['maxy'] - bbox['miny']) / tile_span['y'] - epsilon))


        # Does required bbox intersects bbox of data available on server?
        self.intersects = False
        for col in ['min_col', 'max_col']:
            for row in ['min_row', 'max_row']:
                if (self.t_num_bbox['min_row'] <= self.t_num_bbox[row] and self.t_num_bbox[row] <= mat_num_bbox['max_row']) and \
                   (self.t_num_bbox['min_col'] <= self.t_num_bbox[col] and self.t_num_bbox[col] <= mat_num_bbox['max_col']):
                    self.intersects = True
                    
        if not self.intersects:
            grass.warning(_('Region is out of server data extend.'))
            self.map_region = None
            return

        # crop request bbox to server data bbox extend 
        if self.t_num_bbox['min_col'] < (mat_num_bbox['min_col']):
            self.t_num_bbox['min_col']  = int(mat_num_bbox['min_col'])

        if self.t_num_bbox['max_col'] > (mat_num_bbox['max_col']):
            self.t_num_bbox['max_col']  = int(mat_num_bbox['max_col'])

        if self.t_num_bbox['min_row'] < (mat_num_bbox['min_row']):
            self.t_num_bbox['min_row'] = int(mat_num_bbox['min_row']) 

        if self.t_num_bbox['max_row'] > (mat_num_bbox['max_row']):
            self.t_num_bbox['max_row'] = int(mat_num_bbox['max_row']) 

        num_tiles = (self.t_num_bbox['max_col'] - self.t_num_bbox['min_col'] + 1) * (self.t_num_bbox['max_row'] - self.t_num_bbox['min_row'] + 1) 
        grass.message(_('Fetching %d tiles with %d x %d pixel size per tile...') % (num_tiles, tile_size['x'], tile_size['y']))

        # georeference of raster, where tiles will be merged
        self.map_region = {}
        self.map_region['minx'] = self.t_num_bbox['min_col'] * tile_span['x'] + tl_corner['minx']
        self.map_region['maxy'] = tl_corner['maxy'] - (self.t_num_bbox['min_row']) * tile_span['y']

        self.map_region['maxx'] = (self.t_num_bbox['max_col'] + 1) * tile_span['x'] + tl_corner['minx']
        self.map_region['miny'] = tl_corner['maxy'] - (self.t_num_bbox['max_row'] + 1) * tile_span['y']

        # size of raster, where tiles will be merged
        self.map_region['cols'] = int(tile_size['x'] * (self.t_num_bbox['max_col'] -  self.t_num_bbox['min_col'] + 1))
        self.map_region['rows'] = int(tile_size['y'] * (self.t_num_bbox['max_row'] -  self.t_num_bbox['min_row'] + 1))

        # hold information about current column and row during iteration 
        self.i_col = self.t_num_bbox['min_col']
        self.i_row = self.t_num_bbox['min_row'] 

        # bbox for first tile request
        self.query_bbox = { 
                            'minx' : tl_corner['minx'],
                            'maxy' : tl_corner['maxy'],
                            'maxx' : tl_corner['minx'] + tile_span['x'],
                            'miny' : tl_corner['maxy'] - tile_span['y'],
                          }

        self.tile_ref = {
                          'sizeX' : tile_size['x'],
                          'sizeY' : tile_size['y']
                        }

    def _isGeoProj(self, proj):
        """!Is it geographic projection? 
        """       
        if (proj.find("+proj=latlong")  != -1 or \
            proj.find("+proj=longlat")  != -1):

            return True
        return False

class WMSRequestMgr(BaseRequestMgr):
    def __init__(self, params, bbox, region, tile_size, proj_srs, cap_file = None):
        """!Initialize data needed for iteration through tiles.
        """
        self.version = params['wms_version']
        proj = params['proj_name'] + "=EPSG:"+ str(params['srs'])
        self.url = params['url'] + ("SERVICE=WMS&REQUEST=GetMap&VERSION=%s&LAYERS=%s&WIDTH=%s&HEIGHT=%s&STYLES=%s&BGCOLOR=%s&TRANSPARENT=%s" % \
                  (params['wms_version'], params['layers'], tile_size['cols'], tile_size['rows'], params['styles'], \
                   params['bgcolor'], params['transparent']))
        self.url += "&" +proj+ "&" + "FORMAT=" + params['format']

        self.bbox = bbox
        self.proj_srs = proj_srs
        self.tile_rows = tile_size['rows']
        self.tile_cols = tile_size['cols']

        if params['urlparams'] != "":
            self.url += "&" + params['urlparams']
        
        cols = int(region['cols'])
        rows = int(region['rows'])
        
        # computes parameters of tiles 
        self.num_tiles_x = cols / self.tile_cols 
        self.last_tile_x_size = cols % self.tile_cols
        self.tile_length_x =  float(self.tile_cols) / float(cols) * (self.bbox['maxx'] - self.bbox['minx']) 
        
        self.last_tile_x = False
        if self.last_tile_x_size != 0:
            self.last_tile_x = True
            self.num_tiles_x = self.num_tiles_x + 1
        
        self.num_tiles_y = rows / self.tile_rows 
        self.last_tile_y_size = rows % self.tile_rows
        self.tile_length_y =  float(self.tile_rows) / float(rows) * (self.bbox['maxy'] - self.bbox['miny']) 
        
        self.last_tile_y = False
        if self.last_tile_y_size != 0:
            self.last_tile_y = True
            self.num_tiles_y = self.num_tiles_y + 1
        
        self.tile_bbox = dict(self.bbox)
        self.tile_bbox['maxx'] = self.bbox['minx']  + self.tile_length_x

        self.i_x = 0 
        self.i_y = 0

        self.map_region = self.bbox
        self.map_region['cols'] = cols
        self.map_region['rows'] = rows

    def GetMapRegion(self):
        """!Get size in pixels and bounding box of raster where all tiles will be merged.
        """
        return self.map_region

    def GetNextTile(self):
        """!Get url for tile request from server and information for merging the tile with other tiles
        """

        tile_ref = {}

        if self.i_x >= self.num_tiles_x:
            return None
            
        tile_ref['sizeX'] = self.tile_cols
        if self.i_x == self.num_tiles_x - 1 and self.last_tile_x:
            tile_ref['sizeX'] = self.last_tile_x_size
         
        # set bbox for tile (N, S)
        if self.i_y != 0:
            self.tile_bbox['miny'] -= self.tile_length_y 
            self.tile_bbox['maxy'] -= self.tile_length_y
        else:
            self.tile_bbox['maxy'] = self.bbox['maxy']
            self.tile_bbox['miny'] = self.bbox['maxy'] - self.tile_length_y

        tile_ref['sizeY'] = self.tile_rows
        if self.i_y == self.num_tiles_y - 1 and self.last_tile_y:
            tile_ref['sizeY'] = self.last_tile_y_size 

        if self._isGeoProj(self.proj_srs) and self.version == "1.3.0":                
            query_bbox = self._flipBbox(self.tile_bbox, self.proj_srs, self.version)
        else:
            query_bbox = self.tile_bbox
        query_url = self.url + "&" + "BBOX=%s,%s,%s,%s" % ( query_bbox['minx'],  query_bbox['miny'],  query_bbox['maxx'],  query_bbox['maxy'])

        tile_ref['t_cols_offset'] = int(self.tile_cols * self.i_x)
        tile_ref['t_rows_offset'] = int(self.tile_rows * self.i_y)

        if self.i_y >= self.num_tiles_y - 1:
            self.i_y = 0
            self.i_x += 1
            # set bbox for next tile (E, W)      
            self.tile_bbox['maxx'] += self.tile_length_x 
            self.tile_bbox['minx'] += self.tile_length_x 
        else:
            self.i_y += 1

        return query_url, tile_ref

    def _flipBbox(self, bbox, proj, version):
        """ 
        Flips coordinates if WMS standard is 1.3.0 and 
        projection is geographic.

        value flips between this keys:
        maxy -> maxx
        maxx -> maxy
        miny -> minx
        minx -> miny
        @return copy of bbox with flipped coordinates
        """  

        temp_bbox = dict(bbox)
        new_bbox = {}
        new_bbox['maxy'] = temp_bbox['maxx']
        new_bbox['miny'] = temp_bbox['minx']
        new_bbox['maxx'] = temp_bbox['maxy']
        new_bbox['minx'] = temp_bbox['miny']

        return new_bbox


class WMTSRequestMgr(BaseRequestMgr):
    def __init__(self, params, bbox, region, proj_srs, cap_file = None):
        """!Initializes data needed for iteration through tiles.
        """

        self.proj_srs = proj_srs
        self.meters_per_unit = None

        # constant defined in WMTS standard (in meters)
        self.pixel_size = 0.00028

        # parse capabilities file
        try:
            # checks all elements needed by this class,
            # invalid elements are removed
            cap_tree = WMTSCapabilitiesTree(cap_file)
        except ParseError as error:
            grass.fatal(_("Unable to parse tile service file.\n%s\n") % str(error))
        self.xml_ns = cap_tree.getxmlnshandler()

        root = cap_tree.getroot()

        # get layer tile matrix sets with required projection
        mat_sets = self._getMatSets(root, params['layers'], params['srs'])  #[[TileMatrixSet, TileMatrixSetLink], ....]

        # TODO: what if more tile matrix sets have required srs (returned more than 1)?
        mat_set = mat_sets[0][0]
        mat_set_link = mat_sets[0][1]
        params['tile_matrix_set'] = mat_set.find(self.xml_ns.NsOws('Identifier')).text

        # find tile matrix with resolution closest and smaller to wanted resolution 
        tile_mat  = self._findTileMats(mat_set.findall(self.xml_ns.NsWmts('TileMatrix')), region, bbox)

        # get extend of data available on server expressed in max/min rows and cols of tile matrix
        mat_num_bbox = self._getMatSize(tile_mat, mat_set_link)

        # initialize data needed for iteration through tiles
        self._computeRequestData(tile_mat, params, bbox, mat_num_bbox)

    def GetMapRegion(self):
        """!Get size in pixels and bounding box of raster where all tiles will be merged.
        """
        return self.map_region

    def _getMatSets(self, root, layer_name, srs):
        """!Get matrix sets which are available for chosen layer and have required EPSG.
        """

        contents = root.find(self.xml_ns.NsWmts('Contents'))
        layers = contents.findall(self.xml_ns.NsWmts('Layer'))

        ch_layer = None
        for layer in layers:
            layer_id = layer.find(self.xml_ns.NsOws('Identifier')).text
            if layer_id == layer_name:
                ch_layer = layer 
                break  

        if ch_layer is None:
            grass.fatal(_("Layer '%s' was not found in capabilities file") % layer_name)

        mat_set_links = ch_layer.findall(self.xml_ns.NsWmts('TileMatrixSetLink'))

        suitable_mat_sets = []
        tileMatrixSets = contents.findall(self.xml_ns.NsWmts('TileMatrixSet'))

        for link in  mat_set_links:
            mat_set_link_id = link.find(self.xml_ns.NsWmts('TileMatrixSet')).text
            for mat_set in tileMatrixSets:
                mat_set_id = mat_set.find(self.xml_ns.NsOws('Identifier')).text 
                if mat_set_id != mat_set_link_id:
                    continue
                mat_set_srs = mat_set.find(self.xml_ns.NsOws('SupportedCRS')).text
                if mat_set_srs.lower() == ("EPSG:"+ str(srs)).lower():
                    suitable_mat_sets.append([mat_set, link])

        if not suitable_mat_sets:
            grass.fatal(_("Layer '%s' is not available with %s code.") % (layer_name,  "EPSG:" + str(srs)))

        return suitable_mat_sets # [[TileMatrixSet, TileMatrixSetLink], ....]

    def _findTileMats(self, tile_mats, region, bbox):
        """!Find best tile matrix set for requested resolution.
        """        
        scale_dens = []

        scale_dens.append((bbox['maxy'] - bbox['miny']) / region['rows'] * self._getMetersPerUnit()  / self.pixel_size)
        scale_dens.append((bbox['maxx'] - bbox['minx']) / region['cols'] * self._getMetersPerUnit() / self.pixel_size)

        scale_den = min(scale_dens)

        first = True
        for t_mat in tile_mats:
            mat_scale_den = float(t_mat.find(self.xml_ns.NsWmts('ScaleDenominator')).text)
            if first:
                best_scale_den = mat_scale_den
                best_t_mat = t_mat
                first = False
                continue
                
            best_diff = best_scale_den - scale_den
            mat_diff = mat_scale_den - scale_den
            if (best_diff < mat_diff  and  mat_diff < 0) or \
               (best_diff > mat_diff  and  best_diff > 0):
                best_t_mat = t_mat
                best_scale_den = mat_scale_den

        return best_t_mat

    def _getMetersPerUnit(self):
        """!Get coefficient which allows to convert units of request projection into meters.
        """  
        if self.meters_per_unit:
            return self.meters_per_unit

        # for geographic projection
        if self._isGeoProj(self.proj_srs):
            proj_params = self.proj_srs.split(' ')
            for param in proj_params:
                if '+a' in param:
                    a = float(param.split('=')[1])
                    break
            equator_perim = 2 * pi * a
            # meters per degree on equator
            self.meters_per_unit = equator_perim / 360 

        # other units
        elif '+to_meter' in self.proj_srs:
            proj_params = self.proj_srs.split(' ')
            for param in proj_params:
                if '+to_meter' in param:
                    self.meters_per_unit = 1/float(param.split('=')[1])
                    break
        # coordinate system in meters        
        else:
            self.meters_per_unit = 1

        return self.meters_per_unit

    def _getMatSize(self, tile_mat, mat_set_link):
        """!Get rows and cols extend of data available on server for chosen layer and tile matrix.
        """

        # general tile matrix size
        mat_num_bbox = {}
        mat_num_bbox['min_col'] = mat_num_bbox['min_row'] = 0
        mat_num_bbox['max_col'] = int(tile_mat.find(self.xml_ns.NsWmts('MatrixWidth')).text) - 1
        mat_num_bbox['max_row'] = int(tile_mat.find(self.xml_ns.NsWmts('MatrixHeight')).text) - 1

        # get extend restriction in TileMatrixSetLink for the tile matrix, if exists
        tile_mat_set_limits = mat_set_link.find((self.xml_ns.NsWmts('TileMatrixSetLimits')))
        if tile_mat_set_limits is None:
            return mat_num_bbox

        tile_mat_id = tile_mat.find(self.xml_ns.NsOws('Identifier')).text
        tile_mat_limits = tile_mat_set_limits.findall(self.xml_ns.NsWmts('TileMatrixLimits'))

        for limit in tile_mat_limits:
            limit_tile_mat = limit.find(self.xml_ns.NsWmts('TileMatrix'))
            limit_id = limit_tile_mat.text

            if limit_id == tile_mat_id:
                for i in [['min_row', 'MinTileRow'], ['max_row', 'MaxTileRow'], \
                          ['min_col', 'MinTileCol'], ['max_col', 'MaxTileCol']]:
                    i_tag = limit.find(self.xml_ns.NsWmts(i[1]))

                    mat_num_bbox[i[0]] = int(i_tag.text)
                break
        return mat_num_bbox

    def _computeRequestData(self, tile_mat, params, bbox, mat_num_bbox):
        """!Initialize data needed for iteration through tiles.
        """  
        scale_den = float(tile_mat.find(self.xml_ns.NsWmts('ScaleDenominator')).text)

        pixel_span = scale_den * self.pixel_size / self._getMetersPerUnit()

        tl_str = tile_mat.find(self.xml_ns.NsWmts('TopLeftCorner')).text.split(' ')

        tl_corner = {}
        tl_corner['minx'] = float(tl_str[0])
        tl_corner['maxy'] = float(tl_str[1])

        tile_span = {}
        self.tile_size = {}

        self.tile_size['x'] = int(tile_mat.find(self.xml_ns.NsWmts('TileWidth')).text)
        tile_span['x'] = pixel_span * self.tile_size['x']
        
        self.tile_size['y'] = int(tile_mat.find(self.xml_ns.NsWmts('TileHeight')).text)
        tile_span['y'] = pixel_span * self.tile_size['y']

        self.url = params['url'] + ("SERVICE=WMTS&REQUEST=GetTile&VERSION=1.0.0&" \
                                    "LAYER=%s&STYLE=%s&FORMAT=%s&TILEMATRIXSET=%s&TILEMATRIX=%s" % \
                                   (params['layers'], params['styles'], params['format'],
                                    params['tile_matrix_set'], tile_mat.find(self.xml_ns.NsOws('Identifier')).text ))

        BaseRequestMgr._computeRequestData(self, bbox, tl_corner, tile_span, self.tile_size, mat_num_bbox)

    def GetNextTile(self):
        """!Get url for tile request from server and information for merging the tile with other tiles.
        """
        if not self.intersects or self.i_col > self.t_num_bbox['max_col']:
            return None 
                
        query_url = self.url + "&TILECOL=%i&TILEROW=%i" % (int(self.i_col), int(self.i_row)) 

        self.tile_ref['t_cols_offset'] = int(self.tile_size['x'] * (self.i_col - self.t_num_bbox['min_col']))
        self.tile_ref['t_rows_offset'] = int(self.tile_size['y'] * (self.i_row - self.t_num_bbox['min_row']))

        if self.i_row >= self.t_num_bbox['max_row']:
            self.i_row =  self.t_num_bbox['min_row']
            self.i_col += 1
        else:
            self.i_row += 1 

        return query_url, self.tile_ref

class OnEarthRequestMgr(BaseRequestMgr):
    def __init__(self, params, bbox, region, proj_srs, tile_service):
        """!Initializes data needed for iteration through tiles.
        """
        try:
            # checks all elements needed by this class,
            # invalid elements are removed
            self.cap_tree = OnEarthCapabilitiesTree(tile_service)
        except ParseError as error:
            grass.fatal(_("Unable to parse tile service file.\n%s\n") % str(error))

        root = self.cap_tree.getroot()

        # parse tile service file and get needed data for making tile requests
        url, self.tile_span, t_patt_bbox, self.tile_size = self._parseTileService(root, bbox, region, params)
        self.url = url
        self.url[0] = params['url'] + url[0]
        # initialize data needed for iteration through tiles
        self._computeRequestData(bbox, t_patt_bbox, self.tile_span, self.tile_size)

    def GetMapRegion(self):
        """!Get size in pixels and bounding box of raster where all tiles will be merged.
        """
        return self.map_region

    def _parseTileService(self, root, bbox, region, params):
        """!Get data from tile service file
        """
        tiled_patterns = root.find('TiledPatterns')
        tile_groups = self._getAllTiledGroup(tiled_patterns)
        if not tile_groups:
            grass.fatal(_("Unable to parse tile service file. \n No tag '%s' was found.") % 'TiledGroup')

        req_group = None
        for group in tile_groups:
            name = group.find('Name')
            if name.text == params['layers']:
                req_group = group
                break

        if req_group is None:
            grass.fatal(_("Tiled group '%s' was not found in tile service file") % params['layers'])

        group_t_patts = req_group.findall('TilePattern')
        best_patt = self._parseTilePattern(group_t_patts, bbox, region)

        urls = best_patt.text.split('\n')
        if params['urlparams']:
            url = self._insTimeToTilePatternUrl(params['urlparams'], urls)
        else:
            url = urls[0]
            for u in urls:
                if not 'time=${' in u:
                    url = u

        url, t_bbox, width, height = self.cap_tree.gettilepatternurldata(url)
        tile_span = {}
        tile_span['x'] = abs(t_bbox[0] - t_bbox[2])
        tile_span['y'] = abs(t_bbox[1] - t_bbox[3])
                   
        tile_pattern_bbox = req_group.find('LatLonBoundingBox')

        t_patt_bbox = {}
        for s in ['minx', 'miny', 'maxx', 'maxy']:
            t_patt_bbox[s] = float(tile_pattern_bbox.get(s))

        tile_size = {}
        tile_size['x'] = width
        tile_size['y'] = height

        return url, tile_span, t_patt_bbox, tile_size

    def _getAllTiledGroup(self, parent, tiled_groups = None):
        """!Get all 'TileGroup' elements
        """
        if not tiled_groups:
            tiled_groups = []

        tiled_groups += parent.findall('TiledGroup')
        new_groups = parent.findall('TiledGroups')

        for group in new_groups:
            self._getAllTiledGroup(group, tiled_groups)
        return tiled_groups

    def _parseTilePattern(self, group_t_patts, bbox, region):
        """!Find best tile pattern for requested resolution.
        """
        res = {}
        res['y'] = (bbox['maxy'] - bbox['miny']) / region['rows']
        res['x'] = (bbox['maxx'] - bbox['minx']) / region['cols']

        if res['x'] < res['y']:
            comp_res = 'x'
        else:
            comp_res = 'y'

        t_res = {}
        best_patt = None
        for pattern in group_t_patts:
            url, t_bbox, width, height = self.cap_tree.gettilepatternurldata(pattern.text.split('\n')[0])

            t_res['x'] = abs(t_bbox[0] - t_bbox[2]) / width
            t_res['y'] = abs(t_bbox[1] - t_bbox[3]) / height

            if best_patt is None:
                best_res = t_res[comp_res]
                best_patt = pattern
                first = False
                continue
            
            best_diff = best_res - res[comp_res]
            tile_diff = t_res[comp_res] - res[comp_res]

            if (best_diff < tile_diff  and  tile_diff < 0) or \
               (best_diff > tile_diff  and  best_diff > 0):

                best_res = t_res[comp_res]
                best_patt = pattern

        return best_patt

    def _insTimeToTilePatternUrl(self, url_params, urls):
        """!Time can be variable in some urls in OnEarth TMS. 
            Insert requested time from 'urlparams' into the variable if any url of urls contains the variable.
        """
        url = None
        not_sup_params = []
        url_params_list = url_params.split('&')

        for param in  url_params_list:
            try:
                k, v = param.split('=')
            except ValueError:
                grass.warning(_("Wrong form of parameter '%s' in '%s'. \n \
                                 The parameter was ignored.") % (param, 'urlparams'))

            if k != 'time':
                not_sup_params.append(k)
                continue

            has_time_var = False
            for url in urls: 
                url_p_idxs = self.geturlparamidxs(url, k)
                if not url_p_idxs:
                    continue

                url_p_value = url[url_p_idxs[0] + len(k + '=') : url_p_idxs[1]]

                if url_p_value[:2] == '${'  and \
                   url_p_value[len(url_p_value) - 1] == '}':
                   url = url[:url_p_idxs[0]] + param + url[url_p_idxs[1]:]   
                   has_time_var = True
                   break

            if not has_time_var:
                grass.warning(_("Parameter '%s' in '%s' is not variable in tile pattern url.") % (k, 'urlparams'))

        if not_sup_params:
                grass.warning(_("%s driver supports only '%s' parameter in '%s'. Other parameters are ignored.") % \
                               ('OnEarth GRASS', 'time', 'urlparams'))

        return url

    def _computeRequestData(self, bbox, t_patt_bbox, tile_span, tile_size):
        """!Initialize data needed for iteration through tiles.
        """  
        epsilon = 1e-15
        mat_num_bbox = {}

        mat_num_bbox['min_row'] = mat_num_bbox['min_col'] = 0
        mat_num_bbox['max_row'] = floor((t_patt_bbox['maxy'] - t_patt_bbox['miny'])/ tile_span['y'] + epsilon)  
        mat_num_bbox['max_col'] = floor((t_patt_bbox['maxx'] - t_patt_bbox['minx'])/ tile_span['x'] + epsilon)  

        BaseRequestMgr._computeRequestData(self, bbox, t_patt_bbox, self.tile_span, self.tile_size, mat_num_bbox)


    def GetNextTile(self):
        """!Get url for tile request from server and information for merging the tile with other tiles
        """
        if self.i_col > self.t_num_bbox['max_col']:
            return None 

        x_offset = self.tile_span['x'] * self.i_col
        y_offset = self.tile_span['y'] * self.i_row

        query_url = self.url[0] + "&" + "bbox=%s,%s,%s,%s" % (float(self.query_bbox['minx'] + x_offset),  
                                                              float(self.query_bbox['miny'] - y_offset),  
                                                              float(self.query_bbox['maxx'] + x_offset),  
                                                              float(self.query_bbox['maxy'] - y_offset)) + self.url[1]

        self.tile_ref['t_cols_offset'] = int(self.tile_size['y'] * (self.i_col - self.t_num_bbox['min_col']))
        self.tile_ref['t_rows_offset'] = int(self.tile_size['x'] * (self.i_row - self.t_num_bbox['min_row']))

        if self.i_row >= self.t_num_bbox['max_row']:
            self.i_row =  self.t_num_bbox['min_row']
            self.i_col += 1
        else:
            self.i_row += 1 

        return query_url, self.tile_ref

