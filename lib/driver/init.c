
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

const struct driver *driver;

struct GFONT_CAP *ftcap;

int screen_width;
int screen_height;

double cur_x;
double cur_y;

double text_size_x;
double text_size_y;
double text_rotation;
int matrix_valid;

void LIB_init(const struct driver *drv)
{
    const char *p;

    driver = drv;
    ftcap = parse_freetypecap();

    /* initialize graphics */

    p = getenv("GRASS_WIDTH");
    screen_width = (p && atoi(p)) ? atoi(p) : DEF_WIDTH;

    p = getenv("GRASS_HEIGHT");
    screen_height = (p && atoi(p)) ? atoi(p) : DEF_HEIGHT;

    if (COM_Graph_set() < 0)
	exit(1);

    COM_Set_window(0, screen_height, 0, screen_width);
}
