/* Header file: colortable.h
 **
 ** Author: Paul W. Carlson     April 1992
 */

#include <stdio.h>

struct colortable
{
    double x, y, width;
    double min, max;
    double height;		/* fp legend height */
    char *font;
    char *name;
    char *mapset;
    int fontsize;
    int color;
    int cols;
    int nodata;
    int tickbar;
    int range_override;
};

extern struct colortable ct;
