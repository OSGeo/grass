/**************************************************************
 * compute_georef_matrix (win1, win2)
 *
 */
#include <stdlib.h>
#include "global.h"

static int cmp (const void *, const void *);

int compute_georef_matrix (struct Cell_head *win1, struct Cell_head *win2)
{
    ROWCOL *rmap, *cmap,rr,cc;
    int nrow1, nrow2;
    int ncol1, ncol2;
    double n1,w1,ns_res1,ew_res1;
    double n2,e2,ns_res2,ew_res2;
    double nx,ex;
    double NX, EX;
    int row, col;
    int min, max;

    ns_res1 = win1->ns_res;
    ew_res1 = win1->ew_res;
    nrow1 = win1->rows;
    ncol1 = win1->cols;
    n1 = win1->north;
    w1 = win1->west;

    ns_res2 = win2->ns_res;
    ew_res2 = win2->ew_res;
    nrow2 = win2->rows;
    ncol2 = win2->cols;
    n2 = win2->north;
    e2 = win2->west;
    matrix_rows = nrow2 ;
    matrix_cols = ncol2 ;

/* georef equation is
 * ex = E21a + E21b * col + E21c * row
 * nx = N21a + N21b * col + N21c * row
 *
 * compute common code (for northing) outside east loop
 */

    for (n2 = win2->north, row = 0; row < nrow2; row++, n2 -= ns_res2)
    {
	rmap = row_map[row];
	cmap = col_map[row];
	min = max = -1;
/**** 
#ifdef DEBUG3
fprintf (Bugsr,"\n\t got row = \t%d\n",row);
#endif
****/

        /* compute common code */
	EX = E21a + E21c * row;
	NX = N21a + N21c * row;

	for (e2 = win2->west, col = 0; col < ncol2; col++, e2 += ew_res2)
	{
            /* georef e2,n2 */
	    ex = EX + E21b * col;
	    nx = NX + N21b * col;

	    rr = (n1-nx) / ns_res1;
	    if (rr < 0 || rr >= nrow1)
		rr = -1;
	    else if (min < 0)
		min = max = rr;
	    else if (rr < min)
		min = rr;
	    else if (rr > max)
		max = rr;
	    *rmap++ = rr;

	    cc = (ex-w1) / ew_res1;
	    if (cc < 0 || cc >= ncol1)
		cc = -1;
	    *cmap++ = cc;
/*****
#ifdef DEBUG3
fprintf (Bugsr,"\n\tnx = \t%f \tex = \t%f \n\trr = \t%d \tcc = \t%d\n", nx, ex,rr, cc);
#endif
*****/
	}
	row_min[row] = min;
	row_max[row] = max;
	row_left[row] = 0;
	row_right[row] = matrix_cols-1;
	row_idx[row] = row;
    }
    qsort (row_idx, nrow2, sizeof (IDX), cmp);

    return 0;
}

static int cmp (const void *aa, const void *bb)
{
    const IDX *a = aa, *b = bb;
    return (int) (row_min[*a] - row_min[*b]);
}

