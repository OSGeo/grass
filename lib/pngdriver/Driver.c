
/****************************************************************************
 *
 * MODULE:       PNG driver
 * AUTHOR(S):    Glynn Clements <glynn@gclements.plus.com>
 * COPYRIGHT:    (C) 2007 Glynn Clements
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *****************************************************************************/

#include "pngdriver.h"

const struct driver *PNG_Driver(void)
{
    static struct driver drv;
    static int initialized;

    if (initialized)
	return &drv;

    drv.Box = PNG_Box;
    drv.Erase = PNG_Erase;
    drv.Graph_set = PNG_Graph_set;
    drv.Graph_close = PNG_Graph_close;
    drv.Line_width = PNG_Line_width;
    drv.Polydots = NULL;
    drv.Polyline = NULL;
    drv.Polygon = NULL;
    drv.Set_window = PNG_Set_window;
    drv.Begin_scaled_raster = PNG_begin_scaled_raster;
    drv.Scaled_raster = PNG_scaled_raster;
    drv.End_scaled_raster = NULL;
    drv.Respond = PNG_Respond;
    drv.lookup_color = PNG_lookup_color;
    drv.color = PNG_color;
    drv.draw_line = PNG_draw_line;
    drv.draw_point = PNG_draw_point;
    drv.draw_bitmap = PNG_draw_bitmap;
    drv.draw_text = NULL;
    drv.text_box = NULL;
    drv.Set_font = NULL;
    drv.Font_list = NULL;
    drv.Font_info = NULL;

    initialized = 1;

    return &drv;
}
