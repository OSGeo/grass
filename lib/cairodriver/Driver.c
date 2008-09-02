/*!
  \file cairodriver/Driver.c

  \brief GRASS cairo display driver - driver initialization

  (C) 2007-2008 by Lars Ahlzen and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/

#include "cairodriver.h"

/*!
  \brief Initialize display driver

  \return pointer driver structure
*/
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
    drv.color_rgb = Cairo_color_rgb;
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
