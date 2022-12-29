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
    int nclass = 0, nbreaks, *frequencies;
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
    struct Option *icon_opt;
    struct Option *icon_line_opt;
    struct Option *icon_area_opt;
    struct Option *size_opt;
    struct Option *title_opt;
    struct Flag *legend_flag, *algoinfo_flag, *nodraw_flag, *vlegend_flag;
    char *desc, *deprecated;

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
    int size;
    int nfeatures;
    char title[128];
    char *leg_file;

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
    bwidth_opt->answer = "1";
    bwidth_opt->guisection = _("Boundaries");
    bwidth_opt->description = _("Boundary width");

    bcolor_opt = G_define_standard_option(G_OPT_CN);
    bcolor_opt->key = "boundary_color";
    bcolor_opt->label = _("Boundary color");
    bcolor_opt->guisection = _("Boundaries");

    /* Symbols */
    icon_opt = G_define_option();
    icon_opt->key = "icon";
    icon_opt->type = TYPE_STRING;
    icon_opt->required = NO;
    icon_opt->multiple = NO;
    icon_opt->guisection = _("Symbols");
    icon_opt->answer = "basic/x";
    /* This could also use ->gisprompt = "old,symbol,symbol" instead of ->options */
    icon_opt->options = icon_files();
    icon_opt->description = _("Point and centroid symbol");

    size_opt = G_define_option();
    size_opt->key = "size";
    size_opt->type = TYPE_DOUBLE;
    size_opt->answer = "5";
    size_opt->guisection = _("Symbols");
    size_opt->label = _("Symbol size");

    icon_line_opt = G_define_option();
    icon_line_opt->key = "icon_line";
    icon_line_opt->type = TYPE_STRING;
    icon_line_opt->required = NO;
    icon_line_opt->multiple = NO;
    icon_line_opt->guisection = _("Legend");
    icon_line_opt->answer = "legend/line";
    /* This could also use ->gisprompt = "old,symbol,symbol" instead of ->options */
    icon_line_opt->options = icon_files();
    icon_line_opt->description = _("Legend symbol for lines");

    icon_area_opt = G_define_option();
    icon_area_opt->key = "icon_area";
    icon_area_opt->type = TYPE_STRING;
    icon_area_opt->required = NO;
    icon_area_opt->multiple = NO;
    icon_area_opt->guisection = _("Legend");
    icon_area_opt->answer = "legend/area";
    /* This could also use ->gisprompt = "old,symbol,symbol" instead of ->options */
    icon_area_opt->options = icon_files();
    icon_area_opt->description = _("Legend symbol for areas");

    title_opt = G_define_option();
    title_opt->key = "legend_title";
    title_opt->type = TYPE_STRING;
    title_opt->guisection = _("Legend");
    title_opt->description = _("Thematic map title");

    legend_file_opt = G_define_standard_option(G_OPT_F_OUTPUT);
    legend_file_opt->key = "legendfile";
    deprecated = NULL;
    G_asprintf(&deprecated,
	        "[%s] %s",
	        _("DEPRECATED"),
	        _("Output legend file"));
    legend_file_opt->description = deprecated;
    legend_file_opt->required = NO;
    legend_file_opt->guisection = _("Legend");

    legend_flag = G_define_flag();
    legend_flag->key = 'l';
    legend_flag->description =
	_("Create legend information and send to stdout");
    legend_flag->guisection = _("Legend");

    nodraw_flag = G_define_flag();
    nodraw_flag->key = 'n';
    nodraw_flag->description = _("Do not draw map, only output the legend information");
    nodraw_flag->guisection = _("Legend");

    algoinfo_flag = G_define_flag();
    algoinfo_flag->key = 'e';
    deprecated = NULL;
    G_asprintf(&deprecated,
	        "[%s] %s",
	        _("DEPRECATED"),
	        _("When printing legend info, include extended statistical info from classification algorithm"));
    algoinfo_flag->description = deprecated;
    algoinfo_flag->guisection = _("Legend");
    
    vlegend_flag = G_define_flag();
    vlegend_flag->key = 's';
    vlegend_flag->label = _("Do not show this layer in vector legend");
    vlegend_flag->guisection = _("Legend");
    
    G_option_required(algo_opt, breaks_opt, NULL);
    G_option_exclusive(algo_opt, breaks_opt, NULL);
    G_option_requires(algo_opt, nbclass_opt, NULL);

    /* Check command line */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (algoinfo_flag->answer)
        G_warning(_("Flag -e is deprecated, set verbose mode with --v to get the extended statistical info."));

    if (legend_file_opt->answer)
        G_warning(_("Option legendfile is deprecated, either use flag -l "
                    "to print legend to standard output, "
                    "or set GRASS_LEGEND_FILE environment variable "
                    "(see d.legend.vect for details)."));

    if (G_verbose() > G_verbose_std())
	verbose = TRUE;

    G_get_set_window(&window);

    size = atof(size_opt->answer);

    /* Read map options */
    strcpy(map_name, map_opt->answer);

    /* open vector */
    level = Vect_open_old(&Map, map_name, "");
    
    if (level < 2)
	G_fatal_error(_("%s: You must build topology on vector map. Run v.build."),
		      map_name);
    if (title_opt->answer)
        strcpy(title, title_opt->answer);
    else
        strcpy(title, Map.name);

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

	overlap = 1;
	Vect_get_map_box(&Map, &box);
	if (window.proj != PROJECTION_LL) {
	    overlap =
		G_window_percentage_overlap(&window, box.N, box.S,
		                            box.E, box.W);
	    G_debug(1, "overlap = %f \n", overlap);
	}

	if (overlap == 0) {
	    G_message(_("The bounding box of the map is outside the current region, "
		       "nothing drawn."));
	    stat = 0;
	}
	else {
	    if (overlap < 1)
		Vect_set_constraint_region(&Map, window.north, window.south,
					   window.east, window.west,
					   PORT_DOUBLE_MAX, -PORT_DOUBLE_MAX);

	    /* default line width */
	    D_line_width(default_width);

	    if (Vect_get_num_primitives(&Map, GV_BOUNDARY) > 0)
		stat =
		    dareatheme(&Map, Clist, &cvarr, breakpoints, nbreaks, colors,
			       has_color ? &bcolor : NULL, chcat, &window,
			       default_width);

	    else if ((Vect_get_num_primitives(&Map, GV_POINT) > 0) ||
		     (Vect_get_num_primitives(&Map, GV_LINE) > 0)){
		stat = display_lines(&Map, Clist, chcat, icon_opt->answer, size,
		       default_width, &cvarr, breakpoints, nbreaks, colors,
		       has_color ? &bcolor : NULL);
	    }


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

    /* Print statistics */
    G_verbose_message(_("\nTotal number of records: %.0f\n"),
                      stats.count);
    G_verbose_message(_("Classification of %s into %i classes\n"),
                      column_opt->answer, nbreaks + 1);
    G_verbose_message(_("Using algorithm: *** %s ***\n"),
                      algo_opt->answer);
    G_verbose_message(_("Mean: %f\tStandard deviation = %f\n"),
                      stats.mean, stats.stdev);
    
    if (G_strcasecmp(algo_opt->answer, "dis") == 0)
        G_verbose_message(_("Last chi2 = %f\n"), class_info);
    if (G_strcasecmp(algo_opt->answer, "std") == 0)
        G_verbose_message(_("Stdev multiplied by %.4f to define step\n"),
                          class_info);
    G_verbose_message("\n");

	/* Print legfile to stdout */
    if ((legend_flag->answer) ||
            ((legend_file_opt->answer) && (strcmp(legend_file_opt->answer,"-") == 0))) {
    while (TRUE) {
    nfeatures = Vect_get_num_primitives(&Map, GV_POINT);
        if (nfeatures > 0) {
            write_into_legend_file("stdout", icon_opt->answer,
                                   title, stats.min, stats.max, breakpoints,
                                   nbreaks, size, bcolor, colors, default_width,
                                   frequencies, "point");
            break;
        }
        nfeatures = Vect_get_num_primitives(&Map, GV_LINE);
        if (nfeatures > 0) {
            write_into_legend_file("stdout", icon_line_opt->answer,
                                   title, stats.min, stats.max, breakpoints,
                                   nbreaks, size, bcolor, colors, default_width,
                                   frequencies, "line");
            break;
	}
        nfeatures = Vect_get_num_primitives(&Map, GV_BOUNDARY);
        if (nfeatures > 0) {
            write_into_legend_file("stdout", icon_area_opt->answer,
                                   title, stats.min, stats.max, breakpoints,
                                   nbreaks, size, bcolor, colors, default_width,
                                   frequencies, "area");
            break;
        }
    }
    }

    /* Write into default legfile */
    leg_file = getenv("GRASS_LEGEND_FILE");
    if (leg_file && !vlegend_flag->answer) {
        while (TRUE) {
        nfeatures = Vect_get_num_primitives(&Map, GV_POINT);
            if (nfeatures > 0) {
                write_into_legend_file(leg_file, icon_opt->answer,
                                       title, stats.min, stats.max, breakpoints,
                                       nbreaks, size, bcolor, colors, default_width,
                                       frequencies, "point");
                break;
        }
            nfeatures = Vect_get_num_primitives(&Map, GV_LINE);
            if (nfeatures > 0) {
                write_into_legend_file(leg_file, icon_line_opt->answer,
                                       title, stats.min, stats.max, breakpoints,
                                       nbreaks, size, bcolor, colors, default_width,
                                       frequencies, "line");
                break;
    }
            nfeatures = Vect_get_num_primitives(&Map, GV_BOUNDARY);
            if (nfeatures > 0) {
                write_into_legend_file(leg_file, icon_area_opt->answer,
                                       title, stats.min, stats.max, breakpoints,
                                       nbreaks, size, bcolor, colors, default_width,
                                       frequencies, "area");
                break;
            }
        }
    }

    /* Write into user-specified output file */

    if (legend_file_opt->answer) {
        while (TRUE) {
        nfeatures = Vect_get_num_primitives(&Map, GV_POINT);
            if (nfeatures > 0) {
                write_into_legend_file(legend_file_opt->answer, icon_opt->answer,
                                       title, stats.min, stats.max, breakpoints,
                                       nbreaks, size, bcolor, colors, default_width,
                                       frequencies, "point");
                break;
        }
            nfeatures = Vect_get_num_primitives(&Map, GV_LINE);
            if (nfeatures > 0) {
                write_into_legend_file(legend_file_opt->answer, icon_line_opt->answer,
                                       title, stats.min, stats.max, breakpoints,
                                       nbreaks, size, bcolor, colors, default_width,
                                       frequencies, "line");
                break;
	}
            nfeatures = Vect_get_num_primitives(&Map, GV_BOUNDARY);
            if (nfeatures > 0) {
                write_into_legend_file(legend_file_opt->answer, icon_area_opt->answer,
                                       title, stats.min, stats.max, breakpoints,
                                       nbreaks, size, bcolor, colors, default_width,
                                       frequencies, "area");
                break;
        }
    }
    }

    if (verbose)
	G_done_msg(" ");

    Vect_close(&Map);
    Vect_destroy_cat_list(Clist);

    exit(stat);
}

