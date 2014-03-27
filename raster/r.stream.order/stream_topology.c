#include "local_proto.h"

int ram_number_of_tribs(int r, int c, CELL **streams, CELL **dirs)
{

    int trib = 0;
    int i, j;

    for (i = 1; i < 9; ++i) {
	if (NOT_IN_REGION(i))
	    continue;
	j = DIAG(i);
	if (streams[NR(i)][NC(i)] && dirs[NR(i)][NC(i)] == j)
	    trib++;
    }

    if (trib > 5)
	G_fatal_error(_("Error finding nodes. "
                        "Stream and direction maps probably do not match."));
    if (trib > 3)
	G_warning(_("Stream network may be too dense"));

    return trib;
}

int ram_stream_topology(CELL **streams, CELL **dirs, int number_of_streams)
{

    int d, i, j;		/* d: direction, i: iteration */
    int r, c;
    int next_r, next_c;
    int trib_num, trib = 0;
    int next_stream = -1, cur_stream;
    STREAM *SA = stream_attributes;	/* for better code readability */

    init_num = 0, outlet_num = 0;

    G_message(_("Finding nodes..."));

    outlet_streams = (unsigned int *)G_malloc((number_of_streams) *
					      sizeof(unsigned int));
    init_streams = (unsigned int *)G_malloc((number_of_streams) *
					    sizeof(unsigned int));
    init_cells = (unsigned
		  long int *)G_malloc((number_of_streams) * sizeof(unsigned
								   long int));
    /* free at the end */

    for (r = 0; r < nrows; ++r)
	for (c = 0; c < ncols; ++c)
	    if (streams[r][c]) {
		trib_num = ram_number_of_tribs(r, c, streams, dirs);
		trib = 0;
		d = abs(dirs[r][c]);	/* r.watershed! */
		if (d < 1 || NOT_IN_REGION(d) || !streams[NR(d)][NC(d)])
		    next_stream = -1;
		else
		    next_stream = streams[NR(d)][NC(d)];

		cur_stream = streams[r][c];

		if (cur_stream != next_stream) {	/* junction: building topology */

		    if (outlet_num > (number_of_streams - 1))
			G_fatal_error(_("Error finding nodes. "
                                        "Stream and direction maps probably do not match."));

		    SA[cur_stream].stream = cur_stream;
		    SA[cur_stream].next_stream = next_stream;

		    if (next_stream < 0)	/* is outlet stream */
			outlet_streams[outlet_num++] = cur_stream;
		}

		if (trib_num == 0) {	/* is init */
		    if (init_num > (number_of_streams - 1))
			G_fatal_error(_("Error finding nodes. "
                                        "Stream and direction maps probably do not match."));

		    SA[cur_stream].trib_num = 0;
		    init_cells[init_num] = r * ncols + c;
		    init_streams[init_num++] = cur_stream;	/* collecting inits */
		}

		if (trib_num > 1) {	/* adding tributuaries */
		    SA[cur_stream].trib_num = trib_num;

		    for (i = 1; i < 9; ++i) {
			if (trib > 4)
			    G_fatal_error(_("Error finding nodes. "
                                            "Stream and direction maps probably do not match."));
			if (NOT_IN_REGION(i))
			    continue;
			j = DIAG(i);
			next_r = NR(i);
			next_c = NC(i);
			if (streams[next_r][next_c] &&
			    dirs[next_r][next_c] == j)
			    SA[cur_stream].trib[trib++] =
				streams[next_r][next_c];
		    }		/* end for i... */
		}
	    }			/* end if streams */
    return 0;
}

