/*
 ****************************************************************************
 *
 * MODULE:       r.composite
 * AUTHOR(S):    Glynn Clements - glynn.clements@virgin.net
 * PURPOSE:      Combine red, green and blue layers into a single
 *               layer using a quantisation of the RGB color space.
 *               Using Floyd-Steinberg dithering
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

struct band {
	struct Option *opt_name;
	struct Option *opt_levels;
	char *name;
	int levels;
	int maxlev;
	int offset;
	int file;
	int type;
	int size;
	unsigned char *array[3];
	short *floyd[2];
	struct Colors colors;
};

static char * const color_names[3] = {"red", "green", "blue"};

static struct band B[3];
static int closest;

static void make_color_cube(struct Colors *colors);
static int quantize(int c, int x);

int main(int argc, char **argv)
{
	struct GModule *module;
	struct Option *opt_out;
	struct Option *opt_lev;
	struct Flag *flg_d;
	struct Flag *flg_c;
	int dither;
	char *out_name;
	int out_file;
	CELL *out_array;
	struct Colors out_colors;
	int levels;
	char *mapset;
	int atrow, atcol;
	struct Cell_head window;
	unsigned char *dummy, *nulls;
	int i, j;
	struct History history;

	G_gisinit(argv[0]);

	module = G_define_module();
	module->keywords = _("raster");
    module->description =
		_("Combines red, green and blue map layers into "
		"a single composite map layer.");

	for (i = 0; i < 3; i++)
	{
		struct Option *opt;
		char buff[80];

		B[i].opt_name = opt = G_define_option();

		sprintf(buff, "%s", color_names[i]);
		opt->key        = G_store(buff);

		opt->type       = TYPE_STRING;
		opt->answer     = NULL;
		opt->required   = YES;
		opt->gisprompt  = "old,cell,raster";

		sprintf(buff, _("Name of raster map layer to be used for <%s>"),
			color_names[i]);
		opt->description= G_store(buff);
	}

	opt_lev = G_define_option();
	opt_lev->key        = "levels";
	opt_lev->type       = TYPE_INTEGER;
	opt_lev->required   = NO;
	opt_lev->options    = "1-256";
	opt_lev->answer     = "32";
	opt_lev->description= _("Number of levels to be used for each component");

	for (i = 0; i < 3; i++)
	{
		struct Option *opt;
		char buff[80];

		B[i].opt_levels = opt = G_define_option();

		sprintf(buff, "lev_%s", color_names[i]);
		opt->key        = G_store(buff);

		opt->type       = TYPE_INTEGER;
		opt->required   = NO;
		opt->options    = "1-256";

		sprintf(buff, _("Number of levels to be used for <%s>"),
			color_names[i]);
		opt->description= G_store(buff);
	}

	opt_out = G_define_option();
	opt_out->key        = "output";
	opt_out->type       = TYPE_STRING;
	opt_out->required   = YES;
	opt_out->gisprompt  = "new,cell,raster";
	opt_out->description= _("Name of raster map to contain results");

	flg_d = G_define_flag();
	flg_d->key	    = 'd';
	flg_d->description  = _("Dither");

	flg_c = G_define_flag();
	flg_c->key	    = 'c';
	flg_c->description  = _("Use closest color");

	if (G_parser(argc, argv))
		exit(EXIT_FAILURE);

	levels = atoi(opt_lev->answer);

	dither    = flg_d->answer;
	closest   = flg_c->answer;

	/* read in current window */
	G_get_window(&window);

	dummy = G_malloc(window.cols);

	nulls = G_malloc(window.cols);

	for (i = 0; i < 3; i++)
	{
		struct band *b = &B[i];

		/* Get name of layer to be used */
		b->name = b->opt_name->answer;

		mapset = G_find_cell2(b->name, "");
		if (mapset == NULL)
			G_fatal_error(_("Raster map <%s> not found"), b->name);

		/* Make sure map is available */
		if ((b->file = G_open_cell_old(b->name, mapset)) == -1)
			G_fatal_error(_("Unable to open raster map <%s>"), b->name);

		b->type = G_get_raster_map_type(b->file);

		b->size = G_raster_size(b->type);

		/* Reading color lookup table */
		if (G_read_colors(b->name, mapset, &b->colors) == -1)
			G_fatal_error(_("Color file for <%s> not available"), b->name);

		for (j = 0; j < 3; j++)
			b->array[j] = (i == j)
				? G_malloc(window.cols)
				: dummy;

		b->levels = b->opt_levels->answer
			? atoi(b->opt_levels->answer)
			: levels;
		b->maxlev = b->levels - 1;
		b->offset = 128 / b->maxlev;

		if (dither)
			for (j = 0; j < 2; j++)
				b->floyd[j] = G_calloc(window.cols + 2, sizeof(short));
	}

	/* open output files */
	out_name = opt_out->answer;

	mapset = G_find_cell2(out_name, "");
	if (mapset != NULL)
		G_remove("cell", out_name);

	if ((out_file = G_open_cell_new(out_name)) < 0)
		G_fatal_error(_("Unable to create raster map <%s>"), out_name);

	out_array = G_allocate_cell_buf() ;

	/* Make color table */
	make_color_cube(&out_colors);

	for (atrow = 0; atrow < window.rows; atrow++)
	{
		G_percent(atrow, window.rows, 2);

		for (i = 0; i < 3; i++)
		{
			struct band *b = &B[i];

			if (G_get_raster_row_colors(
				    b->file, atrow, &b->colors,
				    b->array[0],
				    b->array[1],
				    b->array[2],
				    nulls) < 0)
				G_fatal_error(_("Error reading '%s' map"), color_names[i]);

			if (dither)
			{
				short *tmp = b->floyd[0];
				b->floyd[0] = b->floyd[1];
				for (atcol = 0; atcol < window.cols + 2; atcol++)
					tmp[atcol] = 0;
				b->floyd[1] = tmp;
			}
		}

		for (atcol = 0; atcol < window.cols; atcol++)
		{
			int val[3];

			if (nulls[atcol])
			{
				G_set_c_null_value(&out_array[atcol], 1);
				continue;
			}

			for (i = 0; i < 3; i++)
			{
				struct band *b = &B[i];
				int v = b->array[i][atcol];

				if (dither)
				{
					int r, w, d;

					v += b->floyd[0][atcol+1] / 16;
					v =	(v < 0) ? 0 :
						(v > 255) ? 255 :
						v;
					r = quantize(i, v);
					w = r * 255 / b->maxlev;
					d = v - w;
					b->floyd[0][atcol+2] += 7 * d;
					b->floyd[1][atcol+0] += 3 * d;
					b->floyd[1][atcol+1] += 5 * d;
					b->floyd[1][atcol+2] += 1 * d;
					val[i] = r;
				}
				else
					val[i] = quantize(i, v);
			}

			out_array[atcol] = (CELL)
				(val[2] * B[1].levels + val[1]) * B[0].levels +
				val[0];
		}

		if(G_put_raster_row(out_file, out_array, CELL_TYPE) < 0)
			G_fatal_error(_("G_put_raster_row failed (file system full?)"));
	}

	G_percent(window.rows, window.rows, 5);

	/* Close the input files */
	for (i = 0; i < 3; i++)
		G_close_cell(B[i].file);

	/* Close the output file */
	G_close_cell(out_file);
	G_write_colors(out_name, G_mapset(), &out_colors);
	G_short_history(out_name, "raster", &history);
	G_command_history(&history);
	G_write_history(out_name, &history);


	exit(EXIT_SUCCESS);
}

static int quantize(int c, int x)
{
	return	closest
		? (x + B[c].offset) * B[c].maxlev / 256
		: x * B[c].levels / 256;
}

static void make_color_cube(struct Colors *colors)
{
	int nr = B[0].levels;
	int ng = B[1].levels;
	int nb = B[2].levels;
	int mr = B[0].maxlev;
	int mg = B[1].maxlev;
	int mb = B[2].maxlev;
	int g, b;
	int i = 0;

	G_init_colors(colors);

	for (b = 0; b < nb; b++)
	{
		G_percent(b, nb, 5);
		
		for (g = 0; g < ng; g++)
		{
			int blu = b * 255 / mb;
			int grn = g * 255 / mg;
			CELL i0 = i;
			CELL i1 = i + mr;

			G_add_c_raster_color_rule(&i0,   0, grn, blu,
						  &i1, 255, grn, blu,
						  colors);

			i += nr;
		}
	}

	G_percent(nb, nb, 5);
}
