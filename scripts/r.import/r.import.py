#!/usr/bin/env python3

############################################################################
#
# MODULE:       r.import
#
# AUTHOR(S):    Markus Metz
#
# PURPOSE:      Import and reproject on the fly
#
# COPYRIGHT:    (C) 2015-2016 GRASS development team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

#%module
#% description: Imports raster data into a GRASS raster map using GDAL library and reprojects on the fly.
#% keyword: raster
#% keyword: import
#% keyword: projection
#%end
#%option G_OPT_F_BIN_INPUT
#% description: Name of GDAL dataset to be imported
#% guisection: Input
#%end
#%option
#% key: band
#% type: integer
#% required: no
#% multiple: yes
#% description: Input band(s) to select (default is all bands)
#% guisection: Input
#%end
#%option
#% key: memory
#% type: integer
#% required: no
#% multiple: no
#% label: Maximum memory to be used (in MB)
#% description: Cache size for raster rows
#% answer: 300
#%end
#%option G_OPT_R_OUTPUT
#% description: Name for output raster map
#% required: no
#% guisection: Output
#%end
#%option
#% key: resample
#% type: string
#% required: no
#% multiple: no
#% options: nearest,bilinear,bicubic,lanczos,bilinear_f,bicubic_f,lanczos_f
#% description: Resampling method to use for reprojection
#% descriptions: nearest;nearest neighbor;bilinear;bilinear interpolation;bicubic;bicubic interpolation;lanczos;lanczos filter;bilinear_f;bilinear interpolation with fallback;bicubic_f;bicubic interpolation with fallback;lanczos_f;lanczos filter with fallback
#% answer: nearest
#% guisection: Output
#%end
#%option
#% key: extent
#% type: string
#% required: no
#% multiple: no
#% options: input,region
#% answer: input
#% description: Output raster map extent
#% descriptions: region;extent of current region;input;extent of input map
#% guisection: Output
#%end
#%option
#% key: resolution
#% type: string
#% required: no
#% multiple: no
#% answer: estimated
#% options: estimated,value,region
#% description: Resolution of output raster map (default: estimated)
#% descriptions: estimated;estimated resolution;value;user-specified resolution;region;current region resolution
#% guisection: Output
#%end
#%option
#% key: resolution_value
#% type: double
#% required: no
#% multiple: no
#% description: Resolution of output raster map (use with option resolution=value)
#% guisection: Output
#%end
#%option
#% key: title
#% key_desc: phrase
#% type: string
#% required: no
#% description: Title for resultant raster map
#% guisection: Metadata
#%end
#%flag
#% key: e
#% description: Estimate resolution only
#% guisection: Optional
#%end
#%flag
#% key: n
#% description: Do not perform region cropping optimization
#% guisection: Optional
#%end
#%flag
#% key: l
#% description: Force Lat/Lon maps to fit into geographic coordinates (90N,S; 180E,W)
#%end
#%flag
#% key: o
#% label: Override projection check (use current location's projection)
#% description: Assume that the dataset has the same projection as the current location
#%end
#%rules
#% required: output,-e
#%end

import sys
import os
import atexit
import math

import grass.script as grass
from grass.exceptions import CalledModuleError


# initialize global vars
TMPLOC = None
SRCGISRC = None
TGTGISRC = None
GISDBASE = None
TMP_REG_NAME = None


def cleanup():
    if TGTGISRC:
        os.environ['GISRC'] = str(TGTGISRC)
    # remove temp location
    if TMPLOC:
        grass.try_rmdir(os.path.join(GISDBASE, TMPLOC))
    if SRCGISRC:
        grass.try_remove(SRCGISRC)
    if TMP_REG_NAME and grass.find_file(name=TMP_REG_NAME, element='vector',
                                        mapset=grass.gisenv()['MAPSET'])['fullname']:
        grass.run_command('g.remove', type='vector', name=TMP_REG_NAME,
                          flags='f', quiet=True)


