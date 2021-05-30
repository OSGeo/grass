#!/usr/bin/env python3

############################################################################
#
# MODULE:       i.band
# AUTHOR(S):    Martin Landa <landa.martin gmail com>
#
# PURPOSE:      Manages band reference information assigned to a single
#               raster map or to a list of raster maps.
#
# COPYRIGHT:    (C) 2019 by mundialis GmbH & Co.KG, and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

# %module
# % description: Manages band reference information assigned to a single raster map or to a list of raster maps.
# % keyword: general
# % keyword: imagery
# % keyword: band reference
# % keyword: image collections
# %end
# %option G_OPT_R_MAPS
# %end
# %option
# % key: band
# % type: string
# % key_desc: name
# % description: Name of band reference identifier (example: S2_1)
# % required: no
# % multiple: yes
# %end
# %option
# % key: operation
# % type: string
# % required: yes
# % multiple: no
# % options: add,remove,print
# % description: Operation to be performed
# % answer: add

import sys

import grass.script as gs
from grass.exceptions import GrassError, OpenError


def print_map_band_reference(name, band_reader):
    """Print band reference information assigned to a single raster map

    :param str name: raster map name
    """
    from grass.pygrass.raster import RasterRow

    try:
        with RasterRow(name) as rast:
            band_ref = rast.info.band_reference
            if band_ref:
                shortcut, band = band_ref.split("_")
                band_reader.print_info(shortcut, band)
            else:
                gs.info(_("No band reference assigned to <{}>").format(name))
                # code changed here noqa F841 added
    except OpenError as e:  # noqa: F841
        gs.error(_("Map <{}> not found").format(name))


def manage_map_band_reference(name, band_ref):
    """Manage band reference assigned to a single raster map

    :param str name: raster map name
    :param str band_ref: band reference (None for dissociating band reference)

    :return int: return code
    """
    from grass.pygrass.raster import RasterRow

    try:
        with RasterRow(name) as rast:
            if band_ref:
                gs.debug(
                    _("Band reference <{}> assigned to raster map <{}>").format(
                        band_ref, name
                    ),
                    1,
                )
            else:
                gs.debug(
                    _("Band reference dissociated from raster map <{}>").format(name), 1
                )
            try:
                rast.info.band_reference = band_ref
            except GrassError as e:
                gs.error(_("Unable to assign/dissociate band reference. {}").format(e))
                return 1
                # code changed here noqa F841 added
    except OpenError as e:  # noqa: F841
        gs.error(_("Map <{}> not found in current mapset").format(name))
        return 1

    return 0


def main():
    maps = options["map"].split(",")
    if options["operation"] == "add":
        if not options["band"]:
            gs.fatal(
                _("Operation {}: required parameter <{}> not set").format(
                    options["operation"], "band"
                )
            )
        bands = options["band"].split(",")
        if len(bands) > 1 and len(bands) != len(maps):
            gs.fatal(_("Number of maps differs from number of bands"))
    else:
        bands = [None]

    if options["operation"] == "print":
        from grass.bandref import BandReferenceReader

        band_reader = BandReferenceReader()
    else:
        band_reader = None
    multi_bands = len(bands) > 1
    ret = 0
    for i in range(len(maps)):
        band_ref = bands[i] if multi_bands else bands[0]
        if options["operation"] == "print":
            print_map_band_reference(maps[i], band_reader)
        else:
            if manage_map_band_reference(maps[i], band_ref) != 0:
                ret = 1

    return ret


if __name__ == "__main__":
    options, flags = gs.parser()

    sys.exit(main())
