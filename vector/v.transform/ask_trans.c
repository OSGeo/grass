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
 *
 *  ask_transform_coor (n_points) -
 *   Uses Vask to get the sets of coordinates from the user.
 *
 *  shrink_map_coor()  -  condense the arrays used for the transform
 *  library, also turns on the use[] if the coordinate set if valid.
 *
 *  Written by the GRASS Team, 02/16/90, -mh .
 */

#ifndef __MINGW32__
#include <unistd.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/vask.h>
#include <grass/glocale.h>
#include "trans.h"

static int shrink_map_coor(void);

int ask_transform_coor(int n_points)
{
    int i;
    int coor_cnt;
    int at_line;
    int at_point[MAX_COOR];

    /*  at_point must be an array for Vask  */

    char tmp[82];


    /*  number of coordinates we can handle.  this may be the second time
     *   to this menu and some points may have been registered
     */

    coor_cnt = MAX_COOR - reg_cnt;
    V_clear();

    V_line(1, "                               MAP REGISTRATION");
    V_line(2,
	   " ------------------------------------------------------------------------");
    V_line(3,
	   "|             |        Existing Map          |           New Map         |");
    V_line(4,
	   "|   Point #   |   X coord        Y coord     |   X coord        Y coord  |");
    V_line(5,
	   " ------------------------------------------------------------------------");

    for (i = 0; i < MAX_COOR; i++) {
	at_line = i + 7;
	at_point[i] = i + 1;

	V_const(&at_point[i], 'i', at_line, 6, 6);
	V_ques(&ax[i], 'd', at_line, 15, 12);
	V_ques(&ay[i], 'd', at_line, 30, 12);
	V_ques(&bx[i], 'd', at_line, 45, 12);
	V_ques(&by[i], 'd', at_line, 60, 12);
    }

    /*  show min needed and max they can go to  */
    sprintf(tmp, "    Enter %d to %d points.  Current number of points: %d",
	    (MIN_COOR > reg_cnt) ? MIN_COOR - reg_cnt : 0, coor_cnt,
	    n_points);
    V_line(at_line + 3, tmp);

    V_intrpt_ok();

    /* add message before exit */
    if (!V_call()) {
	V_exit();
	G_message(_("ask_transform_coor():  Leaving session.. \n"));
	G_sleep(2);
	return (-1);
    }


    return (shrink_map_coor());
}

/* 
 *  Condense the arrays and update use[].
 */


static int shrink_map_coor(void)
{

    int i, k;

    for (i = 0, k = 0; i < MAX_COOR; i++) {
	if (ax[i] == 0.0 || ay[i] == 0.0 || bx[i] == 0.0 || by[i] == 0.0)
	    continue;
	use[i] = 1;

	/*  same place count it, but skip it  */
	if (i == k) {
	    ++k;
	    continue;
	}

	/*  valid point store them  */
	*(bx + k) = *(bx + i);
	*(by + k) = *(by + i);
	*(ax + k) = *(ax + i);
	*(ay + k) = *(ay + i);
	*(use + k) = *(use + i);
	*(residuals + k) = *(residuals + i);
	++k;
    }

    /*  now make sure everything else is zero'ed out  */
    i = (k <= 0) ? 0 : k;
    for (; i < MAX_COOR; i++) {

	*(bx + i) = 0.0;
	*(by + i) = 0.0;
	*(ax + i) = 0.0;
	*(ay + i) = 0.0;
	*(use + i) = 0;
	*(residuals + i) = 0.0;
    }

    return (k);

}
#endif /* __MINGW32__ */
