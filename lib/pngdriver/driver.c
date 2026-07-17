/*!
   \file lib/pngdriver/driver.c

   \brief GRASS png display driver - driver initialization

   SPDX-FileCopyrightText: 2007-2014 Glynn Clements
   SPDX-FileCopyrightText: Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

   \author Glynn Clements
 */

#include "pngdriver.h"

/*!
   \brief Initialize display driver

   \return pointer driver structure
 */
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
    drv.Graph_get_file = PNG_Graph_get_file;
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
