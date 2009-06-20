/*
 * D_erase()
 *   Erases the window on scree.  Does not affect window contents list.
 */

#include <string.h>
#include <grass/colors.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/display_raster.h>

void D_erase(const char *color)
{
    double t, b, l, r;
    int colorindex;

    D_get_dst(&t, &b, &l, &r);

    /* Parse and select background color */
    colorindex = D_parse_color(color, 0);
    D_use_color(colorindex);

    /* Do the plotting */
    R__begin();
    R__move(l, b);
    R__cont(r, b);
    R__cont(r, t);
    R__cont(l, t);
    R__close();
    R__fill();
}