int ram_stream_geometry(CELL **streams, CELL **dirs)
{

    int i, s, d;		/* s - streams index; d - direction */
    int done = 1;
    int r, c;
    int next_r, next_c;
    int prev_r, prev_c;
    int cur_stream;
    float cur_northing, cur_easting;
    float next_northing, next_easting;
    float init_northing, init_easting;
    double cur_length = 0.;
    double cur_accum_length = 0.;
    STREAM *SA = stream_attributes;	/* for better code readability */
    struct Cell_head window;

    G_get_window(&window);

    G_message(_("Finding longest streams..."));
    G_begin_distance_calculations();

    for (s = 0; s < init_num; ++s) {	/* main loop on springs */
	r = (int)init_cells[s] / ncols;
	c = (int)init_cells[s] % ncols;
	cur_stream = streams[r][c];
	cur_length = 0;
	done = 1;

	SA[cur_stream].init = init_cells[s];	/* stored as index */

	init_northing = window.north - (r + .5) * window.ns_res;
	init_easting = window.west + (c + .5) * window.ew_res;

	while (done) {
	    cur_northing = window.north - (r + .5) * window.ns_res;
	    cur_easting = window.west + (c + .5) * window.ew_res;

	    d = abs(dirs[r][c]);
	    next_r = NR(d);
	    next_c = NC(d);

	    if (d < 1 || NOT_IN_REGION(d) || !streams[next_r][next_c]) {
		cur_length = (window.ns_res + window.ew_res) / 2;
		SA[cur_stream].accum_length += cur_length;
		SA[cur_stream].length += cur_length;
		SA[cur_stream].stright =
		    G_distance(cur_easting, cur_northing, init_easting,
			       init_northing);
		SA[cur_stream].outlet = (r * ncols + c);	/* add outlet to sorting */
		break;
	    }

	    next_northing = window.north - (next_r + .5) * window.ns_res;
	    next_easting = window.west + (next_c + .5) * window.ew_res;
	    cur_length =
		G_distance(next_easting, next_northing, cur_easting,
			   cur_northing);
	    SA[cur_stream].accum_length += cur_length;
	    SA[cur_stream].length += cur_length;
	    prev_r = r;
	    prev_c = c;
	    r = next_r;
	    c = next_c;

	    if (streams[next_r][next_c] != cur_stream) {
		SA[cur_stream].stright =
		    G_distance(next_easting, next_northing, init_easting,
			       init_northing);
		init_northing = cur_northing;
		init_easting = cur_easting;

		SA[cur_stream].outlet = (prev_r * ncols + prev_c);
		cur_stream = streams[next_r][next_c];

		cur_accum_length = 0;
		SA[cur_stream].init = (r * ncols + c);

		for (i = 0; i < SA[cur_stream].trib_num; ++i) {
		    if (SA[SA[cur_stream].trib[i]].accum_length == 0) {
			done = 0;
			cur_accum_length = 0;
			break;	/* do not pass accum */
		    }
		    if (SA[SA[cur_stream].trib[i]].accum_length >
			cur_accum_length)
			cur_accum_length =
			    SA[SA[cur_stream].trib[i]].accum_length;
		}		/* end for i */
		SA[cur_stream].accum_length = cur_accum_length;
	    }			/* end if */
	}			/* end while */
    }				/* end for s */
    return 0;
}

int seg_number_of_tribs(int r, int c, SEGMENT *streams, SEGMENT *dirs)
{

    int trib = 0;
    int i, j;
    int streams_cell = 0;
    int dirs_cell = 0;

    for (i = 1; i < 9; ++i) {
	if (NOT_IN_REGION(i))
	    continue;

	j = DIAG(i);

	segment_get(streams, &streams_cell, NR(i), NC(i));
	segment_get(dirs, &dirs_cell, NR(i), NC(i));

	if (streams_cell && dirs_cell == j)
	    trib++;
    }

    if (trib > 5)
	G_fatal_error(_("Error finding nodes. "
                        "Stream and direction maps probably do not match."));
    if (trib > 3)
	G_warning(_("Stream network may be too dense"));

    return trib;
}

int seg_stream_topology(SEGMENT *streams, SEGMENT *dirs,
			int number_of_streams)
{

    int d, i, j;		/* d: direction, i: iteration */
    int r, c;
    int next_r, next_c;
    int trib_num, trib = 0;
    int next_stream = -1, cur_stream;
    int streams_cell, dirs_cell;
    int next_streams_cell, trib_dirs_cell, trib_stream_cell;
    STREAM *SA = stream_attributes;	/* for better code readability */

    init_num = 0, outlet_num = 0;

    G_message(_("Finding nodes..."));

    outlet_streams = (unsigned int *)G_malloc((number_of_streams) *
					      sizeof(unsigned int));
    init_streams = (unsigned int *)G_malloc((number_of_streams) *
					    sizeof(unsigned int));
    init_cells = (unsigned
		  long int *)G_malloc((number_of_streams) * sizeof(unsigned
								   long int));

    for (r = 0; r < nrows; ++r) {
	G_percent(r, nrows, 2);
	for (c = 0; c < ncols; ++c) {
	    segment_get(streams, &streams_cell, r, c);
	    segment_get(dirs, &dirs_cell, r, c);

	    if (streams_cell) {
		trib_num = seg_number_of_tribs(r, c, streams, dirs);
		trib = 0;

		d = abs(dirs_cell);	/* r.watershed! */
		if (NOT_IN_REGION(d))
		    next_stream = -1;
		else
		    segment_get(streams, &next_streams_cell, NR(d), NC(d));

		if (d < 1 || NOT_IN_REGION(d) || !next_streams_cell)
		    next_stream = -1;
		else
		    segment_get(streams, &next_stream, NR(d), NC(d));

		cur_stream = streams_cell;

		if (cur_stream != next_stream) {	/* junction: building topology */
		    if (outlet_num > (number_of_streams - 1))
			G_fatal_error(_("Error finding nodes. "
                                        "Stream and direction maps probably do not match."));

		    SA[cur_stream].stream = cur_stream;
		    SA[cur_stream].next_stream = next_stream;

		    if (next_stream < 0)	/* is outlet stream */
			outlet_streams[outlet_num++] = cur_stream;
		}

		if (trib_num == 0) {	/* is init */
		    if (init_num > (number_of_streams - 1))
			G_fatal_error(_("Error finding nodes. "
                                        "Stream and direction maps probably do not match."));

		    SA[cur_stream].trib_num = 0;
		    init_cells[init_num] = r * ncols + c;
		    init_streams[init_num++] = cur_stream;	/* collecting inits */
		}

		if (trib_num > 1) {	/* adding tributuaries */
		    SA[cur_stream].trib_num = trib_num;

		    for (i = 1; i < 9; ++i) {

			if (trib > 4)
			    G_fatal_error(_("Error finding nodes. "
                                            "Stream and direction maps probably do not match."));
			if (NOT_IN_REGION(i))
			    continue;
			j = DIAG(i);
			next_r = NR(i);
			next_c = NC(i);
			segment_get(streams, &trib_stream_cell, next_r,
				    next_c);
			segment_get(dirs, &trib_dirs_cell, next_r, next_c);

			if (trib_stream_cell && trib_dirs_cell == j)
			    SA[cur_stream].trib[trib++] = trib_stream_cell;
		    }		/* end for i... */
		}
	    }			/* end if streams */
	}
    }				/* end r, c */
    G_percent(r, nrows, 2);
    return 0;
}

