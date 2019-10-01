
/****************************************************************************
 *
 * MODULE:       r.resample
 * AUTHOR(S):    Michael Shapiro (original CERL contributor),
 *               Markus Neteler <neteler itc.it>,
 *               Roberto Flor <flor itc.it>, 
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Jachym Cepicky <jachym les-ejk.cz>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      resamples the data values in a user-specified raster
 *               input map layer
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    struct History hist;
    struct Categories cats;
    struct Categories newcats;
    struct Colors colr;
    struct Cell_head cellhd;
    struct Range range;
    int hist_ok, colr_ok, cats_ok;
    char *name;
    char *result;
    void *rast;
    int nrows, ncols;
    int row;
    int infd, outfd;
    RASTER_MAP_TYPE data_type, out_type;
    struct GModule *module;
    struct
    {
	struct Option *input, *output;
    } option;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("resample"));
    G_add_keyword(_("nearest neighbor"));
    module->description =
	_("GRASS raster map layer data resampling capability.");

    /* Define the different options */

    option.input = G_define_option();
    option.input->key = "input";
    option.input->type = TYPE_STRING;
    option.input->required = YES;
    option.input->gisprompt = "old,cell,raster";
    option.input->description = _("Name of an input layer");

    option.output = G_define_option();
    option.output->key = "output";
    option.output->type = TYPE_STRING;
    option.output->required = YES;
    option.output->gisprompt = "new,cell,raster";
    option.output->description = _("Name of an output layer");

    /* Define the different flags */

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    name = option.input->answer;
    result = option.output->answer;

    hist_ok = Rast_read_history(name, "", &hist) >= 0;
    colr_ok = Rast_read_colors(name, "", &colr) > 0;
    cats_ok = Rast_read_cats(name, "", &cats) >= 0;
    if (cats_ok) {
	Rast_unmark_cats(&cats);
	Rast_init_cats(Rast_get_cats_title(&cats), &newcats);
    }

    infd = Rast_open_old(name, "");

    /* determine the map type;
       data_type is the type of data being processed,
       out_type is the type of map being created. */
    data_type = Rast_get_map_type(infd);
    out_type = data_type;

    Rast_get_cellhd(name, "", &cellhd);

    /* raster buffer is big enough to hold data */
    rast = Rast_allocate_buf(data_type);
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    if (ncols <= 1)
	rast = G_realloc(rast, 2 * Rast_cell_size(data_type));
    /* we need the buffer at least 2 cells large */

    outfd = Rast_open_new(result, out_type);
    Rast_set_null_value(rast, ncols, out_type);

    G_message(_("Percent complete: "));

    for (row = 0; row < nrows; row++) {
	G_percent(row, nrows, 2);
	Rast_get_row(infd, rast, row, data_type);
	Rast_put_row(outfd, rast, out_type);
	Rast_mark_cats(rast, ncols, &cats, data_type);
    }

    G_percent(row, nrows, 2);

    Rast_close(infd);

    G_message(_("Creating support files for <%s>..."), result);

    Rast_close(outfd);

    Rast_rewind_cats(&cats);

    if (cats_ok) {
	long count;
	void *rast1, *rast2;

	rast1 = rast;
	rast2 = G_incr_void_ptr(rast, Rast_cell_size(data_type));

	G_message(_("Creating new cats file..."));
	while (Rast_get_next_marked_cat(&cats,
					    rast1, rast2, &count, data_type))
	    Rast_set_cat(rast1, rast2,
			     Rast_get_cat(rast1, &cats, data_type),
			     &newcats, data_type);

	Rast_write_cats(result, &newcats);
	Rast_free_cats(&cats);
	Rast_free_cats(&newcats);
    }

    if (colr_ok) {
	if (Rast_read_range(result, G_mapset(), &range) > 0) {
	    CELL min, max, cmin, cmax;

	    Rast_get_range_min_max(&range, &min, &max);
	    Rast_get_c_color_range(&cmin, &cmax, &colr);
	    if (min > cmin)
		cmin = min;
	    if (max < cmax)
		cmax = max;
	    Rast_set_c_color_range(cmin, cmax, &colr);
	}
	Rast_write_colors(result, G_mapset(), &colr);
    }

    if (hist_ok)
	Rast_write_history(result, &hist);

    exit(EXIT_SUCCESS);
}
