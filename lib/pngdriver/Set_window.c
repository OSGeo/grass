
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

void PNG_Set_window(double ft, double fb, double fl, double fr)
{
    int t = (int) floor(ft + 0.5);
    int b = (int) floor(fb + 0.5);
    int l = (int) floor(fl + 0.5);
    int r = (int) floor(fr + 0.5);

    clip_top  = t > screen_top    ? t : screen_top;
    clip_bot  = b < screen_bottom ? b : screen_bottom;
    clip_left = l > screen_left   ? l : screen_left;
    clip_rite = r < screen_right  ? r : screen_right;
}
