#include <stdio.h>
static int rec_num = 1;
static int maxrow, minrow;
static float maxcol, mincol;

void set_limits(int numrows, int numcols)
{
    maxrow = numrows;
    maxcol = (float)(numcols - 1);
    minrow = 1;
    mincol = 0.0;
}

static int check_limits(int *row, float *first_cell, float *last_cell)
{
    if (*row < minrow)
	return (0);
    if (*row > maxrow)
	return (0);
    if (*first_cell > maxcol)
	return (0);
    if (*last_cell < mincol)
	return (0);
    if (*first_cell < mincol)
	*first_cell = mincol;
    if (*last_cell > maxcol)
	*last_cell = maxcol;
    *last_cell = maxcol - *last_cell;
    return (1);
}

void write_record(int row, float first_cell, float last_cell, int category)
{
    float fc, lc;

    fc = first_cell;
    lc = last_cell;

    if (check_limits(&row, &fc, &lc))
	printf("%d %8d:%d:%d:%d\n",
	       row, rec_num++, (int)(100. * fc), (int)(100. * lc), category);
}

void write_end_record(int row, int first_cell, int last_cell, int category)
{
    printf("%d %8d:%d:%d:%d\n",
	   row, rec_num++, first_cell, last_cell, category);
}
