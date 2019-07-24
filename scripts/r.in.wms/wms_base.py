"""!
@brief Preparation of parameters for drivers, which download it, and managing downloaded data.

List of classes:
 - wms_base::WMSBase
 - wms_base::GRASSImporter
 - wms_base::WMSDriversInfo

(C) 2012-2019 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (Mentor: Martin Landa)
"""

import os
from math import ceil

import base64

try:
    from urllib2 import Request, urlopen, HTTPError
    from httplib import HTTPException
except ImportError:
    from urllib.request import Request, urlopen
    from urllib.error import HTTPError
    from http.client import HTTPException


import grass.script as grass
from grass.exceptions import CalledModuleError


class WMSBase(object):

    def __init__(self):
        # these variables are information for destructor
        self.temp_files_to_cleanup = []

        self.params = {}
        self.tile_size = {'bbox': None}

        self.temp_map = None
        self.temp_warpmap = None

    def __del__(self):

        # tries to remove temporary files, all files should be
        # removed before, implemented just in case of unexpected
        # stop of module
        for temp_file in self.temp_files_to_cleanup:
            grass.try_remove(temp_file)

    def _debug(self, fn, msg):
        grass.debug("%s.%s: %s" %
                    (self.__class__.__name__, fn, msg))

    def _initializeParameters(self, options, flags):
        self._debug("_initialize_parameters", "started")

        # initialization of module parameters (options, flags)
        self.params['driver'] = options['driver']
        drv_info = WMSDriversInfo()

        driver_props = drv_info.GetDrvProperties(options['driver'])
        self._checkIgnoeredParams(options, flags, driver_props)

        self.params['capfile'] = options['capfile'].strip()

        for key in ['url', 'layers', 'styles', 'method']:
            self.params[key] = options[key].strip()

        self.flags = flags

        if self.flags['o']:
            self.params['transparent'] = 'FALSE'
        else:
            self.params['transparent'] = 'TRUE'

        for key in ['password', 'username', 'urlparams']:
            self.params[key] = options[key]

        if (self.params ['password'] and self.params ['username'] == '') or \
           (self.params['password'] == '' and self.params['username']):
            grass.fatal(_("Please insert both %s and %s parameters or none of them." %
                          ('password', 'username')))

        self.params['bgcolor'] = options['bgcolor'].strip()

        if options['format'] == "jpeg" and \
           not 'format' in driver_props['ignored_params']:
            if not flags['o'] and \
                    'WMS' in self.params['driver']:
                grass.warning(_("JPEG format does not support transparency"))

        self.params['format'] = drv_info.GetFormat(options['format'])
        if not self.params['format']:
            self.params['format'] = self.params['format']

        # TODO: get srs from Tile Service file in OnEarth_GRASS driver
        self.params['srs'] = int(options['srs'])
        if self.params['srs'] <= 0 and not 'srs' in driver_props['ignored_params']:
            grass.fatal(_("Invalid EPSG code %d") % self.params['srs'])

        self.params['wms_version'] = options['wms_version']
        if "CRS" in GetSRSParamVal(self.params['srs']) and self.params['wms_version'] == "1.1.1":
            self.params['wms_version'] = "1.3.0"
            grass.warning(
                _("WMS version <1.3.0> will be used, because version <1.1.1> does not support <%s>projection") %
                GetSRSParamVal(
                    self.params['srs']))

        if self.params['wms_version'] == "1.3.0":
            self.params['proj_name'] = "CRS"
        else:
            self.params['proj_name'] = "SRS"

        # read projection info
        self.proj_location = grass.read_command('g.proj',
                                                flags='jf').rstrip('\n')
        self.proj_location = self._modifyProj(self.proj_location)

        self.source_epsg = str(GetEpsg(self.params['srs']))
        self.target_epsg = None
        target_crs = grass.parse_command('g.proj', flags='g', delimiter = '=')
        if 'epsg' in target_crs.keys():
            self.target_epsg = target_crs['epsg']
            if self.source_epsg != self.target_epsg:
                grass.warning(_("SRS differences: WMS source EPSG %s != location EPSG %s (use srs=%s to adjust)") %
                              (self.source_epsg, self.target_epsg, self.target_epsg))

        self.proj_srs = grass.read_command('g.proj',
                                           flags='jf',
                                           epsg=str(GetEpsg(self.params['srs'])))
        self.proj_srs = self.proj_srs.rstrip('\n')

        self.proj_srs = self._modifyProj(self.proj_srs)

        if not self.proj_srs or not self.proj_location:
            grass.fatal(_("Unable to get projection info"))

        self.region = options['region']

        min_tile_size = 100
        maxcols = int(options['maxcols'])
        if maxcols <= min_tile_size:
            grass.fatal(_("Maxcols must be greater than 100"))

        maxrows = int(options['maxrows'])
        if maxrows <= min_tile_size:
            grass.fatal(_("Maxrows must be greater than 100"))

        # setting optimal tile size according to maxcols and maxrows constraint
        # and region cols and rows
        self.tile_size['cols'] = int(
            self.region['cols'] /
            ceil(
                self.region['cols'] /
                float(maxcols)))
        self.tile_size['rows'] = int(
            self.region['rows'] /
            ceil(
                self.region['rows'] /
                float(maxrows)))

        # default format for GDAL library
        self.gdal_drv_format = "GTiff"

        self._debug("_initialize_parameters", "finished")

    def _modifyProj(self, proj):
        """!Modify proj.4 string for usage in this module"""

        # add +wktext parameter to avoid droping of +nadgrids parameter (if presented) in gdalwarp
        if '+nadgrids=' in proj and ' +wktext' not in proj:
            proj += ' +wktext'

        return proj

    def _checkIgnoeredParams(self, options, flags, driver_props):
        """!Write warnings for set parameters and flags, which chosen driver does not use."""

        not_relevant_params = []
        for i_param in driver_props['ignored_params']:

            if i_param in options and \
               options[i_param] and \
               i_param not in ['srs', 'wms_version', 'format']:  # params with default value
                not_relevant_params.append('<' + i_param + '>')

        if len(not_relevant_params) > 0:
            grass.warning(_("These parameter are ignored: %s\n\
                             %s driver does not support the parameters." %
                            (','.join(not_relevant_params), options['driver'])))

        not_relevant_flags = []
        for i_flag in driver_props['ignored_flags']:

            if flags[i_flag]:
                not_relevant_flags.append('<' + i_flag + '>')

        if len(not_relevant_flags) > 0:
            grass.warning(_("These flags are ignored: %s\n\
                             %s driver does not support the flags." %
                            (','.join(not_relevant_flags), options['driver'])))

    def GetMap(self, options, flags):
        """!Download data from WMS server."""

        self._initializeParameters(options, flags)
        self.bbox = self._computeBbox()

        self.temp_map = self._download()

        if not self.temp_map:
            return

        self._reprojectMap()

        return self.temp_warpmap

    def _fetchCapabilities(self, options):
        """!Download capabilities from WMS server
        """
        cap_url = options['url'].strip()

        if "?" in cap_url:
            cap_url += "&"
        else:
            cap_url += "?"

        if 'WMTS' in options['driver']:
            cap_url += "SERVICE=WMTS&REQUEST=GetCapabilities&VERSION=1.0.0"
        elif 'OnEarth' in options['driver']:
            cap_url += "REQUEST=GetTileService"
        else:
            cap_url += "SERVICE=WMS&REQUEST=GetCapabilities&VERSION=" + options['wms_version']

        if options['urlparams']:
            cap_url += "&" + options['urlparams']

        grass.debug('Fetching capabilities file.\n%s' % cap_url)

        try:
            cap = self._fetchDataFromServer(cap_url, options['username'], options['password'])
        except (IOError, HTTPException) as e:
            if isinstance(e, HTTPError) and e.code == 401:
                grass.fatal(
                    _("Authorization failed to <%s> when fetching capabilities") %
                    options['url'])
            else:
                msg = _("Unable to fetch capabilities from <{0}>. Reason: ").format(
                    options['url'])

                if hasattr(e, 'reason'):
                    msg += '{0}'.format(e.reason)
                else:
                    msg += '{0}'.format(e)

                grass.fatal(msg)

        grass.debug('Fetching capabilities OK')
        return cap

    def _fetchDataFromServer(self, url, username=None, password=None):
        """!Fetch data from server
        """
        request = Request(url)
        if username and password:
            base64string = base64.encodestring('%s:%s' % (username, password)).replace('\n', '')
            request.add_header("Authorization", "Basic %s" % base64string)

        try:
            return urlopen(request)
        except ValueError as error:
            grass.fatal("%s" % error)

    def GetCapabilities(self, options):
        """!Get capabilities from WMS server
        """
        cap = self._fetchCapabilities(options)
        capfile_output = options['capfile_output'].strip()

        # save to file
        if capfile_output:
            try:
                temp = open(capfile_output, "w")
                temp.write(cap.read())
                temp.close()
                return
            except IOError as error:
                grass.fatal(_("Unabble to open file '%s'.\n%s\n" % (cap_file, error)))

        # print to output
        cap_lines = cap.readlines()
        for line in cap_lines:
            print(line.rstrip())

    def _computeBbox(self):
        """!Get region extent for WMS query (bbox)
        """
        self._debug("_computeBbox", "started")

        bbox_region_items = {'maxy': 'n', 'miny': 's', 'maxx': 'e', 'minx': 'w'}
        bbox = {}

        if self.proj_srs == self.proj_location:  # TODO: do it better
            for bbox_item, region_item in bbox_region_items.items():
                bbox[bbox_item] = self.region[region_item]

        # if location projection and wms query projection are
        # different, corner points of region are transformed into wms
        # projection and then bbox is created from extreme coordinates
        # of the transformed points
        else:
            for bbox_item, region_item in bbox_region_items.items():
                bbox[bbox_item] = None

            temp_region = self._tempfile()

            try:
                temp_region_opened = open(temp_region, 'w')
                temp_region_opened.write("%f %f\n%f %f\n%f %f\n%f %f\n" %
                                         (self.region['e'], self.region['n'],
                                          self.region['w'], self.region['n'],
                                          self.region['w'], self.region['s'],
                                          self.region['e'], self.region['s']))
            except IOError:
                grass.fatal(_("Unable to write data into tempfile"))
            finally:
                temp_region_opened.close()

            points = grass.read_command('m.proj', flags='d',
                                        proj_out=self.proj_srs,
                                        proj_in=self.proj_location,
                                        input=temp_region,
                                        quiet=True)  # TODO: stdin
            grass.try_remove(temp_region)
            if not points:
                grass.fatal(_("Unable to determine region, %s failed") % 'm.proj')

            points = points.splitlines()
            if len(points) != 4:
                grass.fatal(_("Region definition: 4 points required"))

            for point in points:
                try:
                    point = list(map(float, point.split("|")))
                except ValueError:
                    grass.fatal(_('Reprojection of region using m.proj failed.'))
                if not bbox['maxy']:
                    bbox['maxy'] = point[1]
                    bbox['miny'] = point[1]
                    bbox['maxx'] = point[0]
                    bbox['minx'] = point[0]
                    continue

                if bbox['maxy'] < point[1]:
                    bbox['maxy'] = point[1]
                elif bbox['miny'] > point[1]:
                    bbox['miny'] = point[1]

                if bbox['maxx'] < point[0]:
                    bbox['maxx'] = point[0]
                elif bbox['minx'] > point[0]:
                    bbox['minx'] = point[0]

        self._debug("_computeBbox", "finished -> %s" % bbox)

        # Ordering of coordinates axis of geographic coordinate
        # systems in WMS 1.3.0 is flipped. If  self.tile_size['flip_coords'] is
        # True, coords in bbox need to be flipped in WMS query.

        return bbox

    def _reprojectMap(self):
        """!Reproject data  using gdalwarp if needed
        """
        # reprojection of raster
        do_reproject = True
        if self.source_epsg is not None and self.target_epsg is not None \
            and self.source_epsg == self.target_epsg:
            do_reproject = False
        # TODO: correctly compare source and target crs
        if do_reproject == True and self.proj_srs == self.proj_location:
            do_reproject = False
        if do_reproject:
            grass.message(_("Reprojecting raster..."))
            self.temp_warpmap = grass.tempfile() + '.tif'

            if int(os.getenv('GRASS_VERBOSE', '2')) <= 2:
                nuldev = open(os.devnull, 'w+')
            else:
                nuldev = None

            if self.params['method'] == "nearest":
                gdal_method = "near"
            elif self.params['method'] == "linear":
                gdal_method = "bilinear"
            else:
                gdal_method = self.params['method']

            # RGB rasters - alpha layer is added for cropping edges of projected raster
            try:
                if self.temp_map_bands_num == 3:
                    ps = grass.Popen(['gdalwarp',
                                      '-s_srs', '%s' % self.proj_srs,
                                      '-t_srs', '%s' % self.proj_location,
                                      '-r', gdal_method, '-dstalpha',
                                      self.temp_map, self.temp_warpmap], stdout=nuldev)
                # RGBA rasters
                else:
                    ps = grass.Popen(['gdalwarp',
                                      '-s_srs', '%s' % self.proj_srs,
                                      '-t_srs', '%s' % self.proj_location,
                                      '-r', gdal_method,
                                      self.temp_map, self.temp_warpmap], stdout=nuldev)
                ps.wait()
            except OSError as e:
                grass.fatal('%s \nThis can be caused by missing %s utility. ' % (e, 'gdalwarp'))

            if nuldev:
                nuldev.close()

            if ps.returncode != 0:
                grass.fatal(_('%s failed') % 'gdalwarp')
            grass.try_remove(self.temp_map)
        # raster projection is same as projection of location
        else:
            self.temp_warpmap = self.temp_map
            self.temp_files_to_cleanup.remove(self.temp_map)

        return self.temp_warpmap

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


