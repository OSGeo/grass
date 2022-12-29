
/****** path_finder.c ***********************************************
	
	This recursive function traces the least cost path backwards
	to cells from which the cumulative cost was determined 

*********************************************************************/
#include <grass/segment.h>
#include "local_proto.h"


void path_finder(int row, int col, int backrow, int backcol)
{
    int data, new_backrow, new_backcol;
    extern char *value;
    extern int nrows, ncols;
    extern SEGMENT in_row_seg, in_col_seg, out_seg;

    if (row < 0 || row >= nrows || col < 0 || col >= ncols)
	return;			/* outside the window */

    /* if the pt has already been traversed, return */
    value = (char *)&data;
    Segment_get(&out_seg, value, row, col);
    if (data == 1)
	return;			/* already traversed */

    /* otherwise, draw a line on output */
    drawline(row, col, backrow, backcol);
    /*DEBUG
       printf("\nrow=%d, col=%d, backrow=%d, backcol=%d", row, col, backrow, backcol);
     */
    /* update path position */
    if (row == backrow && col == backcol) {
	printf("\n");
	return;
    }				/* reach an origin */

    value = (char *)&new_backrow;
    Segment_get(&in_row_seg, value, backrow, backcol);
    value = (char *)&new_backcol;
    Segment_get(&in_col_seg, value, backrow, backcol);

    path_finder(backrow, backcol, new_backrow, new_backcol);
    return;
}
