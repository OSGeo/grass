/****************************************************************************
 *
 * MODULE:       HTMLMAP
 * AUTHOR(S):    Glynn Clements <glynn gclements.plus.com> (original
 *                  contributor)
 *
 * PURPOSE:      driver to allow HTML image maps
 * SPDX-FileCopyrightText: 2007-2007 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
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

    drv.name = "html";
    drv.Box = HTML_Box;
    drv.Erase = NULL;
    drv.Graph_set = HTML_Graph_set;
    drv.Graph_close = HTML_Graph_close;
    drv.Graph_get_file = NULL;
    drv.Line_width = NULL;
    drv.Set_window = NULL;
    drv.Begin_raster = NULL;
    drv.Raster = NULL;
    drv.End_raster = NULL;
    drv.Begin = HTML_Begin;
    drv.Move = HTML_Move;
    drv.Cont = HTML_Cont;
    drv.Close = HTML_Close;
    drv.Stroke = HTML_Stroke;
    drv.Fill = HTML_Fill;
    drv.Point = NULL;
    drv.Color = NULL;
    drv.Bitmap = NULL;
    drv.Text = HTML_Text;
    drv.Text_box = NULL;
    drv.Set_font = NULL;
    drv.Font_list = NULL;
    drv.Font_info = NULL;

    initialized = 1;

    return &drv;
}

