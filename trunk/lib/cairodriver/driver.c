/*!
  \file lib/cairodriver/driver.c

  \brief GRASS cairo display driver - driver initialization

  (C) 2007-2014 by Lars Ahlzen and the GRASS Development Team
  
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

    drv.name = "cairo";
    drv.Box = Cairo_Box;
    drv.Erase = Cairo_Erase;
    drv.Graph_set = Cairo_Graph_set;
    drv.Graph_get_file = Cairo_Graph_get_file;
    drv.Graph_close = Cairo_Graph_close;
    drv.Line_width = Cairo_Line_width;
    drv.Set_window = Cairo_Set_window;
    drv.Begin_raster = Cairo_begin_raster;
    drv.Raster = Cairo_raster;
    drv.End_raster = Cairo_end_raster;
    drv.Begin = Cairo_Begin;
    drv.Move = Cairo_Move;
    drv.Cont = Cairo_Cont;
    drv.Close = Cairo_Close;
    drv.Stroke = Cairo_Stroke;
    drv.Fill = Cairo_Fill;
    drv.Point = Cairo_Point;
    drv.Color = Cairo_Color;
    drv.Bitmap = Cairo_Bitmap;
    drv.Text = Cairo_Text;
    drv.Text_box = Cairo_text_box;
    drv.Set_font = Cairo_set_font;
    drv.Font_list = Cairo_font_list;
    drv.Font_info = Cairo_font_info;
      
    initialized = 1;

    return &drv;
}
