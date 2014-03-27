#include "local_proto.h"
static int sector_cat = 0;

float calc_dir(int rp, int cp, int rn, int cn)
{
    return
	(cp - cn) == 0 ?
	(rp - rn) > 0 ? 0 : PI :
	(cp - cn) < 0 ?
	PI / 2 + atan((rp - rn) / (float)(cp - cn)) :
	3 * PI / 2 + atan((rp - rn) / (float)(cp - cn));
}

float calc_length(double *distance, int start, int stop)
{
    float cum_length = 0;
    int i;

    for (i = start; i < stop; ++i)
	cum_length += distance[i];
    return cum_length;
}

double calc_drop(float *elevation, int start, int stop)
{
    float result;

    result = elevation[start] - elevation[stop];
    return result < 0 ? 0 : result;
}

double calc_stright(int rp, int cp, int rn, int cn)
{
    double northing, easting, next_northing, next_easting;

    northing = window.north - (rp + .5) * window.ns_res;
    easting = window.west + (cp + .5) * window.ew_res;
    next_northing = window.north - (rn + .5) * window.ns_res;
    next_easting = window.west + (cn + .5) * window.ew_res;
    return G_distance(easting, northing, next_easting, next_northing);
}

int create_sectors(STREAM *cur_stream, int seg_length, int seg_skip,
		   double seg_treshold)
{
    DIRCELLS *streamline;
    unsigned long int *P;	/* alias for points */

    int i, prev_i = 0;
    int number_of_cells;
    int cell_down, cell_up;
    int r, c, r_up, c_up, r_down, c_down;
    int seg_length_short = seg_length / 3;
    float dir_down, dir_up, dir_diff;

    float local_minimum = PI;
    int number_of_sectors = 0;
    int local_minimum_point = 0;
    int sector_index = 0;
    int in_loop = 0;
    int num_of_points = 0, num_of_breakpoints = 0;

    number_of_cells = cur_stream->number_of_cells - 1;
    P = cur_stream->points;

    streamline =
	(DIRCELLS *) G_malloc((number_of_cells + 1) * sizeof(DIRCELLS));

    /* init cells */
    for (i = 0; i < number_of_cells + 1; ++i) {
	streamline[i].long_dir_diff = 0;
	streamline[i].short_dir_diff = 0;
	streamline[i].long_break = 0;
	streamline[i].decision = 0;
    }

    /* upstream: to init, downstream: to outlet */
    for (i = seg_skip; i < number_of_cells - seg_skip; ++i) {
	cell_up = i < seg_length ? i : seg_length;
	cell_down = i > number_of_cells - 1 - seg_length ?
	    number_of_cells - 1 - i : seg_length;

	r = (int)P[i] / ncols;
	c = (int)P[i] % ncols;
	r_up = (int)P[i - cell_up] / ncols;
	c_up = (int)P[i - cell_up] % ncols;
	r_down = (int)P[i + cell_down] / ncols;
	c_down = (int)P[i + cell_down] % ncols;

	dir_down = calc_dir(r, c, r_down, c_down);
	dir_up = calc_dir(r, c, r_up, c_up);
	dir_diff = fabs(dir_up - dir_down);
	streamline[i].long_dir_diff =
	    dir_diff > PI ? PI * 2 - dir_diff : dir_diff;
	streamline[i].long_break =
	    (streamline[i].long_dir_diff < seg_treshold) ? 1 : 0;

	cell_up = i < seg_length_short ? i : seg_length_short;
	cell_down = i > number_of_cells - 1 - seg_length_short ?
	    number_of_cells - 1 - i : seg_length_short;

	r = (int)P[i] / ncols;
	c = (int)P[i] % ncols;
	r_up = (int)P[i - cell_up] / ncols;
	c_up = (int)P[i - cell_up] % ncols;
	r_down = (int)P[i + cell_down] / ncols;
	c_down = (int)P[i + cell_down] % ncols;

	dir_down = calc_dir(r, c, r_down, c_down);
	dir_up = calc_dir(r, c, r_up, c_up);
	dir_diff = fabs(dir_up - dir_down);
	streamline[i].short_dir_diff =
	    dir_diff > PI ? PI * 2 - dir_diff : dir_diff;
    }

    /* look for breakpoints */
    for (i = 0; i < number_of_cells; ++i) {

	if (streamline[i].long_break) {
	    num_of_breakpoints = 0;
	    if (local_minimum > streamline[i].short_dir_diff) {
		local_minimum = streamline[i].short_dir_diff;
		local_minimum_point = i;
		in_loop = 1;
	    }			/* end local minimum */

	}
	else if (!streamline[i].long_break && in_loop) {
	    num_of_breakpoints++;
	    if (num_of_breakpoints == (seg_length / 5)) {
		streamline[local_minimum_point].decision = 1;
		local_minimum = PI;
		in_loop = 0;
	    }
	}
    }

    /* cleaning breakpoints */
    for (i = 0, num_of_points = 0; i < number_of_cells; ++i, ++num_of_points) {

	if (streamline[i].decision) {
	    //printf("       BEFORE  %d %d\n",i,num_of_points);
	    if (i < seg_skip || (i > seg_skip && num_of_points < seg_skip)) {
		streamline[i].decision = 0;
		i = local_minimum_point;
	    }
	    else {
		local_minimum_point = i;
	    }
	    num_of_points = 0;
	}
    }

    /* number of segment in streamline */
    for (i = 0; i < number_of_cells + 1; ++i)
	if (streamline[i].decision == 1 || i == (number_of_cells - 1))
	    number_of_sectors++;


    cur_stream->number_of_sectors = number_of_sectors;
    cur_stream->sector_breakpoints =
	(int *)G_malloc(number_of_sectors * sizeof(int));
    cur_stream->sector_cats =
	(int *)G_malloc(number_of_sectors * sizeof(int));
    cur_stream->sector_directions =
	(float *)G_malloc(number_of_sectors * sizeof(float));
    cur_stream->sector_strights =
	(float *)G_malloc(number_of_sectors * sizeof(float));
    cur_stream->sector_lengths =
	(double *)G_malloc(number_of_sectors * sizeof(double));
    cur_stream->sector_drops =
	(float *)G_malloc(number_of_sectors * sizeof(float));

    /* add attributes */
    for (i = 0, prev_i = 0; i < number_of_cells + 1; ++i) {
	if (streamline[i].decision == 1 || i == (number_of_cells - 1)) {

	    r = (int)P[i] / ncols;
	    c = (int)P[i] % ncols;
	    r_up = (int)P[prev_i] / ncols;
	    c_up = (int)P[prev_i] % ncols;

	    cur_stream->sector_breakpoints[sector_index] = i;

	    cur_stream->sector_directions[sector_index] =
		calc_dir(r_up, c_up, r, c);

	    cur_stream->sector_lengths[sector_index] =
		calc_length(cur_stream->distance, prev_i, i);

	    cur_stream->sector_strights[sector_index] =
		calc_stright(r_up, c_up, r, c);

	    cur_stream->sector_drops[sector_index] =
		calc_drop(cur_stream->elevation, prev_i, i);

	    cur_stream->sector_cats[sector_index] = ++sector_cat;
	    sector_index++;
	    if (i < (number_of_cells - 1))
		prev_i = i;
	}
    }


    /*
       for (i = 0; i < number_of_cells; ++i)
       printf("%d | %f  %f |break %d | Dec %d  \n" ,i,
       streamline[i].long_dir_diff,
       streamline[i].short_dir_diff,
       streamline[i].long_break,
       streamline[i].decision); 

     */
    G_free(streamline);
    return 0;
}

