
/****************************************************************
 *
 * MODULE:     v.kcv
 *
 * AUTHOR(S):  James Darrell McCauley darrell@mccauley-usa.com
 *             OGR support by Martin Landa <landa.martin gmail.com> (2009)
 *
 * PURPOSE:    Randomly partition points into test/train sets.
 *
 * COPYRIGHT:  (C) 2001-2009 by James Darrell McCauley, and the GRASS Development Team
 *
 *             This program is free software under the GNU General
 *             Public License (>=v2).  Read the file COPYING that
 *             comes with GRASS for details.
 *
 ****************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "kcv.h"

static double myrand(void)
{
    return rand() / (1.0 * RAND_MAX);
}

struct Cell_head window;

int main(int argc, char *argv[])
{
    int line, nlines, nlinks;
    double (*rng)(void);
    double east, north;
    int i, j, nsites, np, *p;
    int *pnt_part;
    struct Map_info In, Out;
    static struct line_pnts *Points;
    struct line_cats *Cats;
    struct GModule *module;
    struct Option *in_opt, *out_opt, *col_opt, *npart_opt, *field_opt;
    struct Flag *drand48_flag;
    struct bound_box box;
    double maxdist;

    /* Attributes */
    struct field_info *Fi;
    dbDriver *Driver;
    char buf[2000];
    dbString sql;

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("points"));
    module->description =
	_("Randomly partition points into test/train sets.");

    in_opt = G_define_standard_option(G_OPT_V_INPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);

    npart_opt = G_define_option();
    npart_opt->key = "k";
    npart_opt->type = TYPE_INTEGER;
    npart_opt->required = YES;
    npart_opt->description = _("Number of partitions");
    npart_opt->options = "1-32767";

    col_opt = G_define_option();
    col_opt->key = "column";
    col_opt->type = TYPE_STRING;
    col_opt->required = YES;
    col_opt->multiple = NO;
    col_opt->answer = "part";
    col_opt->description =
	_("Name for new column to which partition number is written");

    drand48_flag = G_define_flag();
    drand48_flag->key = 'd';
#ifdef HAVE_DRAND48
    drand48_flag->description = _("Use drand48()");
#else
    drand48_flag->description = _("Use drand48() (ignored)");
#endif

    G_gisinit(argv[0]);

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    np = atoi(npart_opt->answer);

#ifdef HAVE_DRAND48
    if (drand48_flag->answer) {
	rng = drand48;
	srand48((long)getpid());
    }
    else
#endif
    {
	rng = myrand;
	srand(getpid());
    }

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    /* open input vector */
    Vect_set_open_level(2);
    if (Vect_open_old2(&In, in_opt->answer, "", field_opt->answer) < 2) {
	G_fatal_error(_("Unable to open vector map <%s> at topological level %d"),
		      in_opt->answer, 2);
    }

    Vect_get_map_box(&In, &box);

    nsites = Vect_get_num_primitives(&In, GV_POINT);

    if (nsites < np) {
	G_fatal_error(_("More partitions than points (%d)"), nsites);
    }

    Vect_open_new(&Out, out_opt->answer, Vect_is_3d(&In));

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    /* Copy vector lines */
    Vect_copy_map_lines_field(&In, Vect_get_field_number(&In, field_opt->answer), &Out);

    /* Copy tables */
    if (Vect_copy_tables(&In, &Out, 0))
	G_warning(_("Failed to copy attribute table to output map"));

    /* Add column */
    db_init_string(&sql);

    /* Check if there is a database for output */
    nlinks = Vect_get_num_dblinks(&Out);
    if (nlinks < 1)
	Fi = Vect_default_field_info(&Out, 1, NULL, GV_1TABLE);
    else
	Fi = Vect_get_field(&Out, 1);
    if (Fi == NULL) {
	Vect_delete(Out.name);
	G_fatal_error(_("Unable to get layer info for vector map <%s>"),
		      out_opt->answer);
    }

    Driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (Driver == NULL) {
	Vect_delete(Out.name);
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);
    }

    if (nlinks < 1)
	sprintf(buf, "create table %s (%s integer, %s integer)", Fi->table,
		Fi->key, col_opt->answer);
    else
	sprintf(buf, "alter table %s add column %s integer", Fi->table,
		col_opt->answer);

    db_set_string(&sql, buf);

    G_debug(3, "SQL: %s", db_get_string(&sql));

    if (db_execute_immediate(Driver, &sql) != DB_OK) {
	Vect_delete(Out.name);
	G_fatal_error(_("Unable to alter table: %s"), db_get_string(&sql));
    }
    if (nlinks < 1)
	Vect_map_add_dblink(&Out, Fi->number, Fi->name, Fi->table, Fi->key,
			    Fi->database, Fi->driver);

    /*
     * make histogram of number sites in each test partition since the
     * number of sites will not always be a multiple of the number of
     * partitions. make_histo() returns the maximum bin size.
     */
    make_histo(&p, np, nsites);

    nlines = Vect_get_num_lines(&In);
    pnt_part = (int *)G_calloc(nlines + 1, sizeof(int));

    maxdist = 1.1 * sqrt(pow(box.E - box.W, 2.0) + pow(box.N - box.S, 2.0));

    /* Assign fold to each point */
    for (i = 0; i < np; ++i) {	/* for each partition */
	for (j = 0; j < p[i]; ++j) {	/* create p[i] random points */
	    int nearest = 0;
	    double dist;

	    east = rng() * (box.E - box.W) + box.W;
	    north = rng() * (box.N - box.S) + box.S;

	    G_debug(3, "east = %f north = %f", east, north);

	    /* find nearest */
	    for (line = 1; line <= nlines; line++) {
		int type;
		double cur_dist;

		if (pnt_part[line] > 0)
		    continue;

		type = Vect_read_line(&In, Points, Cats, line);

		if (!(type & GV_POINT))
		    continue;

		cur_dist = hypot(Points->x[0] - east, Points->y[0] - north);

		if (nearest < 1 || cur_dist < dist) {
		    nearest = line;
		    dist = cur_dist;
		}
	    }

	    G_debug(3, "nearest = %d", nearest);

	    /* Update */
	    if (nearest > 0) {	/* shopuld be always */
		int cat;

		Vect_read_line(&In, Points, Cats, nearest);
		Vect_cat_get(Cats, 1, &cat);

		if (nlinks < 1)
		    sprintf(buf, "insert into %s (%s, %s) values (%d, %d)",
			    Fi->table, Fi->key, col_opt->answer, cat, i + 1);
		else
		    sprintf(buf, "update %s set %s = %d where %s = %d",
			    Fi->table, col_opt->answer, i + 1, Fi->key, cat);

		db_set_string(&sql, buf);

		G_debug(3, "SQL: %s", db_get_string(&sql));

		if (db_execute_immediate(Driver, &sql) != DB_OK) {
		    Vect_delete(Out.name);
		    G_fatal_error(_("Unable to insert row: %s"),
				  db_get_string(&sql));
		}
		pnt_part[nearest] = i + 1;

	    }
	}
    }

    Vect_close(&In);

    db_close_database_shutdown_driver(Driver);

    Vect_build(&Out);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}
