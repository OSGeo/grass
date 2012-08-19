/* Function: show_scale
 **
 ** Author: Paul W. Carlson     April 1992
 */
#include "ps_info.h"

#define DY		10.0
#define FONTSIZE	10.0

int show_scale(void)
{
    double fontsize;

    /* TODO: add font name, size, color options */
    fontsize = FONTSIZE;
    fprintf(PS.fp, "(Helvetica) FN %.1lf SF\n", fontsize);
    fprintf(PS.fp, "(SCALE:   %s)\n", PS.scaletext);
    fprintf(PS.fp, "%.1lf %.1lf MS\n", PS.map_left, PS.min_y - DY);
    PS.min_y -= DY;

    return 0;
}
