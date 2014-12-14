
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

    drv.name = "ps";
    drv.Box = PS_Box;
    drv.Erase = PS_Erase;
    drv.Graph_set = PS_Graph_set;
    drv.Graph_close = PS_Graph_close;
    drv.Graph_get_file = PS_Graph_get_file;
    drv.Line_width = PS_Line_width;
    drv.Set_window = PS_Set_window;
    drv.Begin_raster = PS_begin_raster;
    drv.Raster = PS_raster;
    drv.End_raster = PS_end_raster;
    drv.Begin = PS_Begin;
    drv.Move = PS_Move;
    drv.Cont = PS_Cont;
    drv.Close = PS_Close;
    drv.Stroke = PS_Stroke;
    drv.Fill = PS_Fill;
    drv.Point = PS_Point;
    drv.Color = PS_Color;
    drv.Bitmap = PS_Bitmap;
    drv.Text = NULL;
    drv.Text_box = NULL;
    drv.Set_font = NULL;
    drv.Font_list = NULL;
    drv.Font_info = NULL;

    initialized = 1;

    return &drv;
}
