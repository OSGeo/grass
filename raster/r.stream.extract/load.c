#include <grass/raster.h>
#include <grass/glocale.h>
#include "local_proto.h"

/* need elevation map, do A* search on elevation like for r.watershed */

int ele_round(double x)
{
    if (x >= 0.0)
	return x + .5;
    else
	return x - .5;
}

/*
 * loads elevation and optional flow accumulation map to memory and
 * gets start points for A* Search
 * start points are edges
 */
int load_maps(int ele_fd, int acc_fd)
{
    int r, c;
    void *ele_buf, *ptr, *acc_buf = NULL, *acc_ptr = NULL;
    CELL ele_value, *stream_id;
    DCELL dvalue, acc_value;
    size_t ele_size, acc_size = 0;
    int ele_map_type, acc_map_type = 0;
    WAT_ALT *wabuf;

    ASP_FLAG *afbuf;

    if (acc_fd < 0)
	G_message(_("Loading elevation map..."));
    else
	G_message(_("Loading input maps..."));

    n_search_points = n_points = 0;

    ele_map_type = Rast_get_map_type(ele_fd);
    ele_size = Rast_cell_size(ele_map_type);
    ele_buf = Rast_allocate_buf(ele_map_type);

    if (ele_buf == NULL) {
	G_warning(_("Could not allocate memory"));
	return -1;
    }

    if (acc_fd >= 0) {
	acc_map_type = Rast_get_map_type(acc_fd);
	acc_size = Rast_cell_size(acc_map_type);
	acc_buf = Rast_allocate_buf(acc_map_type);
	if (acc_buf == NULL) {
	    G_warning(_("Could not allocate memory"));
	    return -1;
	}
    }

    ele_scale = 1;
    if (ele_map_type == FCELL_TYPE || ele_map_type == DCELL_TYPE)
	ele_scale = 1000;	/* should be enough to do the trick */

    wabuf = G_malloc(ncols * sizeof(WAT_ALT));
    afbuf = G_malloc(ncols * sizeof(ASP_FLAG));
    stream_id = G_malloc(ncols * sizeof(CELL));

    G_debug(1, "start loading %d rows, %d cols", nrows, ncols);
    for (r = 0; r < nrows; r++) {

	G_percent(r, nrows, 2);

	Rast_get_row(ele_fd, ele_buf, r, ele_map_type);
	ptr = ele_buf;

	if (acc_fd >= 0) {
	    Rast_get_row(acc_fd, acc_buf, r, acc_map_type);
	    acc_ptr = acc_buf;
	}

	for (c = 0; c < ncols; c++) {

	    afbuf[c].flag = 0;
	    afbuf[c].asp = 0;
	    stream_id[c] = 0;

	    /* check for masked and NULL cells */
	    if (Rast_is_null_value(ptr, ele_map_type)) {
		FLAG_SET(afbuf[c].flag, NULLFLAG);
		FLAG_SET(afbuf[c].flag, INLISTFLAG);
		FLAG_SET(afbuf[c].flag, WORKEDFLAG);
		FLAG_SET(afbuf[c].flag, WORKED2FLAG);
		Rast_set_c_null_value(&ele_value, 1);
		/* flow accumulation */
		if (acc_fd >= 0) {
		    if (!Rast_is_null_value(acc_ptr, acc_map_type))
			G_fatal_error(_("Elevation map is NULL but accumulation map is not NULL!"));
		}
		Rast_set_d_null_value(&acc_value, 1);
	    }
	    else {
		switch (ele_map_type) {
		case CELL_TYPE:
		    ele_value = *((CELL *) ptr);
		    break;
		case FCELL_TYPE:
		    dvalue = *((FCELL *) ptr);
		    dvalue *= ele_scale;
		    ele_value = ele_round(dvalue);
		    break;
		case DCELL_TYPE:
		    dvalue = *((DCELL *) ptr);
		    dvalue *= ele_scale;
		    ele_value = ele_round(dvalue);
		    break;
		}
		if (acc_fd < 0)
		    acc_value = 1;
		else {
		    if (Rast_is_null_value(acc_ptr, acc_map_type)) {
			/* can this be ok after weighing ? */
			G_fatal_error(_("Accumulation map is NULL but elevation map is not NULL!"));
		    }

		    switch (acc_map_type) {
		    case CELL_TYPE:
			acc_value = *((CELL *) acc_ptr);
			break;
		    case FCELL_TYPE:
			acc_value = *((FCELL *) acc_ptr);
			break;
		    case DCELL_TYPE:
			acc_value = *((DCELL *) acc_ptr);
			break;
		    }
		}

		n_points++;
	    }

	    wabuf[c].wat = acc_value;
	    wabuf[c].ele = ele_value;
	    ptr = G_incr_void_ptr(ptr, ele_size);
	    if (acc_fd >= 0)
		acc_ptr = G_incr_void_ptr(acc_ptr, acc_size);
	}
	seg_put_row(&watalt, (char *) wabuf, r);
	seg_put_row(&aspflag, (char *) afbuf, r);
	cseg_put_row(&stream, stream_id, r);
    }
    G_percent(nrows, nrows, 1);	/* finish it */

    Rast_close(ele_fd);
    G_free(ele_buf);
    G_free(wabuf);
    G_free(afbuf);
    G_free(stream_id);

    if (acc_fd >= 0) {
	Rast_close(acc_fd);
	G_free(acc_buf);
    }
    
    G_debug(1, "%lld non-NULL cells", n_points);

    return n_points;
}
