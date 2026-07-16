/****************************************************************************
 *
 * MODULE:       r.distance
 *
 * AUTHOR(S):    Michael Shapiro - CERL
 *               Sort/reverse sort by distance by Huidae Cho
 *
 * PURPOSE:      Locates the closest points between objects in two
 *               raster maps.
 *
 * SPDX-FileCopyrightText: 2003-2014 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "defs.h"
#include <grass/gis.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct Parms parms;
    struct GModule *module;

    G_gisinit(argv[0]);

    /* Set description */
    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("distance"));
    module->description =
        _("Locates the closest points between objects in two raster maps.");

    parse(argc, argv, &parms);
    if (parms.labels) {
        read_labels(&parms.map1);
        read_labels(&parms.map2);
    }

    find_edge_cells(&parms.map1, parms.null);
    find_edge_cells(&parms.map2, parms.null);

    report(&parms);

    exit(EXIT_SUCCESS);
}

