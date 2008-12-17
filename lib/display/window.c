/*
 * D_erase()
 *   Erases the window on scree.  Does not affect window contents list.
 */

#include <string.h>
#include <grass/colors.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>

void D_erase(const char *color)
{
    double t, b, l, r;
    int colorindex;

    D_get_dst(&t, &b, &l, &r);

    /* Parse and select background color */
    colorindex = D_parse_color(color, 0);
    D_use_color(colorindex);

    /* Do the plotting */
    R_begin();
    R_move(l, b);
    R_cont(r, b);
    R_cont(r, t);
    R_cont(l, t);
    R_close();
    R_fill();
}

