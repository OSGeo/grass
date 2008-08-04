
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

    drv.Box_abs = Cairo_Box_abs;
    drv.Box_rel = NULL;
    drv.Client_Open = NULL;
    drv.Client_Close = Cairo_Client_Close;
    drv.Erase = Cairo_Erase;
    drv.Get_with_box = NULL;
    drv.Get_with_line = NULL;
    drv.Get_with_pointer = NULL;
    drv.Graph_set = Cairo_Graph_set;
    drv.Graph_close = Cairo_Graph_close;
    drv.Line_width = Cairo_Line_width;
    drv.Panel_save = NULL;
    drv.Panel_restore = NULL;
    drv.Panel_delete = NULL;
    drv.Polydots_abs = NULL;
    drv.Polydots_rel = NULL;
    drv.Polyline_abs = Cairo_Polyline_abs;
    drv.Polyline_rel = NULL;
    drv.Polygon_abs = Cairo_Polygon_abs;
    drv.Polygon_rel = NULL;
    drv.Set_window = Cairo_Set_window;
    drv.Begin_scaled_raster = Cairo_begin_scaled_raster;
    drv.Scaled_raster = Cairo_scaled_raster;
    drv.End_scaled_raster = Cairo_end_scaled_raster;
    drv.Respond = Cairo_Respond;
    drv.Work_stream = NULL;
    drv.Do_work = NULL;
    drv.lookup_color = Cairo_lookup_color;
    drv.color = Cairo_color;
    drv.draw_line = Cairo_draw_line;
    drv.draw_point = Cairo_draw_point;
    drv.draw_bitmap = Cairo_draw_bitmap;
    drv.draw_text = NULL;

    initialized = 1;

    return &drv;
}
