/*
 ****************************************************************************
 *
 * MODULE:       d.vect.thematic
 * AUTHOR(S):    Moritz Lennert, based on d.vect
 * PURPOSE:      Display a thematic vector map
 * TODO:         Common part of code merge with d.vect (similarly as r.colors
 *               and r3.colors)
 * COPYRIGHT:    (C) 2007-2014 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/vector.h>
#include <grass/colors.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include <grass/arraystats.h>
#include "plot.h"
#include "local_proto.h"


int main(int argc, char **argv)
{
    int ret, level;
    int i, stat = 0;
    int nclass = 0, nbreaks, *frequencies, boxsize, textsize, ypos;
    int chcat = 0;
    int r, g, b;
    int has_color = 0;
    struct color_rgb *colors, bcolor;
    int default_width;
    int verbose = FALSE;
    char map_name[128];
    struct GModule *module;
    struct Option *map_opt;
    struct Option *column_opt;
    struct Option *breaks_opt;
    struct Option *algo_opt;
    struct Option *nbclass_opt;
    struct Option *colors_opt;
    struct Option *bcolor_opt;
    struct Option *bwidth_opt;
    struct Option *where_opt;
    struct Option *field_opt;
    struct Option *legend_file_opt;
    struct Flag *legend_flag, *algoinfo_flag, *nodraw_flag;
    char *desc;

    struct cat_list *Clist;
    int *cats, ncat, nrec, ctype;
    struct Map_info Map;
    struct field_info *fi;
    dbDriver *driver;
    dbHandle handle;
    dbCatValArray cvarr;
    struct Cell_head window;
    struct bound_box box;
    double overlap, *breakpoints, *data = NULL, class_info = 0.0;
    struct GASTATS stats;
    FILE *fd;

    /* Initialize the GIS calls */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("display"));
    G_add_keyword(_("cartography"));
    G_add_keyword(_("choropleth map"));
    G_add_keyword(_("legend"));
    module->description =
	_("Displays a thematic vector map "
	  "in the active graphics frame.");

    map_opt = G_define_standard_option(G_OPT_V_MAP);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);
    field_opt->description =
	_("Layer number. If -1, all layers are displayed.");
    field_opt->guisection = _("Selection");

    column_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    column_opt->required = YES;
    column_opt->description =
	_("Name of attribute column to be classified");

    breaks_opt = G_define_option();
    breaks_opt->key = "breaks";
    breaks_opt->type = TYPE_STRING;
    breaks_opt->required = NO;
    breaks_opt->multiple = YES;
    breaks_opt->description = _("Class breaks, without minimum and maximum");
    breaks_opt->guisection = _("Classes");
    
    algo_opt = G_define_option();
    algo_opt->key = "algorithm";
    algo_opt->type = TYPE_STRING;
    algo_opt->required = NO;
    algo_opt->multiple = NO;
    algo_opt->options = "int,std,qua,equ,dis";
    algo_opt->description = _("Algorithm to use for classification");
    desc = NULL;
    G_asprintf(&desc,
	        "int;%s;std;%s;qua;%s;equ;%s",
	        _("simple intervals"),
	        _("standard deviations"),
	        _("quantiles"),
	        _("equiprobable (normal distribution)"));
    algo_opt->descriptions = desc;
    /*currently disabled because of bugs       "dis;discontinuities"); */
    algo_opt->guisection = _("Classes");

    nbclass_opt = G_define_option();
    nbclass_opt->key = "nclasses";
    nbclass_opt->type = TYPE_INTEGER;
    nbclass_opt->required = NO;
    nbclass_opt->multiple = NO;
    nbclass_opt->description = _("Number of classes to define");
    nbclass_opt->guisection = _("Classes");

    colors_opt = G_define_option();
    colors_opt->key = "colors";
    colors_opt->type = TYPE_STRING;
    colors_opt->required = YES;
    colors_opt->multiple = YES;
    colors_opt->description = _("Colors (one per class)");
    colors_opt->gisprompt = "old_color,color,color";

    where_opt = G_define_standard_option(G_OPT_DB_WHERE);
    where_opt->guisection = _("Selection");

    bwidth_opt = G_define_option();
    bwidth_opt->key = "boundary_width";
    bwidth_opt->type = TYPE_INTEGER;
    bwidth_opt->answer = "0";
    bwidth_opt->guisection = _("Boundaries");
    bwidth_opt->description = _("Boundary width");

    bcolor_opt = G_define_standard_option(G_OPT_C);
    bcolor_opt->key = "boundary_color";
    bcolor_opt->label = _("Boundary color");
    bcolor_opt->guisection = _("Boundaries");

    legend_file_opt = G_define_standard_option(G_OPT_F_OUTPUT);
    legend_file_opt->key = "legendfile";
    legend_file_opt->description =
	_("File in which to save d.graph instructions for legend display");
    legend_file_opt->required = NO;
    legend_file_opt->guisection = _("Legend");

    legend_flag = G_define_flag();
    legend_flag->key = 'l';
    legend_flag->description =
	_("Create legend information and send to stdout");
    legend_flag->guisection = _("Legend");

    algoinfo_flag = G_define_flag();
    algoinfo_flag->key = 'e';
    algoinfo_flag->description =
	_("When printing legend info, include extended statistical info from classification algorithm");
    algoinfo_flag->guisection = _("Legend");

    nodraw_flag = G_define_flag();
    nodraw_flag->key = 'n';
    nodraw_flag->description = _("Do not draw map, only output the legend");
    nodraw_flag->guisection = _("Legend");
    
    /* Check command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (G_verbose() > G_verbose_std())
	verbose = TRUE;

    G_get_set_window(&window);

    /* Read map options */

    strcpy(map_name, map_opt->answer);

    /* open vector */
    level = Vect_open_old(&Map, map_name, "");

    if (level < 2)
	G_fatal_error(_("%s: You must build topology on vector map. Run v.build."),
		      map_name);

    /* Check database connection and open it */
    Clist = Vect_new_cat_list();
    Clist->field = atoi(field_opt->answer);
    if (Clist->field < 1)
	G_fatal_error(_("'layer' must be > 0"));
    if ((fi = Vect_get_field(&Map, Clist->field)) == NULL)
	G_fatal_error(_("Database connection not defined"));
    if (fi != NULL) {
	driver = db_start_driver(fi->driver);
	if (driver == NULL)
	    G_fatal_error(_("Unable to start driver <%s>"), fi->driver);

	db_init_handle(&handle);
	db_set_handle(&handle, fi->database, NULL);
	if (db_open_database(driver, &handle) != DB_OK)
	    G_fatal_error(_("Unable to open database <%s>"), fi->database);
    }



    /*Get CatValArray needed for plotting and for legend calculations */
    db_CatValArray_init(&cvarr);
    nrec = db_select_CatValArray(driver, fi->table, fi->key,
				 column_opt->answer, where_opt->answer,
				 &cvarr);


    G_debug(3, "nrec (%s) = %d", column_opt->answer, nrec);

    if (cvarr.ctype != DB_C_TYPE_INT && cvarr.ctype != DB_C_TYPE_DOUBLE)
	G_fatal_error(_("Data (%s) not numeric. "
			"Column must be numeric."), column_opt->answer);

    if (nrec < 0)
	G_fatal_error(_("Cannot select data (%s) from table"),
		      column_opt->answer);

    for (i = 0; i < cvarr.n_values; i++) {
	G_debug(4, "cat = %d  %s = %d", cvarr.value[i].cat,
		column_opt->answer,
		(cvarr.ctype ==
		 DB_C_TYPE_INT ? cvarr.value[i].val.i : (int)cvarr.value[i].
		 val.d));
    }

    /*Get the sorted data */
    ret = db_CatValArray_sort_by_value(&cvarr);
    if (ret == DB_FAILED)
	G_fatal_error("Could not sort array of values..");


    data = (double *)G_malloc((nrec) * sizeof(double));
    for (i = 0; i < nrec; i++)
	data[i] = 0.0;

    ctype = cvarr.ctype;
    if (ctype == DB_C_TYPE_INT) {
	for (i = 0; i < nrec; i++)
	    data[i] = cvarr.value[i].val.i;
    }
    else {
	for (i = 0; i < nrec; i++)
	    data[i] = cvarr.value[i].val.d;
    }
    db_CatValArray_sort(&cvarr);


    /*Get the list of relevant cats if where option is given */
    if (where_opt->answer) {
	ncat = db_select_int(driver, fi->table, fi->key, where_opt->answer,
			     &cats);
	chcat = 1;

	Vect_array_to_cat_list(cats, ncat, Clist);
    }

    db_close_database(driver);
    db_shutdown_driver(driver);

    /*get border line width */
    default_width = atoi(bwidth_opt->answer);
    if (default_width < 0)
	default_width = 0;

    /*get border line color */
    bcolor = G_standard_color_rgb(WHITE);
    ret = G_str_to_color(bcolor_opt->answer, &r, &g, &b);
    if (ret == 1) {
	has_color = 1;
	bcolor.r = r;
	bcolor.g = g;
	bcolor.b = b;
    }
    else if (ret == 2) {	/* none */
	has_color = 0;
    }
    else if (ret == 0) {	/* error */
	G_fatal_error(_("Unknown color: [%s]"), bcolor_opt->answer);
    }


    /* if both class breaks and (algorithm or classnumber) are given, give precedence to class 
     * breaks
     */

    if (breaks_opt->answers) {

	if (algo_opt->answer || nbclass_opt->answer)
	    G_warning(_("You gave both manual breaks and a classification algorithm or a number of classes. The manual breaks have precedence and will thus be used."));


	/*Get class breaks */
	nbreaks = 0;
	while (breaks_opt->answers[nbreaks] != NULL)
	    nbreaks++;
	nclass = nbreaks + 1;	/*add one since breaks do not include min and max values */
	G_debug(3, "nclass = %d", nclass);

	breakpoints = (double *)G_malloc((nbreaks) * sizeof(double));
	for (i = 0; i < nbreaks; i++)
	    breakpoints[i] = atof(breaks_opt->answers[i]);

    }
    else {

	if (algo_opt->answer && nbclass_opt->answer) {


	    nclass = atoi(nbclass_opt->answer);
	    nbreaks = nclass - 1;	/* we need one less classbreaks (min and 
					 * max exluded) than classes */

	    breakpoints = (double *)G_malloc((nbreaks) * sizeof(double));
	    for (i = 0; i < nbreaks; i++)
		breakpoints[i] = 0;

	    /* Get classbreaks for given algorithm and number of classbreaks.
	     * class_info takes any info coming from the classification algorithms */
	    class_info = AS_class_apply_algorithm(AS_option_to_algorithm(algo_opt),
                                                  data, nrec, &nbreaks,
                                                  breakpoints);

	}
	else {

	    G_fatal_error(_("You must either give classbreaks or a classification algorithm"));

	}
    };


    /* Fill colors */
    colors = (struct color_rgb *)G_malloc(nclass * sizeof(struct color_rgb));


    if (colors_opt->answers != NULL) {
	for (i = 0; i < nclass; i++) {
	    if (colors_opt->answers[i] == NULL)
		G_fatal_error(_("Not enough colors or error in color specifications.\nNeed %i entries for 'colors' parameter"),
			      nclass);

	    ret = G_str_to_color(colors_opt->answers[i], &r, &g, &b);
	    if (!ret)
		G_fatal_error(_("Error interpreting color %s"),
			      colors_opt->answers[i]);
	    colors[i].r = r;
	    colors[i].g = g;
	    colors[i].b = b;

	}
    }


    if (!nodraw_flag->answer) {
	/* Now's let's prepare the actual plotting */
        D_open_driver();
	
	D_setup(0);

	if (verbose)
	    G_message(_("Plotting ..."));

	Vect_get_map_box(&Map, &box);

	if (window.north < box.S || window.south > box.N ||
	    window.east < box.W ||
	    window.west > G_adjust_easting(box.E, &window)) {
	    G_message(_("The bounding box of the map is outside the current region, "
		       "nothing drawn."));
	    stat = 0;
	}
	else {
	    overlap =
		G_window_percentage_overlap(&window, box.N, box.S, box.E,
					    box.W);
	    G_debug(1, "overlap = %f \n", overlap);
	    if (overlap < 1)
		Vect_set_constraint_region(&Map, window.north, window.south,
					   window.east, window.west,
					   PORT_DOUBLE_MAX, -PORT_DOUBLE_MAX);

	    /* default line width */
	    D_line_width(default_width);


	    stat =
		dareatheme(&Map, Clist, &cvarr, breakpoints, nbreaks, colors,
			   has_color ? &bcolor : NULL, chcat, &window,
			   default_width);


	    /* reset line width: Do we need to get line width from display
	     * driver (not implemented)?  It will help restore previous line
	     * width (not just 0) determined by another module (e.g.,
	     * d.linewidth). */
	    D_line_width(0);

	}			/* end window check if */

	D_save_command(G_recreate_command());
	D_close_driver();

    }				/* end of nodraw_flag condition */

    frequencies = (int *)G_malloc((nbreaks + 1) * sizeof(int));
    for (i = 0; i < nbreaks + 1; i++)
	frequencies[i] = 0.0;
    AS_class_frequencies(data, nrec, nbreaks, breakpoints, frequencies);

    /*Get basic statistics about the data */
    AS_basic_stats(data, nrec, &stats);

    if (legend_flag->answer) {

	if (algoinfo_flag->answer) {


	    fprintf(stdout, _("\nTotal number of records: %.0f\n"),
		    stats.count);
	    fprintf(stdout, _("Classification of %s into %i classes\n"),
		    column_opt->answer, nbreaks + 1);
	    fprintf(stdout, _("Using algorithm: *** %s ***\n"),
		    algo_opt->answer);
	    fprintf(stdout, _("Mean: %f\tStandard deviation = %f\n"),
		    stats.mean, stats.stdev);

	    if (G_strcasecmp(algo_opt->answer, "dis") == 0)
		fprintf(stdout, _("Last chi2 = %f\n"), class_info);
	    if (G_strcasecmp(algo_opt->answer, "std") == 0)
		fprintf(stdout,
			_("Stdev multiplied by %.4f to define step\n"),
			class_info);
	    fprintf(stdout, "\n");

	}

        if(stats.min > breakpoints[0]){
	fprintf(stdout, "<%f|%i|%d:%d:%d\n",
		breakpoints[0], frequencies[0], colors[0].r,
		colors[0].g, colors[0].b);

        } else {
	fprintf(stdout, "%f|%f|%i|%d:%d:%d\n",
		stats.min, breakpoints[0], frequencies[0], colors[0].r,
		colors[0].g, colors[0].b);
        }

	for (i = 1; i < nbreaks; i++) {
	    fprintf(stdout, "%f|%f|%i|%d:%d:%d\n",
		    breakpoints[i - 1], breakpoints[i], frequencies[i],
		    colors[i].r, colors[i].g, colors[i].b);
	}

        if(stats.max < breakpoints[nbreaks-1]){
	fprintf(stdout, ">%f|%i|%d:%d:%d\n",
		breakpoints[nbreaks - 1], frequencies[nbreaks],
		colors[nbreaks].r, colors[nbreaks].g, colors[nbreaks].b);
        } else {
	fprintf(stdout, "%f|%f|%i|%d:%d:%d\n",
		breakpoints[nbreaks - 1], stats.max, frequencies[nbreaks],
		colors[nbreaks].r, colors[nbreaks].g, colors[nbreaks].b);
        }
    }

    if (legend_file_opt->answer) {
	fd = fopen(legend_file_opt->answer, "w");
	boxsize = 25;
	textsize = 8;
	fprintf(fd, "size %i %i\n", textsize, textsize);
	ypos = 10;
	fprintf(fd, "symbol basic/box %i 5 %i black %d:%d:%d\n", boxsize,
		ypos, colors[0].r, colors[0].g, colors[0].b);
	fprintf(fd, "move 8 %i \n", ypos-1);
        if(stats.min > breakpoints[0]){
	fprintf(fd, "text <%f | %i\n", breakpoints[0],
		frequencies[0]);
        } else {
	fprintf(fd, "text %f - %f | %i\n", stats.min, breakpoints[0],
		frequencies[0]);
        }
	for (i = 1; i < nbreaks; i++) {
	    ypos = 10 + i * 6;
	    fprintf(fd, "symbol basic/box %i 5 %i black %d:%d:%d\n", boxsize,
		    ypos, colors[i].r, colors[i].g, colors[i].b);
	    fprintf(fd, "move 8 %i\n", ypos-1);
	    fprintf(fd, "text %f - %f | %i\n", breakpoints[i - 1],
		    breakpoints[i], frequencies[i]);
	}
	ypos = 10 + i * 6;
	fprintf(fd, "symbol basic/box %i 5 %i black %d:%d:%d\n", boxsize,
		ypos, colors[nbreaks].r, colors[nbreaks].g,
		colors[nbreaks].b);
	fprintf(fd, "move 8 %i\n", ypos -1);
        if(stats.max < breakpoints[nbreaks-1]){
	fprintf(fd, "text >%f | %i\n", breakpoints[nbreaks - 1],
		frequencies[nbreaks]);
        } else {
	fprintf(fd, "text %f - %f | %i\n", breakpoints[nbreaks - 1],
		stats.max, frequencies[nbreaks]);
        }
	fclose(fd);
    }

    if (verbose)
	G_done_msg(" ");

    Vect_close(&Map);
    Vect_destroy_cat_list(Clist);

    exit(stat);
}
