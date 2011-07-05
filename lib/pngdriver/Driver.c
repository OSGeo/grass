
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

    drv.name = "png";
    drv.Box = PNG_Box;
    drv.Erase = PNG_Erase;
    drv.Graph_set = PNG_Graph_set;
    drv.Graph_close = PNG_Graph_close;
    drv.Line_width = PNG_Line_width;
    drv.Set_window = PNG_Set_window;
    drv.Begin_raster = PNG_begin_raster;
    drv.Raster = PNG_raster;
    drv.End_raster = NULL;
    drv.Begin = PNG_Begin;
    drv.Move = PNG_Move;
    drv.Cont = PNG_Cont;
    drv.Close = PNG_Close;
    drv.Stroke = PNG_Stroke;
    drv.Fill = PNG_Fill;
    drv.Point = PNG_Point;
    drv.Color = PNG_color_rgb;
    drv.Bitmap = PNG_draw_bitmap;
    drv.Text = NULL;
    drv.Text_box = NULL;
    drv.Set_font = NULL;
    drv.Font_list = NULL;
    drv.Font_info = NULL;

    initialized = 1;

    return &drv;
}
