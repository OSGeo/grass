#include "local_proto.h"

double get_distance(int r, int c, int d)
{
    double northing, easting, next_northing, next_easting;
    int next_r, next_c;

    next_r = NR(d);
    next_c = NC(d);
    northing = window.north - (r + .5) * window.ns_res;
    easting = window.west + (c + .5) * window.ew_res;
    next_northing = window.north - (next_r + .5) * window.ns_res;
    next_easting = window.west + (next_c + .5) * window.ew_res;

    return G_distance(easting, northing, next_easting, next_northing);
}

int ram_trib_nums(int r, int c, CELL ** streams, CELL ** dirs)
{				/* calculate number of tributaries */

    int trib_num = 0;
    int i, j;
    int next_r, next_c;

    for (i = 1; i < 9; ++i) {
	if (NOT_IN_REGION(i))
	    continue;

	j = DIAG(i);
	next_r = NR(i);
	next_c = NC(i);

	if (streams[next_r][next_c] > 0 && dirs[next_r][next_c] == j)
	    trib_num++;
    }

    if (trib_num > 1)
	for (i = 1; i < 9; ++i) {
	    if (NOT_IN_REGION(i))
		continue;

	    j = DIAG(i);
	    next_r = NR(i);
	    next_c = NC(i);

	    if (streams[next_r][next_c] == streams[r][c] &&
		dirs[next_r][next_c] == j)
		trib_num--;
	}

    if (trib_num > 5)
	G_fatal_error(_("Error finding inits. Stream and direction maps probably do not match"));
    if (trib_num > 3)
	G_warning(_("Stream network may be too dense"));

    return trib_num;
}				/* end trib_num */



int ram_number_of_streams(CELL **streams, CELL **dirs)
{
    int r, c;
    int stream_num = 0;

    for (r = 0; r < nrows; ++r)
	for (c = 0; c < ncols; ++c)
	    if (streams[r][c] > 0)
		if (ram_trib_nums(r, c, streams, dirs) != 1)
		    stream_num++;
    return stream_num;
}

int ram_build_streamlines(CELL **streams, CELL **dirs, FCELL **elevation,
			  int number_of_streams)
{
    int r, c, i;
    int d, next_d;
    int prev_r, prev_c;
    int stream_num = 1, cell_num = 0;
    int contrib_cell;
    STREAM *SA;

    stream_num = 1;

    Rast_get_window(&window);

    stream_attributes =
	(STREAM *) G_malloc(number_of_streams * sizeof(STREAM));
    G_message(_("Finding inits..."));
    SA = stream_attributes;

    for (r = 0; r < nrows; ++r)
	for (c = 0; c < ncols; ++c)
	    if (streams[r][c])
		if (ram_trib_nums(r, c, streams, dirs) != 1) {	/* adding inits */
		    if (stream_num > number_of_streams)
			G_fatal_error(_("Error finding inits. Stream and direction maps probably do not match"));

		    SA[stream_num].stream_num = stream_num;
		    SA[stream_num].init_r = r;
		    SA[stream_num++].init_c = c;
		}

    for (i = 1; i < stream_num; ++i) {

	r = SA[i].init_r;
	c = SA[i].init_c;
	SA[i].order = streams[r][c];
	SA[i].number_of_cells = 0;
	do {

	    SA[i].number_of_cells++;
	    d = abs(dirs[r][c]);
	    if (NOT_IN_REGION(d) || d == 0)
		break;
	    r = NR(d);
	    c = NC(d);
	} while (streams[r][c] == SA[i].order);

	SA[i].number_of_cells += 2;	/* add two extra points for init+ and outlet+ */
    }

    for (i = 1; i < number_of_streams; ++i) {

	SA[i].points = (unsigned long int *)
	    G_malloc((SA[i].number_of_cells) * sizeof(unsigned long int));
	SA[i].elevation = (float *)
	    G_malloc((SA[i].number_of_cells) * sizeof(float));
	SA[i].distance = (double *)
	    G_malloc((SA[i].number_of_cells) * sizeof(double));

	r = SA[i].init_r;
	c = SA[i].init_c;
	contrib_cell = ram_find_contributing_cell(r, c, dirs, elevation);
	prev_r = NR(contrib_cell);
	prev_c = NC(contrib_cell);

	/* add one point contributing to init to calculate parameters */
	/* what to do if there is no contributing points? */
	SA[i].points[0] = (contrib_cell == 0) ? -1 : INDEX(prev_r, prev_c);
	SA[i].elevation[0] = (contrib_cell == 0) ? -99999 :
	    elevation[prev_r][prev_c];
	d = (contrib_cell == 0) ? dirs[r][c] : dirs[prev_r][prev_c];
	SA[i].distance[0] = (contrib_cell == 0) ? get_distance(r, c, d) :
	    get_distance(prev_r, prev_c, d);

	SA[i].points[1] = INDEX(r, c);
	SA[i].elevation[1] = elevation[r][c];
	d = abs(dirs[r][c]);
	SA[i].distance[1] = get_distance(r, c, d);

	cell_num = 2;
	do {
	    d = abs(dirs[r][c]);

	    if (NOT_IN_REGION(d) || d == 0) {
		SA[i].points[cell_num] = -1;
		SA[i].distance[cell_num] = SA[i].distance[cell_num - 1];
		SA[i].elevation[cell_num] =
		    2 * SA[i].elevation[cell_num - 1] -
		    SA[i].elevation[cell_num - 2];
		break;
	    }
	    r = NR(d);
	    c = NC(d);
	    SA[i].points[cell_num] = INDEX(r, c);
	    SA[i].elevation[cell_num] = elevation[r][c];
	    next_d = (abs(dirs[r][c]) == 0) ? d : abs(dirs[r][c]);
	    SA[i].distance[cell_num] = get_distance(r, c, next_d);
	    cell_num++;
	    if (cell_num > SA[i].number_of_cells)
		G_fatal_error(_("To many points in stream line"));
	} while (streams[r][c] == SA[i].order);

	if (SA[i].elevation[0] == -99999)
	    SA[i].elevation[0] = 2 * SA[i].elevation[1] - SA[i].elevation[2];
    }

    return 0;
}