class GRASSImporter:

    def __init__(self, opt_output, cleanup_bands):

        self.cleanup_mask = False
        self.cleanup_bands = cleanup_bands

        # output map name
        self.opt_output = opt_output

        # suffix for existing mask (during overriding will be saved
        # into raster named:self.opt_output + this suffix)
        self.original_mask_suffix = "_temp_MASK"

        # check names of temporary rasters, which module may create
        maps = []
        for suffix in ('.red', '.green', '.blue', '.alpha', self.original_mask_suffix):
            rast = self.opt_output + suffix
            if grass.find_file(rast, element='cell', mapset='.')['file']:
                maps.append(rast)

        if len(maps) != 0:
            grass.fatal(_("Please change output name, or change names of these rasters: %s, "
                          "module needs to create this temporary maps during execution.") % ",".join(maps))

    def __del__(self):
        # removes temporary mask, used for import transparent or warped temp_map
        if self.cleanup_mask:
            # clear temporary mask, which was set by module
            try:
                grass.run_command('r.mask', quiet=True, flags='r')
            except CalledModuleError:
                grass.fatal(_('%s failed') % 'r.mask')

            # restore original mask, if exists
            if grass.find_file(self.opt_output + self.original_mask_suffix,
                               element='cell', mapset='.')['name']:
                try:
                    mask_copy = self.opt_output + self.original_mask_suffix
                    grass.run_command('g.copy', quiet=True,
                                      raster=mask_copy + ',MASK')
                except CalledModuleError:
                    grass.fatal(_('%s failed') % 'g.copy')

        # remove temporary created rasters
        maps = []
        rast = self.opt_output + '.alpha'
        if grass.find_file(rast, element='cell', mapset='.')['file']:
            maps.append(rast)

        if self.cleanup_bands:
            for suffix in ('.red', '.green', '.blue', self.original_mask_suffix):
                rast = self.opt_output + suffix
                if grass.find_file(rast, element='cell', mapset='.')['file']:
                    maps.append(rast)

        if maps:
            grass.run_command('g.remove',
                              quiet=True,
                              flags='fb',
                              type='raster',
                              name=','.join(maps))

        # delete environmental variable which overrides region
        if 'GRASS_REGION' in os.environ.keys():
            os.environ.pop('GRASS_REGION')

        maplist = grass.read_command('g.list', type = 'raster',
                                     pattern = '%s*' % (self.opt_output),
                                     mapset = '.', separator = ',').rstrip('\n')

        if len(maplist) == 0:
            grass.fatal(_('WMS import failed, nothing imported'))
        else:
            for raster in maplist.split(','):
                grass.raster_history(raster, overwrite = True)
                grass.run_command('r.support', map=raster, description='generated by r.in.wms')
                grass.message(_('<%s> created.') % raster)


    def ImportMapIntoGRASS(self, raster):
        """!Import raster into GRASS.
        """
        # importing temp_map into GRASS
        try:
            # do not use -o flag !
            grass.run_command('r.in.gdal', flags='o',
                              quiet=True, overwrite=True,
                              input=raster, output=self.opt_output)
        except CalledModuleError:
            grass.fatal(_('%s failed') % 'r.in.gdal')

        # information for destructor to cleanup temp_layers, created
        # with r.in.gdal

        # setting region for full extend of imported raster
        if grass.find_file(self.opt_output + '.red', element='cell', mapset='.')['file']:
            region_map = self.opt_output + '.red'
        else:
            region_map = self.opt_output
        os.environ['GRASS_REGION'] = grass.region_env(rast=region_map)

        # mask created from alpha layer, which describes real extend
        # of warped layer (may not be a rectangle), also mask contains
        # transparent parts of raster
        if grass.find_file(self.opt_output + '.alpha', element='cell', mapset='.')['name']:
            # saving current mask (if exists) into temp raster
            if grass.find_file('MASK', element='cell', mapset='.')['name']:
                try:
                    mask_copy = self.opt_output + self.original_mask_suffix
                    grass.run_command('g.copy', quiet=True,
                                      raster='MASK,' + mask_copy)
                except CalledModuleError:
                    grass.fatal(_('%s failed') % 'g.copy')

            # info for destructor
            self.cleanup_mask = True
            try:
                grass.run_command('r.mask',
                                  quiet=True, overwrite=True,
                                  maskcats="0",
                                  flags='i',
                                  raster=self.opt_output + '.alpha')
            except CalledModuleError:
                grass.fatal(_('%s failed') % 'r.mask')

            if not self.cleanup_bands:
                # use the MASK to set NULL vlues
                for suffix in ('.red', '.green', '.blue'):
                    rast = self.opt_output + suffix
                    if grass.find_file(rast, element='cell', mapset='.')['file']:
                        grass.run_command('g.rename', rast='%s,%s' % (rast, rast + '_null'), quiet = True)
                        grass.run_command('r.mapcalc', expression = '%s = %s' % (rast, rast + '_null'), quiet = True)
                        grass.run_command('g.remove', type='raster', name='%s' % (rast + '_null'), flags = 'f', quiet = True)

        # TODO one band + alpha band?
        if grass.find_file(self.opt_output + '.red', element='cell', mapset='.')['file'] and self.cleanup_bands:
            try:
                grass.run_command('r.composite',
                                  quiet=True, overwrite=True,
                                  red=self.opt_output + '.red',
                                  green=self.opt_output + '.green',
                                  blue=self.opt_output + '.blue',
                                  output=self.opt_output)
            except CalledModuleError:
                grass.fatal(_('%s failed') % 'r.composite')


