/* Header file: colortable.h
 **
 ** Author: Paul W. Carlson     April 1992
 */

#include <stdio.h>
#include "clr.h"

struct colortable
{
    double x, y;	/* where */
    double width;	/* column/table width */
    double min, max;	/* range */
    double height;	/* fp legend height */
    double lwidth;	/* line width for columns and tickbars */
    char *font;
    char *name;
    const char *mapset;
    int fontsize;
    PSCOLOR color;	/* text color, it seems */
    int cols;		/* number of columns */
    int nodata;
    int tickbar;
    int discrete;	/* force discrete bands or continuous gradient */
    int range_override;
};

extern struct colortable ct;
