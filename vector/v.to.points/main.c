
/***************************************************************
 *
 * MODULE:       v.to.points
 * 
 * AUTHOR(S):    Radim Blazek
 *               OGR support by Martin Landa <landa.martin gmail.com>
 *               
 * PURPOSE:      Create points along lines 
 *               
 * COPYRIGHT:    (C) 2002-2010, 2013 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#include "local_proto.h"

int main(int argc, char **argv)
{
    int field, type, vertex_type;
    double dmax;
    char buf[DB_SQL_MAX];

    struct {
        struct Option *input, *output, *type, *dmax, *lfield, *use;
    } opt;
    struct {
        struct Flag *table, *inter;
    } flag;
    struct GModule *module;
    struct Map_info In, Out;
    struct line_cats *LCats;
    struct line_pnts *LPoints;

    dbDriver *driver;
    struct field_info *Fi;

    dbString stmt;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    G_add_keyword("3D");
    module->description =
	_("Creates points along input lines in new vector map with 2 layers.");

    opt.input = G_define_standard_option(G_OPT_V_INPUT);

    opt.lfield = G_define_standard_option(G_OPT_V_FIELD);
    opt.lfield->key = "llayer";
    opt.lfield->answer = "1";
    opt.lfield->label = "Line layer number or name";
    opt.lfield->guisection = _("Selection");

    opt.type = G_define_standard_option(G_OPT_V3_TYPE);
    opt.type->answer = "point,line,boundary,centroid,face";
    opt.type->guisection = _("Selection");

    opt.output = G_define_standard_option(G_OPT_V_OUTPUT);

    opt.use = G_define_option();
    opt.use->key = "use";
    opt.use->type = TYPE_STRING;
    opt.use->required = NO;
    opt.use->description = _("Use line nodes or vertices only");
    opt.use->options = "node,vertex";

    opt.dmax = G_define_option();
    opt.dmax->key = "dmax";
    opt.dmax->type = TYPE_DOUBLE;
    opt.dmax->required = NO;
    opt.dmax->answer = "100";
    opt.dmax->description = _("Maximum distance between points in map units");

    flag.inter = G_define_flag();
    flag.inter->key = 'i';
    flag.inter->description = _("Interpolate points between line vertices (only for use=vertex)");
    

    flag.table = G_define_standard_flag(G_FLG_V_TABLE);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    LCats = Vect_new_cats_struct();
    LPoints = Vect_new_line_struct();
    db_init_string(&stmt);

    type = Vect_option_to_types(opt.type);
    dmax = atof(opt.dmax->answer);

    vertex_type = 0;
    if (opt.use->answer) {
        if (opt.use->answer[0] == 'n')
            vertex_type = GV_NODE;
        else
            vertex_type = GV_VERTEX;
    }
    
    Vect_check_input_output_name(opt.input->answer, opt.output->answer,
				 G_FATAL_EXIT);

    /* Open input lines */
    Vect_set_open_level(2);
    Vect_open_old2(&In, opt.input->answer, "", opt.lfield->answer);
    Vect_set_error_handler_io(&In, &Out);
    
    field = Vect_get_field_number(&In, opt.lfield->answer);
    
    /* Open output segments */
    Vect_open_new(&Out, opt.output->answer, Vect_is_3d(&In));
    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    /* Table */
    Fi = NULL;
    if (!flag.table->answer) {
	struct field_info *Fin;

	/* copy input table */
	Fin = Vect_get_field(&In, field);
	if (Fin) {		/* table defined */
	    int ret;

	    Fi = Vect_default_field_info(&Out, 1, NULL, GV_MTABLE);
	    Vect_map_add_dblink(&Out, 1, NULL, Fi->table, Fin->key,
				Fi->database, Fi->driver);

	    ret = db_copy_table(Fin->driver, Fin->database, Fin->table,
				Fi->driver, Vect_subst_var(Fi->database,
							   &Out), Fi->table);

	    if (ret == DB_FAILED) {
		G_fatal_error(_("Unable to copy table <%s>"),
			      Fin->table);
	    }
	}

	Fi = Vect_default_field_info(&Out, 2, NULL, GV_MTABLE);
	Vect_map_add_dblink(&Out, 2, NULL, Fi->table, GV_KEY_COLUMN, Fi->database,
			    Fi->driver);

	/* Open driver */
	driver = db_start_driver_open_database(Fi->driver, Fi->database);
	if (driver == NULL)
	    G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
			  Fi->database, Fi->driver);
        db_set_error_handler_driver(driver);

	if (field == -1) 
            sprintf(buf,
                "create table %s ( cat int, along double precision )",
                Fi->table);
         else
            sprintf(buf,
		"create table %s ( cat int, lcat int, along double precision )",
		Fi->table);
	db_append_string(&stmt, buf);

	if (db_execute_immediate(driver, &stmt) != DB_OK) {
	    G_fatal_error(_("Unable to create table: '%s'"),
			  db_get_string(&stmt));
	}

	if (db_create_index2(driver, Fi->table, GV_KEY_COLUMN) != DB_OK)
	    G_warning(_("Unable to create index for table <%s>, key <%s>"),
		      Fi->table, GV_KEY_COLUMN);

	if (db_grant_on_table (driver, Fi->table, DB_PRIV_SELECT,
                               DB_GROUP | DB_PUBLIC) != DB_OK)
	    G_fatal_error(_("Unable to grant privileges on table <%s>"),
			  Fi->table);

	db_begin_transaction(driver);
    }

    if (type & (GV_POINTS | GV_LINES | GV_FACE)) {
	int line, nlines;

	nlines = Vect_get_num_lines(&In);
	for (line = 1; line <= nlines; line++) {
	    int ltype, cat;

	    G_debug(3, "line = %d", line);
	    G_percent(line, nlines, 2);
            
	    ltype = Vect_read_line(&In, LPoints, LCats, line);
	    if (!(ltype & type))
		continue;
            if (!Vect_cat_get(LCats, field, &cat) && field != -1)
		continue;

            /* Assign CAT for layer 0 objects (i.e. boundaries) */
            if (field == -1)
                cat = -1;

	    if (LPoints->n_points <= 1) {
		write_point(&Out, LPoints->x[0], LPoints->y[0], LPoints->z[0],
			    cat, 0.0, driver, Fi);
	    }
	    else {		/* lines */
		write_line(&Out, LPoints, cat, vertex_type,
			   flag.inter->answer, dmax, driver, Fi);
	    }
	}
    }

    if (type == GV_AREA) {
	int area, nareas, centroid, cat;

	nareas = Vect_get_num_areas(&In);
	for (area = 1; area <= nareas; area++) {
	    int i, isle, nisles;

	    G_percent(area, nareas, 2);
            
	    centroid = Vect_get_area_centroid(&In, area);
	    cat = -1;
	    if (centroid > 0) {
		Vect_read_line(&In, NULL, LCats, centroid);
		if (!Vect_cat_get(LCats, field, &cat))
		  continue;
	    }

	    Vect_get_area_points(&In, area, LPoints);

	    write_line(&Out, LPoints, cat, vertex_type, flag.inter->answer,
		       dmax, driver, Fi);

	    nisles = Vect_get_area_num_isles(&In, area);

	    for (i = 0; i < nisles; i++) {
		isle = Vect_get_area_isle(&In, area, i);
		Vect_get_isle_points(&In, isle, LPoints);

		write_line(&Out, LPoints, cat, vertex_type,
			   flag.inter->answer, dmax, driver, Fi);
	    }
	}
    }

    if (!flag.table->answer) {
	db_commit_transaction(driver);
	db_close_database_shutdown_driver(driver);
    }

    Vect_build(&Out);

    /* Free, close ... */
    Vect_close(&In);

    G_done_msg(_("%d points written to output vector map."),
               Vect_get_num_primitives(&Out, GV_POINT));

    Vect_close(&Out);
    
    exit(EXIT_SUCCESS);
}
