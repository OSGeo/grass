
/****************************************************************************
 *
 * MODULE:       driver
 * AUTHOR(S):    Glynn Clements <glynn gclements.plus.com> (original contributor)
 *               Huidae Cho <grass4u gmail.com>
 * PURPOSE:      
 * COPYRIGHT:    (C) 2006-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <grass/config.h>

#include <stdio.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/freetypecap.h>
#include "driverlib.h"
#include "driver.h"
#include "pad.h"

const struct driver *driver;

struct GFONT_CAP *ftcap;

int NCOLORS;

int screen_left;
int screen_right;
int screen_bottom;
int screen_top;

int cur_x;
int cur_y;

double text_size_x;
double text_size_y;
double text_rotation;

int mouse_button[3] = { 1, 2, 3 };

int LIB_init(const struct driver *drv, int argc, char **argv)
{
    const char *p;

    driver = drv;
    ftcap = parse_freetypecap();

    /* initialize graphics */

    p = getenv("GRASS_WIDTH");
    screen_left = 0;
    screen_right = (p && atoi(p)) ? atoi(p) : DEF_WIDTH;

    p = getenv("GRASS_HEIGHT");
    screen_top = 0;
    screen_bottom = (p && atoi(p)) ? atoi(p) : DEF_HEIGHT;

    /* read mouse button setting */
    if ((p = getenv("GRASS_MOUSE_BUTTON"))) {
	int i;

	for (i = 0; i < 3 && p[i]; i++) {
	    if (p[i] < '1' || p[i] > '3')
		break;
	}
	if (i == 3 && p[0] != p[1] && p[1] != p[2] && p[0] != p[2]) {
	    for (i = 0; i < 3; i++)
		mouse_button[i] = p[i] - '0';
	}
    }

    if (COM_Graph_set(argc, argv) < 0)
	exit(1);

    /* initialize the pads */
    create_pad("");		/* scratch pad */

    return 0;
}
