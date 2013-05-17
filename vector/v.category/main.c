/* ***************************************************************
 * *
 * * MODULE:       v.category
 * *
 * * AUTHOR(S):    Radim Blazek
 * *               OGR support by Martin Landa <landa.martin gmail.com> (2009)
 * *
 * * PURPOSE:      Category manipulations
 * *
 * * COPYRIGHT:    (C) 2001-2009 by the GRASS Development Team
 * *
 * *               This program is free software under the
 * *               GNU General Public License (>=v2).
 * *               Read the file COPYING that comes with GRASS
 * *               for details.
 * *
 * **************************************************************/
#include <stdlib.h>

#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/vector.h>

#define O_ADD  1
#define O_DEL  2
#define O_REP  3
#define O_PRN  4
#define O_SUM  5
#define O_CHFIELD 6
#define O_TYPE_REP 7		/* report number of features for each type */
#define O_TRANS 8
#define O_LYR 9

#define FRTYPES 9		/* number of field report types */

#define FR_POINT    0
#define FR_LINE     1
#define FR_BOUNDARY 2
#define FR_CENTROID 3
#define FR_AREA     4
#define FR_FACE     5
#define FR_KERNEL   6
#define FR_UNKNOWN  7
#define FR_ALL      8

typedef struct
{
    int field;
    char *table;
    int count[FRTYPES];
    int min[FRTYPES], max[FRTYPES];
} FREPORT;

