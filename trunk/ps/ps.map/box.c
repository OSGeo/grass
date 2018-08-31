/* box functions
 **
 ** Author: Paul W. Carlson     March 1992
 */

#include "local_proto.h"

int box_path(double t, double b, double l, double r)
{
    fprintf(PS.fp, "%.1f %.1f %.1f %.1f B ", l, b, r, t);

    return 0;
}

int box_clip(double t, double b, double l, double r)
{
    box_path(t, b, l, r);
    fprintf(PS.fp, "clip newpath\n");

    return 0;
}

int box_fill(double t, double b, double l, double r, int color_number)
{
    set_rgb_color(color_number);
    box_path(t, b, l, r);
    fprintf(PS.fp, "F\n");

    return 0;
}

int box_draw(double t, double b, double l, double r)
{
    box_path(t, b, l, r);
    fprintf(PS.fp, "D\n");

    return 0;
}