int seg_stream_geometry(SEGMENT *streams, SEGMENT *dirs)
{

    int i, s, d;		/* s - streams index; d - direction */
    int done = 1;
    int r, c;
    int next_r, next_c;
    int prev_r, prev_c;
    int cur_stream, next_stream, dirs_cell;
    float cur_northing, cur_easting;
    float next_northing, next_easting;
    float init_northing, init_easting;
    double cur_length = 0.;
    double cur_accum_length = 0.;
    STREAM *SA = stream_attributes;	/* for better code readability */
    struct Cell_head window;

    G_get_window(&window);

    G_message(_("Finding longest streams..."));
    G_begin_distance_calculations();

    for (s = 0; s < init_num; ++s) {	/* main loop on springs */
	G_percent(s, init_num, 2);
	r = (int)init_cells[s] / ncols;
	c = (int)init_cells[s] % ncols;
	segment_get(streams, &cur_stream, r, c);
	cur_length = 0;
	done = 1;

	SA[cur_stream].init = init_cells[s];	/* stored as index */

	init_northing = window.north - (r + .5) * window.ns_res;
	init_easting = window.west + (c + .5) * window.ew_res;

	while (done) {
	    cur_northing = window.north - (r + .5) * window.ns_res;
	    cur_easting = window.west + (c + .5) * window.ew_res;

	    segment_get(dirs, &dirs_cell, r, c);
	    d = abs(dirs_cell);
	    next_r = NR(d);
	    next_c = NC(d);
	    if (NOT_IN_REGION(d))
		Rast_set_c_null_value(&next_stream, 1);
	    else
		segment_get(streams, &next_stream, next_r, next_c);

	    if (d < 1 || NOT_IN_REGION(d) || !next_stream) {
		cur_length = (window.ns_res + window.ew_res) / 2;
		SA[cur_stream].accum_length += cur_length;
		SA[cur_stream].length += cur_length;
		SA[cur_stream].stright =
		    G_distance(cur_easting, cur_northing, init_easting,
			       init_northing);
		SA[cur_stream].outlet = (r * ncols + c);	/* add outlet to sorting */
		break;
	    }

	    next_northing = window.north - (next_r + .5) * window.ns_res;
	    next_easting = window.west + (next_c + .5) * window.ew_res;
	    cur_length =
		G_distance(next_easting, next_northing, cur_easting,
			   cur_northing);
	    SA[cur_stream].accum_length += cur_length;
	    SA[cur_stream].length += cur_length;
	    prev_r = r;
	    prev_c = c;
	    r = next_r;
	    c = next_c;

	    if (next_stream != cur_stream) {
		SA[cur_stream].stright =
		    G_distance(next_easting, next_northing, init_easting,
			       init_northing);
		init_northing = cur_northing;
		init_easting = cur_easting;

		SA[cur_stream].outlet = (prev_r * ncols + prev_c);
		cur_stream = next_stream;
		cur_accum_length = 0;
		SA[cur_stream].init = (r * ncols + c);

		for (i = 0; i < SA[cur_stream].trib_num; ++i) {
		    if (SA[SA[cur_stream].trib[i]].accum_length == 0) {
			done = 0;
			cur_accum_length = 0;
			break;	/* do not pass accum */
		    }
		    if (SA[SA[cur_stream].trib[i]].accum_length >
			cur_accum_length)
			cur_accum_length =
			    SA[SA[cur_stream].trib[i]].accum_length;
		}		/* end for i */
		SA[cur_stream].accum_length = cur_accum_length;
	    }			/* end if */
	}			/* end while */
    }				/* end for s */
    G_percent(1, 1, 1);
    return 0;
}
