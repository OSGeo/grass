/* Set the GC foreground value to the number passed to color. All
 * subsequent graphics calls will use this number, hence they will be
 * drawn in that color's number.
 * 
 * Called by: Color() in ../lib/Color.c */

#include <stdio.h>
#include "includes.h"
#include <grass/colors.h>
#include <grass/gis.h>
#include "XDRIVER.h"

int current_color;

void XD_color(int number)
{
    if (number >= NCOLORS || number < 0) {
	G_warning("Color: can't set color %d\n", number);
	return;
    }

    current_color = number;

    if (use_visual->class >= TrueColor)
	current_color = number;
    else
	current_color = xpixels[number];

    XSetForeground(dpy, gc, current_color);
}
