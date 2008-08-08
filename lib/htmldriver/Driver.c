
/****************************************************************************
 *
 * MODULE:       HTMLMAP
 * AUTHOR(S):    Glynn Clements <glynn gclements.plus.com> (original contributor)
 *               
 * PURPOSE:      driver to allow HTML image maps
 * COPYRIGHT:    (C) 2007-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include "driver.h"
#include "htmlmap.h"

const struct driver *HTML_Driver(void)
{
    static struct driver drv;
    static int initialized;

    if (initialized)
	return &drv;

    drv.Box = NULL;
    drv.Erase = NULL;
    drv.Graph_set = HTML_Graph_set;
    drv.Graph_close = HTML_Graph_close;
    drv.Line_width = NULL;
    drv.Polydots = NULL;
    drv.Polyline = NULL;
    drv.Polygon = HTML_Polygon;
    drv.Set_window = NULL;
    drv.Begin_scaled_raster = NULL;
    drv.Scaled_raster = NULL;
    drv.End_scaled_raster = NULL;
    drv.Respond = NULL;
    drv.lookup_color = NULL;
    drv.color = NULL;
    drv.draw_line = NULL;
    drv.draw_point = NULL;
    drv.draw_bitmap = NULL;
    drv.draw_text = HTML_Text;

    initialized = 1;

    return &drv;
}
