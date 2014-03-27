#include "local_proto.h"
static int tail, head, fifo_count;

int fifo_insert(POINT point)
{
    if (fifo_count == fifo_max)
        G_fatal_error(_("Circular buffer too small"));

    fifo_points[tail++] = point;
    if (tail > fifo_max) {
	G_debug(1, "tail > fifo_max");
	tail = 0;
    }
    fifo_count++;
    return 0;
}

POINT fifo_return_del(void)
{
    if (head >= fifo_max) {
	G_debug(1, "head >= fifo_max");
	head = -1;
    }
    fifo_count--;

    return fifo_points[++head];
}

int ram_init_streams(CELL **streams, CELL **dirs, FCELL **elevation)
{
    int d, i;		/* d: direction, i: iteration */
    int r, c;
    int next_stream = -1, cur_stream;
    int out_max = ncols + nrows;
    POINT *outlets;

    outlets = (POINT *) G_malloc((out_max) * sizeof(POINT));
    outlets_num = 0;

    for (r = 0; r < nrows; ++r)
	for (c = 0; c < ncols; ++c)
	    if (streams[r][c] > 0) {
		if (outlets_num > (out_max - 1)) {
		    out_max *= 2;
		    outlets =
			(POINT *) G_realloc(outlets, out_max * sizeof(POINT));
		}
		d = abs(dirs[r][c]);
		if (NOT_IN_REGION(d))
		    next_stream = -1;	/* border */
		else {
		    next_stream = streams[NR(d)][NC(d)];
		    if (next_stream < 1)
			next_stream = -1;
		}

		if (d == 0)
		    next_stream = -1;
		cur_stream = streams[r][c];

		if (cur_stream != next_stream) {	/* is outlet or node! */
		    outlets[outlets_num].r = r;
		    outlets[outlets_num].c = c;

		    if (next_stream == -1)
			outlets[outlets_num].is_outlet = 1;
		    else
			outlets[outlets_num].is_outlet = 0;

		    outlets_num++;

		}
	    }			/* end if streams */

    stat_streams = (STREAM *) G_malloc((outlets_num) * sizeof(STREAM));

    for (i = 0; i < outlets_num; ++i) {
	stat_streams[i].r = outlets[i].r;
	stat_streams[i].c = outlets[i].c;
	stat_streams[i].is_outlet = outlets[i].is_outlet;
	stat_streams[i].index = i;
	stat_streams[i].slope = 0.;
	stat_streams[i].gradient = 0.;
	stat_streams[i].length = 0.;
	stat_streams[i].elev_diff = 0.;
	stat_streams[i].elev_spring = 0.;
	stat_streams[i].elev_outlet = elevation[outlets[i].r][outlets[i].c];
	stat_streams[i].order = streams[outlets[i].r][outlets[i].c];
	stat_streams[i].basin_area = 0.;
	stat_streams[i].cell_num = 0;
    }

    G_free(outlets);
    return 0;
}

/////

int seg_init_streams(SEGMENT *streams, SEGMENT *dirs, SEGMENT *elevation)
{
    int d, i;		/* d: direction, i: iteration */
    int r, c;
    int next_stream = -1, cur_stream;
    int out_max = ncols + nrows;
    CELL streams_cell, dirs_cell;
    POINT *outlets;

    outlets = (POINT *) G_malloc((out_max) * sizeof(POINT));
    outlets_num = 0;

    for (r = 0; r < nrows; ++r)
	for (c = 0; c < ncols; ++c) {
	    segment_get(streams, &streams_cell, r, c);
	    if (streams_cell > 0) {
		if (outlets_num > (out_max - 1)) {
		    out_max *= 2;
		    outlets =
			(POINT *) G_realloc(outlets, out_max * sizeof(POINT));
		}

		segment_get(dirs, &dirs_cell, r, c);
		d = abs(dirs_cell);
		if (NOT_IN_REGION(d))
		    next_stream = -1;	/* border */
		else {
		    segment_get(streams, &next_stream, NR(d), NC(d));
		    if (next_stream < 1)
			next_stream = -1;
		}

		if (d == 0)
		    next_stream = -1;
		cur_stream = streams_cell;

		if (cur_stream != next_stream) {	/* is outlet or node! */
		    outlets[outlets_num].r = r;
		    outlets[outlets_num].c = c;

		    if (next_stream == -1)
			outlets[outlets_num].is_outlet = 1;
		    else
			outlets[outlets_num].is_outlet = 0;

		    outlets_num++;


		}
	    }			/* end if streams */
	}

    stat_streams = (STREAM *) G_malloc((outlets_num) * sizeof(STREAM));

    for (i = 0; i < outlets_num; ++i) {
	stat_streams[i].r = outlets[i].r;
	stat_streams[i].c = outlets[i].c;
	stat_streams[i].is_outlet = outlets[i].is_outlet;
	stat_streams[i].index = i;
	stat_streams[i].slope = 0.;
	stat_streams[i].gradient = 0.;
	stat_streams[i].length = 0.;
	stat_streams[i].elev_diff = 0.;
	stat_streams[i].elev_spring = 0.;
	segment_get(elevation, &(stat_streams[i].elev_outlet), outlets[i].r,
		    outlets[i].c);
	segment_get(streams, &(stat_streams[i].order), outlets[i].r,
		    outlets[i].c);
	stat_streams[i].basin_area = 0.;
	stat_streams[i].cell_num = 0;
    }

    G_free(outlets);
    return 0;
}


