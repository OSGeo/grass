/* Header file: decorate.h
 **
 */

#include <stdio.h>

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
};

extern struct scalebar sb;