class WMSDriversInfo:

    def __init__(self):
        """!Provides information about driver parameters.
        """

        # format labels
        self.f_labels = ["geotiff", "tiff", "png", "jpeg", "gif", "png8"]

        # form for request
        self.formats = [
            "image/geotiff",
            "image/tiff",
            "image/png",
            "image/jpeg",
            "image/gif",
            "image/png8"]

        self.srs = ("epsg", "crs")

    def GetDrvProperties(self, driver):
        """!Get information about driver parameters.
        """
        if driver == 'WMS_GDAL':
            return self._GDALDrvProperties()
        if 'WMS' in driver:
            return self._WMSProperties()
        if 'WMTS' in driver:
            return self._WMTSProperties()
        if 'OnEarth' in driver:
            return self._OnEarthProperties()

    def _OnEarthProperties(self):

        props = {}
        props['ignored_flags'] = ['o']
        props['ignored_params'] = ['bgcolor', 'styles', 'capfile_output',
                                   'format', 'srs', 'wms_version']
        props['req_multiple_layers'] = False

        return props

    def _WMSProperties(self):

        props = {}
        props['ignored_params'] = ['capfile']
        props['ignored_flags'] = []
        props['req_multiple_layers'] = True

        return props

    def _WMTSProperties(self):

        props = {}
        props['ignored_flags'] = ['o']
        props['ignored_params'] = ['urlparams', 'bgcolor', 'wms_version']
        props['req_multiple_layers'] = False

        return props

    def _GDALDrvProperties(self):

        props = {}
        props['ignored_flags'] = []
        props['ignored_params'] = ['urlparams', 'bgcolor', 'capfile', 'capfile_output',
                                   'username', 'password']
        props['req_multiple_layers'] = True

        return props

    def GetFormatLabel(self, format):
        """!Convert format request form to value in parameter 'format'.
        """
        if format in self.formats:
            return self.f_labels[self.formats.index(format)]
        return None

    def GetFormat(self, label):
        """!Convert value in parameter 'format' to format request form.
        """
        if label in self.f_labels:
            return self.formats[self.f_labels.index(label)]
        return None

    def GetSrs(self):
        """!Get supported srs prefixes (e.g. epsg/crs)

        @todo filter according to version and driver params
        """
        return self.srs


# TODO move to utils?
def GetSRSParamVal(srs):
    """!Decides whether to use CRS or EPSG prefix according to srs number.
    """

    if srs in [84, 83, 27]:
        return "CRS:%d" % srs
    else:
        return "EPSG:%d" % srs


def GetEpsg(srs):
    """
     @return EPSG number
             If srs is CRS number, return EPSG number which corresponds to CRS number.
    """
    if srs == 84:
        return 4326
    if srs == 83:
        return 4269
    if srs == 27:
        return 4267

    return srs
