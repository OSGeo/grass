/****************************************************************************
 *
 * MODULE:       r3.in.bin
 *
 * AUTHOR(S):   Soeren Gebbert
 *   			Based on r.in.bin from: Bob Covill
 *
 * PURPOSE:     Imports a binary raster file into a GRASS 3D raster map.
 *
 * COPYRIGHT:   (C) 2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>

/* Some global variables */
RASTER3D_Map *map;
RASTER3D_Region region;
FILE *fp;
unsigned char *in_cell;

static void swap_2(void *p) {
	unsigned char *q = p;
	unsigned char t;
	t = q[0];
	q[0] = q[1];
	q[1] = t;
}

static void swap_4(void *p) {
	unsigned char *q = p;
	unsigned char t;
	t = q[0];
	q[0] = q[3];
	q[3] = t;
	t = q[1];
	q[1] = q[2];
	q[2] = t;
}

static void swap_8(void *p) {
	unsigned char *q = p;
	unsigned char t;
	t = q[0];
	q[0] = q[7];
	q[7] = t;
	t = q[1];
	q[1] = q[6];
	q[6] = t;
	t = q[2];
	q[2] = q[5];
	q[5] = t;
	t = q[3];
	q[3] = q[4];
	q[4] = t;
}

static void read_cell(DCELL *out_cell, int is_integer, int is_signed, int bytes,
		int byte_swap) {

	if (fread(in_cell, bytes, 1, fp) != 1)
		G_fatal_error(_("Error reading binary data"));

	if (byte_swap) {
		switch (bytes) {
		case 1:
			break;
		case 2:
			swap_2(in_cell);
			break;
		case 4:
			swap_4(in_cell);
			break;
		case 8:
			swap_8(in_cell);
			break;
		}
	}

	if (!is_integer) {
		switch (bytes) {
		case 4:
			*out_cell = (DCELL) *(float *) in_cell;
			break;
		case 8:
			*out_cell = (DCELL) *(double *) in_cell;
			break;
		}
	} else if (is_signed) {
		switch (bytes) {
		case 1:
			*out_cell = (DCELL) *(signed char *) in_cell;
			break;
		case 2:
			*out_cell = (DCELL) *(short *) in_cell;
			break;
		case 4:
			*out_cell = (DCELL) *(int *) in_cell;
			break;
#ifdef HAVE_LONG_LONG_INT
		case 8:
			*out_cell = (DCELL) *(long long *) in_cell;
			break;
#endif
		}
	} else {
		switch (bytes) {
		case 1:
			*out_cell = (DCELL) *(unsigned char *) in_cell;
			break;
		case 2:
			*out_cell = (DCELL) *(unsigned short *) in_cell;
			break;
		case 4:
			*out_cell = (DCELL) *(unsigned int *) in_cell;
			break;
#ifdef HAVE_LONG_LONG_INT
		case 8:
			*out_cell = (DCELL) *(unsigned long long *) in_cell;
			break;
#endif
		}
	}
}

static void bin_to_raster3d(char *null, int map_type, int is_integer,
		int is_signed, int bytes, int byte_swap, int row_swap, int depth_swap) {
	int x, y, z;
	int col, row, depth;
	DCELL value;
	FCELL fvalue;
	DCELL null_value;
	int tileX, tileY, tileZ;

	if (null)
		null_value = atof(null);

	Rast3d_get_tile_dimensions_map(map, &tileX, &tileY, &tileZ);
	Rast3d_min_unlocked(map, RASTER3D_USE_CACHE_X);

	Rast3d_autolock_on(map);
	Rast3d_unlock_all(map);
	G_message(_("Loading %s data with %i  bytes ...  (%dx%dx%d)"),
			(is_integer? "integer":"floating point "), bytes, region.cols,
			region.rows, region.depths);

	for (z = 0; z < region.depths; z++) {
		G_percent(z, region.depths, 1);

		if ((z % tileZ) == 0)
			Rast3d_unlock_all(map);

		for (y = 0; y < region.rows; y++) {/* go south to north */
			for (x = 0; x < region.cols; x++) {

				/* From west to east */
				col = x;
				/* The default is to read rows from north to south */
				row = y;
				/* From bottom to the top */
				depth = z;

				/* Read rows as from south to north */
				if (row_swap)
					row = region.rows - y - 1;

				/* Read XY layer from top to bottom */
				if (depth_swap)
					depth = region.depths - z - 1;

				/* Read value from binary file */
				read_cell(&value, is_integer, is_signed, bytes, byte_swap);

				/* Write value to the 3D raster map */
				if (map_type == DCELL_TYPE) {
					if (null && value == null_value)
						Rast3d_set_null_value(&value, 1, DCELL_TYPE);
					Rast3d_put_double(map, col, row, depth, value);
				} else {
					fvalue = (FCELL) value;
					if (null && value == null_value)
						Rast3d_set_null_value(&fvalue, 1, FCELL_TYPE);
					Rast3d_put_double(map, col, row, depth, fvalue);

				}
			}
		}
	}

	if (!Rast3d_flush_all_tiles(map))
		G_fatal_error(_("Error flushing tiles"));

	Rast3d_autolock_off(map);
	Rast3d_unlock_all(map);

	G_percent(1, 1, 1);
}

