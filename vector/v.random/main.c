
/****************************************************************
 *
 * MODULE:       v.random (based on s.rand)
 *
 * AUTHOR(S):    James Darrell McCauley darrell@mccauley-usa.com
 * 	         http://mccauley-usa.com/
 *               OGR support by Martin Landa <landa.martin gmail.com>
 *               Area support by Markus Metz
 *
 * PURPOSE:      Randomly generate a 2D/3D GRASS vector points map.
 *
 * Modification History:
 *
 * s.rand v 0.5B <25 Jun 1995> Copyright (c) 1993-1995. James Darrell McCauley
 * <?? ??? 1993> - began coding and released test version (jdm)
 * <10 Jan 1994> - changed RAND_MAX for rand(), since it is different for
 *                 SunOS 4.1.x and Solaris 2.3. stdlib.h in the latter defines
 *                 RAND_MAX, but it doesn't in the former. v0.2B (jdm)
 * <02 Jan 1995> - clean Gmakefile, man page. added html v0.3B (jdm)
 * <25 Feb 1995> - cleaned 'gcc -Wall' warnings (jdm)
 * <25 Jun 1995> - new site API (jdm)
 * <13 Sep 2000> - released under GPL
 *
 * COPYRIGHT:    (C) 2003-2018 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
**************************************************************/

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

/* for qsort */

typedef struct {
    int i;
    double size;
    struct bound_box box;
} BOX_SIZE;

static int sort_by_size(const void *a, const void *b)
{
    BOX_SIZE *as = (BOX_SIZE *)a;
    BOX_SIZE *bs = (BOX_SIZE *)b;
    
    if (as->size < bs->size)
	return -1;

    return (as->size > bs->size);
}

