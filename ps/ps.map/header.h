/* Header file: header.h
 **
 ** Author: Paul W. Carlson     April 1992
 */

#include <stdio.h>
#include "clr.h"

struct header
{
    char *file;
    char *font;
    int fontsize;
    PSCOLOR color;
    FILE *fp;
};

extern struct header hdr;
