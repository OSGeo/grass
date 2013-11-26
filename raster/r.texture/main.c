
/****************************************************************************
 *
 * MODULE:       r.texture
 * AUTHOR(S):    Carmine Basco - basco@unisannio.it
 *               with hints from: 
 * 			prof. Giulio Antoniol - antoniol@ieee.org
 * 			prof. Michele Ceccarelli - ceccarelli@unisannio.it
 *
 * PURPOSE:      Create map raster with textural features.
 *
 * COPYRIGHT:    (C) 2003 by University of Sannio (BN), Benevento, Italy 
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted. This 
 * software is provided "as is" without express or implied warranty.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "h_measure.h"

struct menu
{
    char *name;			/* measure name */
    char *desc;			/* menu display - full description */
    char *suffix;		/* output suffix */
    char useme;			/* calculate this measure if set */
    int idx;			/* measure index */
};

/* modify this table to add new measures */
static struct menu menu[] = {
    {"asm",      "Angular Second Moment",    "_ASM",   0,  0},
    {"contrast", "Contrast",                 "_Contr", 0,  1},
    {"corr",     "Correlation",              "_Corr",  0,  2},
    {"var",      "Variance",                 "_Var",   0,  3},
    {"idm",      "Inverse Diff Moment",      "_IDM",   0,  4},
    {"sa",       "Sum Average",              "_SA",    0,  5},
    {"se",       "Sum Entropy",              "_SE",    0,  6},
    {"sv",       "Sum Variance",             "_SV",    0,  7},
    {"entr",     "Entropy",                  "_Entr",  0,  8},
    {"dv",       "Difference Variance",      "_DV",    0,  9},
    {"de",       "Difference Entropy",       "_DE",    0, 10},
    {"moc1",     "Measure of Correlation-1", "_MOC-1", 0, 11},
    {"moc2",     "Measure of Correlation-2", "_MOC-2", 0, 12},
    {NULL, NULL, NULL, 0, -1}
};

static int find_measure(const char *measure_name)
{
    int i;

    for (i = 0; menu[i].name; i++)
	if (strcmp(menu[i].name, measure_name) == 0)
	    return i;

    G_fatal_error(_("Unknown measure <%s>"), measure_name);

    return -1;
}

