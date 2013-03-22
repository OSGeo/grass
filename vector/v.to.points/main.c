
/***************************************************************
 *
 * MODULE:       v.to.points
 * 
 * AUTHOR(S):    Radim Blazek
 *               OGR support by Martin Landa <landa.martin gmail.com>
 *               
 * PURPOSE:      Create points along lines 
 *               
 * COPYRIGHT:    (C) 2002-2010 by the GRASS Development Team
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

/*
 * local macros
 */
#define GV_NODE   1
#define GV_VERTEX 2

static int point_cat;
static struct line_cats *PCats;
static struct line_pnts *PPoints;
static dbString stmt;
static dbDriver *driver;
static struct field_info *Fi;

void write_point(struct Map_info *Out, double x, double y, double z,
		 int line_cat, double along, int table)
{
    char buf[2000];

    G_debug(3, "write_point()");

    Vect_reset_line(PPoints);
    Vect_reset_cats(PCats);

    /* Write point */
    Vect_append_point(PPoints, x, y, z);
    if (line_cat > 0) {
        Vect_cat_set(PCats, 1, line_cat);
        Vect_cat_set(PCats, 2, point_cat);
    }
    else {
        Vect_cat_set(PCats, 2, point_cat);
    }
    Vect_write_line(Out, GV_POINT, PPoints, PCats);

    /* Attributes */
    if (!table) {
	db_zero_string(&stmt);
        if (line_cat > 0)
            sprintf(buf, "insert into %s values ( %d, %d, %.15g )", Fi->table,
                point_cat, line_cat, along);
        else
            sprintf(buf, "insert into %s values ( %d, %.15g )", Fi->table,
                point_cat, along);
	db_append_string(&stmt, buf);

	if (db_execute_immediate(driver, &stmt) != DB_OK) {
	    G_warning(_("Unable to insert new record: '%s'"),
		      db_get_string(&stmt));
	}
    }
    point_cat++;
}

void write_line(struct Map_info *Out, struct line_pnts *LPoints, int cat,
		int vertex, int interpolate, double dmax, int table)
{
    if (vertex == GV_VERTEX || vertex == GV_NODE) {	/* use line vertices */
	double along;
	int vert;

	along = 0;
	for (vert = 0; vert < LPoints->n_points; vert++) {
	    G_debug(3, "vert = %d", vert);

	    if (vertex == GV_VERTEX ||
		(vertex == GV_NODE &&
		 (vert == 0 || vert == LPoints->n_points - 1))) {
		write_point(Out, LPoints->x[vert], LPoints->y[vert],
			    LPoints->z[vert], cat, along, table);
	    }

	    if (vert < LPoints->n_points - 1) {
		double dx, dy, dz, len;

		dx = LPoints->x[vert + 1] - LPoints->x[vert];
		dy = LPoints->y[vert + 1] - LPoints->y[vert];
		dz = LPoints->z[vert + 1] - LPoints->z[vert];
		len = hypot(hypot(dx, dy), dz);

		/* interpolate segment */
		if (interpolate && vert < (LPoints->n_points - 1)) {
		    int i, n;
		    double x, y, z, dlen;

		    if (len > dmax) {
			n = len / dmax + 1;	/* number of segments */
			dx /= n;
			dy /= n;
			dz /= n;
			dlen = len / n;

			for (i = 1; i < n; i++) {
			    x = LPoints->x[vert] + i * dx;
			    y = LPoints->y[vert] + i * dy;
			    z = LPoints->z[vert] + i * dz;

			    write_point(Out, x, y, z, cat, along + i * dlen,
					table);
			}
		    }
		}
		along += len;
	    }
	}
    }
    else {			/* do not use vertices */
	int i, n;
	double len, dlen, along, x, y, z;

	len = Vect_line_length(LPoints);
	n = len / dmax + 1;	/* number of segments */
	dlen = len / n;		/* length of segment */

	G_debug(3, "n = %d len = %f dlen = %f", n, len, dlen);

	for (i = 0; i <= n; i++) {
	    if (i > 0 && i < n) {
		along = i * dlen;
		Vect_point_on_line(LPoints, along, &x, &y, &z, NULL, NULL);
	    }
	    else {		/* first and last vertex */
		if (i == 0) {
		    along = 0;
		    x = LPoints->x[0];
		    y = LPoints->y[0];
		    z = LPoints->z[0];
		}
		else {		/* last */
		    along = len;
		    x = LPoints->x[LPoints->n_points - 1];
		    y = LPoints->y[LPoints->n_points - 1];
		    z = LPoints->z[LPoints->n_points - 1];
		}
	    }
	    G_debug(3, "  i = %d along = %f", i, along);
	    write_point(Out, x, y, z, cat, along, table);
	}
    }
}

