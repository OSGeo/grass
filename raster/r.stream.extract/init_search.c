#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

int init_search(int depr_fd)
{
    int r, c, r_nbr, c_nbr, ct_dir;
    CELL *depr_buf, ele_value;
    int nextdr[8] = { 1, -1, 0, 0, -1, 1, 1, -1 };
    int nextdc[8] = { 0, 0, -1, 1, 1, -1, 1, -1 };
    char asp_value, is_null;
    WAT_ALT wa;
    ASP_FLAG af, af_nbr;
    GW_LARGE_INT n_depr_cells = 0;

    nxt_avail_pt = heap_size = 0;

    /* load edge cells and real depressions to A* heap */
    if (depr_fd >= 0)
	depr_buf = Rast_allocate_buf(CELL_TYPE);
    else
	depr_buf = NULL;

    G_message(_("Initializing A* Search..."));
    for (r = 0; r < nrows; r++) {
	G_percent(r, nrows, 2);

	if (depr_fd >= 0) {
	    Rast_get_row(depr_fd, depr_buf, r, CELL_TYPE);
	}

	for (c = 0; c < ncols; c++) {

	    seg_get(&aspflag, (char *)&af, r, c);
	    is_null = FLAG_GET(af.flag, NULLFLAG);

	    if (is_null)
		continue;

	    asp_value = 0;
	    if (r == 0 || r == nrows - 1 || c == 0 || c == ncols - 1) {

		if (r == 0 && c == 0)
		    asp_value = -7;
		else if (r == 0 && c == ncols - 1)
		    asp_value = -5;
		else if (r == nrows - 1 && c == 0)
		    asp_value = -1;
		else if (r == nrows - 1 && c == ncols - 1)
		    asp_value = -3;
		else if (r == 0)
		    asp_value = -2;
		else if (c == 0)
		    asp_value = -4;
		else if (r == nrows - 1)
		    asp_value = -6;
		else if (c == ncols - 1)
		    asp_value = -8;

		seg_get(&watalt, (char *)&wa, r, c);
		ele_value = wa.ele;
		heap_add(r, c, ele_value);
		FLAG_SET(af.flag, INLISTFLAG);
		FLAG_SET(af.flag, EDGEFLAG);
		af.asp = asp_value;
		seg_put(&aspflag, (char *)&af, r, c);
		continue;
	    }

	    /* any neighbour NULL ? */
	    for (ct_dir = 0; ct_dir < sides; ct_dir++) {
		/* get r, c (r_nbr, c_nbr) for neighbours */
		r_nbr = r + nextdr[ct_dir];
		c_nbr = c + nextdc[ct_dir];

		seg_get(&aspflag, (char *)&af_nbr, r_nbr, c_nbr);
		is_null = FLAG_GET(af_nbr.flag, NULLFLAG);

		if (is_null) {
		    asp_value = -1 * drain[r - r_nbr + 1][c - c_nbr + 1];
		    seg_get(&watalt, (char *)&wa, r, c);
		    ele_value = wa.ele;
		    heap_add(r, c, ele_value);
		    FLAG_SET(af.flag, INLISTFLAG);
		    FLAG_SET(af.flag, EDGEFLAG);
		    af.asp = asp_value;
		    seg_put(&aspflag, (char *)&af, r, c);

		    break;
		}
	    }
	    if (asp_value) /* some neighbour was NULL, point added to list */
		continue;
	    
	    /* real depression ? */
	    if (depr_fd >= 0) {
		if (!Rast_is_c_null_value(&depr_buf[c]) && depr_buf[c] != 0) {
		    seg_get(&watalt, (char *)&wa, r, c);
		    ele_value = wa.ele;
		    heap_add(r, c, ele_value);
		    FLAG_SET(af.flag, INLISTFLAG);
		    FLAG_SET(af.flag, DEPRFLAG);
		    af.asp = asp_value;
		    seg_put(&aspflag, (char *)&af, r, c);
		    n_depr_cells++;
		}
	    }
	}
    }
    G_percent(nrows, nrows, 2);	/* finish it */

    if (depr_fd >= 0) {
	Rast_close(depr_fd);
	G_free(depr_buf);
    }

    G_debug(1, "%lld edge cells", heap_size - n_depr_cells);
    if (n_depr_cells)
	G_debug(1, "%lld cells in depressions", n_depr_cells);

    return 1;
}
