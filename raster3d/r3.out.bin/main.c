
/****************************************************************************
 *
 * MODULE:       r3.out.bin
 *
 * AUTHOR(S):   Soeren Gebbert
 *   			Based on r.out.bin from: Bob Covill and Glynn Clements
 *
 * PURPOSE:     Exports a GRASS 3D raster map to a binary array.
 *
 * COPYRIGHT:   (C) 2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>


RASTER3D_Map *map;
RASTER3D_Region region;

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

static void write_float(FILE *fp, int swap_flag, float x) {
	if (swap_flag)
		swap_4(&x);

	if (fwrite(&x, 4, 1, fp) != 1)
		G_fatal_error(_("Error writing data"));
}

static void write_double(FILE *fp, int swap_flag, double x) {
	if (swap_flag)
		swap_8(&x);

	if (fwrite(&x, 8, 1, fp) != 1)
		G_fatal_error(_("Error writing data"));
}

void raster3d_to_bin(FILE * fp, DCELL null_value, int byte_swap, int row_swap,
		int depth_swap) {
	DCELL dvalue;
	FCELL fvalue;
	int x, y, z;
	int rows, cols, depths, typeIntern;
	int col, row, depth;

	rows = region.rows;
	cols = region.cols;
	depths = region.depths;

	typeIntern = Rast3d_tile_type_map(map);

	for (z = 0; z < depths; z++) {
		G_percent(z, depths, 1);
		for (y = 0; y < rows; y++) { /* g3d rows count from south to north */
			for (x = 0; x < cols; x++) {

				/* From west to east */
				col = x;
				/* The default is to write rows from north to south
				 to be r.in.ascii compatible
				 */
				row = y;
				/* From bottom to the top */
				depth = z;

				/* Write rows from south to north */
				if (row_swap)
					row = rows - y - 1;

				/* write XY layer from top to bottom */
				if (depth_swap)
					depth = depths - z - 1;

				if (typeIntern == FCELL_TYPE) {

					Rast3d_get_value(map, col, row, depth, &fvalue, FCELL_TYPE);

					if (Rast3d_is_null_value_num(&fvalue, FCELL_TYPE))
						write_float(fp, byte_swap, (float)null_value);
					else
						write_float(fp, byte_swap, fvalue);
				} else {

					Rast3d_get_value(map, col, row, depth, &dvalue, DCELL_TYPE);

					if (Rast3d_is_null_value_num(&dvalue, DCELL_TYPE))
						write_double(fp, byte_swap, null_value);
					else
						write_double(fp, byte_swap, dvalue);
				}
			}
		}
	}
	G_percent(1, 1, 1);
	G_percent_reset();
}

int main(int argc, char *argv[]) {
	struct GModule *module;
	struct {
		struct Option *input;
		struct Option *output;
		struct Option *null;
		struct Option *order;
	} parm;
	struct {
		struct Flag *swap, *row, *depth;
	} flag;
	char *name;
	char *outfile;
	double null_val;
	int do_stdout;
	int order;
	int swap_flag;
	FILE *fp;

	G_gisinit(argv[0]);

	module = G_define_module();
	G_add_keyword(_("3D raster"));
	G_add_keyword(_("export"));
	module->description = _("Exports a GRASS 3D raster map to a binary array.");

	/* Define the different options */

	parm.input = G_define_standard_option(G_OPT_R3_INPUT);

	parm.output = G_define_standard_option(G_OPT_F_OUTPUT);

	parm.null = G_define_option();
	parm.null->key = "null";
	parm.null->type = TYPE_DOUBLE;
	parm.null->required = NO;
	parm.null->answer = "0";
	parm.null->description = _("Value to write out for null");

	parm.order = G_define_option();
	parm.order->key = "order";
	parm.order->type = TYPE_STRING;
	parm.order->required = NO;
	parm.order->options = "big,little,native,swap";
	parm.order->description = _("Output byte order");
	parm.order->answer = "native";

	flag.swap = G_define_flag();
	flag.swap->key = 's';
	flag.swap->description = _("Byte swap output");

	flag.row = G_define_flag();
	flag.row->key = 'r';
	flag.row->description = _("Switch the row order in output from "
			"north->south to south->north");

	flag.depth = G_define_flag();
	flag.depth->key = 'd';
	flag.depth->description = _("Switch the depth order in output "
			"from bottom->top to top->bottom");

	if (G_parser(argc, argv))
		exit(EXIT_FAILURE);

	if (sscanf(parm.null->answer, "%lf", &null_val) != 1)
		G_fatal_error(_("Invalid value for null (integers only)"));

	name = parm.input->answer;

	if (parm.output->answer)
		outfile = parm.output->answer;
	else {
		outfile = G_malloc(strlen(name) + 4 + 1);
		G_snprintf(outfile, sizeof(outfile), "%s.bin", name);
	}

	if (G_strcasecmp(parm.order->answer, "big") == 0)
		order = 0;
	else if (G_strcasecmp(parm.order->answer, "little") == 0)
		order = 1;
	else if (G_strcasecmp(parm.order->answer, "native") == 0)
		order = G_is_little_endian() ? 1 : 0;
	else if (G_strcasecmp(parm.order->answer, "swap") == 0)
		order = G_is_little_endian() ? 0 : 1;

	if (flag.swap->answer) {
		if (strcmp(parm.order->answer, "native") != 0)
			G_fatal_error(_("order= and -s are mutually exclusive"));
		order = G_is_little_endian() ? 0 : 1;
	}

	swap_flag = order == (G_is_little_endian() ? 0 : 1);

	do_stdout = strcmp("-", outfile) == 0;

	if (NULL == G_find_raster3d(parm.input->answer, ""))
		Rast3d_fatal_error(_("3D raster map <%s> not found"),
				parm.input->answer);

	/* Initiate the default settings */
	Rast3d_init_defaults();

	/* Figure out the current region settings */
	Rast3d_get_window(&region);

	/* Open the map and use XY cache mode */
	map = Rast3d_open_cell_old(parm.input->answer,
			G_find_raster3d(parm.input->answer, ""), &region,
			RASTER3D_TILE_SAME_AS_FILE, RASTER3D_USE_CACHE_DEFAULT);

	if (map == NULL)
		Rast3d_fatal_error(_("Error opening 3d raster map <%s>"),
				parm.input->answer);

	/* open bin file for writing */
	if (do_stdout)
		fp = stdout;
	else if (NULL == (fp = fopen(outfile, "w")))
		G_fatal_error(_("Unable to create file <%s>"), outfile);

	G_verbose_message(_("Using the current region settings..."));
	G_verbose_message(_("north=%f"), region.north);
	G_verbose_message(_("south=%f"), region.south);
	G_verbose_message(_("east=%f"), region.east);
	G_verbose_message(_("west=%f"), region.west);
	G_verbose_message(_("top=%f"), region.top);
	G_verbose_message(_("bottom=%f"), region.bottom);
	G_verbose_message(_("rows=%d"), region.rows);
	G_verbose_message(_("cols=%d"), region.cols);
	G_verbose_message(_("depths=%d"), region.depths);

	raster3d_to_bin(fp, null_val, swap_flag, flag.row->answer,
			flag.depth->answer);

	Rast3d_close(map);

	fclose(fp);

	return EXIT_SUCCESS;
}

