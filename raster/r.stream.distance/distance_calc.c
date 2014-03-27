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


int ram_calculate_downstream(CELL ** dirs, FCELL ** distance,
			     FCELL ** elevation, OUTLET outlet, int outs)
{

    int r, c, i, j;
    int next_r, next_c;
    POINT n_cell;
    float cur_dist = 0;
    float tmp_dist = 0;
    float target_elev;		/* eleavation at stream or outlet */
    float easting, northing;
    float cell_easting, cell_northing;
    struct Cell_head window;

    Rast_get_window(&window);

    tail = 0;
    head = -1;
    r = outlet.r;
    c = outlet.c;

    if (elevation) {
	target_elev = elevation[r][c];
	elevation[r][c] = 0.;
    }

    while (tail != head) {
	easting = window.west + (c + .5) * window.ew_res;
	northing = window.north - (r + .5) * window.ns_res;

	for (i = 1; i < 9; ++i) {

	    if (NOT_IN_REGION(i))
		continue;	/* border */

	    j = DIAG(i);
	    next_r = NR(i);
	    next_c = NC(i);
	    if (dirs[NR(i)][NC(i)] == j) {	/* countributing cell, reset distance and elevation */

		if (outs) {	/* outlet mode */

		    if (distance[NR(i)][NC(i)] == 0)
			continue;	/* continue loop, point is not added to the queue! */
		    else {
			cell_northing =
			    window.north - (next_r + .5) * window.ns_res;
			cell_easting =
			    window.west + (next_c + .5) * window.ew_res;
			cur_dist =
			    tmp_dist + G_distance(easting, northing,
						  cell_easting,
						  cell_northing);
			distance[NR(i)][NC(i)] = cur_dist;
		    }

		}
		else {		/* stream mode */

		    if (distance[next_r][next_c] == 0) {
			cur_dist = 0;
			if (elevation)
			    target_elev = elevation[next_r][next_c];
		    }
		    else {
			cell_northing =
			    window.north - (next_r + .5) * window.ns_res;
			cell_easting =
			    window.west + (next_c + .5) * window.ew_res;
			cur_dist =
			    tmp_dist + G_distance(easting, northing,
						  cell_easting,
						  cell_northing);
			distance[NR(i)][NC(i)] = cur_dist;
		    }
		}		/* end stream mode */

		if (elevation) {
		    elevation[next_r][next_c] =
			elevation[next_r][next_c] - target_elev;
		    n_cell.target_elev = target_elev;
		}

		n_cell.r = next_r;
		n_cell.c = next_c;
		n_cell.cur_dist = cur_dist;
		fifo_insert(n_cell);
	    }
	}			/* end for i... */

	n_cell = fifo_return_del();
	r = n_cell.r;
	c = n_cell.c;
	tmp_dist = n_cell.cur_dist;
	target_elev = n_cell.target_elev;

    }				/* end while */
    return 0;
}

