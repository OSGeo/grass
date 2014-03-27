#include "local_proto.h"
int stream_init(int min_index_of_stream, int max_index_of_stream)
{
    int i, j;
    int number_of_streams = max_index_of_stream;

    if (max_index_of_stream == 0)
        G_fatal_error(_("Empty stream input raster map"));
    if (min_index_of_stream < 0)
	G_fatal_error(_("Stream map has negative values"));

    stream_attributes =
	(STREAM *) G_malloc((number_of_streams + 1) * sizeof(STREAM));
    for (i = 0; i < max_index_of_stream + 1; ++i) {
	stream_attributes[i].next_stream = -1;
	stream_attributes[i].stream = -1;
	stream_attributes[i].trib_num = -1;
	stream_attributes[i].trib[0] = 0;
	stream_attributes[i].trib[1] = 0;
	stream_attributes[i].trib[2] = 0;
	stream_attributes[i].trib[3] = 0;
	stream_attributes[i].trib[4] = 0;
	stream_attributes[i].cells_num = 0;
	stream_attributes[i].init = 0;
	stream_attributes[i].outlet = 0;
	stream_attributes[i].length = 0.;
	stream_attributes[i].accum_length = 0.;
	stream_attributes[i].distance = 0.;
	stream_attributes[i].stright = 0.;
	stream_attributes[i].accum = 0.;
	stream_attributes[i].init_elev = -10000;
	stream_attributes[i].outlet_elev = -10000;
    }

    all_orders = (int **)G_malloc(orders_size * sizeof(int *));
    for (i = 0; i < orders_size; ++i)
	all_orders[i] =
	    (int *)G_malloc((number_of_streams + 1) * sizeof(int));

    for (i = 0; i < orders_size; ++i)
	for (j = 0; j < number_of_streams + 1; ++j)
	    all_orders[i][j] = -1;

    return 0;
}

int compare_inits(const void *a, const void *b)
{
    return (*(STREAM **) a)->init - (*(STREAM **) b)->init;
}

int compare_outlets(const void *a, const void *b)
{
    return (*(STREAM **) a)->outlet - (*(STREAM **) b)->outlet;
}

int stream_sample_map(char *input_map_name, int number_of_streams, int what)
{

    /* if what == 2 sample outputs only for accum map */
    /* if what == 1 sample outputs only for elev map */
    /* if what == 0 sample inits only for elev map) */

    char *mapset;
    int input_map_fd;
    int i;
    int r, c, cur_row = -1;
    RASTER_MAP_TYPE input_data_type;
    size_t input_data_size;
    void *input_buffer;
    void *input_ptr;

    STREAM **pointers_to_stream;

    pointers_to_stream =
	(STREAM **) G_malloc(sizeof(STREAM *) * (number_of_streams));
    for (i = 0; i < (number_of_streams); ++i)
	*(pointers_to_stream + i) = stream_attributes + i;

    if (what)
	qsort(pointers_to_stream, (number_of_streams), sizeof(STREAM *),
	      compare_outlets);
    else
	qsort(pointers_to_stream, (number_of_streams), sizeof(STREAM *),
	      compare_inits);


    mapset = (char *)G_find_raster2(input_map_name, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), input_map_name);
    input_map_fd = Rast_open_old(input_map_name, mapset);
    input_data_type = Rast_map_type(input_map_name, mapset);
    input_data_size = Rast_cell_size(input_data_type);
    input_buffer = Rast_allocate_buf(input_data_type);

    for (i = 0; i < (number_of_streams); ++i) {
	if (pointers_to_stream[i]->stream == -1)
	    continue;

	if (what) {
	    r = (int)pointers_to_stream[i]->outlet / ncols;
	    c = (int)pointers_to_stream[i]->outlet % ncols;
	}
	else {
	    r = (int)pointers_to_stream[i]->init / ncols;
	    c = (int)pointers_to_stream[i]->init % ncols;
	}

	if (r > cur_row) {
	    Rast_get_row(input_map_fd, input_buffer, r, input_data_type);
	    input_ptr = input_buffer;
	}
	switch (what) {
	case 0:		/* inits for elev */
	    pointers_to_stream[i]->init_elev =
		Rast_get_d_value(input_ptr + c * input_data_size,
				 input_data_type);
	    break;
	case 1:		/* outlets for elev */
	    pointers_to_stream[i]->outlet_elev =
		Rast_get_d_value(input_ptr + c * input_data_size,
				 input_data_type);
	    break;
	case 2:		/* outlets for accum */
	    pointers_to_stream[i]->accum =
		Rast_get_d_value(input_ptr + c * input_data_size,
				 input_data_type);
	    break;
	}
    }
    G_free(input_buffer);
    G_free(pointers_to_stream);
    return 0;
}
