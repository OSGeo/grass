/* Header file: decorate.h
 **
 */

#include <stdio.h>


/* units options */
#define SB_UNITS_AUTO	0
#define SB_UNITS_METERS	1
#define SB_UNITS_KM	2
#define SB_UNITS_FEET	3
#define SB_UNITS_MILES	4
#define SB_UNITS_NMILES	5

struct scalebar
{
    char type[50];
    double x, y;
    double length, height;
    char *font;
    int segment;
    int numbers;
    double width;
    int fontsize;
    int color, bgcolor;
    int units;
};

extern struct scalebar sb;

