/*
 ****************************************************************************
 *
 * MODULE:       v.transform
 * AUTHOR(S):    See other files as well...
 *               Eric G. Miller <egm2@jps.net>
 * PURPOSE:      To transform a vector layer's coordinates via a set of tie
 *               points.
 * COPYRIGHT:    (C) 2002 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/

/*
 *  create_transform_conversion () - main driver routine to prepare
 *    the transformation equation.
 *
 *  Written by the GRASS Team, 02/16/90, -mh.
 */

#include <stdio.h>
#include <stdlib.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include "trans.h"
#include "local_proto.h"

int create_transform_from_file(struct file_info *Coord, int quiet)
{
    int status;
    int n_points;

    init_transform_arrays();

    n_points = 0;
    /*  Get the coordinates from the file.  */
    if ((n_points = get_coor_from_file(Coord->fp)) < 0)
	exit(-1);

    status = setup_transform(n_points);

    if (status != ALL_OK) {
	G_message(_("Number of points that have been entered [%d]"),
		  n_points);
	print_transform_error(status);
	exit(-1);
    }

    if (!quiet)
	print_transform_resids(n_points);

    return (0);

}				/*  create_transform_from_file()  */
