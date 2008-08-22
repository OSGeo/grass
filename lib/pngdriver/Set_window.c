
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

#include <math.h>
#include "pngdriver.h"

void PNG_Set_window(double t, double b, double l, double r)
{
    png.clip_top  = t > 0          ? t : 0;
    png.clip_bot  = b < png.height ? b : png.height;
    png.clip_left = l > 0          ? l : 0;
    png.clip_rite = r < png.width  ? r : png.width;
}