int seg_calculate_downstream(SEGMENT *dirs, SEGMENT * distance,
			     SEGMENT *elevation, OUTLET outlet, int outs)
{

    int r, c, i, j;
    int next_r, next_c;
    POINT n_cell;
    float cur_dist = 0;
    float tmp_dist = 0;
    float target_elev;		/* eleavation at stream or outlet */
    float easting, northing;
    float cell_easting, cell_northing;
    CELL dirs_cell;
    FCELL distance_cell, elevation_cell;
    FCELL zero_cell = 0;
    struct Cell_head window;

    Rast_get_window(&window);

    tail = 0;
    head = -1;
    r = outlet.r;
    c = outlet.c;

    if (elevation) {
	segment_get(elevation, &target_elev, r, c);
	segment_put(elevation, &zero_cell, r, c);
    }

    while (tail != head) {
	easting = window.west + (c + .5) * window.ew_res;
	northing = window.north - (r + .5) * window.ns_res;

	for (i = 1; i < 9; ++i) {

	    if (NOT_IN_REGION(i))
		continue;	/* border */

	    j = DIAG(i);
	    next_r = NR(i);
	    next_c = NC(i);

	    segment_get(dirs, &dirs_cell, next_r, next_c);
	    if (dirs_cell == j) {	/* countributing cell, reset distance and elevation */

		if (outs) {	/* outlet mode */
		    segment_get(distance, &distance_cell, next_r, next_c);
		    if (distance_cell == 0)
			continue;	/* continue loop, point is not added to the queue! */
		    else {
			cell_northing =
			    window.north - (next_r + .5) * window.ns_res;
			cell_easting =
			    window.west + (next_c + .5) * window.ew_res;
			cur_dist =
			    tmp_dist + G_distance(easting, northing,
						  cell_easting,
						  cell_northing);
			segment_put(distance, &cur_dist, next_r, next_c);

		    }

		}
		else {		/* stream mode */
		    segment_get(distance, &distance_cell, next_r, next_c);
		    if (distance_cell == 0) {
			cur_dist = 0;
			if (elevation)
			    segment_get(elevation, &target_elev, next_r,
					next_c);
		    }
		    else {
			cell_northing =
			    window.north - (next_r + .5) * window.ns_res;
			cell_easting =
			    window.west + (next_c + .5) * window.ew_res;
			cur_dist =
			    tmp_dist + G_distance(easting, northing,
						  cell_easting,
						  cell_northing);
			segment_put(distance, &cur_dist, next_r, next_c);
		    }
		}		/* end stream mode */

		if (elevation) {
		    segment_get(elevation, &elevation_cell, next_r, next_c);
		    elevation_cell -= target_elev;
		    segment_put(elevation, &elevation_cell, next_r, next_c);
		    n_cell.target_elev = target_elev;
		}

		n_cell.r = next_r;
		n_cell.c = next_c;
		n_cell.cur_dist = cur_dist;
		fifo_insert(n_cell);
	    }
	}			/* end for i... */

	n_cell = fifo_return_del();
	r = n_cell.r;
	c = n_cell.c;
	tmp_dist = n_cell.cur_dist;
	target_elev = n_cell.target_elev;

    }				/* end while */
    return 0;
}

int ram_fill_basins(OUTLET outlet, FCELL ** distance, CELL ** dirs)
{
    /* fill empty spaces with zeros but leave -1 as a markers of NULL */
    int r, c, i, j;
    int next_r, next_c;
    float stop, val;
    POINT n_cell;

    tail = 0;
    head = -1;
    r = outlet.r;
    c = outlet.c;
    val = 1;
    stop = 0;

    distance[r][c] = stop;

    while (tail != head) {
	for (i = 1; i < 9; ++i) {
	    if (NOT_IN_REGION(i))
		continue;	/* out of border */

	    j = DIAG(i);
	    next_r = NR(i);
	    next_c = NC(i);

	    if (dirs[next_r][next_c] == j) {	/* countributing cell */

		distance[next_r][next_c] =
		    (distance[next_r][next_c] == stop) ? stop : val;
		n_cell.r = next_r;
		n_cell.c = next_c;
		fifo_insert(n_cell);
	    }

	}			/* end for i... */

	n_cell = fifo_return_del();
	r = n_cell.r;
	c = n_cell.c;
    }

    return 0;
}

int seg_fill_basins(OUTLET outlet, SEGMENT * distance, SEGMENT * dirs)
{
    /* fill empty spaces with zeros but leave -1 as a markers of NULL */
    int r, c, i, j;
    int next_r, next_c;
    float stop, val;
    POINT n_cell;
    CELL dirs_cell;
    FCELL distance_cell;

    tail = 0;
    head = -1;
    r = outlet.r;
    c = outlet.c;
    val = 1;
    stop = 0;

    segment_put(distance, &stop, r, c);

    while (tail != head) {

	for (i = 1; i < 9; ++i) {
	    if (NOT_IN_REGION(i))
		continue;	/* out of border */

	    j = DIAG(i);
	    next_r = NR(i);
	    next_c = NC(i);

	    segment_get(dirs, &dirs_cell, next_r, next_c);

	    if (dirs_cell == j) {	/* countributing cell */

		segment_get(distance, &distance_cell, next_r, next_c);
		distance_cell = (distance_cell == stop) ? stop : val;
		segment_put(distance, &distance_cell, next_r, next_c);
		n_cell.r = next_r;
		n_cell.c = next_c;
		fifo_insert(n_cell);

	    }
	}			/* end for i... */

	n_cell = fifo_return_del();
	r = n_cell.r;
	c = n_cell.c;
    }

    return 0;
}