int main(int argc, char **argv)
{
    int field, type, vertex_type;
    double dmax;
    struct Option *in_opt, *out_opt, *type_opt, *dmax_opt, *lfield_opt;
    struct Flag *inter_flag, *vertex_flag, *table_flag, *node_flag;
    struct GModule *module;
    struct Map_info In, Out;
    struct line_cats *LCats;
    struct line_pnts *LPoints;
    char buf[2000];

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    module->description =
	_("Creates points along input lines in new vector map with 2 layers.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    in_opt->label = _("Name of vector map containing lines");

    lfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    lfield_opt->key = "llayer";
    lfield_opt->answer = "1";
    lfield_opt->label = "Line layer number or name";
    lfield_opt->guisection = _("Selection");

    type_opt = G_define_standard_option(G_OPT_V_TYPE);
    type_opt->answer = "point,line,boundary,centroid";
    type_opt->guisection = _("Selection");

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    out_opt->description =
	_("Name for output vector map where points will be written");

    node_flag = G_define_flag();
    node_flag->key = 'n';
    node_flag->description = _("Write line nodes");

    vertex_flag = G_define_flag();
    vertex_flag->key = 'v';
    vertex_flag->description = _("Write line vertices");

    inter_flag = G_define_flag();
    inter_flag->key = 'i';
    inter_flag->description = _("Interpolate points between line vertices");

    dmax_opt = G_define_option();
    dmax_opt->key = "dmax";
    dmax_opt->type = TYPE_DOUBLE;
    dmax_opt->required = NO;
    dmax_opt->answer = "100";
    dmax_opt->description = _("Maximum distance between points in map units");

    table_flag = G_define_standard_flag(G_FLG_V_TABLE);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    LCats = Vect_new_cats_struct();
    PCats = Vect_new_cats_struct();
    LPoints = Vect_new_line_struct();
    PPoints = Vect_new_line_struct();
    db_init_string(&stmt);

    type = Vect_option_to_types(type_opt);
    dmax = atof(dmax_opt->answer);

    if (node_flag->answer && vertex_flag->answer)
	G_fatal_error(_("Use either -n or -v flag, not both"));

    if (node_flag->answer)
	vertex_type = GV_NODE;
    else if (vertex_flag->answer)
	vertex_type = GV_VERTEX;
    else
	vertex_type = 0;

    Vect_check_input_output_name(in_opt->answer, out_opt->answer,
				 G_FATAL_EXIT);

    /* Open input lines */
    Vect_set_open_level(2);
    Vect_open_old2(&In, in_opt->answer, "", lfield_opt->answer);
    field = Vect_get_field_number(&In, lfield_opt->answer);
    
    /* Open output segments */
    Vect_open_new(&Out, out_opt->answer, Vect_is_3d(&In));
    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    /* Table */
    if (!table_flag->answer) {
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
	    db_close_database_shutdown_driver(driver);
	    G_fatal_error(_("Unable to create table: '%s'"),
			  db_get_string(&stmt));
	}

	if (db_create_index2(driver, Fi->table, GV_KEY_COLUMN) != DB_OK)
	    G_warning(_("Unable to create index for table <%s>, key <%s>"),
		      Fi->table, GV_KEY_COLUMN);

	if (db_grant_on_table
	    (driver, Fi->table, DB_PRIV_SELECT,
	     DB_GROUP | DB_PUBLIC) != DB_OK)
	    G_fatal_error(_("Unable to grant privileges on table <%s>"),
			  Fi->table);

	db_begin_transaction(driver);
    }

    point_cat = 1;

    if (type & (GV_POINTS | GV_LINES)) {
	int line, nlines;

	nlines = Vect_get_num_lines(&In);
	for (line = 1; line <= nlines; line++) {
	    int ltype, cat;

	    G_debug(3, "line = %d", line);

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
			    cat, 0.0, table_flag->answer);
	    }
	    else {		/* lines */
		write_line(&Out, LPoints, cat, vertex_type,
			   inter_flag->answer, dmax, table_flag->answer);
	    }
	    G_percent(line, nlines, 2);
	}
    }

    if (type == GV_AREA) {
	int area, nareas, centroid, cat;

	nareas = Vect_get_num_areas(&In);
	for (area = 1; area <= nareas; area++) {
	    int i, isle, nisles;

	    centroid = Vect_get_area_centroid(&In, area);
	    cat = -1;
	    if (centroid > 0) {
		Vect_read_line(&In, NULL, LCats, centroid);
		if (!Vect_cat_get(LCats, field, &cat))
		  continue;
	    }

	    Vect_get_area_points(&In, area, LPoints);

	    write_line(&Out, LPoints, cat, vertex_type, inter_flag->answer,
		       dmax, table_flag->answer);

	    nisles = Vect_get_area_num_isles(&In, area);

	    for (i = 0; i < nisles; i++) {
		isle = Vect_get_area_isle(&In, area, i);
		Vect_get_isle_points(&In, isle, LPoints);

		write_line(&Out, LPoints, cat, vertex_type,
			   inter_flag->answer, dmax, table_flag->answer);
	    }
	    G_percent(area, nareas, 2);
	}
    }

    if (!table_flag->answer) {
	db_commit_transaction(driver);
	db_close_database_shutdown_driver(driver);
    }

    Vect_build(&Out);

    /* Free, close ... */
    Vect_close(&In);
    Vect_close(&Out);

    G_done_msg(_("%d points written to output vector map."), point_cat - 1);

    exit(EXIT_SUCCESS);
}
