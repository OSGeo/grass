/*
 * Copyright (C) 1995.  Bill Brown <brown@gis.uiuc.edu> & Michael Shapiro
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */

int datetime_is_between(int x, int a, int b)
{
    if (a <= b)
	return a <= x && x <= b;
    else
	return b <= x && x <= a;
}
