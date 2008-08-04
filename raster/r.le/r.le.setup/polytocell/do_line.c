/* @(#)do_line.c        2.2   8/31/87 */
#include "ply_to_cll.h"

void do_line(double *xarray, double *yarray, int num_verticies, int category)
{
    int node;

    num_verticies--;

    line_initialize();
    for (node = 0; node < num_verticies; node++) {
	line(category,
	     (int)(xarray[node]),
	     (int)(yarray[node] + .5),
	     (int)(xarray[node + 1]), (int)(yarray[node + 1] + .5));
    }
    line_flush();
}

void line(int cat, int x0, int y0, int x1, int y1)
{
    int dx, dy;
    int xinc, yinc;
    int res1;
    int res2;

#ifdef DEBUG
    fprintf(stderr, "Line %d %d %d %d\n", x0, y0, x1, y1);
#endif

    xinc = 1;
    yinc = 1;
    if ((dx = x1 - x0) < 0) {
	xinc = -1;
	dx = -dx;
    }
    if ((dy = y1 - y0) < 0) {
	yinc = -1;
	dy = -dy;
    }

    if (dy == 0) {		/* If dy is zero, dispatch immediately  */
	if (xinc < 0)
	    save_line(y0, x1, x0, 0, cat);
	else
	    save_line(y0, x0, x1, 0, cat);
#ifdef DEBUG
	fprintf(stderr, " dy==0 save %d %d %d\n", y0, x0, x1);
#endif
    }
    else {
	res1 = 0;
	if (dx > dy) {
	    res2 = dx;
	    while (x0 != x1) {	/* for dx < dy  */
		save_line(y0, x0, x0, 0, cat);
#ifdef DEBUG
		fprintf(stderr, " dx>dy save %d %d %d\n", y0, x0, x0);
#endif
		if (res1 > res2) {
		    res2 += dx - res1;
		    res1 = 0;
		    y0 += yinc;
		}
		res1 += dy;
		x0 += xinc;
	    }
	    save_line(y0, x0, x0, 0, cat);
	}
	else if (dx < dy) {
	    res2 = dy;
	    while (y0 != y1) {	/* for dx < dy  */
		save_line(y0, x0, x0, 0, cat);
#ifdef DEBUG
		fprintf(stderr, " dx<dy save %d %d %d\n", y0, x0, x0);
#endif
		if (res1 > res2) {
		    res2 += dy - res1;
		    res1 = 0;
		    x0 += xinc;
		}
		res1 += dx;
		y0 += yinc;
	    }
	    save_line(y0, x0, x0, 0, cat);
	}
	else {
	    while (x0 != x1) {	/* For dx == dy */
		save_line(y0, x0, x0, 0, cat);
#ifdef DEBUG
		fprintf(stderr, " dx<dy save %d %d %d\n", y0, x0, x0);
#endif
		y0 += yinc;
		x0 += xinc;
	    }
	    save_line(y0, x0, x0, 0, cat);
	}
	/*
	   if (x0 > x1)
	   save_line(y0, x1, x0, 0, cat ) ;
	   else if (x1 > x0)
	   save_line(y0, x0, x1, 0, cat ) ;
	 */
#ifdef DEBUG
	fprintf(stderr, " END   save %d %d %d\n", y0, x0, x0);
#endif
    }
}

static int l_row, l_col1, l_col2, l_dum, l_cat;
static int have_first = 0;

void line_initialize(void)
{
    l_row = 0;
    l_col1 = 0;
    l_col2 = 0;
    l_cat = 0;
    l_dum = 0;
    have_first = 0;
}

void line_flush(void)
{
    if (have_first)
	write_record(l_row, (float)l_col1, (float)l_col2, l_cat);
}

void save_line(int row, int col1, int col2, int dum, int cat)
{
    have_first = 1;
    if ((row != l_row) || (col1 != l_col1) ||
	(col2 != l_col2) || (cat != l_cat)) {
	if ((row != l_row) || (cat != l_cat)) {
	    write_record(l_row, (float)l_col1, (float)l_col2, l_cat);
	    l_row = row;
	    l_col1 = col1;
	    l_col2 = col2;
	    l_cat = cat;
	    l_dum = dum;
	}
	else {
	    if ((col1 >= l_col2) && (col1 - l_col2 < 2))
		l_col2 = col2;
	    else {
		if ((l_col1 >= col2) && (l_col1 - col2 < 2))
		    l_col1 = col1;
		else {
		    write_record(l_row, (float)l_col1, (float)l_col2, l_cat);
		    l_row = row;
		    l_col1 = col1;
		    l_col2 = col2;
		    l_cat = cat;
		    l_dum = dum;
		}
	    }
	}
    }
}
