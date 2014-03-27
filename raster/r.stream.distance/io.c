#include "io.h"
/* all in ram functions section */

int ram_create_map(MAP * map, RASTER_MAP_TYPE data_type)
{

    /* 
     * allocates 0 filled nrows*ncols map of type void;
     * map parameters are stored in structure;
     * map: map to be created;
     * map type to be created must be CELL, FCELL, DCELL;
     * */

    int r;

    if (data_type < 0 || data_type > 2)
	G_fatal_error(_("Unable to create map of unrecognised type"));

    map->data_type = data_type;
    map->map_name = NULL;
    map->nrows = Rast_window_rows();
    map->ncols = Rast_window_cols();
    map->data_size = Rast_cell_size(data_type);

    /* preparing internal map */
    switch (map->data_type) {
    case CELL_TYPE:
	map->map = G_calloc(map->nrows, sizeof(CELL *));
	break;

    case FCELL_TYPE:
	map->map = G_calloc(map->nrows, sizeof(FCELL *));
	break;

    case DCELL_TYPE:
	map->map = G_calloc(map->nrows, sizeof(DCELL *));
	break;
    }

    for (r = 0; r < map->nrows; ++r)
	(map->map)[r] = G_calloc(map->ncols, map->data_size);

    return 0;
}

int ram_read_map(MAP * map, char *input_map_name, int check_res,
		 RASTER_MAP_TYPE check_data_type)
{
    /*
     * Funciton read external map and put it in MAP structure (created with create_map)
     * map: map to be read can be of any data type, read map is converted to target map if neccesary.
     * input_map_name: name of the map to be read;
     * map pointer to map stucture (created with create_map);
     * check_res: [1]: check res correspondence between region and map [0 no check];
     * check_data_type [CELL, FCELL, DCELL] check if reading map is of particular type, [-1] no check;
     */

    int r, c;
    char *mapset;
    struct Cell_head cellhd, this_window;
    char *maptypes[] = { "CELL", "FCELL", "DCELL" };
    int input_map_fd;
    RASTER_MAP_TYPE input_data_type;
    size_t input_data_size;
    void *input_buffer = NULL;
    void *input_pointer;

    /* checking if map exist */
    mapset = (char *)G_find_raster2(input_map_name, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"), input_map_name);

    /* checking if region and input are the same */
    G_get_window(&this_window);
    Rast_get_cellhd(input_map_name, mapset, &cellhd);
    if (check_res)
	if (this_window.ew_res != cellhd.ew_res ||
	    this_window.ns_res != cellhd.ns_res)
          G_fatal_error(_("Region resolution and raster map <%s> resolution differs. "
                          "Run 'g.region rast=%s' to set proper region resolution."),
                        input_map_name, input_map_name);

    /* checking if input map is of required type */
    if (check_data_type != map->data_type)
	G_debug(1,
		"ram_open:required map type and internal map type differs: conversion forced!");
    input_data_type = Rast_map_type(input_map_name, mapset);
    if (check_data_type != -1)
	if (input_data_type != check_data_type)
	    G_fatal_error(_("Raster map <%s> is not of type '%s'"),
			  input_map_name, maptypes[check_data_type]);

    input_map_fd = Rast_open_old(input_map_name, mapset);
    input_data_size = Rast_cell_size(input_data_type);

    {				/* reading range */
	struct Range map_range;
	struct FPRange map_fp_range;
	int min, max;

	if (input_data_type == CELL_TYPE) {
	    Rast_init_range(&map_range);
	    Rast_read_range(input_map_name, mapset, &map_range);
	    Rast_get_range_min_max(&map_range, &min, &max);
	    map->min = (double)min;
	    map->max = (double)max;
	}
	else {
	    Rast_init_fp_range(&map_fp_range);
	    Rast_read_fp_range(input_map_name, mapset, &map_fp_range);
	    Rast_get_fp_range_min_max(&map_fp_range, &(map->min),
				      &(map->max));
	}
    }
    /* end opening and checking */

    input_buffer = Rast_allocate_buf(input_data_type);

    /* start reading */
    G_message(_("Reading raster map <%s>..."), input_map_name);

    for (r = 0; r < map->nrows; ++r) {
	G_percent(r, map->nrows, 2);

	Rast_get_row(input_map_fd, input_buffer, r, input_data_type);
	input_pointer = input_buffer;

	for (c = 0; c < map->ncols; ++c)
	    if (!Rast_is_null_value
		(input_pointer + c * input_data_size, input_data_type))
		switch (map->data_type) {
		case CELL_TYPE:
		    ((CELL **) map->map)[r][c] =
			Rast_get_c_value(input_pointer + c * input_data_size,
					 input_data_type);
		    break;
		case FCELL_TYPE:
		    ((FCELL **) map->map)[r][c] =
			Rast_get_f_value(input_pointer + c * input_data_size,
					 input_data_type);
		    break;
		case DCELL_TYPE:
		    ((DCELL **) map->map)[r][c] =
			Rast_get_d_value(input_pointer + c * input_data_size,
					 input_data_type);
		    break;
		default:
		    G_fatal_error(_("Wrong internal data type"));
		    break;
		}
    }				/*end for r */

    G_free(input_buffer);
    G_percent(r, map->nrows, 2);
    Rast_close(input_map_fd);
    return 0;
}				/* end create floating point map */

int ram_reset_map(MAP * map, int value)
{
    /*
     * set all cells in the map to value
     */
    int r;

    for (r = 0; r < map->nrows; ++r)
	memset((map->map)[r], value, map->ncols * map->data_size);
    return 0;
}

int ram_write_map(MAP * map, char *output_map_name,
		  RASTER_MAP_TYPE output_data_type, int convert_to_null,
		  double value)
{
    /* 
     * write map to disk with output_map_name and output_data_type [CELL, FCELL, DCELL];
     * if output_data_type = -1 than internal map type is used for output;
     * if output map != -1 and types differ data_type, conversion is forced
     * convert to null: check if convert to null a particular value in dataset;
     */

    int r, c;
    int output_fd = 0;
    struct History history;
    void *row;

    /* check for output format */
    if (output_data_type == -1)
	output_data_type = map->data_type;

    if (output_data_type != map->data_type)
	G_debug(1,
		"ram_write:required map type and internal map type differs: conversion forced!");

    G_message(_("Writing raster map <%s>..."), output_map_name);
    output_fd = Rast_open_new(output_map_name, output_data_type);

    /* writing */
    for (r = 0; r < map->nrows; ++r) {
	G_percent(r, map->nrows, 2);

	if (convert_to_null) {
	    row = map->map[r];
	    switch (map->data_type) {
	    case CELL_TYPE:
		for (c = 0; c < map->ncols; ++c)
		    if (((CELL *) row)[c] == (CELL) value)
			Rast_set_c_null_value(row + c * (map->data_size), 1);
		break;
	    case FCELL_TYPE:
		for (c = 0; c < map->ncols; ++c)
		    if (((FCELL *) row)[c] == (FCELL) value)
			Rast_set_f_null_value(row + c * (map->data_size), 1);
		break;
	    case DCELL_TYPE:
		for (c = 0; c < map->ncols; ++c)
		    if (((DCELL *) row)[c] == (DCELL) value)
			Rast_set_d_null_value(row + c * (map->data_size), 1);
		break;
	    default:
		G_debug(1, "Cannot Cannot convert to null at: %d %d", r, c);
	    }
	}

	Rast_put_row(output_fd, (map->map)[r], output_data_type);
    }
    G_percent(r, map->nrows, 2);
    Rast_close(output_fd);
    Rast_short_history(output_map_name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(output_map_name, &history);
    /* G_message(_("<%s> Done"), output_map_name); */
    return 0;
}

int ram_release_map(MAP *map)
{
    /* 
     * free memory allocated for map, set pointer to null;
     */
    int r;

    for (r = 0; r < map->nrows; ++r)
	G_free((map->map)[r]);
    G_free(map->map);
    map = NULL;
    return 0;
}


/* memory swap functions section */


int seg_create_map(SEG * seg, int srows, int scols, int number_of_segs,
		   RASTER_MAP_TYPE data_type)
{
    /* create segment  and returns pointer to it;
     * seg must be declared first;
     * parameters are stored in structure;
     * seg: segment to be created;
     * srows, scols segment size
     * number of segs max number of segs stored in memory
     * data_type to be created must be CELL, FCELL, DCELL;
     */

    char *filename;
    int fd;
    int local_number_of_segs;

    seg->fd = -1;
    seg->filename = NULL;
    seg->map_name = NULL;
    seg->mapset = NULL;
    seg->data_type = data_type;
    seg->nrows = Rast_window_rows();
    seg->ncols = Rast_window_cols();

    local_number_of_segs =
	(seg->nrows / srows + 1) * (seg->ncols / scols + 1);
    number_of_segs =
	(number_of_segs >
	 local_number_of_segs) ? local_number_of_segs : number_of_segs;

    G_debug(3, "seg_creat:number of segments %d", number_of_segs);

    switch (seg->data_type) {
    case CELL_TYPE:
	seg->data_size = sizeof(CELL);
	break;
    case FCELL_TYPE:
	seg->data_size = sizeof(FCELL);
	break;
    case DCELL_TYPE:
	seg->data_size = sizeof(DCELL);
	break;
    default:
	G_fatal_error(_("Unrecognisable data type"));
    }

    filename = G_tempfile();
    fd = creat(filename, 0666);

    if (0 >
	segment_format(fd, seg->nrows, seg->ncols, srows, scols,
		       seg->data_size)) {
	close(fd);
	unlink(filename);
	G_fatal_error(_("Unable to format segment"));
    }

    close(fd);
    if (0 > (fd = open(filename, 2))) {
	unlink(filename);
	G_fatal_error(_("Unable to re-open file '%s'"), filename);
    }

    if (0 > (fd = segment_init(&(seg->seg), fd, number_of_segs))) {
	unlink(filename);
	G_fatal_error(_("Unable to init segment file or out of memory"));
    }

    seg->filename = G_store(filename);
    seg->fd = fd;
    return 0;
}

int seg_read_map(SEG * seg, char *input_map_name, int check_res,
		 RASTER_MAP_TYPE check_data_type)
{

    /*
     * Funciton read external map and put it in SEG structure (created with seg_create_map)
     * map to be read can be of any data type, read map is converted if neccesary.
     * input_map_name: name of the map to be read;
     * seg: pointer to map stucture (created with create_map);
     * check_res: [1]: check res correspondence between region and map [0 no check];
     * check_data_type [CELL, FCELL, DCELL] check if reading map is of particular type, [-1] no check;
     */

    int input_fd;
    int r, c;
    char *mapset;
    struct Cell_head cellhd, this_window;
    char *maptypes[] = { "CELL", "FCELL", "DCELL" };
    RASTER_MAP_TYPE input_data_type;
    size_t input_data_size;
    void *input_buffer = NULL;
    void *target_buffer = NULL;
    void *input_pointer = NULL;

    /* checking if map exist */
    mapset = (char *)G_find_raster2(input_map_name, "");
    if (mapset == NULL)
	G_fatal_error(_("Raster map <%s> not found"),
		      input_map_name);
    seg->mapset = mapset;

    /* checking if region and input are the same */
    G_get_window(&this_window);
    Rast_get_cellhd(input_map_name, mapset, &cellhd);

    /* check resolution equal anyinteger check;  equal 0 no check */
    if (check_res)
	if (this_window.ew_res != cellhd.ew_res ||
	    this_window.ns_res != cellhd.ns_res)
	    G_fatal_error(_("Region resolution and raster map <%s> resolution differs. "
                            "Run 'g.region rast=%s' to set proper region resolution."),
			  input_map_name, input_map_name);
    
    if (check_data_type != seg->data_type)
	G_debug(1,
		"ram_open:required map type and internal map type differs: conversion forced!");
    input_data_type = Rast_map_type(input_map_name, mapset);
    if (check_data_type != -1)
	if (input_data_type != check_data_type)
	    G_fatal_error(_("Raster <%s> is not of type '%s'"),
			  input_map_name, maptypes[check_data_type]);

    input_fd = Rast_open_old(input_map_name, mapset);
    input_data_size = Rast_cell_size(input_data_type);

    {				/* reading range */
	struct Range map_range;
	struct FPRange map_fp_range;
	int min, max;

	if (input_data_type == CELL_TYPE) {
	    Rast_init_range(&map_range);
	    Rast_read_range(input_map_name, mapset, &map_range);
	    Rast_get_range_min_max(&map_range, &min, &max);
	    seg->min = (double)min;
	    seg->max = (double)max;
	}
	else {
	    Rast_init_fp_range(&map_fp_range);
	    Rast_read_fp_range(input_map_name, mapset, &map_fp_range);
	    Rast_get_fp_range_min_max(&map_fp_range, &(seg->min),
				      &(seg->max));
	}
    }

    /* end opening and checking */

    G_message(_("Reading raster map <%s>..."), input_map_name);
    input_buffer = Rast_allocate_buf(input_data_type);

    target_buffer = Rast_allocate_buf(seg->data_type);

    for (r = 0; r < seg->nrows; ++r) {
	G_percent(r, seg->nrows, 2);
	Rast_get_row(input_fd, input_buffer, r, input_data_type);
	input_pointer = input_buffer;
	memset(target_buffer, 0, seg->ncols * seg->data_size);

	for (c = 0; c < seg->ncols; ++c)
	    if (!Rast_is_null_value
		(input_pointer + c * input_data_size, input_data_type)) {
		switch (seg->data_type) {
		case CELL_TYPE:
		    ((CELL *) target_buffer)[c] =
			Rast_get_c_value(input_pointer + c * input_data_size,
					 input_data_type);
		    break;
		case FCELL_TYPE:
		    ((FCELL *) target_buffer)[c] =
			Rast_get_f_value(input_pointer + c * input_data_size,
					 input_data_type);
		    break;
		case DCELL_TYPE:
		    ((DCELL *) target_buffer)[c] =
			Rast_get_d_value(input_pointer + c * input_data_size,
					 input_data_type);
		    break;
		default:
		    G_fatal_error(_("Wrong internal data type"));
		    break;
		}
	    }

	if (0 > segment_put_row(&(seg->seg), target_buffer, r)) {
	    G_free(input_buffer);
	    G_free(target_buffer);
	    Rast_close(input_fd);
	    G_fatal_error(_("Unable to segment put row %d for raster map <%s>"),
			  r, input_map_name);
	}
    }				/* end for row */

    G_percent(r, seg->nrows, 2);
    Rast_close(input_fd);
    G_free(input_buffer);
    G_free(target_buffer);

    seg->map_name = G_store(input_map_name);
    seg->mapset = G_store(mapset);

    return 0;
}

int seg_reset_map(SEG * seg, int value)
{
    /*
     * set all cells in the map to value
     */
    int r, c;

    for (r = 0; r < seg->nrows; ++r)
	for (c = 0; c < seg->ncols; ++c)
	    segment_put(&(seg->seg), &value, r, c);

    return 0;
}

int seg_write_map(SEG * seg, char *output_map_name,
		  RASTER_MAP_TYPE output_data_type, int convert_to_null,
		  double value)
{
    /* 
     * write seg to disk with output_map_name and output_data_type [CELL, FCELL, DCELL];
     * if output_data_type = -1 than internal map type is used for output;
     * if output map != -1 and types differ data_type, conversion is forced
     * convert to null: check if convert to null a particular value in dataset;
     */
    int output_fd;
    int r, c;
    void *output_buffer;
    void *row;
    struct History history;

    /* check for output format */
    if (output_data_type == -1)
	output_data_type = seg->data_type;

    if (output_data_type != seg->data_type)
	G_debug(1,
		"ram_write:required map type and internal map type differs: conversion forced!");

    G_message(_("Writing raster map <%s>..."), output_map_name);
    output_fd = Rast_open_new(output_map_name, output_data_type);
    output_buffer = Rast_allocate_buf(output_data_type);
    segment_flush(&(seg->seg));

    /* writing */
    for (r = 0; r < seg->nrows; ++r) {

	G_percent(r, seg->nrows, 2);
	if (0 > segment_get_row(&(seg->seg), output_buffer, r))
	    G_warning(_("Unable to segment read row %d for raster map <%s>"),
		      r, output_map_name);

	if (convert_to_null) {

	    row = output_buffer;
	    switch (seg->data_type) {
	    case CELL_TYPE:
		for (c = 0; c < seg->ncols; ++c)
		    if (((CELL *) output_buffer)[c] == (CELL) value)
			Rast_set_c_null_value(row + c * (seg->data_size), 1);
		break;
	    case FCELL_TYPE:
		for (c = 0; c < seg->ncols; ++c)
		    if (((FCELL *) output_buffer)[c] == (FCELL) value)
			Rast_set_f_null_value(row + c * (seg->data_size), 1);
		break;
	    case DCELL_TYPE:
		for (c = 0; c < seg->ncols; ++c)
		    if (((DCELL *) output_buffer)[c] == (DCELL) value)
			Rast_set_d_null_value(row + c * (seg->data_size), 1);
		break;
	    default:
		G_warning(_("Unable to convert to NULL at: %d %d"), r,
			  c);
	    }
	}
	Rast_put_row(output_fd, output_buffer, output_data_type);
    }

    G_percent(r, seg->nrows, 2);
    G_free(output_buffer);
    Rast_close(output_fd);
    Rast_short_history(output_map_name, "raster", &history);
    Rast_command_history(&history);
    Rast_write_history(output_map_name, &history);
    /* G_message(_("%s Done"), output_map_name); */

    return 0;
}

int seg_release_map(SEG * seg)
{
    /* 
     * release segment close files, set pointers to null;
     */
    segment_release(&(seg->seg));
    close(seg->fd);
    unlink(seg->filename);

    if (seg->map_name)
	G_free(seg->map_name);
    if (seg->mapset)
	G_free(seg->mapset);

    return 0;
}