int main(int argc, char *argv[])
{
    struct Cell_head cellhd;
    char *name, *result;
    char **mapname;
    FCELL **fbuf;
    int n_measures, n_outputs, *measure_idx;
    int nrows, ncols;
    int row, col, first_row, last_row, first_col, last_col;
    int i, j;
    CELL **data;		/* Data structure containing image */
    DCELL *dcell_row;
    struct FPRange range;
    DCELL min, max, inscale;
    FCELL measure;		/* Containing measure done */
    int dist, size;	/* dist = value of distance, size = s. of moving window */
    int offset;
    int have_px, have_py, have_sentr, have_pxpys, have_pxpyd;
    int infd, *outfd;
    RASTER_MAP_TYPE data_type, out_data_type;
    struct GModule *module;
    struct Option *opt_input, *opt_output, *opt_size, *opt_dist, *opt_measure;
    struct Flag *flag_ind, *flag_all;
    struct History history;
    char p[1024];

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("algebra"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("texture"));
    module->description =
	_("Generate images with textural features from a raster map.");
    module->overwrite = 1;

    /* Define the different options */

    opt_input = G_define_standard_option(G_OPT_R_INPUT);

    opt_output = G_define_option();
    opt_output->key = "prefix";
    opt_output->type = TYPE_STRING;
    opt_output->required = YES;
    opt_output->description = _("Prefix for output raster map(s)");

    opt_size = G_define_option();
    opt_size->key = "size";
    opt_size->key_desc = "value";
    opt_size->type = TYPE_INTEGER;
    opt_size->required = NO;
    opt_size->description = _("The size of moving window (odd and >= 3)");
    opt_size->answer = "3";

    /* Textural character is in direct relation of the spatial size of the texture primitives. */

    opt_dist = G_define_option();
    opt_dist->key = "distance";
    opt_dist->key_desc = "value";
    opt_dist->type = TYPE_INTEGER;
    opt_dist->required = NO;
    opt_dist->description = _("The distance between two samples (>= 1)");
    opt_dist->answer = "1";

    for (i = 0; menu[i].name; i++) {
	if (i)
	    strcat(p, ",");
	else
	    *p = 0;
	strcat(p, menu[i].name);
    }
    opt_measure = G_define_option();
    opt_measure->key = "method";
    opt_measure->type = TYPE_STRING;
    opt_measure->required = NO;
    opt_measure->multiple = YES;
    opt_measure->options = p;
    opt_measure->description = _("Textural measurement method");

    flag_ind = G_define_flag();
    flag_ind->key = 's';
    flag_ind->description = _("Separate output for each angle (0, 45, 90, 135)");

    flag_all = G_define_flag();
    flag_all->key = 'a';
    flag_all->description = _("Calculate all textural measurements");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    name = opt_input->answer;
    result = opt_output->answer;
    size = atoi(opt_size->answer);
    if (size <= 0)
	G_fatal_error(_("Size of the moving window must be > 0"));
    if (size % 2 != 1)
	G_fatal_error(_("Size of the moving window must be odd"));
    dist = atoi(opt_dist->answer);
    if (dist <= 0)
	G_fatal_error(_("The distance between two samples must be > 0"));

    n_measures = 0;
    if (flag_all->answer) {
	for (i = 0; menu[i].name; i++) {
	    menu[i].useme = 1;
	}
	n_measures = i;
    }
    else {
	for (i = 0; opt_measure->answers[i]; i++) {
	    if (opt_measure->answers[i]) {
		const char *measure_name = opt_measure->answers[i];
		int n = find_measure(measure_name);

		menu[n].useme = 1;
		n_measures++;
	    }
	}
    }
    if (!n_measures)
	G_fatal_error(_("Nothing to compute. Use at least one textural measure."));
	
    measure_idx = G_malloc(n_measures * sizeof(int));
    j = 0;
    for (i = 0; menu[i].name; i++) {
	if (menu[i].useme == 1) {
	    measure_idx[j] = menu[i].idx;
	    j++;
	}
    }

    /* variables needed */
    if (menu[2].useme || menu[11].useme || menu[12].useme)
	have_px = 1;
    else
	have_px = 0;
    if (menu[11].useme || menu[12].useme)
	have_py = 1;
    else
	have_py = 0;
    if (menu[6].useme || menu[7].useme)
	have_sentr = 1;
    else
	have_sentr = 0;
    if (menu[5].useme || menu[6].useme || menu[7].useme)
	have_pxpys = 1;
    else
	have_pxpys = 0;
    if (menu[9].useme || menu[10].useme)
	have_pxpyd = 1;
    else
	have_pxpyd = 0;

    infd = Rast_open_old(name, "");

    /* determine the inputmap type (CELL/FCELL/DCELL) */
    data_type = Rast_get_map_type(infd);

    Rast_get_cellhd(name, "", &cellhd);

    out_data_type = FCELL_TYPE;
    /* Allocate output buffers, use FCELL data_type */
    n_outputs = n_measures;
    if (flag_ind->answer) {
	n_outputs = n_measures * 4;
    }

    fbuf = G_malloc(n_outputs * sizeof(FCELL *));
    mapname = G_malloc(n_outputs * sizeof(char *));
    for (i = 0; i < n_outputs; i++) {
	mapname[i] = G_malloc(GNAME_MAX * sizeof(char));
	fbuf[i] = Rast_allocate_buf(out_data_type);
    }

    /* open output maps */
    outfd = G_malloc(n_outputs * sizeof(int));
    for (i = 0; i < n_measures; i++) {
	if (flag_ind->answer) {
	    for (j = 0; j < 4; j++) {
		sprintf(mapname[i * 4 + j], "%s%s_%d", result,
		        menu[measure_idx[i]].suffix, j * 45);
		outfd[i * 4 + j] = Rast_open_new(mapname[i * 4 + j], out_data_type);
	    }
	}
	else {
	    sprintf(mapname[i], "%s%s", result,
	            menu[measure_idx[i]].suffix);
	    outfd[i] = Rast_open_new(mapname[i], out_data_type);
	}
    }
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();

    /* Load raster map. */

    /* allocate the space for one row of cell map data *A* */
    dcell_row = Rast_allocate_d_buf();

    /* Allocate appropriate memory for the structure containing the image */
    data = (int **)G_malloc(nrows * sizeof(int *));
    for (i = 0; i < nrows; i++) {
	data[i] = (int *)G_malloc(ncols * sizeof(int));
    }

    /* read input range */
    Rast_init_fp_range(&range);
    Rast_read_fp_range(name, "", &range);
    Rast_get_fp_range_min_max(&range, &min, &max);
    inscale = 0;
    if (min < 0 || max > 255) {
	inscale = 255. / (max - min);
    }
    /* input has 0 - 1 range */
    else if (max <= 1.) {
	inscale = 255. / (max - min);
    }

    /* Read in cell map values */
    /* TODO: use r.proj cache */
    G_important_message(_("Reading raster map..."));
    for (j = 0; j < nrows; j++) {
	Rast_get_row(infd, dcell_row, j, DCELL_TYPE);
	for (i = 0; i < ncols; i++) {
	    if (Rast_is_d_null_value(&(dcell_row[i])))
		data[j][i] = -1;
	    else if (inscale) {
		data[j][i] = (CELL)((dcell_row[i] - min) * inscale);
	    }
	    else
		data[j][i] = (CELL)dcell_row[i];
	}
    }

    /* close input cell map and release the row buffer */
    Rast_close(infd);
    G_free(dcell_row);

    /* Now raster map is loaded to memory. */

    /* *************************************************************************************************
     *
     * Compute of the matrix S.G.L.D. (Spatial Gray-Level Dependence Matrices) or co-occurrence matrix.
     * The image is analized for piece, every piece is naming moving window (s.w.). The s.w. must be    
     * square with number of size's samples odd, that because we want the sample at the center of matrix. 
     *
     ***************************************************************************************************/

    offset = size / 2;
    first_row = first_col = offset;
    last_row = nrows - offset;
    last_col = ncols - offset;
    Rast_set_f_null_value(fbuf[0], ncols);
    for (row = 0; row < first_row; row++) {
	for (i = 0; i < n_outputs; i++) {
	    Rast_put_row(outfd[i], fbuf[0], out_data_type);
	}
    }
    if (n_measures > 1)
	G_message(_("Calculating %d texture measures"), n_measures);
    else
	G_message(_("Calculating %s"), menu[measure_idx[0]].desc);
    alloc_vars(size, dist);
    for (row = first_row; row < last_row; row++) {
	G_percent(row, nrows, 2);

	for (i = 0; i < n_outputs; i++)
	    Rast_set_f_null_value(fbuf[i], ncols);

	/*process the data */
	for (col = first_col; col < last_col; col++) {

	    if (!set_vars(data, row, col, size, offset, dist)) {
		for (i = 0; i < n_outputs; i++)
		    Rast_set_f_null_value(&(fbuf[i][col]), 1);
		continue;
	    }

	    /* for all angles (0, 45, 90, 135) */
	    for (i = 0; i < 4; i++) {
		set_angle_vars(i, have_px, have_py, have_sentr, have_pxpys, have_pxpyd);
		/* for all requested textural measures */
		for (j = 0; j < n_measures; j++) {

		    measure = (FCELL) h_measure(measure_idx[j]);

		    if (flag_ind->answer) {
			/* output for each angle separately */
			fbuf[j * 4 + i][col] = measure;
		    }
		    else {
			/* use average over all angles for each measure */
			if (i == 0)
			    fbuf[j][col] = measure;
			else if (i < 3)
			    fbuf[j][col] += measure;
			else 
			    fbuf[j][col] = (fbuf[j][col] + measure) / 4.0;
		    }
		}
	    }
	}
	for (i = 0; i < n_outputs; i++) {
	    Rast_put_row(outfd[i], fbuf[i], out_data_type);
	}
    }
    Rast_set_f_null_value(fbuf[0], ncols);
    for (row = last_row; row < nrows; row++) {
	for (i = 0; i < n_outputs; i++) {
	    Rast_put_row(outfd[i], fbuf[0], out_data_type);
	}
    }
    G_percent(nrows, nrows, 1);

    for (i = 0; i < n_outputs; i++) {
	Rast_close(outfd[i]);

	Rast_short_history(mapname[i], "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(mapname[i], &history);
	G_free(fbuf[i]);
    }

    G_free(fbuf);
    G_free(data);

    exit(EXIT_SUCCESS);
}
