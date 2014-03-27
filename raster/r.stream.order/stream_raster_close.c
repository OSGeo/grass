#include "local_proto.h"
int ram_close_raster_order(CELL **streams, int number_of_streams,
			   int zerofill)
{

    G_debug(3, "ram_close_raster_order(): number_of_streams=%d", number_of_streams);

    G_message("Writing outpout raster maps...");
    int *output_fd;
    int r, c, i;
    CELL *output_buffer, *streams_buffer;
    struct History history;
    size_t data_size;

    output_fd = (int *)G_malloc(orders_size * sizeof(int));
    for (i = 0; i < orders_size; ++i) {
	if (output_map_names[i] == NULL)
	    continue;
	output_fd[i] = Rast_open_c_new(output_map_names[i]);
    }

    /* this is not very elegant but use for compatibility with seg version */

    data_size = Rast_cell_size(CELL_TYPE);
    output_buffer = Rast_allocate_c_buf();

    for (r = 0; r < nrows; ++r) {
	streams_buffer = streams[r];

	for (i = 0; i < orders_size; ++i) {

	    if (output_map_names[i] == NULL)
		continue;

	    if (zerofill)
		memset(output_buffer, 0, ncols * data_size);
	    else
		Rast_set_c_null_value(output_buffer, ncols);

	    for (c = 0; c < ncols; ++c)
		if (streams_buffer[c])
		    output_buffer[c] = all_orders[i][streams_buffer[c]];

	    Rast_put_c_row(output_fd[i], output_buffer);
	}			/* end i */
    }

    G_free(output_buffer);
    for (i = 0; i < orders_size; ++i)
	G_free(all_orders[i]);
    G_free(all_orders);


    for (i = 0; i < orders_size; ++i) {
	if (output_map_names[i] == NULL)
	    continue;
	Rast_close(output_fd[i]);
	Rast_short_history(output_map_names[i], "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(output_map_names[i], &history);
	/* G_message(_("%s Done"), output_map_names[i]); */
    }

    G_free(output_fd);
    return 0;
}


int seg_close_raster_order(SEGMENT *streams, int number_of_streams,
			   int zerofill)
{

    int *output_fd;
    int r, c, i;
    CELL *output_buffer, *streams_buffer;
    struct History history;
    size_t data_size;

    G_debug(3, "seg_close_raster_order(): number_of_streams=%d", number_of_streams);

    output_fd = (int *)G_malloc(orders_size * sizeof(int));
    for (i = 0; i < orders_size; ++i) {
	if (output_map_names[i] == NULL)
	    continue;
	output_fd[i] = Rast_open_c_new(output_map_names[i]);
    }

    data_size = Rast_cell_size(CELL_TYPE);
    output_buffer = Rast_allocate_c_buf();
    streams_buffer = Rast_allocate_c_buf();
    segment_flush(streams);

    for (r = 0; r < nrows; ++r) {
	if (0 > segment_get_row(streams, streams_buffer, r))
	    G_warning(_("Unable to segment read row %d for raster map <%s>"),
		      r, output_map_names[i]);

	for (i = 0; i < orders_size; ++i) {

	    if (output_map_names[i] == NULL)
		continue;

	    if (zerofill)
		memset(output_buffer, 0, ncols * data_size);
	    else
		Rast_set_c_null_value(output_buffer, ncols);

	    for (c = 0; c < ncols; ++c)
		if (!Rast_is_c_null_value(&streams_buffer[c]))
		    output_buffer[c] = all_orders[i][streams_buffer[c]];

	    Rast_put_c_row(output_fd[i], output_buffer);
	}			/* end i */
    }

    G_free(output_buffer);
    G_free(streams_buffer);
    for (i = 0; i < orders_size; ++i)
	G_free(all_orders[i]);
    G_free(all_orders);

    for (i = 0; i < orders_size; ++i) {
	if (output_map_names[i] == NULL)
	    continue;
	Rast_close(output_fd[i]);
	Rast_short_history(output_map_names[i], "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(output_map_names[i], &history);
	/* G_message(_("%s Done"), output_map_names[i]);*/
    }

    G_free(output_fd);
    return 0;
}
