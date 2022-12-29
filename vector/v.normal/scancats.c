/*
 * Copyright (C) 1994-1995. James Darrell McCauley. (darrell@mccauley-usa.com)
 *                                http://mccauley-usa.com/
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */

#include<stdio.h>
int scan_cats(char *s, long *x, long *y)
{
    char dummy[2];

    *dummy = 0;
    if (sscanf(s, "%ld-%ld%1s", x, y, dummy) == 2)
	return (*dummy == 0 && *x <= *y);
    *dummy = 0;
    if (sscanf(s, "%ld%1s", x, dummy) == 1 && *dummy == 0) {
	*y = *x;
	return 1;
    }
    return 0;
}
