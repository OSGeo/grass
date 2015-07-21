#!/usr/bin/env python

############################################################################
#
# MODULE:       v.import
#
# AUTHOR(S):    Markus Metz
#
# PURPOSE:      Import and reproject vector data on the fly
#
# COPYRIGHT:    (C) 2015 by GRASS development team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

#%module
#% description: Imports vector data into a GRASS vector map using OGR library and reproject on the fly.
#% keyword: vector
#% keyword: import
#% keyword: projection
#%end
#%option 
#% key: input
#% type: string
#% required: yes
#% description: Name of OGR datasource to be imported
#% gisprompt: old,datasource,datasource
#%end
#%option
#% key: layer
#% type: string
#% multiple: yes
#% description: OGR layer name. If not given, all available layers are imported
#% guisection: Input
#% gisprompt: old,datasource_layer,datasource_layer
#%end
#%option G_OPT_V_OUTPUT
#% description: Name for output vector map (default: input)
#% required: no
#% guisection: Output
#%end
#%option
#% key: extents
#% type: string
#% options: input,region
#% answer: input
#% description: Ouput vector map extents
#% descriptions: input;extents of input map;region;extents of current region
#% guisection: Output
#%end
#%option
#% key: encoding
#% type: string
#% label: Encoding value for attribute data
#% descriptions: Overrides encoding interpretation, useful when importing ESRI Shapefile
#% guisection: Output
#%end
#%flag
#% key: f
#% description: List supported OGR formats and exit
#% suppress_required: yes
#%end
#%flag
#% key: l
#% description: List available OGR layers in data source and exit
#%end


import sys
import os
import shutil
import atexit
import math

import grass.script as grass

    
def cleanup():
    # remove temp location
    if tmploc:
        grass.try_rmdir(os.path.join(gisdbase, tmploc))
    if srcgisrc:
        grass.try_remove(srcgisrc)

def main():
    global tmploc, srcgisrc, gisdbase

    # initialize global vars
    tmploc = None
    srcgisrc = None
    gisdbase = None

    # list formats and exit
    if flags['f']:
        grass.run_command('v.in.ogr', flags='f')
        return 0

    # list layers and exit
    if flags['l']:
        grass.run_command('v.in.ogr', flags='l', input=options['input'])
        return 0
    
    OGRdatasource = options['input']
    output = options['output']
    layers = options['layer']
    
    vflags = None
    if options['extents'] == 'region':
        vflags = 'r'
    vopts = {}
    if options['encoding']:
        vopts['encoding'] = options['encoding']
    
    grassenv = grass.gisenv()
    tgtloc = grassenv['LOCATION_NAME']
    tgtmapset = grassenv['MAPSET']
    gisdbase = grassenv['GISDBASE']
    tgtgisrc = os.environ['GISRC']
    srcgisrc = grass.tempfile()
    
    tmploc = 'temp_import_location_' + str(os.getpid())

    f = open(srcgisrc, 'w')
    f.write('MAPSET: PERMANENT\n')
    f.write('GISDBASE: %s\n' % gisdbase)
    f.write('LOCATION_NAME: %s\n' % tmploc);
    f.write('GUI: text\n')
    f.close()

    tgtsrs = grass.read_command('g.proj', flags = 'j', quiet = True)

    # create temp location from input without import
    grass.message(_("Creating temporary location for <%s>...") % OGRdatasource)
    if layers:
        vopts['layer'] = layers
    if output:
        vopts['output'] = output
    returncode = grass.run_command('v.in.ogr', input = OGRdatasource,
                                   location = tmploc, flags = 'i', quiet = True, **vopts)
    # if it fails, return
    if returncode != 0:
        grass.fatal(_("Unable to create location from OGR datasource <%s>") % OGRdatasource)

    # switch to temp location
    os.environ['GISRC'] = str(srcgisrc)

    # compare source and target srs
    insrs = grass.read_command('g.proj', flags = 'j', quiet = True)

    # switch to target location
    os.environ['GISRC'] = str(tgtgisrc)

    if insrs == tgtsrs:
        # try v.in.ogr directly
        grass.message(_("Importing <%s>...") % OGRdatasource) 
        returncode = grass.run_command('v.in.ogr', input = OGRdatasource,
                                       flags = vflags, **vopts)
        # if it succeeds, return
        if returncode == 0:
            grass.message(_("Input <%s> successfully imported without reprojection") % OGRdatasource) 
            return 0
        else:
            grass.fatal(_("Unable to import <%s>") % OGRdatasource)
    
    # make sure target is not xy
    if grass.parse_command('g.proj', flags = 'g')['name'] == 'xy_location_unprojected':
        grass.fatal(_("Coordinate reference system not available for current location <%s>") % tgtloc)
    
    # switch to temp location
    os.environ['GISRC'] = str(srcgisrc)

    # make sure input is not xy
    if grass.parse_command('g.proj', flags = 'g')['name'] == 'xy_location_unprojected':
        grass.fatal(_("Coordinate reference system not available for input <%s>") % GDALdatasource)
    
    if options['extents'] == 'region':
        # switch to target location
        os.environ['GISRC'] = str(tgtgisrc)

        # v.in.region in tgt
        vreg = 'vreg_' + str(os.getpid())
        grass.run_command('v.in.region', output = vreg, quiet = True)

        # reproject to src
        # switch to temp location
        os.environ['GISRC'] = str(srcgisrc)
        returncode = grass.run_command('v.proj', input = vreg, output = vreg, 
                                       location = tgtloc, mapset = tgtmapset, quiet = True)
        
        if returncode != 0:
            grass.fatal(_("Unable to reproject to source location"))
        
        # set region from region vector
        grass.run_command('g.region', res = '1')
        grass.run_command('g.region', vector = vreg)


    # import into temp location
    grass.message(_("Importing <%s> ...") % OGRdatasource)
    returncode = grass.run_command('v.in.ogr', input = OGRdatasource,
                                   flags = vflags, **vopts)
    
    # if it fails, return
    if returncode != 0:
        grass.fatal(_("Unable to import OGR datasource <%s>") % OGRdatasource)

    # if output is not define check source mapset
    if not output:
        output = grass.list_grouped('vector')['PERMANENT'][0]
    
    # switch to target location
    os.environ['GISRC'] = str(tgtgisrc)

    # check if map exists
    if not grass.overwrite() and \
       grass.find_file(output, element='vector', mapset='.')['mapset']:
        grass.fatal(_("option <%s>: <%s> exists.") % ('output', output))
    
    if options['extents'] == 'region':
        grass.run_command('g.remove', type = 'vector', name = vreg,
                          flags = 'f', quiet = True)

    # v.proj
    grass.message(_("Reprojecting <%s>...") % output)
    returncode = grass.run_command('v.proj', location = tmploc,
                                   mapset = 'PERMANENT', input = output,
                                   quiet = True)
    if returncode != 0:
        grass.fatal(_("Unable to to reproject vector <%s>") % output)
    
    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    sys.exit(main())