int cmp(const void *a, const void *b)
{
    return (strcmp(*(char **)a, *(char **)b));
}

/* adopted from r.colors */
char *icon_files(void)
{
    char **list, *ret;
    char buf[GNAME_MAX], path[GPATH_MAX], path_i[GPATH_MAX];
    int i, count;
    size_t len;
    DIR *dir, *dir_i;
    struct dirent *d, *d_i;

    list = NULL;
    len = 0;
    sprintf(path, "%s/etc/symbol", G_gisbase());

    dir = opendir(path);
    if (!dir)
    return NULL;

    count = 0;

    /* loop over etc/symbol */
    while ((d = readdir(dir))) {
    if (d->d_name[0] == '.')
        continue;

    sprintf(path_i, "%s/etc/symbol/%s", G_gisbase(), d->d_name);
    dir_i = opendir(path_i);

    if (!dir_i)
        continue;

    /* loop over each directory in etc/symbols */
    while ((d_i = readdir(dir_i))) {
        if (d_i->d_name[0] == '.')
        continue;

        list = G_realloc(list, (count + 1) * sizeof(char *));

        sprintf(buf, "%s/%s", d->d_name, d_i->d_name);
        list[count++] = G_store(buf);

        len += strlen(d->d_name) + strlen(d_i->d_name) + 2; /* '/' + ',' */
    }

    closedir(dir_i);
    }

    closedir(dir);

    qsort(list, count, sizeof(char *), cmp);

    if (len > 0) {
    ret = G_malloc((len + 1) * sizeof(char)); /* \0 */
    *ret = '\0';
    for (i = 0; i < count; i++) {
        if (i > 0)
        strcat(ret, ",");
        strcat(ret, list[i]);
        G_free(list[i]);
    }
    G_free(list);
    }
    else {
    ret = G_store("");
    }

    return ret;
}
