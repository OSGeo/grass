#!/usr/bin/env python

############################################################################
#
# MODULE:       i.band
# AUTHOR(S):    Martin Landa <landa.martin gmail com>
#
# PURPOSE:      Manages band reference information to a single raster map or
#               a list of raster maps
#
# COPYRIGHT:    (C) 2019 by mundialis GmbH & Co.KG, and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

#%module
#% description: Manages band reference information to a single raster map or a list of raster maps.
#% keyword: general
#% keyword: imagery
#% keyword: image collections
#% keyword: bands
#%end
#%option G_OPT_R_MAPS
#%end
#%option
#% key: band
#% type: string
#% key_desc: name
#% description: Name of band reference identifier (example: S2A_1)
#% required: no
#% multiple: yes
#%end
#%flag
#% key: r
#% description: Dissociate band reference from a raster map(s) (overrides other options)
#%end
#%flag
#% key: p
#% description: Print detailed band reference information assigned to a raster map(s)
#%end
#%rules
#% required:-p,-r,band
#%end

import sys

import grass.script as gs
from grass.pygrass.raster import RasterRow
from grass.exceptions import GrassError, OpenError

def print_map_band_reference(name, band_reader):
    """Print band reference information assigned to a single raster map

    :param str name: raster map name
    """
    try:
        with RasterRow(name) as rast:
            band_ref = rast.info.band_reference
            if band_ref:
                shortcut, band = band_ref.split('_')
                band_reader.print_info(shortcut, band)
    except OpenError as e:
        gs.error(_("Map <{}> not found").format(name))

def manage_map_band_reference(name, band_ref):
    """Manage band reference assigned to a single raster map

    :param str name: raster map name
    :param str band_ref: band reference (None for dissociating band reference)
    """
    try:
        with RasterRow(name) as rast:
            if band_ref:
                gs.debug(_("Band reference <{}> assigned to raster map <{}>").format(
                    band_ref, name), 1)
            else:
                gs.debug(_("Band reference dissociated from raster map <{}>").format(
                    name), 1)
            try:
                rast.info.band_reference = band_ref
            except GrassError as e:
                gs.error(_("Unable to assign/dissociate band reference. {}").format(e))
    except OpenError as e:
        gs.error(_("Map <{}> not found in current mapset").format(name))

def main():
    gs.utils.set_path('g.bands')
    from reader import BandReader

    maps = options['map'].split(',')
    if not flags['r']:
        bands = options['band'].split(',')
        if len(bands) > 1 and len(bands) != len(maps):
            gs.fatal(_("Number of maps differs from number of bands"))
    else:
        bands = [None]

    if flags['p']:
        gs.utils.set_path('g.bands')
        from reader import BandReader
        band_reader = BandReader()
    else:
        band_reader = None
    multi_bands = len(bands) > 1
    for i in range(len(maps)):
        band_ref = bands[i] if multi_bands else bands[0]
        if flags['p']:
            print_map_band_reference(maps[i], band_reader)
        else:
            manage_map_band_reference(maps[i], band_ref)

if __name__ == "__main__":
    options, flags = gs.parser()

    sys.exit(
        main()
    )
