#include "local_proto.h"
int ram_set_null_output(DCELL **output)
{
    int r;

    for (r = 0; r < nrows; ++r)
	Rast_set_d_null_value(output[r], ncols);

    return 0;
}

int seg_set_null_output(SEGMENT *output)
{
    int r, c;
    double output_cell;

    for (r = 0; r < nrows; ++r) {
	for (c = 0; c < ncols; ++c) {
	    Rast_set_d_null_value(&output_cell, 1);
	    segment_put(output, &output_cell, r, c);
	}
    }

    return 0;
}

int ram_calculate_identifiers(CELL **identifier, int number_of_streams,
			      int downstream)
{
    int r, c;
    int i, j;
    STREAM *SA;

    G_debug(3, "ram_calculate_identifiers(): downstream=%d", downstream);
    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {
	for (j = 1; j < SA[i].number_of_cells - 1; ++j) {
	    r = (int)SA[i].points[j] / ncols;
	    c = (int)SA[i].points[j] % ncols;
	    identifier[r][c] = SA[i].stream_num;
	}
    }

    return 0;
}

int seg_calculate_identifiers(SEGMENT *identifier, int number_of_streams,
			      int downstream)
{
    int r, c;
    int i, j;
    STREAM *SA;

    G_debug(3, "seg_calculate_identifiers(): downstream=%d", downstream);
    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {
	for (j = 1; j < SA[i].number_of_cells - 1; ++j) {
	    r = (int)SA[i].points[j] / ncols;
	    c = (int)SA[i].points[j] % ncols;
	    segment_put(identifier, &(SA[i].stream_num), r, c);
	}
    }

    return 0;
}


int ram_calculate_distance(DCELL **output, int number_of_streams,
			   int downstream)
{
    int r, c;
    double cum_length;
    int i, j;
    STREAM *SA;

    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {
	cum_length = 0;
	if (!downstream)
	    for (j = 1; j < SA[i].number_of_cells - 1; ++j) {
		cum_length += SA[i].distance[j];
		r = (int)SA[i].points[j] / ncols;
		c = (int)SA[i].points[j] % ncols;
		output[r][c] = cum_length;
	    }
	else
	    for (j = SA[i].number_of_cells - 2; j > 0; --j) {
		cum_length += SA[i].distance[j];
		r = (int)SA[i].points[j] / ncols;
		c = (int)SA[i].points[j] % ncols;
		output[r][c] = cum_length;
	    }
    }

    return 0;
}

int seg_calculate_distance(SEGMENT *output, int number_of_streams,
			   int downstream)
{
    int r, c;
    double cum_length;
    int i, j;
    STREAM *SA;

    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {
	cum_length = 0;
	if (!downstream)
	    for (j = 1; j < SA[i].number_of_cells - 1; ++j) {
		cum_length += SA[i].distance[j];
		r = (int)SA[i].points[j] / ncols;
		c = (int)SA[i].points[j] % ncols;
		segment_put(output, &cum_length, r, c);
	    }
	else
	    for (j = SA[i].number_of_cells - 2; j > 0; --j) {
		cum_length += SA[i].distance[j];
		r = (int)SA[i].points[j] / ncols;
		c = (int)SA[i].points[j] % ncols;
		segment_put(output, &cum_length, r, c);
	    }
    }

    return 0;
}

int ram_calculate_cell(DCELL **output, int number_of_streams, int downstream)
{
    int r, c;
    int i, j, k;
    STREAM *SA;

    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {

	k = SA[i].number_of_cells - 2;
	for (j = 1; j < SA[i].number_of_cells - 1; ++j, --k) {
	    r = (int)SA[i].points[j] / ncols;
	    c = (int)SA[i].points[j] % ncols;
	    output[r][c] = downstream ? k : j;
	}
    }

    return 0;
}

int seg_calculate_cell(SEGMENT *output, int number_of_streams,
		       int downstream)
{
    int r, c;
    int i, j, k;
    double output_cell;
    STREAM *SA;

    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {

	k = SA[i].number_of_cells - 2;
	for (j = 1; j < SA[i].number_of_cells - 1; ++j, --k) {
	    r = (int)SA[i].points[j] / ncols;
	    c = (int)SA[i].points[j] % ncols;
	    output_cell = downstream ? k : j;
	    segment_put(output, &output_cell, r, c);
	}
    }

    return 0;
}

int ram_calculate_difference(DCELL **output, int number_of_streams,
			     int downstream)
{
    int r, c;
    int i, j;
    double result;
    STREAM *SA;

    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {

	for (j = 1; j < SA[i].number_of_cells - 1; ++j) {
	    result = downstream ?
		SA[i].elevation[j - 1] - SA[i].elevation[j] :
		SA[i].elevation[j] - SA[i].elevation[j + 1];
	    r = (int)SA[i].points[j] / ncols;
	    c = (int)SA[i].points[j] % ncols;
	    output[r][c] = result;
	}
    }

    return 0;
}