int ram_calculate_upstream(FCELL ** distance, CELL ** dirs,
			   FCELL ** elevation, FCELL ** tmp_elevation,
			   int near)
{
    int r, c;
    int next_r, next_c;
    float easting, northing;
    float cell_easting, cell_northing;
    int i, j, k, d;
    int done;
    int counter;
    int n_inits = 0;
    float cur_dist;
    POINT *d_inits;
    float tmp_dist = 0;
    float target_elev = 0;
    size_t elevation_data_size;
    struct Cell_head window;

    Rast_get_window(&window);

    if (elevation) {
	elevation_data_size = Rast_cell_size(FCELL_TYPE);
	for (r = 0; r < nrows; ++r)
	    memcpy(tmp_elevation[r], elevation[r],
		   ncols * elevation_data_size);
    }

    for (r = 0; r < nrows; ++r)
	for (c = 0; c < ncols; ++c) {

	    for (i = 1; i < 9; ++i) {
		if (NOT_IN_REGION(i))
		    continue;	/* out of border */

		j = DIAG(i);
		next_r = NR(i);
		next_c = NC(i);
		if (dirs[next_r][next_c] == j && distance[r][c] != 0) {	/* is contributing cell */
		    distance[r][c] = -1;
		    break;
		}
	    }
	    if (distance[r][c] == 1 && dirs[r][c] > 0)
		n_inits++;
	    else if (dirs[r][c] > 0)
		distance[r][c] = -1;
	}

    d_inits = (POINT *) G_malloc(n_inits * sizeof(POINT));

    k = 0;
    for (r = 0; r < nrows; ++r)
	for (c = 0; c < ncols; ++c) {

	    if (distance[r][c] == 1) {

		distance[r][c] = 0;
		if (elevation)
		    elevation[r][c] = 0;

		d = dirs[r][c];

		if (dirs[NR(d)][NC(d)] < 0)
		    continue;

		d_inits[k].r = r;
		d_inits[k].c = c;
		d_inits[k].cur_dist = 0;


		if (elevation)
		    d_inits[k].target_elev = tmp_elevation[r][c];

		k++;
	    }
	}

    counter = n_inits = k;
    /* return 0; */
    G_message(_("Calculate upstream parameters..."));
    while (n_inits > 0) {
	k = 0;
	G_percent((counter - n_inits), counter, 10);
	for (i = 0; i < n_inits; ++i) {
	    r = d_inits[i].r;
	    c = d_inits[i].c;
	    d = dirs[r][c];
	    next_r = NR(d);
	    next_c = NC(d);
	    tmp_dist = d_inits[i].cur_dist;

	    if (elevation)
		target_elev = d_inits[i].target_elev;

	    easting = window.west + (c + 0.5) * window.ew_res;
	    northing = window.north - (r + 0.5) * window.ns_res;
	    cell_easting = window.west + (next_c + 0.5) * window.ew_res;
	    cell_northing = window.north - (next_r + 0.5) * window.ns_res;

	    cur_dist = tmp_dist +
		G_distance(easting, northing, cell_easting, cell_northing);

	    if (near)
		done = (distance[next_r][next_c] > cur_dist ||
			distance[next_r][next_c] <= 0) ? 1 : 0;
	    else
		done = (distance[next_r][next_c] < cur_dist ||
			distance[next_r][next_c] <= 0) ? 1 : 0;

	    if (done) {
		distance[next_r][next_c] = cur_dist;
		if (elevation) {
		    elevation[next_r][next_c] =
			target_elev - tmp_elevation[next_r][next_c];
		}
		if (dirs[NR(d)][NC(d)] < 1)
		    continue;

		d_inits[k].r = next_r;
		d_inits[k].c = next_c;
		d_inits[k].cur_dist = cur_dist;

		if (elevation)
		    d_inits[k].target_elev = target_elev;
		k++;
	    }			/* end of if done */
	}
	n_inits = k;
    }
    G_percent((counter - n_inits), counter, 10);
    return 0;
}


