#!/usr/bin/env python3

############################################################################
#
# MODULE:       r.semantic.label
# AUTHOR(S):    Martin Landa <landa.martin gmail com>
#
# PURPOSE:      Manages semantic label information assigned to a single
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
# % description: Manages semantic label information assigned to a single raster map or to a list of raster maps.
# % keyword: general
# % keyword: imagery
# % keyword: semantic label
# % keyword: image collections
# %end
# %option G_OPT_R_MAPS
# %end
# %option
# % key: semantic_label
# % type: string
# % key_desc: name
# % description: Name of semantic label identifier (example: S2_1)
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


def print_map_semantic_label(name, label_reader):
    """Print semantic label information assigned to a single raster map

    :param str name: raster map name
    """
    from grass.pygrass.raster import RasterRow

    try:
        with RasterRow(name) as rast:
            semantic_label = rast.info.semantic_label
            if semantic_label:
                label_reader.print_info(semantic_label=semantic_label)
            else:
                gs.info(_("No semantic label assigned to <{}>").format(name))
    except OpenError:
        gs.error(_("Map <{}> not found").format(name))


def manage_map_semantic_label(name, semantic_label):
    """Manage semantic label assigned to a single raster map

    :param str name: raster map name
    :param str semantic_label: semantic label (None for dissociating semantic label)

    :return int: return code
    """
    from grass.pygrass.raster import RasterRow

    try:
        with RasterRow(name) as rast:
            if semantic_label:
                gs.debug(
                    _("Semantic label <{}> assigned to raster map <{}>").format(
                        semantic_label, name
                    ),
                    1,
                )
            else:
                gs.debug(
                    _("Semantic label dissociated from raster map <{}>").format(name), 1
                )
            try:
                rast.info.semantic_label = semantic_label
            except GrassError as e:
                gs.error(_("Unable to assign/dissociate semantic label. {}").format(e))
                return 1
    except OpenError:
        gs.error(_("Map <{}> not found in current mapset").format(name))
        return 1

    return 0


def main():
    maps = options["map"].split(",")
    if options["operation"] == "add":
        if not options["semantic_label"]:
            gs.fatal(
                _("Operation {}: required parameter <{}> not set").format(
                    options["operation"], "semantic_label"
                )
            )
        semantic_labels = options["semantic_label"].split(",")
        if len(semantic_labels) > 1 and len(semantic_labels) != len(maps):
            gs.fatal(_("Number of maps differs from number of semantic labels"))
    else:
        semantic_labels = [None]

    if options["operation"] == "print":
        from grass.semantic_label import SemanticLabelReader

        label_reader = SemanticLabelReader()
    else:
        label_reader = None
    multi_labels = len(semantic_labels) > 1
    ret = 0
    for i in range(len(maps)):
        semantic_label = semantic_labels[i] if multi_labels else semantic_labels[0]
        if options["operation"] == "print":
            print_map_semantic_label(maps[i], label_reader)
        elif manage_map_semantic_label(maps[i], semantic_label) != 0:
            ret = 1

    return ret


if __name__ == "__main__":
    options, flags = gs.parser()

    sys.exit(main())
