/*
 ****************************************************************************
 *
 * MODULE:       v.transform
 * AUTHOR(S):    See other files as well...
 *               Eric G. Miller <egm2@jps.net>
 * PURPOSE:      Read all the registration (map) coordinates in from the file
 * COPYRIGHT:    (C) 2002 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include "trans.h"
#include <grass/gis.h>
#include <grass/glocale.h>

int get_coor_from_file(FILE * fp)
{

    int i;
    char buff[128];

    for (i = 0; i < MAX_COOR; i++) {

	if (fgets(buff, sizeof(buff), fp) == NULL)
	    break;

	if (sscanf(buff, "%lf %lf %lf %lf", &ax[i], &ay[i], &bx[i], &by[i]) !=
	    4) {
	    /* comment or illegal line found */
	    if (!buff[0] == '#')
		G_fatal_error(_("Reading coordinates from file."));
	    else
		i--;		/* just comment found */
	}
	use[i] = 1;

    }				/*  for (i)  */

    return (i);

}				/*    get_coor_from_file ()   */
