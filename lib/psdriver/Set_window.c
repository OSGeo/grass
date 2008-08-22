
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

#include "psdriver.h"

void PS_Set_window(double t, double b, double l, double r)
{
    output("%.1f %.1f %.1f %.1f %s\n", t, b, l, r,
	   ps.encapsulated ? "EPSWINDOW" : "WINDOW");
}