int main(int argc, char *argv[])
{
    struct Map_info In, Out;
    static struct line_pnts *Points;
    struct line_cats *Cats, *TCats;
    struct field_info *Fi;
    struct cat_list *Clist;
    int i, j, ret, option, otype, type, with_z, step, id;
    int n_areas, centr, new_centr, nmodified;
    int open_level;
    double x, y;
    int cat, ocat, scat, *fields, nfields, field;
    struct GModule *module;
    struct Option *in_opt, *out_opt, *option_opt, *type_opt;
    struct Option *cat_opt, *field_opt, *step_opt, *id_opt;
    struct Flag *shell;
    FREPORT **freps;
    int nfreps, rtype, fld;
    char *desc;

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("category"));
    module->description =
	_("Attaches, deletes or reports vector categories to map geometry.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);
    field_opt->multiple = YES;
    field_opt->guisection = _("Selection");

    type_opt = G_define_standard_option(G_OPT_V3_TYPE);
    type_opt->answer = "point,line,area,face";
    type_opt->guisection = _("Selection");

    id_opt = G_define_standard_option(G_OPT_V_IDS);
    id_opt->label = _("Feature ids (by default all features are processed)");
    id_opt->guisection = _("Selection");

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    out_opt->required = NO;

    option_opt = G_define_option();
    option_opt->key = "option";
    option_opt->type = TYPE_STRING;
    option_opt->required = YES;
    option_opt->multiple = NO;
    option_opt->options = "add,del,chlayer,sum,report,print,layers,transfer";
    option_opt->description = _("Action to be done");
    desc = NULL;
    G_asprintf(&desc,
	       "add;%s;"
	       "del;%s;"
	       "chlayer;%s;"
	       "sum;%s;"
	       "transfer;%s;"
	       "report;%s;"
	       "print;%s;"
	       "layers;%s",
	       _("add a new category"),
	       _("delete category (-1 to delete all categories of given layer)"),
	       _("change layer number (e.g. layer=3,1 changes layer 3 to layer 1)"),
	       _("add the value specified by cat option to the current category value"),
	       _("copy values from one layer to another (e.g. layer=1,2,3 copies values from layer 1 to layer 2 and 3)"),
	       _("print report (statistics), in shell style: layer type count min max"),
	       _("print category values, more cats in the same layer are separated by '/'"),
	       _("print only layer numbers"));
    option_opt->descriptions = desc;
    
    cat_opt = G_define_standard_option(G_OPT_V_CAT);
    cat_opt->answer = "1";

    step_opt = G_define_option();
    step_opt->key = "step";
    step_opt->type = TYPE_INTEGER;
    step_opt->required = NO;
    step_opt->multiple = NO;
    step_opt->answer = "1";
    step_opt->description = _("Category increment");

    shell = G_define_flag();
    shell->key = 'g';
    shell->label = _("Shell script style, currently only for report");
    shell->description = _("Format: layer type count min max");

    G_gisinit(argv[0]);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* read options */
    option = 0;
    switch (option_opt->answer[0]) {
    case ('a'):
	option = O_ADD;
	break;
    case ('d'):
	option = O_DEL;
	break;
    case ('c'):
	option = O_CHFIELD;
	G_warning(_("Database connection and attribute tables for concerned layers are not changed"));
	break;
    case ('s'):
	option = O_SUM;
	break;
    case ('t'):
        option = O_TRANS;
        break;
    case ('r'):
	option = O_REP;
	break;
    case ('p'):
	option = O_PRN;
	break;
    case ('l'):
	option = O_LYR;
	break;
    }

    if (option == O_LYR) {
	/* print vector layer numbers */
	/* open vector on level 2 head only, this is why this option
	 * is processed here, all other options need (?) to fully open 
	 * the input vector */
	Vect_set_open_level(2);
	if (Vect_open_old_head2(&In, in_opt->answer, "", field_opt->answer) < 2) {
	    G_fatal_error(_("Unable to open vector map <%s> at topological level %d"),
			  Vect_get_full_name(&In), 2);
	}
	if (In.format == GV_FORMAT_NATIVE) {
	    nfields = Vect_cidx_get_num_fields(&In);
	    for (i = 0; i < nfields; i++) {
		if ((field = Vect_cidx_get_field_number(&In, i)) > 0)
		    fprintf(stdout, "%d\n", field);
	    }
	}
	else
	    fprintf(stdout, "%s\n", field_opt->answer);

	Vect_close(&In);
	exit(EXIT_SUCCESS);
    }

    cat = atoi(cat_opt->answer);
    step = atoi(step_opt->answer);
    otype = Vect_option_to_types(type_opt);

    if (cat < 0 && option == O_ADD)
	G_fatal_error(_("Invalid category number (must be equal to or greater than 0). "
			"Normally category number starts at 1."));

    /* collect ids */
    if (id_opt->answer) {
	Clist = Vect_new_cat_list();
	Clist->field = atoi(field_opt->answer);
	ret = Vect_str_to_cat_list(id_opt->answer, Clist);
	if (ret > 0) {
	    G_warning(_("%d errors in id option"), ret);
	}
    }
    else {
	Clist = NULL;
    }

    if ((option != O_REP) && (option != O_PRN) && (option != O_LYR)) {
	if (out_opt->answer == NULL)
	    G_fatal_error(_("Output vector wasn't entered"));

	Vect_check_input_output_name(in_opt->answer, out_opt->answer,
				     G_FATAL_EXIT);
    }

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* do we need topology ? */
    if ((option == O_ADD && (otype & GV_AREA)) ||
	(option == O_REP && (otype & GV_AREA)) ||
        (option == O_TRANS) || /* topo for cidx check */
        (option == O_LYR)) /* topo for cidx check */
	open_level = 2;
    else
	open_level = 1;

    /* open input vector */
    if (open_level > 1) {
	Vect_set_open_level(open_level);
	if (Vect_open_old2(&In, in_opt->answer, "", field_opt->answer) < open_level) {
	    G_warning(_("Unable to open vector map <%s> at topological level %d"),
			  Vect_get_full_name(&In), open_level);
	    open_level = 1;
	}
    }
    if (open_level == 1) {
	Vect_set_open_level(open_level);
	if (Vect_open_old2(&In, in_opt->answer, "", field_opt->answer) < open_level) {
	    G_fatal_error(_("Unable to open vector map <%s> at topological level %d"),
			  Vect_get_full_name(&In), open_level);
	}
    }

    /* read fields */
    i = nfields = 0;
    while (field_opt->answers[i++])
	nfields++;
    fields = (int *)G_malloc(nfields * sizeof(int));
    
    i = 0;
    while (field_opt->answers[i]) {
	fields[i] = Vect_get_field_number(&In, field_opt->answers[i]);
	i++;
    }
    if (nfields > 1 && option != O_PRN && option != O_CHFIELD && option != O_TRANS)
	G_fatal_error(_("Too many layers for this operation"));
    
    if (nfields != 2 && option == O_CHFIELD)
	G_fatal_error(_("2 layers must be specified"));

    if (option == O_TRANS && open_level == 1 && nfields < 2) {
	G_fatal_error(_("2 layers must be specified"));
    }

    if (option == O_TRANS && open_level > 1) {
	/* check if field[>0] already exists */
	if (nfields > 1) {
	    for(i = 1; i < nfields; i++) {
		if (Vect_cidx_get_field_index(&In, fields[i]) != -1)
		    G_warning(_("Categories already exist in layer %d"), fields[i]);
	    }
	}
	/* find next free layer number */
	else if (nfields == 1) {
	    int max = -1;
	    
	    for (i = 0; i < Vect_cidx_get_num_fields(&In); i++) {
		if (max < Vect_cidx_get_field_number(&In, i))
		    max = Vect_cidx_get_field_number(&In, i);
	    }
	    max++;

	    nfields++;
	    fields = (int *)G_realloc(fields, nfields * sizeof(int));
	    fields[nfields - 1] = max;
	}
    }

    if (otype & GV_AREA && option == O_TRANS && !(otype & GV_CENTROID))
	otype |= GV_CENTROID;

    /* open output vector if needed */
    if (option == O_ADD || option == O_DEL || option == O_CHFIELD ||
	option == O_SUM || option == O_TRANS) {
	with_z = Vect_is_3d(&In);

	if (0 > Vect_open_new(&Out, out_opt->answer, with_z)) {
	    Vect_close(&In);
	    exit(EXIT_FAILURE);
	}

	Vect_copy_head_data(&In, &Out);
	Vect_hist_copy(&In, &Out);
	Vect_hist_command(&Out);
    }

    id = 0;

    nmodified = 0;

    if (option == O_ADD || option == O_DEL || option == O_CHFIELD ||
	option == O_SUM || option == O_TRANS) {
	G_message(_("Processing features..."));
    }

    switch (option) {
    case (O_ADD):
	/* Lines */
	while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	    id++;
	    if (type & otype && (!Clist ||
				 (Clist &&
				  Vect_cat_in_cat_list(id, Clist) == TRUE))) {
		if ((Vect_cat_get(Cats, fields[0], &ocat)) == 0) {
		    if (ocat < 0) {
			if (Vect_cat_set(Cats, fields[0], cat) > 0) {
			    nmodified++;
			}
			cat += step;
		    }
		}
	    }
	    Vect_write_line(&Out, type, Points, Cats);
	}
	/* Areas */
	if ((otype & GV_AREA) && open_level > 1) {
	    n_areas = Vect_get_num_areas(&In);
	    new_centr = 0;
	    for (i = 1; i <= n_areas; i++) {
		centr = Vect_get_area_centroid(&In, i);
		if (centr > 0)
		    continue;	/* Centroid exists and may be processed as line */
		ret = Vect_get_point_in_area(&In, i, &x, &y);
		if (ret < 0) {
		    G_warning(_("Unable to calculate area centroid"));
		    continue;
		}
		Vect_reset_line(Points);
		Vect_reset_cats(Cats);
		Vect_append_point(Points, x, y, 0.0);
		if (Vect_cat_set(Cats, fields[0], cat) > 0) {
		    nmodified++;
		}
		cat += step;
		Vect_write_line(&Out, GV_CENTROID, Points, Cats);
		new_centr++;
	    }
	    if (new_centr > 0) 
		G_message(_("%d new centroids placed in output map"), new_centr);
	}
	break;

    case (O_TRANS):
	TCats = Vect_new_cats_struct();

	/* Lines */
	while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	    Vect_reset_cats(TCats);
	    id++;
	    if (type & otype && (!Clist ||
				 (Clist &&
				  Vect_cat_in_cat_list(id, Clist) == TRUE))) {
		int n = Cats->n_cats;

		scat = -1;
		for (i = 0; i < n; i++) {
		    if (Cats->field[i] == fields[0]) {
			scat = Cats->cat[i];
			Vect_cat_set(TCats, 1, scat);
		    }
		}
		if (scat > -1) {
		    n = TCats->n_cats;
		    for (i = 0; i < n; i++) {
			scat = TCats->cat[i];
			for (i = 0; i < nfields; i++) {
			    if (Vect_cat_set(Cats, fields[i], scat) > 0) {
				G_debug(4, "Copy cat %i of field %i to into field %i", scat, fields[0], fields[i]);
			    }
			}
		    }
		    Vect_write_line(&Out, type, Points, Cats);
		    nmodified++;
		}
	    }
	}
	Vect_destroy_cats_struct(TCats);
	break;

    case (O_DEL):
	while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	    id++;
	    if (type & otype && (!Clist ||
				 (Clist &&
				  Vect_cat_in_cat_list(id, Clist) == TRUE))) {
		ret = Vect_field_cat_del(Cats, fields[0], cat);
		if (ret > 0) {
		    nmodified++;
		}
	    }
	    Vect_write_line(&Out, type, Points, Cats);
	}
	break;

    case (O_CHFIELD):
	while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	    id++;
	    if (type & otype && (!Clist ||
				 (Clist &&
				  Vect_cat_in_cat_list(id, Clist) == TRUE))) {
		i = 0;
		while (i < Cats->n_cats) {
		    if (Cats->field[i] == fields[0]) {
			int found = -1;
			
			/* check if cat already exists in layer fields[1] */
			for (j = 0; j < Cats->n_cats; j++) {
			    if (Cats->field[j] == fields[1] &&
				Cats->cat[j] == Cats->cat[i]) {
				found = j;
				break;
			    }
			}
			/* does not exist, change layer */
			if (found < 0) {
			    Cats->field[i] = fields[1];
			    i++;
			}
			/* exists already in fields[1], delete from fields[0] */
			else
			    Vect_field_cat_del(Cats, fields[0], Cats->cat[found]);
			nmodified++;
		    }
		}
	    }
	    Vect_write_line(&Out, type, Points, Cats);
	}
	break;

    case (O_SUM):
	while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	    id++;
	    if (type & otype && (!Clist ||
				 (Clist &&
				  Vect_cat_in_cat_list(id, Clist) == TRUE))) {
		for (i = 0; i < Cats->n_cats; i++) {
		    if (Cats->field[i] == fields[0]) {
			Cats->cat[i] += cat;
		    }
		}
		nmodified++;
	    }
	    Vect_write_line(&Out, type, Points, Cats);
	}
	break;

    case (O_REP):
	nfreps = 0;
	freps = NULL;
	while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	    id++;
	    if (Clist && Vect_cat_in_cat_list(id, Clist) == FALSE)
		continue;

	    switch (type) {
	    case (GV_POINT):
		rtype = FR_POINT;
		break;
	    case (GV_LINE):
		rtype = FR_LINE;
		break;
	    case (GV_BOUNDARY):
		rtype = FR_BOUNDARY;
		break;
	    case (GV_CENTROID):
		rtype = FR_CENTROID;
		break;
	    case (GV_FACE):
		rtype = FR_FACE;
		break;
	    case (GV_KERNEL):
		rtype = FR_KERNEL;
		break;
	    default:
		rtype = FR_UNKNOWN;
	    }

	    for (i = 0; i < Cats->n_cats; i++) {
		field = Cats->field[i];
		cat = Cats->cat[i];


		ret = FALSE;
		for (j = 0; j < nfreps; j++) {
		    if (freps[j]->field == field) {
			fld = j;
			ret = TRUE;
			break;
		    }
		}
		if (!ret) {	/* field report doesn't exist */
		    nfreps++;
		    freps =
			(FREPORT **) G_realloc(freps,
					       nfreps * sizeof(FREPORT *));
		    fld = nfreps - 1;
		    freps[fld] = (FREPORT *) G_calloc(1, sizeof(FREPORT));
		    freps[fld]->field = field;
		    for (j = 0; j < FRTYPES; j++) {
			/* cat '0' is valid category number */
			freps[fld]->min[j] = -1;
		    }
		    if ((Fi = Vect_get_field(&In, field)) != NULL) {
			freps[fld]->table = G_store(Fi->table);
		    }
		    else {
			freps[fld]->table = '\0';
		    }
		}

		freps[fld]->count[rtype]++;
		freps[fld]->count[FR_ALL]++;

		if (freps[fld]->min[rtype] == -1 ||
		    freps[fld]->min[rtype] > cat)
		    freps[fld]->min[rtype] = cat;

		if ((freps[fld]->max[rtype] == 0) ||
		    freps[fld]->max[rtype] < cat)
		    freps[fld]->max[rtype] = cat;

		if (freps[fld]->min[FR_ALL] == -1 ||
		    freps[fld]->min[FR_ALL] > cat)
		    freps[fld]->min[FR_ALL] = cat;

		if ((freps[fld]->max[FR_ALL] == 0) ||
		    freps[fld]->max[FR_ALL] < cat)
		    freps[fld]->max[FR_ALL] = cat;
	    }
	}
	/* Areas */
	if ((otype & GV_AREA) && open_level > 1 && !Clist) {
	    n_areas = Vect_get_num_areas(&In);
	    for (i = 1; i <= n_areas; i++) {
		int k;

		centr = Vect_get_area_centroid(&In, i);
		if (centr <= 0)
		    continue;	/* Area without centroid */
		    
		Vect_read_line(&In, NULL, Cats, centr);
		for (j = 0; j < Cats->n_cats; j++) {
		    field = Cats->field[j];
		    cat = Cats->cat[j];


		    ret = FALSE;
		    for (k = 0; k < nfreps; k++) {
			if (freps[k]->field == field) {
			    fld = k;
			    ret = TRUE;
			    break;
			}
		    }
		    if (!ret) {	/* field report doesn't exist */
			nfreps++;
			freps =
			    (FREPORT **) G_realloc(freps,
						   nfreps * sizeof(FREPORT *));
			fld = nfreps - 1;
			freps[fld] = (FREPORT *) G_calloc(1, sizeof(FREPORT));
			freps[fld]->field = field;
			for (j = 0; j < FRTYPES; j++) {
			    /* cat '0' is valid category number */
			    freps[fld]->min[k] = -1;
			}
			if ((Fi = Vect_get_field(&In, field)) != NULL) {
			    freps[fld]->table = G_store(Fi->table);
			}
			else {
			    freps[fld]->table = '\0';
			}
		    }

		    freps[fld]->count[FR_AREA]++;

		    if (freps[fld]->min[FR_AREA] == -1 ||
			freps[fld]->min[FR_AREA] > cat)
			freps[fld]->min[FR_AREA] = cat;

		    if ((freps[fld]->max[FR_AREA] == 0) ||
			freps[fld]->max[FR_AREA] < cat)
			freps[fld]->max[FR_AREA] = cat;
		}
	    }
	}
	for (i = 0; i < nfreps; i++) {
	    if (shell->answer) {
		if (freps[i]->count[FR_POINT] > 0)
		    fprintf(stdout, "%d point %d %d %d\n", freps[i]->field,
			    freps[i]->count[FR_POINT],
			    (freps[i]->min[FR_POINT] < 0 ? 0 : freps[i]->min[FR_POINT]),
			    freps[i]->max[FR_POINT]);

		if (freps[i]->count[FR_LINE] > 0)
		    fprintf(stdout, "%d line %d %d %d\n", freps[i]->field,
			    freps[i]->count[FR_LINE],
			    (freps[i]->min[FR_LINE] < 0 ? 0 : freps[i]->min[FR_LINE]),
			    freps[i]->max[FR_LINE]);

		if (freps[i]->count[FR_BOUNDARY] > 0)
		    fprintf(stdout, "%d boundary %d %d %d\n", freps[i]->field,
			    freps[i]->count[FR_BOUNDARY],
			    (freps[i]->min[FR_BOUNDARY] < 0 ? 0 : freps[i]->min[FR_BOUNDARY]),
			    freps[i]->max[FR_BOUNDARY]);

		if (freps[i]->count[FR_CENTROID] > 0)
		    fprintf(stdout, "%d centroid %d %d %d\n", freps[i]->field,
			    freps[i]->count[FR_CENTROID],
			    (freps[i]->min[FR_BOUNDARY] < 0 ? 0 : freps[i]->min[FR_BOUNDARY]),
			    freps[i]->max[FR_CENTROID]);

		if (freps[i]->count[FR_AREA] > 0)
		    fprintf(stdout, "%d area %d %d %d\n", freps[i]->field,
			    freps[i]->count[FR_AREA],
			    (freps[i]->min[FR_AREA] < 0 ? 0 : freps[i]->min[FR_AREA]),
			    freps[i]->max[FR_AREA]);

		if (freps[i]->count[FR_FACE] > 0)
		    fprintf(stdout, "%d face %d %d %d\n", freps[i]->field,
			    freps[i]->count[FR_FACE],
			    (freps[i]->min[FR_FACE] < 0 ? 0 : freps[i]->min[FR_FACE]),
			    freps[i]->max[FR_FACE]);

		if (freps[i]->count[FR_KERNEL] > 0)
		    fprintf(stdout, "%d kernel %d %d %d\n", freps[i]->field,
			    freps[i]->count[FR_KERNEL],
			    (freps[i]->min[FR_KERNEL] < 0 ? 0 : freps[i]->min[FR_KERNEL]),
			    freps[i]->max[FR_KERNEL]);

		if (freps[i]->count[FR_ALL] > 0)
		    fprintf(stdout, "%d all %d %d %d\n", freps[i]->field,
			    freps[i]->count[FR_ALL],
			    (freps[i]->min[FR_ALL] < 0 ? 0 : freps[i]->min[FR_ALL]),
			    freps[i]->max[FR_ALL]);
	    }
	    else {
		if (freps[i]->table != '\0') {
		    fprintf(stdout, "%s: %d/%s\n", _("Layer/table"),
			    freps[i]->field, freps[i]->table);
		}
		else {
		    fprintf(stdout, "%s: %d\n", _("Layer"), freps[i]->field);
		}
		fprintf(stdout, _("type       count        min        max\n"));
		fprintf(stdout, "%s    %7d %10d %10d\n", _("point"),
			freps[i]->count[FR_POINT],
			(freps[i]->min[FR_POINT] < 0) ? 0 : freps[i]->min[FR_POINT],
			freps[i]->max[FR_POINT]);
		fprintf(stdout, "%s     %7d %10d %10d\n", _("line"),
			freps[i]->count[FR_LINE],
			(freps[i]->min[FR_LINE] < 0) ? 0 : freps[i]->min[FR_LINE],
			freps[i]->max[FR_LINE]);
		fprintf(stdout, "%s %7d %10d %10d\n", _("boundary"),
			freps[i]->count[FR_BOUNDARY],
			(freps[i]->min[FR_BOUNDARY] < 0) ? 0 : freps[i]->min[FR_BOUNDARY],
			freps[i]->max[FR_BOUNDARY]);
		fprintf(stdout, "%s %7d %10d %10d\n", _("centroid"),
			freps[i]->count[FR_CENTROID],
			(freps[i]->min[FR_CENTROID] < 0) ? 0 : freps[i]->min[FR_CENTROID],
			freps[i]->max[FR_CENTROID]);
		fprintf(stdout, "%s     %7d %10d %10d\n", _("area"),
			freps[i]->count[FR_AREA],
			(freps[i]->min[FR_AREA] < 0) ? 0 : freps[i]->min[FR_AREA],
			freps[i]->max[FR_AREA]);
		fprintf(stdout, "%s     %7d %10d %10d\n", _("face"),
			freps[i]->count[FR_FACE],
			(freps[i]->min[FR_FACE] < 0) ? 0 : freps[i]->min[FR_FACE],
			freps[i]->max[FR_FACE]);
		fprintf(stdout, "%s   %7d %10d %10d\n", _("kernel"),
			freps[i]->count[FR_KERNEL],
			(freps[i]->min[FR_KERNEL] < 0) ? 0 : freps[i]->min[FR_KERNEL],
			freps[i]->max[FR_KERNEL]);
		fprintf(stdout, "%s      %7d %10d %10d\n", _("all"),
			freps[i]->count[FR_ALL],
			(freps[i]->min[FR_ALL] < 0) ? 0 : freps[i]->min[FR_ALL],
			freps[i]->max[FR_ALL]);
	    }
	}
	break;

    case (O_PRN):
	while ((type = Vect_read_next_line(&In, Points, Cats)) > 0) {
	    id++;
	    int has = 0;

	    if (!(type & otype))
		continue;

	    if (Clist && Vect_cat_in_cat_list(id, Clist) == FALSE)
		continue;

	    /* Check if the line has at least one cat */
	    for (i = 0; i < nfields; i++) {
		for (j = 0; j < Cats->n_cats; j++) {
		    if (Cats->field[j] == fields[i]) {
			has = 1;
			break;
		    }
		}
	    }

	    if (!has)
		continue;

	    for (i = 0; i < nfields; i++) {
		int first = 1;

		if (i > 0)
		    fprintf(stdout, "|");
		for (j = 0; j < Cats->n_cats; j++) {
		    if (Cats->field[j] == fields[i]) {
			if (!first)
			    fprintf(stdout, "/");
			fprintf(stdout, "%d", Cats->cat[j]);
			first = 0;
		    }
		}
	    }
	    fprintf(stdout, "\n");
	}
	break;
    }

    if (option == O_ADD || option == O_DEL || option == O_CHFIELD ||
	option == O_SUM || option == O_TRANS) {
	G_message(_("Copying attribute table(s)..."));
        if (Vect_copy_tables(&In, &Out, 0))
            G_warning(_("Failed to copy attribute table to output map"));
	Vect_build(&Out);
	Vect_close(&Out);

	if (option == O_TRANS && nmodified > 0)
	    for(i = 1; i < nfields; i++)
		G_important_message(_("Categories copied from layer %d to layer %d"),
				fields[0], fields[i]);
	G_done_msg(_("%d features modified."), nmodified);
    }
    Vect_close(&In);

    exit(EXIT_SUCCESS);
}