int seg_calculate_difference(SEGMENT *output, int number_of_streams,
			     int downstream)
{
    int r, c;
    int i, j;
    double output_cell;
    STREAM *SA;

    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {

	for (j = 1; j < SA[i].number_of_cells - 1; ++j) {
	    output_cell = downstream ?
		SA[i].elevation[j - 1] - SA[i].elevation[j] :
		SA[i].elevation[j] - SA[i].elevation[j + 1];
	    r = (int)SA[i].points[j] / ncols;
	    c = (int)SA[i].points[j] % ncols;
	    segment_put(output, &output_cell, r, c);
	}
    }

    return 0;
}

int ram_calculate_drop(DCELL **output, int number_of_streams, int downstream)
{
    int r, c;
    int i, j;
    double init;
    STREAM *SA;

    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {
	if (!downstream) {
	    init = SA[i].elevation[1];
	    for (j = 1; j < SA[i].number_of_cells - 1; ++j) {
		r = (int)SA[i].points[j] / ncols;
		c = (int)SA[i].points[j] % ncols;
		output[r][c] = init - SA[i].elevation[j];
	    }
	}
	else {
	    init = SA[i].elevation[SA[i].number_of_cells - 2];
	    for (j = SA[i].number_of_cells - 2; j > 0; --j) {
		r = (int)SA[i].points[j] / ncols;
		c = (int)SA[i].points[j] % ncols;
		output[r][c] = SA[i].elevation[j] - init;
	    }
	}
    }

    return 0;
}

int seg_calculate_drop(SEGMENT *output, int number_of_streams,
		       int downstream)
{
    int r, c;
    int i, j;
    double init;
    double output_cell;
    STREAM *SA;

    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {
	if (!downstream) {
	    init = SA[i].elevation[1];
	    for (j = 1; j < SA[i].number_of_cells - 1; ++j) {
		r = (int)SA[i].points[j] / ncols;
		c = (int)SA[i].points[j] % ncols;
		output_cell = init - SA[i].elevation[j];
		segment_put(output, &output_cell, r, c);
	    }
	}
	else {
	    init = SA[i].elevation[SA[i].number_of_cells - 2];
	    for (j = SA[i].number_of_cells - 2; j > 0; --j) {
		r = (int)SA[i].points[j] / ncols;
		c = (int)SA[i].points[j] % ncols;
		output_cell = SA[i].elevation[j] - init;
		segment_put(output, &output_cell, r, c);
	    }
	}
    }

    return 0;
}

int ram_calculate_gradient(DCELL **output, int number_of_streams,
			   int downstream)
{
    int r, c;
    int i, j;
    double init;
    double cum_length;
    STREAM *SA;

    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {
	cum_length = 0;
	if (!downstream) {
	    init = SA[i].elevation[0];
	    for (j = 1; j < SA[i].number_of_cells - 1; ++j) {
		cum_length += SA[i].distance[j];
		r = (int)SA[i].points[j] / ncols;
		c = (int)SA[i].points[j] % ncols;
		output[r][c] = (init - SA[i].elevation[j]) / cum_length;
	    }
	}
	else {
	    init = SA[i].elevation[SA[i].number_of_cells - 1];
	    for (j = SA[i].number_of_cells - 2; j > 0; --j) {
		cum_length += SA[i].distance[j];
		r = (int)SA[i].points[j] / ncols;
		c = (int)SA[i].points[j] % ncols;
		output[r][c] = (SA[i].elevation[j] - init) / cum_length;
	    }
	}
    }

    return 0;
}

int seg_calculate_gradient(SEGMENT *output, int number_of_streams,
			   int downstream)
{
    int r, c;
    int i, j;
    double init;
    double output_cell;
    double cum_length;
    STREAM *SA;

    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {
	cum_length = 0;
	if (!downstream) {
	    init = SA[i].elevation[1];
	    for (j = 1; j < SA[i].number_of_cells - 1; ++j) {
		cum_length += SA[i].distance[j];
		r = (int)SA[i].points[j] / ncols;
		c = (int)SA[i].points[j] % ncols;
		output_cell = (init - SA[i].elevation[j]) / cum_length;
		segment_put(output, &output_cell, r, c);

	    }
	}
	else {
	    init = SA[i].elevation[SA[i].number_of_cells - 2];
	    for (j = SA[i].number_of_cells - 2; j > 0; --j) {
		cum_length += SA[i].distance[j];
		r = (int)SA[i].points[j] / ncols;
		c = (int)SA[i].points[j] % ncols;
		output_cell = (SA[i].elevation[j] - init) / cum_length;
		segment_put(output, &output_cell, r, c);
	    }
	}
    }

    return 0;
}