int main(int argc, char *argv[])
{
    char *output, buf[DB_SQL_MAX];
    double (*rng)(void) = G_drand48;
    double zmin, zmax;
    int seed;
    unsigned long i, n, total_n;
    int j, k, type, usefloat;
    int area, nareas, field, cat_area;
    int cat, icol, ncols;
    struct boxlist *List = NULL;
    BOX_SIZE *size_list = NULL;
    int alloc_size_list = 0;
    struct Map_info In, Out;
    struct line_pnts *Points;
    struct line_cats *Cats;
    struct cat_list *cat_list;
    struct bound_box box;
    struct Cell_head window;
    struct GModule *module;
    struct
    {
	struct Option *input, *field, *cats, *where, *output, *nsites,
		      *zmin, *zmax, *zcol, *ztype, *seed;
    } parm;
    struct
    {
	struct Flag *z, *notopo, *a;
    } flag;
    int notable;
    struct field_info *Fi, *Fi_input;
    dbDriver *driver, *driver_input;
    dbString sql;
    dbTable *table;
    dbCatValI *cats_array = NULL;
    
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("sampling"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("random"));
    G_add_keyword(_("point pattern"));
    G_add_keyword(_("stratified random sampling"));
    G_add_keyword(_("level1"));

    module->description = _("Generates random 2D/3D vector points.");

    parm.output = G_define_standard_option(G_OPT_V_OUTPUT);

    parm.nsites = G_define_option();
    parm.nsites->key = "npoints";
    parm.nsites->type = TYPE_INTEGER;
    parm.nsites->required = YES;
    parm.nsites->description = _("Number of points to be created");

    parm.input = G_define_standard_option(G_OPT_V_INPUT);
    parm.input->key = "restrict";
    parm.input->required = NO;
    parm.input->description = _("Restrict points to areas in input vector");
    parm.input->guisection = _("Selection");
    parm.input->guisection = _("Restrict");
 
    parm.field = G_define_standard_option(G_OPT_V_FIELD_ALL);
    parm.field->guisection = _("Selection");

    parm.cats = G_define_standard_option(G_OPT_V_CATS);
    parm.cats->guisection = _("Selection");
    
    parm.where = G_define_standard_option(G_OPT_DB_WHERE);
    parm.where->guisection = _("Selection");

    parm.zmin = G_define_option();
    parm.zmin->key = "zmin";
    parm.zmin->type = TYPE_DOUBLE;
    parm.zmin->required = NO;
    parm.zmin->description =
	_("Minimum z height (needs -z flag or column name)");
    parm.zmin->answer = "0.0";
    parm.zmin->guisection = _("3D output");

    parm.zmax = G_define_option();
    parm.zmax->key = "zmax";
    parm.zmax->type = TYPE_DOUBLE;
    parm.zmax->required = NO;
    parm.zmax->description =
	_("Maximum z height (needs -z flag or column name)");
    parm.zmax->answer = "0.0";
    parm.zmax->guisection = _("3D output");

    parm.seed = G_define_option();
    parm.seed->key = "seed";
    parm.seed->type = TYPE_INTEGER;
    parm.seed->required = NO;
    parm.seed->description =
	_("The seed to initialize the random generator. If not set the process ID is used");

    parm.zcol = G_define_standard_option(G_OPT_DB_COLUMN);
    parm.zcol->label = _("Name of column for z values");
    parm.zcol->description =
	_("Writes z values to column");
    parm.zcol->guisection = _("3D output");

    parm.ztype = G_define_option();
    parm.ztype->key = "column_type";
    parm.ztype->type = TYPE_STRING;
    parm.ztype->required = NO;
    parm.ztype->multiple = NO;
    parm.ztype->description = _("Type of column for z values");
    parm.ztype->options = "integer,double precision";
    parm.ztype->answer = "double precision";
    parm.ztype->guisection = _("3D output");

    flag.z = G_define_flag();
    flag.z->key = 'z';
    flag.z->description = _("Create 3D output");
    flag.z->guisection = _("3D output");

    flag.a = G_define_flag();
    flag.a->key = 'a';
    flag.a->description = _("Generate n points for each individual area (requires restrict parameter)");
    flag.a->guisection = _("Restrict");
    
    flag.notopo = G_define_standard_flag(G_FLG_V_TOPO);

    G_option_requires(flag.a, parm.input, NULL);
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    output = parm.output->answer;
    n = strtoul(parm.nsites->answer, NULL, 10);

    seed = 0;
    if(parm.seed->answer)
        seed = atoi(parm.seed->answer);

    if (n <= 0) {
	G_fatal_error(_("Number of points must be > 0 (%ld given)"), n);
    }

    nareas = 0;
    cat_list = NULL;
    field = -1;
    if (parm.input->answer) {
	Vect_set_open_level(2); /* topology required */
	if (2 > Vect_open_old2(&In, parm.input->answer, "", parm.field->answer))
	    G_fatal_error(_("Unable to open vector map <%s>"),
			  parm.input->answer);

	if (parm.field->answer)
	    field = Vect_get_field_number(&In, parm.field->answer);

	if ((parm.cats->answer || parm.where->answer) && field == -1) {
	    G_warning(_("Invalid layer number (%d). Parameter '%s' or '%s' specified, assuming layer '1'."),
		      field, parm.cats->key, parm.where->key);
	    field = 1;
	}
	if (field > 0)
	    cat_list = Vect_cats_set_constraint(&In, field, parm.where->answer,
						parm.cats->answer);
	nareas = Vect_get_num_areas(&In);
	if (nareas == 0) {
	    Vect_close(&In);
	    G_fatal_error(_("No areas in vector map <%s>"), parm.input->answer);
	}
    }

    /* create new vector map */
    if (-1 == Vect_open_new(&Out, output, flag.z->answer ? WITH_Z : WITHOUT_Z))
        G_fatal_error(_("Unable to create vector map <%s>"), output);
    Vect_set_error_handler_io(NULL, &Out);

    /* Do we need to write random values into attribute table? */
    usefloat = -1;
    notable = !(parm.zcol->answer || (parm.input -> answer && field > 0));
    driver = NULL;
    driver_input = NULL;
    Fi = NULL;
    Fi_input = NULL;
    ncols = 0;
    if (!notable) {
	Fi = Vect_default_field_info(&Out, 1, NULL, GV_1TABLE);
	driver =
	    db_start_driver_open_database(Fi->driver,
					  Vect_subst_var(Fi->database, &Out));
	if (driver == NULL) {
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Vect_subst_var(Fi->database, &Out), Fi->driver);
	}
        db_set_error_handler_driver(driver);
        
	db_begin_transaction(driver);

	db_init_string(&sql);

        /* Create table */
        sprintf(buf, "create table %s (%s integer", Fi->table, GV_KEY_COLUMN);
        db_set_string(&sql, buf);
        if (parm.zcol->answer) {
            sprintf(buf, ", %s %s", parm.zcol->answer, parm.ztype->answer);
            db_append_string(&sql, buf);
        }
        if (parm.input->answer && field > 0) {
            dbString table_name;
            dbColumn *col;
            
            Fi_input = Vect_get_field2(&In, parm.field->answer);
            if (Fi_input == NULL)
                G_fatal_error(_("Database connection not defined for layer <%s>"),
                              parm.field->answer);
            driver_input = db_start_driver_open_database(
                Fi_input->driver,
                Vect_subst_var(Fi_input->database, &In));
            if (driver_input == NULL) {
                G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                              Vect_subst_var(Fi_input->database, &In), Fi_input->driver);
            }
            db_set_error_handler_driver(driver_input);
            
            db_init_string(&table_name);
            db_set_string(&table_name, Fi_input->table);
            if (db_describe_table(driver_input, &table_name, &table) != DB_OK)
                G_fatal_error(_("Unable to describe table <%s>"),
                              Fi_input->table);

            ncols = db_get_table_number_of_columns(table);
            for (icol = 0; icol < ncols; icol++) {
                col = db_get_table_column(table, icol);
                sprintf(buf, ",%s_%s %s", parm.input->answer,
                        db_get_column_name(col),
                        db_sqltype_name(db_get_column_sqltype(col)));
                db_append_string(&sql, buf);
            }
        }
        db_append_string(&sql, ")");
        
	if (db_execute_immediate(driver, &sql) != DB_OK) {
	    G_fatal_error(_("Unable to create table: %s"),
			  db_get_string(&sql));
	}

	/* Create index */
	if (db_create_index2(driver, Fi->table, Fi->key) != DB_OK)
	    G_warning(_("Unable to create index"));

	/* Grant */
	if (db_grant_on_table
	    (driver, Fi->table, DB_PRIV_SELECT,
	     DB_GROUP | DB_PUBLIC) != DB_OK) {
	    G_fatal_error(_("Unable to grant privileges on table <%s>"),
			  Fi->table);
	}

	/* OK. Let's check what type of column user has created */
	db_set_string(&sql, Fi->table);
	if (db_describe_table(driver, &sql, &table) != DB_OK) {
	    G_fatal_error(_("Unable to describe table <%s>"), Fi->table);
	}

        if (parm.zcol->answer) {
            type = db_get_column_sqltype(db_get_table_column(table, 1));
            if (type == DB_SQL_TYPE_SMALLINT || type == DB_SQL_TYPE_INTEGER)
                usefloat = 0;
            if (type == DB_SQL_TYPE_REAL || type == DB_SQL_TYPE_DOUBLE_PRECISION)
                usefloat = 1;
            if (usefloat < 0) {
                G_fatal_error(_("You have created unsupported column type. This module supports only INTEGER"
                                " and DOUBLE PRECISION column types."));
            }
        }

        Vect_map_add_dblink(&Out, 1, NULL, Fi->table, GV_KEY_COLUMN, Fi->database,
			    Fi->driver);
    }

    Vect_hist_command(&Out);

    /* Init the random seed */
    if(parm.seed->answer)
	G_srand48(seed);
    else
	G_srand48_auto();

    G_get_window(&window);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    if (nareas > 0) {
	int first = 1, count;
	struct bound_box abox, bbox;

	box.W = window.west;
	box.E = window.east;
	box.S = window.south;
	box.N = window.north;
	box.B = -PORT_DOUBLE_MAX;
	box.T = PORT_DOUBLE_MAX;

	count = 0;

	for (i = 1; i <= nareas; i++) {
	    
	    if (!Vect_get_area_centroid(&In, i))
		continue;

	    if (field > 0) {
		if (Vect_get_area_cats(&In, i, Cats))
		    continue;

		if (!Vect_cats_in_constraint(Cats, field, cat_list))
		    continue;
	    }

	    Vect_get_area_box(&In, i, &abox);
	    if (!Vect_box_overlap(&abox, &box))
		continue;

	    if (first) {
		Vect_box_copy(&bbox, &abox);
		first = 0;
	    }
	    else
		Vect_box_extend(&bbox, &abox);
	    count++;
	}
	if (count == 0) {
	    Vect_close(&In);
	    Vect_close(&Out);
	    Vect_delete(output);
	    G_fatal_error(_("Selected areas in input vector <%s> do not overlap with the current region"),
			  parm.input->answer);
	}
	Vect_box_copy(&box, &bbox);

	/* does the vector overlap with the current region ? */
	if (box.W >= window.east || box.E <= window.west ||
	    box.S >= window.north || box.N <= window.south) {

	    Vect_close(&In);
	    Vect_close(&Out);
	    Vect_delete(output);
	    G_fatal_error(_("Input vector <%s> does not overlap with the current region"),
	                  parm.input->answer);
	}

	/* try to reduce the current region */
	if (window.east > box.E)
	    window.east = box.E;
	if (window.west < box.W)
	    window.west = box.W;
	if (window.north > box.N)
	    window.north = box.N;
	if (window.south < box.S)
	    window.south = box.S;

	List = Vect_new_boxlist(1);
	alloc_size_list = 10;
	size_list = G_malloc(alloc_size_list * sizeof(BOX_SIZE));
    }

    zmin = zmax = 0;
    if (flag.z->answer || parm.zcol->answer) {
	zmax = atof(parm.zmax->answer);
	zmin = atof(parm.zmin->answer);
    }

    G_message(_("Generating points..."));
    if (flag.a->answer && nareas > 0) {
	struct bound_box abox, bbox;
	cat = 1;

	/* n points for each area */
	nareas = Vect_get_num_areas(&In);
        
        /* init cat/cat_area array */
        total_n = n * nareas;
        cats_array = G_malloc(total_n * sizeof(dbCatValI));

	G_percent(0, nareas, 1);
	for (area = 1; area <= nareas; area++) {

	    G_percent(area, nareas, 1);

	    if (!Vect_get_area_centroid(&In, area))
		continue;

	    if (field > 0) {
		if (Vect_get_area_cats(&In, area, Cats))
		    continue;

		if (!Vect_cats_in_constraint(Cats, field, cat_list)) {
		    continue;
		}
	    }

	    box.W = window.west;
	    box.E = window.east;
	    box.S = window.south;
	    box.N = window.north;
	    box.B = -PORT_DOUBLE_MAX;
	    box.T = PORT_DOUBLE_MAX;
	    
	    Vect_get_area_box(&In, area, &abox);
	    if (!Vect_box_overlap(&box, &abox))
		continue;
		
	    bbox = abox;
	    if (bbox.W < box.W)
		bbox.W = box.W;
	    if (bbox.E > box.E)
		bbox.E = box.E;
	    if (bbox.S < box.S)
		bbox.S = box.S;
	    if (bbox.N > box.N)
		bbox.N = box.N;

	    cat_area = -1;
            if (field > 0) {
		if (cat_list) {
		    for (i = 0; i < Cats->n_cats; i++) {
			if (Cats->field[i] == field &&
			    Vect_cat_in_cat_list(Cats->cat[i], cat_list)) {
			    cat_area = Cats->cat[i];
			    break;
			}
		    }
		}
		else {
		    Vect_cat_get(Cats, field, &cat_area);
		}
		if (cat_area < 0)
		    continue;
	    }

	    for (i = 0; i < n; ++i) {
		double x, y, z;
		int outside = 1;
		int ret;

                if (field > 0) {
                    cats_array[cat-1].cat = cat;
                    cats_array[cat-1].val = cat_area;
                }

		Vect_reset_line(Points);
		Vect_reset_cats(Cats);

		while (outside) {
		    x = rng() * (bbox.W - bbox.E) + bbox.E;
		    y = rng() * (bbox.N - bbox.S) + bbox.S;
		    z = rng() * (zmax - zmin) + zmin;

		    ret = Vect_point_in_area(x, y, &In, area, &abox);

		    G_debug(3, "    area = %d Vect_point_in_area() = %d", area, ret);

		    if (ret >= 1) {
			outside = 0;
		    }
		}

		if (flag.z->answer)
		    Vect_append_point(Points, x, y, z);
		else
		    Vect_append_point(Points, x, y, 0.0);

		if (!notable) {
                    sprintf(buf, "insert into %s (%s", Fi->table, Fi->key);
                    db_set_string(&sql, buf);
                    if (parm.zcol->answer) {
                        sprintf(buf, ", %s", parm.zcol->answer);
                        db_append_string(&sql, buf);
                    }
                    sprintf(buf, ") values ( %d", cat);
                    db_append_string(&sql, buf);
                    if (parm.zcol->answer) {
                        /* Round random value if column is integer type */
                        if (usefloat)
                            sprintf(buf, ", %f", z);
                        else
                            sprintf(buf, ", %.0f", z);
                        db_append_string(&sql, buf);
                    }
                    db_append_string(&sql, ")");
                    
                    G_debug(3, "%s", db_get_string(&sql));
                    if (db_execute_immediate(driver, &sql) != DB_OK) {
                        G_fatal_error(_("Unable to insert new row: %s"),
                                      db_get_string(&sql));
                    }
		}

		Vect_cat_set(Cats, 1, cat++);
		Vect_write_line(&Out, GV_POINT, Points, Cats);
	    }
	}
    }
    else {
        total_n = n;
        if (parm.input->answer && field > 0)
            cats_array = G_malloc(n * sizeof(dbCatValI));

	/* n points in total */
	for (i = 0; i < n; ++i) {
	    double x, y, z;

	    G_percent(i, n, 4);

	    Vect_reset_line(Points);
	    Vect_reset_cats(Cats);

	    x = rng() * (window.west - window.east) + window.east;
	    y = rng() * (window.north - window.south) + window.south;
	    z = rng() * (zmax - zmin) + zmin;
	    
	    if (nareas) {
		int outside = 1;

		do {
		    /* select areas by box */
		    box.E = x;
		    box.W = x;
		    box.N = y;
		    box.S = y;
		    box.T = PORT_DOUBLE_MAX;
		    box.B = -PORT_DOUBLE_MAX;
		    Vect_select_areas_by_box(&In, &box, List);
		    G_debug(3, "  %d areas selected by box", List->n_values);

		    /* sort areas by size, the smallest is likely to be the nearest */
		    if (alloc_size_list < List->n_values) {
			alloc_size_list = List->n_values;
			size_list = G_realloc(size_list, alloc_size_list * sizeof(BOX_SIZE));
		    }

		    k = 0;
		    for (j = 0; j < List->n_values; j++) {
			area = List->id[j];

			if (!Vect_get_area_centroid(&In, area))
			    continue;

			if (field > 0) {
			    if (Vect_get_area_cats(&In, area, Cats))
				continue;

			    if (!Vect_cats_in_constraint(Cats, field, cat_list)) {
				continue;
			    }
			}
                        
			List->id[k] = List->id[j];
			List->box[k] = List->box[j];
			size_list[k].i = List->id[k];
			box = List->box[k];
			size_list[k].box = List->box[k];
			size_list[k].size = (box.N - box.S) * (box.E - box.W);
			k++;
		    }
		    List->n_values = k;
		    
		    if (List->n_values == 2) {
			/* simple swap */
			if (size_list[1].size < size_list[0].size) {
			    size_list[0].i = List->id[1];
			    size_list[1].i = List->id[0];
			    size_list[0].box = List->box[1];
			    size_list[1].box = List->box[0];
			}
		    }
		    else if (List->n_values > 2)
			qsort(size_list, List->n_values, sizeof(BOX_SIZE), sort_by_size);

		    for (j = 0; j < List->n_values; j++) {
			int ret;

			area = size_list[j].i;
			ret = Vect_point_in_area(x, y, &In, area, &size_list[j].box);

			G_debug(3, "    area = %d Vect_point_in_area() = %d", area, ret);

			if (ret >= 1) {
                            if (field > 0) {
                                /* read categories for matched area */
                                Vect_get_area_cats(&In, area, Cats);
                            }
			    outside = 0;
			    break;
			}
		    }
		    if (outside) {
			x = rng() * (window.west - window.east) + window.east;
			y = rng() * (window.north - window.south) + window.south;
			z = rng() * (zmax - zmin) + zmin;
		    }
		} while (outside);
	    }

	    if (flag.z->answer)
		Vect_append_point(Points, x, y, z);
	    else
		Vect_append_point(Points, x, y, 0.0);

            cat = i + 1;
            
            if (!notable) {
                if (parm.input->answer && field > 0) {
                    Vect_cat_get(Cats, field, &cat_area);

                    cats_array[i].cat = cat;
                    cats_array[i].val = cat_area;
                    
                }
                
                sprintf(buf, "insert into %s (%s", Fi->table, Fi->key);
                db_set_string(&sql, buf);
                if (parm.zcol->answer) {
                    sprintf(buf, ", %s", parm.zcol->answer);
                    db_append_string(&sql, buf);
                }
                sprintf(buf, ") values ( %ld", i + 1);
                db_append_string(&sql, buf);
                if (parm.zcol->answer) {
                    /* Round random value if column is integer type */
                    if (usefloat)
                        sprintf(buf, ", %f", z);
                    else
                        sprintf(buf, ", %.0f", z);
                    db_append_string(&sql, buf);
                }
                db_append_string(&sql, ")");
                
                G_debug(3, "%s", db_get_string(&sql));
                if (db_execute_immediate(driver, &sql) != DB_OK) {
                    G_fatal_error(_("Unable to insert new row: %s"),
                                  db_get_string(&sql));
                }
            }

	    Vect_cat_set(Cats, 1, cat);
	    Vect_write_line(&Out, GV_POINT, Points, Cats);
	}
	G_percent(1, 1, 1);
    }
    
    if (parm.input->answer && field > 0) {
        int more, ctype;
        const char *column_name;
        dbColumn *column;
        dbValue *value;
        dbString value_str, update_str;
        dbCursor cursor;
            
        db_init_string(&value_str);
        db_init_string(&update_str);
        sprintf(buf, "select * from %s", Fi_input->table);
        db_set_string(&sql, buf);
        if (db_open_select_cursor(driver_input, &sql,
                                  &cursor, DB_SEQUENTIAL) != DB_OK)
            G_fatal_error(_("Unable to open select cursor"));
        table = db_get_cursor_table(&cursor);

        while (TRUE) {
            if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
                G_fatal_error(_("Unable to fetch data from table <%s>"),
                              Fi_input->table);
            
            if (!more) {
                break;
            }

            sprintf(buf, "update %s set ", Fi->table);
            db_set_string(&update_str, buf);
            for (icol = 0; icol < ncols; icol++) {
                column = db_get_table_column(table, icol);
                column_name = db_get_column_name(column);
                value = db_get_column_value(column);
                if (strcmp(column_name, Fi_input->key) == 0)
                    cat_area = db_get_value_int(value);
               
                if (icol > 0)
                    db_append_string(&update_str, ", ");
                sprintf(buf, "%s_%s = ", parm.input->answer, column_name);
                db_append_string(&update_str, buf);
                ctype = db_sqltype_to_Ctype(db_get_column_sqltype(column));
                db_convert_value_to_string(value, ctype, &value_str);
                if (ctype == DB_C_TYPE_INT || ctype == DB_C_TYPE_DOUBLE)
                    sprintf(buf, "%s", db_get_string(&value_str));
                else
                    sprintf(buf, "'%s'", db_get_string(&value_str));
                db_append_string(&update_str, buf);
            }
            for (i = 0; i < total_n; i++) {
                if (cat_area == cats_array[i].val) {
                    db_copy_string(&sql, &update_str);
                    sprintf(buf, " where %s = %d", Fi->key, cats_array[i].cat);
                    db_append_string(&sql, buf);
                    G_debug(3, "%s", db_get_string(&sql));
                    if (db_execute_immediate(driver, &sql) != DB_OK) {
                        G_fatal_error(_("Unable to update row: %s"),
                                      db_get_string(&sql));
                    }
                }
            }
        }
        G_free(cats_array);
    }

    if (!notable) {
        db_commit_transaction(driver);
        db_close_database_shutdown_driver(driver);
    }

    if (!flag.notopo->answer) {
	Vect_build(&Out);
    }
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
