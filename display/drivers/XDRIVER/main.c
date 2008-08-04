
/****************************************************************************
 *
 * MODULE:       XDRIVER
 * AUTHOR(S):    various authors at CERL (original contributor)
 *               Glynn Clements <glynn gclements.plus.com> 
 *               Huidae Cho <grass4u gmail.com>
 * PURPOSE:      display driver for displaying in X11 environment
 * COPYRIGHT:    (C) 2006-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include "XDRIVER.h"

int main(int argc, char **argv)
{
    struct driver drv;

    drv.Box_abs = XD_Box_abs;
    drv.Box_rel = NULL;
    drv.Client_Open = XD_Client_Open;
    drv.Client_Close = XD_Client_Close;
    drv.Erase = NULL;
    drv.Get_with_box = XD_Get_location_with_box;
    drv.Get_with_line = XD_Get_location_with_line;
    drv.Get_with_pointer = XD_Get_location_with_pointer;
    drv.Graph_set = XD_Graph_set;
    drv.Graph_close = XD_Graph_close;
    drv.Line_width = XD_Line_width;
    drv.Panel_save = XD_Panel_save;
    drv.Panel_restore = XD_Panel_restore;
    drv.Panel_delete = XD_Panel_delete;
    drv.Polydots_abs = XD_Polydots_abs;
    drv.Polydots_rel = XD_Polydots_rel;
    drv.Polyline_abs = XD_Polyline_abs;
    drv.Polyline_rel = XD_Polyline_rel;
    drv.Polygon_abs = XD_Polygon_abs;
    drv.Polygon_rel = XD_Polygon_rel;
    drv.Set_window = XD_Set_window;
    drv.Begin_scaled_raster = XD_begin_scaled_raster;
    drv.Scaled_raster = XD_scaled_raster;
    drv.End_scaled_raster = NULL;
    drv.Respond = XD_Respond;
    drv.Work_stream = XD_Work_stream;
    drv.Do_work = XD_Do_work;
    drv.lookup_color = XD_lookup_color;
    drv.color = XD_color;
    drv.draw_line = XD_draw_line;
    drv.draw_point = XD_draw_point;
    drv.draw_bitmap = XD_draw_bitmap;
    drv.draw_text = NULL;

    LIB_init(&drv, argc, argv);

    return LIB_main(argc, argv);
}
