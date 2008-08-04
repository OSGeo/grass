
/****************************************************************************
 *
 * MODULE:       PS driver
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

#include "psdriver.h"

const struct driver *PS_Driver(void)
{
    static struct driver drv;
    static int initialized;

    if (initialized)
	return &drv;

    drv.Box_abs = PS_Box_abs;
    drv.Box_rel = NULL;
    drv.Client_Open = NULL;
    drv.Client_Close = PS_Client_Close;
    drv.Erase = PS_Erase;
    drv.Get_with_box = NULL;
    drv.Get_with_line = NULL;
    drv.Get_with_pointer = NULL;
    drv.Graph_set = PS_Graph_set;
    drv.Graph_close = PS_Graph_close;
    drv.Line_width = PS_Line_width;
    drv.Panel_save = NULL;
    drv.Panel_restore = NULL;
    drv.Panel_delete = NULL;
    drv.Polydots_abs = NULL;
    drv.Polydots_rel = NULL;
    drv.Polyline_abs = PS_Polyline_abs;
    drv.Polyline_rel = NULL;
    drv.Polygon_abs = PS_Polygon_abs;
    drv.Polygon_rel = NULL;
    drv.Set_window = PS_Set_window;
    drv.Begin_scaled_raster = PS_begin_scaled_raster;
    drv.Scaled_raster = PS_scaled_raster;
    drv.End_scaled_raster = PS_end_scaled_raster;
    drv.Respond = PS_Respond;
    drv.Work_stream = NULL;
    drv.Do_work = NULL;
    drv.lookup_color = PS_lookup_color;
    drv.color = PS_color;
    drv.draw_line = PS_draw_line;
    drv.draw_point = PS_draw_point;
    drv.draw_bitmap = PS_draw_bitmap;
    drv.draw_text = NULL;

    initialized = 1;

    return &drv;
}
