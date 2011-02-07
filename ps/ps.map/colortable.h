/* Header file: colortable.h
 **
 ** Author: Paul W. Carlson     April 1992
 */

#include <stdio.h>
#include "clr.h"

struct colortable
{
    double x, y, width;
    double min, max;
    double height;	/* fp legend height */
    char *font;
    char *name;
    const char *mapset;
    int fontsize;
    PSCOLOR color;
    int cols;
    int nodata;
    int tickbar;
    int discrete;	/* force discrete bands or continuous gradient */
    int range_override;
};

extern struct colortable ct;
