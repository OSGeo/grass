/* Header file: comment.h
 **
 ** Author: Paul W. Carlson     April 1992
 */

#include <stdio.h>

struct comment
{
    double x, y;
    char *font;
    int fontsize;
    int color;
};

extern struct comment cmt;
