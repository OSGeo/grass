/*!
  \file lib/cairodriver/draw.c

  \brief GRASS cairo display driver

  (C) 2007-2008 by Lars Ahlzen, Glynn Clements and the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Lars Ahlzen <lars ahlzen.com> (original contibutor)
  \author Glynn Clements  
*/

#include "cairodriver.h"

void Cairo_Begin(void)
{
    cairo_new_path(cairo);
}

void Cairo_Move(double x, double y)
{
    cairo_move_to(cairo, x, y);
}

void Cairo_Cont(double x, double y)
{
    cairo_line_to(cairo, x, y);
}

void Cairo_Close(void)
{
    cairo_close_path(cairo);
}

void Cairo_Stroke(void)
{
    cairo_stroke(cairo);
    ca.modified = 1;
}

void Cairo_Fill(void)
{
    cairo_fill(cairo);
    ca.modified = 1;
}

void Cairo_Point(double x, double y)
{
    static double point_size = 1.0;
    double half_point_size = point_size / 2;

    cairo_new_path(cairo);
    cairo_rectangle(cairo,
		    x - half_point_size, y - half_point_size,
		    point_size, point_size);
    cairo_fill(cairo);
    ca.modified = 1;
}

