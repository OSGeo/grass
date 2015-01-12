
/****************************************************************************
 *
 * MODULE:       r.carve
 *
 * AUTHOR(S):    Original author Bill Brown, UIUC GIS Laboratory
 *               Brad Douglas <rez touchofmadness com>
 *
 * PURPOSE:      Takes vector stream data, converts it to 3D raster and
 *               subtracts a specified depth
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
****************************************************************************/

#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "enforce.h"


/*
 * open_new_vect - opens new vector map for writing
 */
int open_new_vect(struct Map_info *map, char *vect)
{
    if (Vect_open_new(map, vect, 1) < 0)
	G_fatal_error(_("Unable to create vector map <%s>"), vect);

    Vect_set_map_name(map, vect);
    Vect_set_comment(map, G_recreate_command());
    Vect_hist_command(map);

    return 1;
}


/*
 * close_vect - builds vector support and frees up resources
 */
int close_vect(struct Map_info *map, const int build_support)
{
    if (build_support)
	Vect_build(map);

    Vect_set_release_support(map);
    Vect_close(map);

    return 1;
}