int ram_calculate_streams(CELL **streams, CELL **dirs, FCELL **elevation)
{

    int i, j, s, d;		/* s - streams index */
    int done = 1;
    int r, c;
    int next_r, next_c;
    float cur_northing, cur_easting;
    float next_northing, next_easting;
    float diff_elev;
    double cur_length;
    struct Cell_head window;

    G_get_window(&window);
    G_begin_distance_calculations();

    for (s = 0; s < outlets_num; ++s) {
	r = stat_streams[s].r;
	c = stat_streams[s].c;

	cur_northing = window.north - (r + .5) * window.ns_res;
	cur_easting = window.west + (c + .5) * window.ew_res;
	d = (dirs[r][c] == 0) ? 2 : abs(dirs[r][c]);

	next_northing = window.north - (NR(d) + .5) * window.ns_res;
	next_easting = window.west + (NC(d) + .5) * window.ew_res;

	stat_streams[s].length =
	    G_distance(next_easting, next_northing, cur_easting,
		       cur_northing);

	done = 1;

	while (done) {
	    done = 0;
	    cur_northing = window.north - (r + .5) * window.ns_res;
	    cur_easting = window.west + (c + .5) * window.ew_res;

	    stat_streams[s].cell_num++;
	    stat_streams[s].elev_spring = elevation[r][c];

	    for (i = 1; i < 9; ++i) {
		if (NOT_IN_REGION(i))
		    continue;	/* border */

		j = DIAG(i);
		next_r = NR(i);
		next_c = NC(i);

		if (streams[next_r][next_c] == stat_streams[s].order &&
		    dirs[next_r][next_c] == j) {

		    next_northing =
			window.north - (next_r + .5) * window.ns_res;
		    next_easting =
			window.west + (next_c + .5) * window.ew_res;
		    cur_length =
			G_distance(next_easting, next_northing, cur_easting,
				   cur_northing);
		    diff_elev = elevation[next_r][next_c] - elevation[r][c];
		    diff_elev = (diff_elev < 0) ? 0. : diff_elev;	/* water cannot flow up */

		    stat_streams[s].length += cur_length;
		    stat_streams[s].slope += (diff_elev / cur_length);

		    r = next_r;
		    c = next_c;
		    done = 1;
		    break;
		}		/* end if */
	    }			/* end for i */
	}			/* end while */
    }				/* end for s */
    return 0;
}


////