def main():
    global TMPLOC, SRCGISRC, TGTGISRC, GISDBASE, TMP_REG_NAME

    GDALdatasource = options['input']
    output = options['output']
    method = options['resample']
    memory = options['memory']
    bands = options['band']
    tgtres = options['resolution']
    title = options["title"]
    if flags['e'] and not output:
        output = 'rimport_tmp'  # will be removed with the entire tmp location
    if options['resolution_value']:
        if tgtres != 'value':
            grass.fatal(_("To set custom resolution value, select 'value' in resolution option"))
        tgtres_value = float(options['resolution_value'])
        if tgtres_value <= 0:
            grass.fatal(_("Resolution value can't be smaller than 0"))
    elif tgtres == 'value':
        grass.fatal(
            _("Please provide the resolution for the imported dataset or change to 'estimated' resolution"))

    # try r.in.gdal directly first
    additional_flags = 'l' if flags['l'] else ''
    if flags['o']:
        additional_flags += 'o'
    region_flag = ''
    if options['extent'] == 'region':
        region_flag += 'r'
    if flags['o'] or grass.run_command('r.in.gdal', input=GDALdatasource, flags='j',
                                       errors='status', quiet=True) == 0:
        parameters = dict(input=GDALdatasource, output=output,
                          memory=memory, flags='ak' + additional_flags + region_flag)
        if bands:
            parameters['band'] = bands
        try:
            grass.run_command('r.in.gdal', **parameters)
            grass.verbose(
                _("Input <%s> successfully imported without reprojection") %
                GDALdatasource)
            return 0
        except CalledModuleError as e:
            grass.fatal(_("Unable to import GDAL dataset <%s>") % GDALdatasource)

    grassenv = grass.gisenv()
    tgtloc = grassenv['LOCATION_NAME']

    # make sure target is not xy
    if grass.parse_command('g.proj', flags='g')['name'] == 'xy_location_unprojected':
        grass.fatal(
            _("Coordinate reference system not available for current location <%s>") %
            tgtloc)

    tgtmapset = grassenv['MAPSET']
    GISDBASE = grassenv['GISDBASE']
    TGTGISRC = os.environ['GISRC']
    SRCGISRC = grass.tempfile()

    TMPLOC = 'temp_import_location_' + str(os.getpid())
    TMP_REG_NAME = 'vreg_tmp_' + str(os.getpid())

    f = open(SRCGISRC, 'w')
    f.write('MAPSET: PERMANENT\n')
    f.write('GISDBASE: %s\n' % GISDBASE)
    f.write('LOCATION_NAME: %s\n' % TMPLOC)
    f.write('GUI: text\n')
    f.close()

    tgtsrs = grass.read_command('g.proj', flags='j', quiet=True)

    # create temp location from input without import
    grass.verbose(_("Creating temporary location for <%s>...") % GDALdatasource)
    parameters = dict(input=GDALdatasource, output=output,
                      memory=memory, flags='c', title=title,
                      location=TMPLOC, quiet=True)
    if bands:
        parameters['band'] = bands
    try:
        grass.run_command('r.in.gdal', **parameters)
    except CalledModuleError:
        grass.fatal(_("Unable to read GDAL dataset <%s>") % GDALdatasource)

    # prepare to set region in temp location
    if 'r' in region_flag:
        tgtregion = TMP_REG_NAME
        grass.run_command('v.in.region', **dict(output=tgtregion, flags='d'))

    # switch to temp location
    os.environ['GISRC'] = str(SRCGISRC)

    # print projection at verbose level
    grass.verbose(grass.read_command('g.proj', flags='p').rstrip(os.linesep))

    # make sure input is not xy
    if grass.parse_command('g.proj', flags='g')['name'] == 'xy_location_unprojected':
        grass.fatal(_("Coordinate reference system not available for input <%s>") % GDALdatasource)

    # import into temp location
    grass.verbose(_("Importing <%s> to temporary location...") % GDALdatasource)
    parameters = dict(input=GDALdatasource, output=output,
                      memory=memory, flags='ak' + additional_flags)
    if bands:
        parameters['band'] = bands
    if 'r' in region_flag:
        grass.run_command('v.proj', **dict(location=tgtloc, mapset=tgtmapset,
                          input=tgtregion, output=tgtregion))
        grass.run_command('g.region', **dict(vector=tgtregion))
        parameters['flags'] = parameters['flags'] + region_flag
    try:
        grass.run_command('r.in.gdal', **parameters)
    except CalledModuleError:
        grass.fatal(_("Unable to import GDAL dataset <%s>") % GDALdatasource)

    outfiles = grass.list_grouped('raster')['PERMANENT']

    # is output a group?
    group = False
    path = os.path.join(GISDBASE, TMPLOC, 'group', output)
    if os.path.exists(path):
        group = True
        path = os.path.join(GISDBASE, TMPLOC, 'group', output, 'POINTS')
        if os.path.exists(path):
            grass.fatal(_("Input contains GCPs, rectification is required"))

    if 'r' in region_flag:
        grass.run_command('g.remove', type="vector", flags="f",
                          name=tgtregion)

    # switch to target location
    os.environ['GISRC'] = str(TGTGISRC)

    if 'r' in region_flag:
        grass.run_command('g.remove', **dict(type="vector", flags="f",
                          name=tgtregion))

    region = grass.region()

    rflags = None
    if flags['n']:
        rflags = 'n'

    vreg = TMP_REG_NAME

    for outfile in outfiles:

        n = region['n']
        s = region['s']
        e = region['e']
        w = region['w']

        grass.use_temp_region()

        if options['extent'] == 'input':
            # r.proj -g
            try:
                tgtextents = grass.read_command('r.proj', location=TMPLOC,
                                                mapset='PERMANENT',
                                                input=outfile, flags='g',
                                                memory=memory, quiet=True)
            except CalledModuleError:
                grass.fatal(_("Unable to get reprojected map extent"))
            try:
                srcregion = grass.parse_key_val(tgtextents, val_type=float, vsep=' ')
                n = srcregion['n']
                s = srcregion['s']
                e = srcregion['e']
                w = srcregion['w']
            except ValueError:  # import into latlong, expect 53:39:06.894826N
                srcregion = grass.parse_key_val(tgtextents, vsep=' ')
                n = grass.float_or_dms(srcregion['n'][:-1]) * \
                    (-1 if srcregion['n'][-1] == 'S' else 1)
                s = grass.float_or_dms(srcregion['s'][:-1]) * \
                    (-1 if srcregion['s'][-1] == 'S' else 1)
                e = grass.float_or_dms(srcregion['e'][:-1]) * \
                    (-1 if srcregion['e'][-1] == 'W' else 1)
                w = grass.float_or_dms(srcregion['w'][:-1]) * \
                    (-1 if srcregion['w'][-1] == 'W' else 1)

            grass.run_command('g.region', n=n, s=s, e=e, w=w)

        # v.in.region in tgt
        grass.run_command('v.in.region', output=vreg, quiet=True)

        grass.del_temp_region()

        # reproject to src
        # switch to temp location
        os.environ['GISRC'] = str(SRCGISRC)
        try:
            grass.run_command('v.proj', input=vreg, output=vreg,
                              location=tgtloc, mapset=tgtmapset, quiet=True)
            # test if v.proj created a valid area
            if grass.vector_info_topo(vreg)['areas'] != 1:
                rass.fatal(_("Please check the 'extent' parameter"))
        except CalledModuleError:
            grass.fatal(_("Unable to reproject to source location"))

        # set region from region vector
        grass.run_command('g.region', raster=outfile)
        grass.run_command('g.region', vector=vreg)
        # align to first band
        grass.run_command('g.region', align=outfile)
        # get number of cells
        cells = grass.region()['cells']

        estres = math.sqrt((n - s) * (e - w) / cells)
        # remove from source location for multi bands import
        grass.run_command('g.remove', type='vector', name=vreg,
                          flags='f', quiet=True)

        os.environ['GISRC'] = str(TGTGISRC)
        grass.run_command('g.remove', type='vector', name=vreg,
                          flags='f', quiet=True)

        grass.message(
            _("Estimated target resolution for input band <{out}>: {res}").format(
                out=outfile, res=estres))
        if flags['e']:
            continue

        if options['extent'] == 'input' or tgtres == 'value':
            grass.use_temp_region()

        if options['extent'] == 'input':
            grass.run_command('g.region', n=n, s=s, e=e, w=w)

        res = None
        if tgtres == 'estimated':
            res = estres
        elif tgtres == 'value':
            res = tgtres_value
            grass.message(
                _("Using given resolution for input band <{out}>: {res}").format(
                    out=outfile, res=res))
            # align to requested resolution
            grass.run_command('g.region', res=res, flags='a')
        else:
            curr_reg = grass.region()
            grass.message(_("Using current region resolution for input band "
                            "<{out}>: nsres={ns}, ewres={ew}").format(out=outfile, ns=curr_reg['nsres'],
                                                                      ew=curr_reg['ewres']))

        # r.proj
        grass.message(_("Reprojecting <%s>...") % outfile)
        try:
            grass.run_command('r.proj', location=TMPLOC,
                              mapset='PERMANENT', input=outfile,
                              method=method, resolution=res,
                              memory=memory, flags=rflags, quiet=True)
        except CalledModuleError:
            grass.fatal(_("Unable to to reproject raster <%s>") % outfile)

        if grass.raster_info(outfile)['min'] is None:
            grass.fatal(_("The reprojected raster <%s> is empty") % outfile)

        if options['extent'] == 'input' or tgtres == 'value':
            grass.del_temp_region()

    if flags['e']:
        return 0

    if group:
        grass.run_command('i.group', group=output, input=','.join(outfiles))

    # TODO: write metadata with r.support

    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    sys.exit(main())
