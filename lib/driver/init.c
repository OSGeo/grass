/*!
  \file lib/driver/init.c

  \brief Display Driver - initialization

  (C) 2006-2011 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.

  \author Glynn Clements <glynn gclements.plus.com> (original contributor)
  \author Huidae Cho <grass4u gmail.com>
*/

#include <grass/config.h>

#include <stdio.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/fontcap.h>
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
double text_sinrot;
double text_cosrot;
int matrix_valid;

/*!
  \brief Initialize display driver

  \param drv pointer to driver structure
*/
void LIB_init(const struct driver *drv)
{
    const char *p;

    driver = drv;
    ftcap = parse_fontcap();

    /* initialize graphics */
    p = getenv("GRASS_RENDER_WIDTH");
    screen_width = (p && atoi(p)) ? atoi(p) : DEF_WIDTH;

    p = getenv("GRASS_RENDER_HEIGHT");
    screen_height = (p && atoi(p)) ? atoi(p) : DEF_HEIGHT;

    if (COM_Graph_set() < 0)
	exit(1);

    COM_Set_window(0, screen_height, 0, screen_width);
}
