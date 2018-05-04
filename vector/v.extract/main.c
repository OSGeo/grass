
/****************************************************************
 *
 * MODULE:     v.extract
 * 
 * AUTHOR(S):  R.L.Glenn , Soil Conservation Service, USDA
 *             Updated for 5.7 by Radim Blazek
 *             Updated for 7.0 by Martin Landa <landa.martin gmail.com>
 *               
 * PURPOSE:    Selects vector features from an existing vector map and
 *             creates a new vector map containing only the selected
 *             features.
 *
 * COPYRIGHT:  (C) 2002-2009, 2011 by the GRASS Development Team
 *
 *             This program is free software under the GNU General
 *             Public License (>=v2). Read the file COPYING that
 *             comes with GRASS for details.
 *
 * TODO:       - fix white space problems for file= option
 ****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/gmath.h>
#include <grass/glocale.h>

#include "local_proto.h"

static int *cat_array, cat_count, cat_size;

static int scan_cats(char *, int *, int *);
static void add_cat(int);

int main(int argc, char **argv)
{
    int i, new_cat, type, ncats, *cats, c, native;
    int field, dissolve, x, y, type_only;
    char buffr[1024], text[80];
    char *input, *output;

    FILE *in;

    struct GModule *module;
    struct {
	struct Option *input, *output, *file, *new, *type, *list,
	    *field, *where, *nrand, *d_key;
    } opt;
    struct {
	struct Flag *t, *d, *r;
    } flag;
    
    struct Map_info In, Out;
    struct field_info *Fi;
    
    dbDriver *driver;
    
    struct Cat_index *ci;
    
    int ucat_count, *ucat_array, prnd, seed, nrandom, nfeatures;
    char *dissolve_key;

    Fi = NULL;
    ucat_array = NULL;
    
    G_gisinit(argv[0]);

    /* set up the options and flags for the command line parser */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("extract"));
    G_add_keyword(_("select"));
    G_add_keyword(_("dissolve"));
    G_add_keyword(_("random"));
    module->description =
	_("Selects vector features from an existing vector map and "
	  "creates a new vector map containing only the selected features.");

    flag.d = G_define_flag();
    flag.d->key = 'd';
    flag.d->description = _("Dissolve common boundaries (default is no)");

    flag.t = G_define_flag();
    flag.t->key = 't';
    flag.t->description = _("Do not copy attributes (see also 'new' parameter)");
    flag.t->guisection = _("Attributes");

    flag.r = G_define_flag();
    flag.r->key = 'r';
    flag.r->description = _("Reverse selection");
    flag.r->guisection = _("Selection");

    opt.input = G_define_standard_option(G_OPT_V_INPUT);

    opt.field = G_define_standard_option(G_OPT_V_FIELD_ALL);
    opt.field->answer = "1";
    opt.field->guisection = _("Selection");
    
    opt.type = G_define_standard_option(G_OPT_V_TYPE);
    opt.type->answer = "point,line,boundary,centroid,area,face";
    opt.type->options = "point,line,boundary,centroid,area,face";
    opt.type->label = _("Types to be extracted");
    opt.type->guisection = _("Selection");

    opt.list = G_define_standard_option(G_OPT_V_CATS);
    opt.list->guisection = _("Selection");

    opt.where = G_define_standard_option(G_OPT_DB_WHERE);
    opt.where->guisection = _("Selection");
    
    opt.output = G_define_standard_option(G_OPT_V_OUTPUT);
    
    opt.file = G_define_standard_option(G_OPT_F_INPUT);
    opt.file->key = "file";
    opt.file->required = NO;
    opt.file->label =
	_("Input text file with category numbers/number ranges to be extracted");
    opt.file->description = _("If '-' given reads from standard input");
    opt.file->guisection = _("Selection");

    opt.nrand = G_define_option();
    opt.nrand->key = "random";
    opt.nrand->type = TYPE_INTEGER;
    opt.nrand->required = NO;
    opt.nrand->label =
	_("Number of random categories matching vector objects to extract");
    opt.nrand->description =
	_("Number must be smaller than unique cat count in layer");
    opt.nrand->guisection = _("Selection");

    opt.new = G_define_option();
    opt.new->key = "new";
    opt.new->type = TYPE_INTEGER;
    opt.new->required = NO;
    opt.new->answer = "-1";
    opt.new->label = _("Desired new category value "
		       "(enter -1 to keep original categories)");
    opt.new->description = _("If new >= 0, attributes is not copied");
    opt.new->guisection = _("Attributes");
    
    opt.d_key = G_define_standard_option(G_OPT_DB_COLUMN);
    opt.d_key->key = "dissolve_column";
    opt.d_key->label = _("Name of attribute column for dissolving areas");
    opt.d_key->description = _("Preserves category values");
    opt.d_key->required = NO;

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    
    /* start checking options and flags */
    c = 0;
    if (opt.file->answer != NULL)
	c++;
    if (opt.list->answers != NULL)
	c++;
    if (opt.where->answer != NULL)
	c++;
    if (opt.nrand->answer != NULL)
	c++;
    if (c > 1)
	G_fatal_error(_("Options <%s>, <%s>, <%s> and <%s> options are exclusive. "
			"Please specify only one of them."),
		      opt.list->key, opt.file->key, opt.where->key,
		      opt.nrand->key);
    
    type_only = FALSE;
    if (!opt.list->answers && !opt.file->answer && !opt.where->answer &&
	!opt.nrand->answer) {
	type_only = TRUE;
    }

    input = opt.input->answer;
    output = opt.output->answer;
    Vect_check_input_output_name(input, output,
				 G_FATAL_EXIT);
    
    if (!opt.new->answer)
	new_cat = 0;
    else
	new_cat = atoi(opt.new->answer);

    /* Do initial read of input file */
    Vect_set_open_level(2); /* topology required */

    if (Vect_open_old2(&In, input, "", opt.field->answer) < 0)
	G_fatal_error(_("Unable to open vector map <%s>"), input);
    
    field = Vect_get_field_number(&In, opt.field->answer);
    
    type = Vect_option_to_types(opt.type);
    if (type & GV_AREA) {
	type |= GV_CENTROID;
    }

    dissolve_key = NULL;
    if (flag.d->answer && 
        ((type & GV_AREA) || ((type & GV_CENTROID) && (type & GV_BOUNDARY)))) {
	dissolve = TRUE;
	if (field > 0 && opt.d_key->answer) {
	    int ncols, ret;
	    dbTable *Table;
	    dbColumn *Col;
	    dbString tabname;

	    dissolve_key = opt.d_key->answer;

	    Fi = Vect_get_field(&In, field);
	    if (!Fi) {
		G_fatal_error(_("Database connection not defined for layer <%s>"),
			      opt.field->answer);
	    }

	    G_verbose_message(_("Searching for column <%s> in table <%s>..."),
	                      dissolve_key, Fi->table);

	    driver = db_start_driver_open_database(Fi->driver, Fi->database);
	    if (driver == NULL)
		G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			      Fi->database, Fi->driver);

	    db_init_string(&tabname);
	    db_set_string(&tabname, Fi->table);

	    if (db_describe_table(driver, &tabname, &Table) != DB_OK) {
		G_fatal_error(_("Unable to describe table <%s>"), Fi->table);
	    }

	    ret = 0;

	    ncols = db_get_table_number_of_columns(Table);
	    G_debug(3, "ncol = %d", ncols);

	    for (i = 0; i < ncols; i++) {
		Col = db_get_table_column(Table, i);
		if (G_strcasecmp(db_get_column_name(Col), dissolve_key) == 0) {
		    ret = 1;
		    break;
		}
	    }
	    db_free_table(Table);
	    db_free_string(&tabname);
	    db_close_database_shutdown_driver(driver);
	    
	    if (!ret)
		G_fatal_error(_("Column <%s> does not exist for layer %d"),
		              dissolve_key, field);
	}
    }
    else
	dissolve = FALSE;
	
    /* Read categoy list */
    cat_count = 0;
    if (opt.list->answer != NULL) {
	/* no file of categories to read, process cat list */
	/* first check for valid list */
	for (i = 0; opt.list->answers[i]; i++) {
	    G_debug(2, "catlist item: %s", opt.list->answers[i]);
	    if (!scan_cats(opt.list->answers[i], &x, &y))
		G_fatal_error(_("Category value in '%s' not valid"),
			      opt.list->answers[i]);
	}

	/* valid list, put into cat value array */
	for (i = 0; opt.list->answers[i]; i++) {
	    scan_cats(opt.list->answers[i], &x, &y);
	    while (x <= y)
		add_cat(x++);
	}
    }
    else if (opt.file->answer != NULL) {	/* got a file of category numbers */
	if (strcmp(opt.file->answer, "-") == 0) {
	    in = stdin;
	}
	else {
	    G_verbose_message(_("Process file <%s> for category numbers..."),
			      opt.file->answer);

	    /* open input file */
	    if ((in = fopen(opt.file->answer, "r")) == NULL)
		G_fatal_error(_("Unable to open specified file <%s>"),
			      opt.file->answer);
	}

	while (TRUE) {
	    if (!fgets(buffr, 39, in))
		break;
	    /* eliminate some white space, we accept numbers and dashes only */
	    G_chop(buffr);	
	    sscanf(buffr, "%[-0-9]", text);
	    if (strlen(text) == 0)
		G_warning(_("Ignored text entry: %s"), buffr);

	    scan_cats(text, &x, &y);
	    /* two BUGS here: - fgets stops if white space appears
	       - if white space appears, number is not read correctly */
	    while (x <= y && x >= 0 && y >= 0)
		add_cat(x++);
	}

	if (strcmp(opt.file->answer, "-") != 0)
	    fclose(in);
    }
    else if (opt.where->answer != NULL) {
	Fi = Vect_get_field(&In, field);
	if (!Fi) {
	    G_fatal_error(_("Database connection not defined for layer <%s>"),
			  opt.field->answer);
	}

	G_verbose_message(_("Loading categories from table <%s>..."), Fi->table);

	driver = db_start_driver_open_database(Fi->driver, Fi->database);
	if (driver == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);
	
	ncats = db_select_int(driver, Fi->table, Fi->key, opt.where->answer,
			      &cats);
	if (ncats == -1)
		G_fatal_error(_("Unable select records from table <%s>"),
			      Fi->table);
	G_verbose_message(n_("%d category loaded",
                             "%d categories loaded",
                             ncats), ncats);
	    
	db_close_database(driver);
	db_shutdown_driver(driver);

	for (i = 0; i < ncats; i++)
	    add_cat(cats[i]);
	if (ncats >= 0)
	    G_free(cats);
    }
    else if (opt.nrand->answer != NULL) { /* Generate random category list */
	/* We operate on layer's CAT's and thus valid layer is required */
	if (Vect_cidx_get_field_index(&In, field) < 0)
	    G_fatal_error(_("This map has no categories attached. "
			    "Use v.category to attach categories to "
			    "this vector map."));

	/* Don't do any processing, if user input is wrong */
	nrandom = atoi(opt.nrand->answer);
	if (nrandom < 1)
	    G_fatal_error(_("Please specify random number larger than 0"));

	nfeatures = Vect_cidx_get_type_count(&In, field, type);
	if (nrandom >= nfeatures)
	    G_fatal_error(_("Random category count must be smaller than feature count. "
			    "There are only %d features of type(s): %s"),
			  nfeatures, opt.type->answer);

	/* Let's create an array of uniq CAT values
	   According to Vlib/build.c, cidx should be already sorted by dig_cidx_sort() */
	ci = &(In.plus.cidx[Vect_cidx_get_field_index(&In, field)]);
	ucat_count = 0;
	for (c = 0; c < ci->n_cats; c++) {
	    /* Bitwise AND compares ci feature type with user's requested types */
	    if (ci->cat[c][1] & type) {
		/* Don't do anything if such value already exists */
		if (ucat_count > 0 &&
		    ucat_array[ucat_count - 1] == ci->cat[c][0])
		    continue;
		ucat_array =
		    G_realloc(ucat_array, (ucat_count + 1) * sizeof(int));
		ucat_array[ucat_count] = ci->cat[c][0];
		ucat_count++;
	    }
	}

	if (nrandom >= ucat_count)
	    G_fatal_error(_("Random category count is larger or equal to "
			    "uniq <%s> feature category count %d"),
			  opt.type->answer, ucat_count);
	
	/* Initialise random number generator */
	/* FIXME - allow seed to be specified for repeatability */
	G_math_srand_auto();

	/* Fill cat_array with list of valid random numbers */
	while (cat_count < nrandom) {
	    /* Random number in range from 0 to largest CAT value */
	    prnd = (int) floor(G_math_rand() *
			       (ucat_array[ucat_count - 1] + 1));
	    qsort(cat_array, cat_count, sizeof(int), cmp);
	    /* Check if generated number isn't already in 
	       final list and is in list of existing CATs */
	    if (bsearch(&prnd, cat_array, cat_count, sizeof(int), cmp) == NULL
		&& bsearch(&prnd, ucat_array, ucat_count, sizeof(int),
			   cmp) != NULL)
		add_cat(prnd);
	}
	G_free(ucat_array);
	qsort(cat_array, cat_count, sizeof(int), cmp);
    }

    if (Vect_open_new(&Out, output, Vect_is_3d(&In)) < 0)
	G_fatal_error(_("Unable to create vector map <%s>"), output);

    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    /* Read and write header info */
    Vect_copy_head_data(&In, &Out);
    
    G_message(_("Extracting features..."));
    
    native = Vect_maptype(&Out) == GV_FORMAT_NATIVE;
    if (!flag.t->answer && !native) {
	/* Copy attributes for OGR output */
	Vect_copy_map_dblinks(&In, &Out, TRUE);
    }

    extract_line(cat_count, cat_array, &In, &Out, new_cat, type, 
		 dissolve, dissolve_key, field, type_only, flag.r->answer ? 1 : 0);

    Vect_build(&Out);

    /* Copy tables */
    if (!flag.t->answer && native) {
	copy_tabs(&In, field, new_cat, &Out);
    }
    
    Vect_close(&In);
    
    /* remove duplicate centroids */
    if (dissolve) {
	int line, nlines, ltype, area;

	G_message(_("Removing duplicate centroids..."));
	nlines = Vect_get_num_lines(&Out);
	for (line = 1; line <= nlines; line++) {
	    if (!Vect_line_alive(&Out, line))
		continue;	/* should not happen */

	    ltype = Vect_read_line(&Out, NULL, NULL, line);
	    if (!(ltype & GV_CENTROID)) {
		continue;
	    }
	    area = Vect_get_centroid_area(&Out, line);

	    if (area < 0) {
		Vect_delete_line(&Out, line);
	    }
	}
	Vect_build_partial(&Out, GV_BUILD_NONE);
	Vect_build(&Out);
    }
    
    Vect_close(&Out);
    
    exit(EXIT_SUCCESS);
}

void add_cat(int x)
{
    G_debug(2, "add_cat %d", x);

    if (cat_count >= cat_size) {
	cat_size = (cat_size < 1000) ? 1000 : cat_size * 2;
	cat_array = G_realloc(cat_array, cat_size * sizeof(int));
    }

    cat_array[cat_count++] = x;
}

int scan_cats(char *s, int *x, int *y)
{
    char dummy[2];

    if (strlen(s) == 0) {	/* error */
	*y = *x = -1;
	return 1;
    }

    *dummy = 0;
    if (sscanf(s, "%d-%d%1s", x, y, dummy) == 2)
	return (*dummy == 0 && *x <= *y);

    *dummy = 0;
    if (sscanf(s, "%d%1s", x, dummy) == 1 && *dummy == 0) {
	*y = *x;
	return 1;
    }
    return 0;
}