int calc_tangents(STREAM *cur_stream, int seg_length, int seg_skip,
		  int number_streams)
{

    int i;
    int cell_up, cell_down;
    int r, c, r_up, c_up, r_down, c_down;
    STREAM *SA = stream_attributes;
    unsigned long int *P = cur_stream->points;
    int next_stream = cur_stream->next_stream;
    unsigned int outlet = cur_stream->outlet;
    int last_cell = cur_stream->number_of_cells - 1;
    int reached_end = 1;

    G_debug(3, "calc_tangents(): number_streams=%d", number_streams);
    /*before calc tangents add rest of streamline attributes */
    r_up = (int)P[1] / ncols;
    c_up = (int)P[1] % ncols;
    r_down = (int)P[last_cell] / ncols;
    c_down = (int)P[last_cell] % ncols;

    cur_stream->direction = calc_dir(r_up, c_up, r_down, c_down);
    cur_stream->length = calc_length(cur_stream->distance, 1, last_cell);
    cur_stream->stright = calc_stright(r_up, c_up, r_down, c_down);
    cur_stream->drop = calc_drop(cur_stream->elevation, 1, last_cell);

    if (next_stream < 1) {
	cur_stream->tangent = -1;
	cur_stream->continuation = -1;
	return 0;
    }

    /* find location of outlet in next stream */
    for (i = 1; i < SA[next_stream].number_of_cells; ++i) {
	if (SA[next_stream].points[i] == outlet) {
	    reached_end = 0;
	    break;
	}
    }

    /* outlet not lies on the next stream */
    if (reached_end) {
	G_warning(_("Network topology error: cannot identify stream join for stream %d"),
		  cur_stream->stream);
	cur_stream->tangent = -1;
	cur_stream->continuation = -1;
	return 0;
    }

    cell_up = i <= seg_length ? i - 1 : seg_length;
    cell_down = i >= (SA[next_stream].number_of_cells - seg_length) ?
	SA[next_stream].number_of_cells - seg_length - 1 : seg_length;

    r = (int)SA[next_stream].points[i] / ncols;
    c = (int)SA[next_stream].points[i] % ncols;
    r_up = (int)SA[next_stream].points[i - cell_up] / ncols;
    c_up = (int)SA[next_stream].points[i - cell_up] % ncols;
    r_down = (int)SA[next_stream].points[i + cell_down] / ncols;
    c_down = (int)SA[next_stream].points[i + cell_down] % ncols;

    cur_stream->continuation = calc_dir(r, c, r_down, c_down);
    cur_stream->tangent = i == 1 ? -1 :
	i < seg_skip ? cur_stream->continuation : calc_dir(r_up, c_up, r_down,
							   c_down);

    return 0;
}