int main(int argc, char *argv[]) {
	struct GModule *module;
	struct {
		struct Option *input;
		struct Option *output;
		struct Option *null;
		struct Option *bytes;
		struct Option *order;
		struct Option *north;
		struct Option *south;
		struct Option *top;
		struct Option *bottom;
		struct Option *east;
		struct Option *west;
		struct Option *rows;
		struct Option *cols;
		struct Option *depths;
	} parm;
	struct {
		struct Flag *integer_in;
		struct Flag *sign;
		struct Flag *depth;
		struct Flag *row;
	} flag;
	const char *input;
	const char *output;
	int is_integer;
	int is_signed;
	int bytes;
	int order;
	int byte_swap;
	RASTER_MAP_TYPE map_type;
	off_t file_size;
	struct History history;
	off_t expected;
	/* Need to be allocated later */
	in_cell = NULL;

	G_gisinit(argv[0]);

	/* Set description */
	module = G_define_module();
	G_add_keyword(_("raster3d"));
	G_add_keyword(_("import"));
	module->description =
			_("Imports a binary raster file into a GRASS 3D raster map.");

	parm.input = G_define_standard_option(G_OPT_F_INPUT);
	parm.input->description = _("Name of binary 3D raster file to be imported");
	parm.input->gisprompt = "old,bin,file";

	parm.output = G_define_standard_option(G_OPT_R3_OUTPUT);

	parm.bytes = G_define_option();
	parm.bytes->key = "bytes";
	parm.bytes->type = TYPE_INTEGER;
	parm.bytes->required = YES;
	parm.bytes->options = "1,2,4,8";
	parm.bytes->description = _("Number of bytes per cell in binary file");
	parm.bytes->guisection = _("Settings");

	parm.order = G_define_option();
	parm.order->key = "order";
	parm.order->type = TYPE_STRING;
	parm.order->required = NO;
	parm.order->options = "big,little,native,swap";
	parm.order->description = _("Byte order in binary file");
	parm.order->answer = "native";

	parm.north = G_define_option();
	parm.north->key = "north";
	parm.north->type = TYPE_DOUBLE;
	parm.north->required = YES;
	parm.north->description =
			_("Northern limit of geographic region (outer edge)");
	parm.north->guisection = _("Bounds");

	parm.south = G_define_option();
	parm.south->key = "south";
	parm.south->type = TYPE_DOUBLE;
	parm.south->required = YES;
	parm.south->description =
			_("Southern limit of geographic region (outer edge)");
	parm.south->guisection = _("Bounds");

	parm.east = G_define_option();
	parm.east->key = "east";
	parm.east->type = TYPE_DOUBLE;
	parm.east->required = YES;
	parm.east->description =
			_("Eastern limit of geographic region (outer edge)");
	parm.east->guisection = _("Bounds");

	parm.west = G_define_option();
	parm.west->key = "west";
	parm.west->type = TYPE_DOUBLE;
	parm.west->required = YES;
	parm.west->description =
			_("Western limit of geographic region (outer edge)");
	parm.west->guisection = _("Bounds");

	parm.bottom = G_define_option();
	parm.bottom->key = "bottom";
	parm.bottom->type = TYPE_DOUBLE;
	parm.bottom->required = YES;
	parm.bottom->description =
			_("Bottom limit of geographic region (outer edge)");
	parm.bottom->guisection = _("Bounds");

	parm.top = G_define_option();
	parm.top->key = "top";
	parm.top->type = TYPE_DOUBLE;
	parm.top->required = YES;
	parm.top->description = _("Top limit of geographic region (outer edge)");
	parm.top->guisection = _("Bounds");

	parm.rows = G_define_option();
	parm.rows->key = "rows";
	parm.rows->type = TYPE_INTEGER;
	parm.rows->required = YES;
	parm.rows->description = _("Number of rows");
	parm.rows->guisection = _("Bounds");

	parm.cols = G_define_option();
	parm.cols->key = "cols";
	parm.cols->type = TYPE_INTEGER;
	parm.cols->required = YES;
	parm.cols->description = _("Number of columns");
	parm.cols->guisection = _("Bounds");

	parm.depths = G_define_option();
	parm.depths->key = "depths";
	parm.depths->type = TYPE_INTEGER;
	parm.depths->required = YES;
	parm.depths->description = _("Number of depths");
	parm.depths->guisection = _("Bounds");

	parm.null = G_define_option();
	parm.null->key = "null";
	parm.null->type = TYPE_DOUBLE;
	parm.null->required = NO;
	parm.null->description = _("Set Value to NULL");
	parm.null->guisection = _("Settings");

	flag.row = G_define_flag();
	flag.row->key = 'r';
	flag.row->description = _("Switch the row order in output from "
			"north->south to south->north");

	flag.depth = G_define_flag();
	flag.depth->key = 'd';
	flag.depth->description = _("Switch the depth order in output "
			"from bottom->top to top->bottom");

	flag.integer_in = G_define_flag();
	flag.integer_in->key = 'i';
	flag.integer_in->description =
			_("Binary data is of type integer");

	flag.sign = G_define_flag();
	flag.sign->key = 's';
	flag.sign->description = _("Signed data (two's complement)");
	flag.sign->guisection = _("Settings");

	if (G_parser(argc, argv))
		exit(EXIT_FAILURE);

	input = parm.input->answer;
	output = parm.output->answer;

	if (G_strcasecmp(parm.order->answer, "big") == 0)
		order = 0;
	else if (G_strcasecmp(parm.order->answer, "little") == 0)
		order = 1;
	else if (G_strcasecmp(parm.order->answer, "native") == 0)
		order = G_is_little_endian() ? 1 : 0;
	else if (G_strcasecmp(parm.order->answer, "swap") == 0)
		order = G_is_little_endian() ? 0 : 1;

	byte_swap = order == (G_is_little_endian() ? 0 : 1);

	is_signed = !!flag.sign->answer;

	is_integer = 0;
	bytes = 8;

	if (parm.bytes->answer)
		bytes = atoi(parm.bytes->answer);

	if (!flag.integer_in->answer) {
		if (bytes && bytes < 4)
			G_fatal_error(
					_("bytes=%d; must be 4 or 8 in case of floating point input"),
					bytes);
		if (!bytes)
			bytes = 4;
	} else {
		is_integer = 1;
	}

#ifndef HAVE_LONG_LONG_INT
	if (is_integer && bytes > 4)
	G_fatal_error(_("Integer input doesn't support size=8 in this build"));
#endif

	if (bytes != 1 && bytes != 2 && bytes != 4 && bytes != 8)
		G_fatal_error(_("bytes= must be 1, 2, 4 or 8"));

	region.zone = G_zone();
	region.proj = G_projection();
	region.rows = atoi(parm.rows->answer);
	region.cols = atoi(parm.cols->answer);
	region.depths = atoi(parm.depths->answer);
	region.top = atof(parm.top->answer);
	region.bottom = atof(parm.bottom->answer);

	if (!G_scan_northing(parm.north->answer, &region.north, region.proj))
		G_fatal_error(_("Illegal north coordinate <%s>"), parm.north->answer);
	if (!G_scan_northing(parm.south->answer, &region.south, region.proj))
		G_fatal_error(_("Illegal south coordinate <%s>"), parm.south->answer);
	if (!G_scan_easting(parm.east->answer, &region.east, region.proj))
		G_fatal_error(_("Illegal east coordinate <%s>"), parm.east->answer);
	if (!G_scan_easting(parm.west->answer, &region.west, region.proj))
		G_fatal_error(_("Illegal west coordinate <%s>"), parm.west->answer);

	Rast3d_adjust_region(&region);

	expected = (off_t) region.rows * region.cols * region.depths * bytes;

	fp = fopen(input, "rb");
	if (!fp)
		G_fatal_error(_("Unable to open <%s>"), input);

	/* Find File Size in Byte and Check against byte size */
	G_fseek(fp, 0, SEEK_END);
	file_size = G_ftell(fp);
	G_fseek(fp, 0, SEEK_SET);

	if (file_size != expected) {
		G_warning(_("File Size %lld ... Total Bytes %lld"),
				(long long int) file_size, (long long int) expected);
		G_fatal_error(_("Bytes do not match file size"));
	}

	map_type = (bytes > 4 ? DCELL_TYPE : FCELL_TYPE);

	if(is_integer && bytes >= 4)
		map_type = DCELL_TYPE;

	Rast3d_init_defaults();

	/*Open the new 3D raster map */
	map = Rast3d_open_new_opt_tile_size(output, RASTER3D_USE_CACHE_DEFAULT,
			&region, map_type, 32);

	if (map == NULL)
		G_fatal_error(_("Unable to open 3D raster map"));

	in_cell = G_malloc(bytes);

	bin_to_raster3d(parm.null->answer, map_type, is_integer, is_signed, bytes,
			byte_swap, flag.row->answer, flag.depth->answer);

	if (!Rast3d_close(map))
		G_fatal_error(_("Unable to close 3D raster map"));

	/* write input name to map history */
	Rast3d_read_history(output, G_mapset(), &history);
	Rast_set_history(&history, HIST_DATSRC_1, input);
	Rast3d_write_history(output, &history);

	fclose(fp);
	if (in_cell)
		G_free(in_cell);

	return EXIT_SUCCESS;
}