int seg_calculate_upstream(SEGMENT * distance, SEGMENT * dirs,
			   SEGMENT * elevation, SEGMENT * tmp_elevation,
			   int near)
{
    int r, c;
    int next_r, next_c;
    float easting, northing;
    float cell_easting, cell_northing;
    int i, j, k, d, d_next;
    FCELL minus_one_cell = -1;
    FCELL zero_cell = 0;
    int done;
    int counter;
    int n_inits = 0;
    float cur_dist;
    POINT *d_inits;
    float tmp_dist = 0;
    float target_elev = 0;
    CELL dirs_cell;
    FCELL distance_cell, elevation_cell, tmp_elevation_cell;
    /* size_t elevation_data_size; */
    struct Cell_head window;

    Rast_get_window(&window);

    if (elevation) {
        /* elevation_data_size = Rast_cell_size(FCELL_TYPE); */
	for (r = 0; r < nrows; ++r)
	    for (c = 0; c < ncols; ++c) {
		segment_get(elevation, &elevation_cell, r, c);
		segment_put(tmp_elevation, &elevation_cell, r, c);
	    }
    }

    for (r = 0; r < nrows; ++r)
	for (c = 0; c < ncols; ++c) {

	    segment_get(distance, &distance_cell, r, c);

	    for (i = 1; i < 9; ++i) {
		if (NOT_IN_REGION(i))
		    continue;	/* out of border */

		j = DIAG(i);
		next_r = NR(i);
		next_c = NC(i);

		segment_get(dirs, &dirs_cell, next_r, next_c);

		if (dirs_cell == j && distance_cell != 0) {	/* is contributing cell */
		    segment_put(distance, &minus_one_cell, r, c);
		    break;
		}
	    }			/* end for i */

	    segment_get(distance, &distance_cell, r, c);
	    segment_get(dirs, &dirs_cell, r, c);
	    if (distance_cell == 1) {
		if (distance_cell == 1 && dirs_cell > 0)
		    n_inits++;
		else if (dirs_cell > 0)
		    segment_put(distance, &minus_one_cell, r, c);
	    }

	}

    d_inits = (POINT *) G_malloc(n_inits * sizeof(POINT));

    k = 0;
    for (r = 0; r < nrows; ++r)
	for (c = 0; c < ncols; ++c) {

	    segment_get(distance, &distance_cell, r, c);
	    if (distance_cell == 1) {

		segment_put(distance, &zero_cell, r, c);
		if (elevation)
		    segment_put(elevation, &zero_cell, r, c);

		segment_get(dirs, &d, r, c);
		segment_get(dirs, &d_next, NR(d), NR(d));

		if (d_next < 0)
		    continue;

		d_inits[k].r = r;
		d_inits[k].c = c;
		d_inits[k].cur_dist = 0;

		if (elevation)
		    segment_get(tmp_elevation, &(d_inits[k].target_elev), r,
				c);
		k++;
	    }
	}

    counter = n_inits = k;

    G_message(_("Calculate upstream parameters..."));
    while (n_inits > 0) {
	k = 0;
	G_percent((counter - n_inits), counter, 10);

	for (i = 0; i < n_inits; ++i) {
	    r = d_inits[i].r;
	    c = d_inits[i].c;

	    segment_get(dirs, &d, r, c);
	    next_r = NR(d);
	    next_c = NC(d);
	    tmp_dist = d_inits[i].cur_dist;

	    if (elevation)
		target_elev = d_inits[i].target_elev;

	    easting = window.west + (c + 0.5) * window.ew_res;
	    northing = window.north - (r + 0.5) * window.ns_res;
	    cell_easting = window.west + (next_c + 0.5) * window.ew_res;
	    cell_northing = window.north - (next_r + 0.5) * window.ns_res;

	    cur_dist = tmp_dist +
		G_distance(easting, northing, cell_easting, cell_northing);

	    segment_get(distance, &distance_cell, next_r, next_c);

	    if (near)
		done = (distance_cell > cur_dist ||
			distance_cell <= 0) ? 1 : 0;
	    else
		done = (distance_cell < cur_dist ||
			distance_cell <= 0) ? 1 : 0;

	    if (done) {
		segment_put(distance, &cur_dist, next_r, next_c);
		if (elevation) {
		    segment_get(tmp_elevation, &tmp_elevation_cell, next_r,
				next_c);
		    tmp_elevation_cell = target_elev - tmp_elevation_cell;
		    segment_put(elevation, &tmp_elevation_cell, next_r,
				next_c);
		}

		segment_get(dirs, &dirs_cell, NR(d), NC(d));
		if (dirs_cell < 1)
		    continue;

		d_inits[k].r = next_r;
		d_inits[k].c = next_c;
		d_inits[k].cur_dist = cur_dist;

		if (elevation)
		    d_inits[k].target_elev = target_elev;
		k++;
	    }			/* end of if done */
	}
	n_inits = k;
    }
    G_percent(1, 1, 1);
    return 0;
}