int seg_calculate_streams(SEGMENT *streams, SEGMENT *dirs,
			  SEGMENT *elevation)
{

    int i, j, s, d;		/* s - streams index */
    int done = 1;
    int r, c;
    int next_r, next_c;
    float cur_northing, cur_easting;
    float next_northing, next_easting;
    float diff_elev;
    double cur_length;
    CELL streams_cell, dirs_cell;
    FCELL elevation_cell, elevation_next_cell;
    struct Cell_head window;

    G_get_window(&window);
    G_begin_distance_calculations();

    for (s = 0; s < outlets_num; ++s) {
	r = stat_streams[s].r;
	c = stat_streams[s].c;

	cur_northing = window.north - (r + .5) * window.ns_res;
	cur_easting = window.west + (c + .5) * window.ew_res;

	segment_get(dirs, &dirs_cell, r, c);
	d = (dirs_cell == 0) ? 2 : abs(dirs_cell);

	next_northing = window.north - (NR(d) + .5) * window.ns_res;
	next_easting = window.west + (NC(d) + .5) * window.ew_res;

	stat_streams[s].length =
	    G_distance(next_easting, next_northing, cur_easting,
		       cur_northing);

	done = 1;

	while (done) {
	    done = 0;
	    cur_northing = window.north - (r + .5) * window.ns_res;
	    cur_easting = window.west + (c + .5) * window.ew_res;

	    stat_streams[s].cell_num++;
	    segment_get(elevation, &(stat_streams[s].elev_spring), r, c);

	    for (i = 1; i < 9; ++i) {
		if (NOT_IN_REGION(i))
		    continue;	/* border */

		j = DIAG(i);
		next_r = NR(i);
		next_c = NC(i);

		segment_get(streams, &streams_cell, next_r, next_c);
		segment_get(dirs, &dirs_cell, next_r, next_c);

		if (streams_cell == stat_streams[s].order && dirs_cell == j) {

		    next_northing =
			window.north - (next_r + .5) * window.ns_res;
		    next_easting =
			window.west + (next_c + .5) * window.ew_res;
		    cur_length =
			G_distance(next_easting, next_northing, cur_easting,
				   cur_northing);

		    segment_get(elevation, &elevation_next_cell, next_r,
				next_c);
		    segment_get(elevation, &elevation_cell, r, c);
		    diff_elev = elevation_next_cell - elevation_cell;
		    diff_elev = (diff_elev < 0) ? 0. : diff_elev;	/* water cannot flow up */

		    stat_streams[s].length += cur_length;
		    stat_streams[s].slope += (diff_elev / cur_length);

		    r = next_r;
		    c = next_c;
		    done = 1;
		    break;
		}		/* end if */
	    }			/* end for i */
	}			/* end while */
    }				/* end for s */
    return 0;
}

double ram_calculate_basins_area(CELL **dirs, int r, int c)
{
    int i, j;
    int next_r, next_c;
    double area;
    POINT n_cell;

    tail = 0;
    head = -1;

    area = G_area_of_cell_at_row(r);

    while (tail != head) {
	for (i = 1; i < 9; ++i) {

	    if (NOT_IN_REGION(i))
		continue;	/* border */

	    j = DIAG(i);
	    next_r = NR(i);
	    next_c = NC(i);

	    if (dirs[next_r][next_c] == j) {	/* countributing cell */
		area += G_area_of_cell_at_row(r);
		n_cell.r = next_r;
		n_cell.c = next_c;
		fifo_insert(n_cell);
	    }
	}			/* end for i... */

	n_cell = fifo_return_del();
	r = n_cell.r;
	c = n_cell.c;
    }				/* end while */
    return area;
}


int ram_calculate_basins(CELL **dirs)
{
    int i;

    total_basins = 0.;

    G_begin_cell_area_calculations();
    fifo_max = 4 * (nrows + ncols);
    fifo_points = (POINT *) G_malloc((fifo_max + 1) * sizeof(POINT));

    for (i = 0; i < outlets_num; ++i) {
	stat_streams[i].basin_area =
	    ram_calculate_basins_area(dirs, stat_streams[i].r,
				      stat_streams[i].c);

	if (stat_streams[i].is_outlet)
	    total_basins += stat_streams[i].basin_area;
    }

    G_free(fifo_points);
    return 0;
}

double seg_calculate_basins_area(SEGMENT *dirs, int r, int c)
{
    int i, j;
    int next_r, next_c;
    double area;
    CELL dirs_cell;
    POINT n_cell;

    tail = 0;
    head = -1;

    area = G_area_of_cell_at_row(r);

    while (tail != head) {
	for (i = 1; i < 9; ++i) {

	    if (NOT_IN_REGION(i))
		continue;	/* border */

	    j = DIAG(i);
	    next_r = NR(i);
	    next_c = NC(i);

	    segment_get(dirs, &dirs_cell, next_r, next_c);

	    if (dirs_cell == j) {	/* countributing cell */
		area += G_area_of_cell_at_row(r);
		n_cell.r = next_r;
		n_cell.c = next_c;
		fifo_insert(n_cell);
	    }
	}			/* end for i... */

	n_cell = fifo_return_del();
	r = n_cell.r;
	c = n_cell.c;
    }				/* end while */
    return area;
}

int seg_calculate_basins(SEGMENT *dirs)
{
    int i;

    total_basins = 0.;

    G_begin_cell_area_calculations();
    fifo_max = 4 * (nrows + ncols);
    fifo_points = (POINT *) G_malloc((fifo_max + 1) * sizeof(POINT));

    for (i = 0; i < outlets_num; ++i) {
	stat_streams[i].basin_area =
	    seg_calculate_basins_area(dirs, stat_streams[i].r,
				      stat_streams[i].c);
	if (stat_streams[i].is_outlet)
	    total_basins += stat_streams[i].basin_area;
    }

    G_free(fifo_points);
    return 0;
}