int seg_trib_nums(int r, int c, SEGMENT * streams, SEGMENT * dirs)
{				/* calculate number of tributaries */

    int trib_num = 0;
    int i, j;
    int next_r, next_c;
    int streams_cell, streams_next_cell, dirs_next_cell;

    segment_get(streams, &streams_cell, r, c);
    for (i = 1; i < 9; ++i) {
	if (NOT_IN_REGION(i))
	    continue;

	j = DIAG(i);
	next_r = NR(i);
	next_c = NC(i);
	segment_get(streams, &streams_next_cell, next_r, next_c);
	segment_get(dirs, &dirs_next_cell, next_r, next_c);
	if (streams_next_cell > 0 && dirs_next_cell == j)
	    trib_num++;
    }

    if (trib_num > 1)
	for (i = 1; i < 9; ++i) {
	    if (NOT_IN_REGION(i))
		continue;

	    j = DIAG(i);
	    next_r = NR(i);
	    next_c = NC(i);
	    segment_get(streams, &streams_next_cell, next_r, next_c);
	    segment_get(dirs, &dirs_next_cell, next_r, next_c);
	    if (streams_next_cell == streams_cell && dirs_next_cell == j)
		trib_num--;
	}

    if (trib_num > 5)
	G_fatal_error(_("Error finding inits. Stream and direction maps probably do not match..."));
    if (trib_num > 3)
	G_warning(_("Stream network may be too dense..."));

    return trib_num;
}				/* end trib_num */

int seg_number_of_streams(SEGMENT *streams, SEGMENT *dirs)
{
    int r, c;
    int stream_num = 0;
    int streams_cell;

    for (r = 0; r < nrows; ++r)
	for (c = 0; c < ncols; ++c) {
	    segment_get(streams, &streams_cell, r, c);
	    if (streams_cell > 0)
		if (seg_trib_nums(r, c, streams, dirs) != 1)
		    stream_num++;
	}
    G_message("%d", stream_num);
    return stream_num;
}