int ram_calculate_local_gradient(DCELL **output, int number_of_streams,
				 int downstream)
{
    int r, c;
    int i, j;
    double elev_diff;
    STREAM *SA;

    G_debug(3, "ram_calculate_local_gradient(): downstream=%d", downstream);
    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {
	for (j = 1; j < SA[i].number_of_cells - 1; ++j) {
	    r = (int)SA[i].points[j] / ncols;
	    c = (int)SA[i].points[j] % ncols;
	    elev_diff =
		(SA[i].elevation[j] - SA[i].elevation[j + 1]) <
		0 ? 0 : (SA[i].elevation[j] - SA[i].elevation[j + 1]);
	    output[r][c] = elev_diff / SA[i].distance[j];
	}
    }

    return 0;
}

int seg_calculate_local_gradient(SEGMENT *output, int number_of_streams,
				 int downstream)
{
    int r, c;
    int i, j;
    double elev_diff;
    double output_cell;
    STREAM *SA;

    G_debug(3, "seg_calculate_local_gradient(): downstream=%d", downstream);
    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {
	for (j = 1; j < SA[i].number_of_cells - 1; ++j) {
	    r = (int)SA[i].points[j] / ncols;
	    c = (int)SA[i].points[j] % ncols;
	    elev_diff =
		(SA[i].elevation[j] - SA[i].elevation[j + 1]) <
		0 ? 0 : (SA[i].elevation[j] - SA[i].elevation[j + 1]);
	    output_cell = elev_diff / SA[i].distance[j];
	    segment_put(output, &output_cell, r, c);
	}
    }

    return 0;
}


int ram_calculate_local_distance(DCELL **output, int number_of_streams,
				 int downstream)
{
    int r, c;
    int i, j;
    STREAM *SA;

    G_debug(3, "ram_calculate_local_distance(): downstream=%d", downstream);
    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {
	for (j = 1; j < SA[i].number_of_cells - 1; ++j) {
	    r = (int)SA[i].points[j] / ncols;
	    c = (int)SA[i].points[j] % ncols;
	    output[r][c] = SA[i].distance[j];
	}
    }

    return 0;
}

int seg_calculate_local_distance(SEGMENT *output, int number_of_streams,
				 int downstream)
{
    int r, c;
    int i, j;
    double output_cell;
    STREAM *SA;

    G_debug(3, "seg_calculate_local_distance(): downstream=%d", downstream);
    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {
	for (j = 1; j < SA[i].number_of_cells - 1; ++j) {
	    r = (int)SA[i].points[j] / ncols;
	    c = (int)SA[i].points[j] % ncols;
	    output_cell = SA[i].distance[j];
	    segment_put(output, &output_cell, r, c);
	}
    }

    return 0;
}

int ram_calculate_curvature(DCELL **output, int number_of_streams,
			    int downstream)
{
    int r, c;
    int i, j;
    STREAM *SA;
    double first_derivative, second_derivative;

    G_debug(3, "ram_calculate_curvature(): downstream=%d", downstream);
    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {
	for (j = 1; j < SA[i].number_of_cells - 1; ++j) {
	    r = (int)SA[i].points[j] / ncols;
	    c = (int)SA[i].points[j] % ncols;
	    first_derivative =
		(SA[i].elevation[j - 1] -
		 SA[i].elevation[j + 1]) / (SA[i].distance[j - 1] +
					    SA[i].distance[j]);
	    second_derivative =
		((SA[i].elevation[j - 1] - SA[i].elevation[j]) -
		 (SA[i].elevation[j] -
		  SA[i].elevation[j + 1])) / (SA[i].distance[j - 1] +
					      SA[i].distance[j]);
	    output[r][c] =
		first_derivative /
		pow((1 + second_derivative * second_derivative), 1.5);
	}
    }

    return 0;
}

int seg_calculate_curvature(SEGMENT *output, int number_of_streams,
			    int downstream)
{
    int r, c;
    int i, j;
    double output_cell;
    STREAM *SA;
    double first_derivative, second_derivative;

    G_debug(3, "seg_calculate_curvature(): downstream=%d", downstream);
    SA = stream_attributes;

    for (i = 1; i < number_of_streams; ++i) {
	for (j = 1; j < SA[i].number_of_cells - 1; ++j) {
	    r = (int)SA[i].points[j] / ncols;
	    c = (int)SA[i].points[j] % ncols;
	    first_derivative =
		(SA[i].elevation[j - 1] -
		 SA[i].elevation[j + 1]) / (SA[i].distance[j - 1] +
					    SA[i].distance[j]);
	    second_derivative =
		((SA[i].elevation[j - 1] - SA[i].elevation[j]) -
		 (SA[i].elevation[j] -
		  SA[i].elevation[j + 1])) / (SA[i].distance[j - 1] +
					      SA[i].distance[j]);
	    output_cell =
		first_derivative /
		pow((1 + second_derivative * second_derivative), 1.5);
	    segment_put(output, &output_cell, r, c);
	}
    }

    return 0;
}
