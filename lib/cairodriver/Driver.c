
/****************************************************************************
 *
 * MODULE:       Cairo driver
 * AUTHOR(S):    Lars Ahlzen <lars@ahlzen.com>
 * COPYRIGHT:    (C) 2007 Lars Ahlzen
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

#include "cairodriver.h"

const struct driver *Cairo_Driver(void)
{
    static struct driver drv;
    static int initialized;

    if (initialized)
	return &drv;

    drv.Box = Cairo_Box;
    drv.Erase = Cairo_Erase;
    drv.Graph_set = Cairo_Graph_set;
    drv.Graph_close = Cairo_Graph_close;
    drv.Line_width = Cairo_Line_width;
    drv.Polydots = Cairo_Polydots;
    drv.Polyline = Cairo_Polyline;
    drv.Polygon = Cairo_Polygon;
    drv.Set_window = Cairo_Set_window;
    drv.Begin_scaled_raster = Cairo_begin_scaled_raster;
    drv.Scaled_raster = Cairo_scaled_raster;
    drv.End_scaled_raster = Cairo_end_scaled_raster;
    drv.Respond = Cairo_Respond;
    drv.lookup_color = Cairo_lookup_color;
    drv.color = Cairo_color;
    drv.draw_line = Cairo_draw_line;
    drv.draw_point = Cairo_draw_point;
    drv.draw_bitmap = Cairo_draw_bitmap;
    drv.draw_text = Cairo_draw_text;
    drv.text_box = Cairo_text_box;
    drv.Set_font = Cairo_set_font;
    drv.Font_list = Cairo_font_list;
    drv.Font_info = Cairo_font_info;

    initialized = 1;

    return &drv;
}
