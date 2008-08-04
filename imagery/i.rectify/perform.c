#include "global.h"

static ROWCOL *rmap, *cmap, left, right;
static int do_cell(int, void *, void *);

int rast_size;

int perform_georef(int infd, void *rast)
{
    int row;
    int curidx;
    int idx;
    int i;

    rast_size = G_raster_size(map_type);

    for (row = 0; row < matrix_rows; row++)
	G_set_null_value(cell_buf[row], matrix_cols, map_type);

    curidx = 0;
    while (1) {
	/* find first row */
	while (curidx < matrix_rows) {
	    idx = row_idx[curidx];
	    row = row_min[idx];
	    if (row >= 0)
		break;
	    curidx++;
	}
	/*
	   fprintf (stderr, " curidx %d\n", curidx);
	 */
	if (curidx >= matrix_rows)
	    break;
	/*
	   fprintf (stderr, "read row %d\n", row);
	 */

	if (G_get_raster_row_nomask
	    (infd, G_incr_void_ptr(rast, rast_size), row, map_type) < 0)
	    return 0;

	for (i = curidx; i < matrix_rows; i++) {
	    idx = row_idx[i];
	    if (row != row_min[idx])
		break;
	    /*
	       fprintf (stderr, "  process matrix row %d\n", idx);
	     */
	    rmap = row_map[idx];
	    cmap = col_map[idx];
	    left = row_left[idx];
	    right = row_right[idx];
	    do_cell(row, G_incr_void_ptr(rast, rast_size), cell_buf[idx]);

	    row_min[idx]++;
	    if (row_min[idx] > row_max[idx])
		row_min[idx] = -1;
	    row_left[idx] = left;
	    row_right[idx] = right;
	}
    }

    return 0;
}

static int do_cell(int row, void *in, void *out)
{
    int col;
    void *inptr, *outptr;

    for (; left <= right; left++) {
	inptr = G_incr_void_ptr(in, cmap[left] * rast_size);
	outptr = G_incr_void_ptr(out, left * rast_size);
	if (rmap[left] < 0)
	    continue;
	if (rmap[left] != row)
	    break;
	G_raster_cpy(outptr, inptr, 1, map_type);
    }
    for (; left <= right; right--) {
	inptr = G_incr_void_ptr(in, cmap[right] * rast_size);
	outptr = G_incr_void_ptr(out, right * rast_size);
	if (rmap[right] < 0)
	    continue;
	if (rmap[right] != row)
	    break;
	G_raster_cpy(outptr, inptr, 1, map_type);
    }
    for (col = left; col <= right; col++) {
	inptr = G_incr_void_ptr(in, cmap[col] * rast_size);
	outptr = G_incr_void_ptr(out, col * rast_size);
	if (rmap[col] == row)
	    G_raster_cpy(outptr, inptr, 1, map_type);
    }

    return 0;
}