int seg_build_streamlines(SEGMENT *streams, SEGMENT *dirs,
			  SEGMENT *elevation, int number_of_streams)
{
    int r, c, i;
    int d, next_d;
    int prev_r, prev_c;
    int streams_cell, dirs_cell;
    float elevation_prev_cell;
    int stream_num = 1, cell_num = 0;
    int contrib_cell;
    STREAM *SA;

    stream_num = 1;


    stream_attributes =
	(STREAM *) G_malloc(number_of_streams * sizeof(STREAM));
    G_message("Finding inits...");
    SA = stream_attributes;

    /* finding inits */
    for (r = 0; r < nrows; ++r)
	for (c = 0; c < ncols; ++c) {
	    segment_get(streams, &streams_cell, r, c);

	    if (streams_cell)
		if (seg_trib_nums(r, c, streams, dirs) != 1) {	/* adding inits */
		    if (stream_num > number_of_streams)
			G_fatal_error(_("Error finding inits. Stream and direction maps probably do not match"));

		    SA[stream_num].stream_num = stream_num;
		    SA[stream_num].init_r = r;
		    SA[stream_num++].init_c = c;
		}
	}

    /* building streamline */
    for (i = 1; i < stream_num; ++i) {

	r = SA[i].init_r;
	c = SA[i].init_c;
	segment_get(streams, &(SA[i].order), r, c);
	//segment_get(streams,&streams_cell,r,c);
	SA[i].number_of_cells = 0;

	do {
	    SA[i].number_of_cells++;
	    segment_get(dirs, &dirs_cell, r, c);

	    d = abs(dirs_cell);
	    if (NOT_IN_REGION(d) || d == 0)
		break;
	    r = NR(d);
	    c = NC(d);
	    segment_get(streams, &streams_cell, r, c);
	} while (streams_cell == SA[i].order);

	SA[i].number_of_cells += 2;	/* add two extra points for init+ and outlet+ */
    }

    for (i = 1; i < number_of_streams; ++i) {

	SA[i].points = (unsigned long int *)
	    G_malloc((SA[i].number_of_cells) * sizeof(unsigned long int));
	SA[i].elevation = (float *)
	    G_malloc((SA[i].number_of_cells) * sizeof(float));
	SA[i].distance = (double *)
	    G_malloc((SA[i].number_of_cells) * sizeof(double));

	r = SA[i].init_r;
	c = SA[i].init_c;
	contrib_cell =
	    seg_find_contributing_cell(r, c, dirs, elevation);
	prev_r = NR(contrib_cell);
	prev_c = NC(contrib_cell);

	/* add one point contributing to init to calculate parameters */
	/* what to do if there is no contributing points? */
	SA[i].points[0] = (contrib_cell == 0) ? -1 : INDEX(prev_r, prev_c);

	segment_get(elevation, &elevation_prev_cell, prev_r, prev_c);
	SA[i].elevation[0] = (contrib_cell == 0) ? -99999 :
	    elevation_prev_cell;

	if (contrib_cell == 0)
	    segment_get(dirs, &d, r, c);
	else
	    segment_get(dirs, &d, prev_r, prev_c);
	SA[i].distance[0] = (contrib_cell == 0) ? get_distance(r, c, d) :
	    get_distance(prev_r, prev_c, d);

	SA[i].points[1] = INDEX(r, c);
	segment_get(elevation, &(SA[i].elevation[1]), r, c);
	segment_get(dirs, &d, r, c);
	SA[i].distance[1] = get_distance(r, c, d);

	cell_num = 2;
	do {
	    segment_get(dirs, &dirs_cell, r, c);
	    d = abs(dirs_cell);

	    if (NOT_IN_REGION(d) || d == 0) {
		SA[i].points[cell_num] = -1;
		SA[i].distance[cell_num] = SA[i].distance[cell_num - 1];
		SA[i].elevation[cell_num] =
		    2 * SA[i].elevation[cell_num - 1] -
		    SA[i].elevation[cell_num - 2];
		break;
	    }
	    r = NR(d);
	    c = NC(d);
	    SA[i].points[cell_num] = INDEX(r, c);
	    segment_get(elevation, &(SA[i].elevation[cell_num]), r, c);
	    segment_get(dirs, &next_d, r, c);
	    next_d = (abs(next_d) == 0) ? d : abs(next_d);
	    SA[i].distance[cell_num] = get_distance(r, c, next_d);
	    cell_num++;
	    segment_get(streams, &streams_cell, r, c);
	    if (cell_num > SA[i].number_of_cells)
		G_fatal_error(_("To much points in stream line..."));
	} while (streams_cell == SA[i].order);

	if (SA[i].elevation[0] == -99999)
	    SA[i].elevation[0] = 2 * SA[i].elevation[1] - SA[i].elevation[2];
    }

    return 0;
}

int ram_find_contributing_cell(int r, int c, CELL **dirs, FCELL **elevation)
{
    int i, j = 0;
    int next_r, next_c;
    float elev_min = 9999;

    for (i = 1; i < 9; ++i) {
	if (NOT_IN_REGION(i))
	    continue;
	next_r = NR(i);
	next_c = NC(i);
	if (dirs[next_r][next_c] == DIAG(i) &&
	    elevation[next_r][next_c] < elev_min) {
	    elev_min = elevation[next_r][next_c];
	    j = i;
	}
    }

    return j;
}

int seg_find_contributing_cell(int r, int c, SEGMENT *dirs,
			       SEGMENT *elevation)
{
    int i, j = 0;
    int next_r, next_c;
    float elev_min = 9999;
    int dirs_next_cell;
    float elevation_next_cell;

    for (i = 1; i < 9; ++i) {
	if (NOT_IN_REGION(i))
	    continue;
	next_r = NR(i);
	next_c = NC(i);
	segment_get(dirs, &dirs_next_cell, next_r, next_c);
	segment_get(elevation, &elevation_next_cell, next_r, next_c);
	if (dirs_next_cell == DIAG(i) && elevation_next_cell < elev_min) {
	    elev_min = elevation_next_cell;
	    j = i;
	}
    }
    return j;
}

int free_attributes(int number_of_streams)
{
    int i;
    STREAM *SA;

    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {
	G_free(SA[i].points);
	G_free(SA[i].elevation);
	G_free(SA[i].distance);
    }
    G_free(stream_attributes);

    return 0;
}
