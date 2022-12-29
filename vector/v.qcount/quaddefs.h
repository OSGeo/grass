/*
 * Copyright (C) 1994-1995. James Darrell McCauley. (darrell@mccauley-usa.com)
 *                                http://mccauley-usa.com/
 *
 * This program is free software under the GPL (>=v2)
 * Read the file GPL.TXT coming with GRASS for details.
 */

typedef struct
{
    double x, y;
} COOR;

void count_sites(COOR *, int, int *, double, struct Map_info *, int);
COOR *find_quadrats(int, double, struct Cell_head);
void qindices(int *, int, double *, double *,
	      double *, double *, double *, double *);
