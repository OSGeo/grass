
/****************************************************************
 *
 * MODULE:     v.kcv
 *
 * AUTHOR(S):  James Darrell McCauley darrell@mccauley-usa.com
 *             OGR support by Martin Landa <landa.martin gmail.com> (2009)
 *             Spped-up by Jan Vandrol and Jan Ruzicka (2013)
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

static double myrand(void)
{
    return rand() / (1.0 + RAND_MAX);
}

struct Cell_head window;

int main(int argc, char *argv[]) 
{
    int i, line, nlines, nlinks;
    int idx, *line_idx, lines_left;
    double (*rng) ();
    int nsites, p, np, min_count, spill;
    struct partition {
	int id, count, max;
    } *part;
    int last_pidx;
    struct Map_info Map;
    struct line_pnts *Points;
    struct line_cats *Cats;
    struct GModule *module;
    struct Option *map_opt, *col_opt, *npart_opt, *field_opt;
    struct Flag *drand48_flag;
 
    /* Attributes */
    struct field_info *Fi;
    int layer;
    dbDriver * Driver;
    char buf[2000];
    dbString sql;


    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("statistics"));
    G_add_keyword(_("points"));
    module->description =
	_("Randomly partition points into test/train sets.");

    map_opt = G_define_standard_option(G_OPT_V_MAP);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);

    npart_opt = G_define_option();
    npart_opt->key = "k";
    npart_opt->type = TYPE_INTEGER;
    npart_opt->required = YES;
    npart_opt->label = _("Number of partitions");
    npart_opt->description = _("Must be > 1");

    col_opt = G_define_standard_option(G_OPT_DB_COLUMN);
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
    if (np < 2)
	G_fatal_error(_("'%s' must be > 1"), npart_opt->key);

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
    if (Vect_open_old2(&Map, map_opt->answer, G_mapset(), field_opt->answer) < 2) {
	G_fatal_error(_("Unable to open vector map <%s> at topological level %d"),
		      map_opt->answer, 2);
    }

    layer = Vect_get_field_number(&Map, field_opt->answer);
    if (layer <= 0)
	G_fatal_error(_("Layer number must be positive"));

    nsites = Vect_get_num_primitives(&Map, GV_POINT);

    if (nsites < np) {
	G_fatal_error(_("More partitions than points (%d)"), nsites);
    }

    /* Add column */
    db_init_string(&sql);

    /* Check if there is a database for output */
    nlinks = Vect_get_num_dblinks(&Map);
    if (nlinks < 1)
	Fi = Vect_default_field_info(&Map, layer, NULL, GV_1TABLE);
    else
	Fi = Vect_get_field(&Map, layer);
    if (Fi == NULL) {
	G_fatal_error(_("Unable to get layer info for vector map <%s>"),
		      map_opt->answer);
    }

    Driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (Driver == NULL) {
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);
    }

    buf[0] = '\0';
    if (nlinks < 1)
	sprintf(buf, "create table %s (%s integer, %s integer)", Fi->table,
		Fi->key, col_opt->answer);
    else {
	dbColumn *column = NULL;

	/* check if column exists */
	db_get_column(Driver, Fi->table, col_opt->answer, &column);
	if (column) {
            int sqltype = db_get_column_sqltype(column);
	    int ctype = db_sqltype_to_Ctype(sqltype);

	    if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE) {
		G_fatal_error(_("Column <%s> already exists but is not numeric"),
		              col_opt->answer);
	    }
	}
	else
	    sprintf(buf, "alter table %s add column %s integer", Fi->table,
		    col_opt->answer);
    }

    if (*buf) {
	db_set_string(&sql, buf);

	G_debug(3, "SQL: %s", db_get_string(&sql));

	if (db_execute_immediate(Driver, &sql) != DB_OK) {
	    G_fatal_error(_("Unable to alter table: %s"), db_get_string(&sql));
	}
    }

    if (nlinks < 1) {
	Vect_set_open_level(1);
	Vect_close(&Map);
	if (Vect_open_update_head(&Map, map_opt->answer, G_mapset()) < 1)
	    G_fatal_error(_("Unable to modify vector map stored in other mapset"));
	Vect_map_add_dblink(&Map, layer, Fi->name, Fi->table, Fi->key,
			    Fi->database, Fi->driver);
	Vect_close(&Map);

	if (db_create_index2(Driver, Fi->table, Fi->key) !=
	    DB_OK)
	    G_warning(_("Cannot create index"));

	if (db_grant_on_table(Driver, Fi->table, DB_PRIV_SELECT,
	                      DB_GROUP | DB_PUBLIC) != DB_OK)
	    G_warning(_("Cannot grant privileges on table %s"),
		      Fi->table);

	G_important_message(_("Select privileges were granted on the table"));

	Vect_set_open_level(2);
	if (Vect_open_old2(&Map, map_opt->answer, G_mapset(), field_opt->answer) < 2) {
	    G_fatal_error(_("Unable to open vector map <%s> at topological level %d"),
			  map_opt->answer, 2);
	}
    }

    /* nlines - count of records */ 
    nlines = Vect_get_num_lines(&Map);

    /* last_pidx - current maximal index of partition array */ 
    last_pidx = np - 1;

    /* minimum number of features in one partition */
    min_count = nsites / np;
    G_debug(1, "min count: %d", min_count);

    /* number of partitions that need min_count + 1 features */
    spill = nsites - np * min_count;  /* nsites % np, but modulus is evil */
    G_debug(1, "spill: %d", spill);

    /* array of available partitions */ 
    part = (struct partition *)G_calloc(np, sizeof(struct partition));
    if (!part)
	G_fatal_error(_("Out of memory"));

    /* line index */ 
    line_idx = (int *)G_calloc(nlines, sizeof(int));

    /* initialization of arrays */ 
    for (p = 0; p < np; p++) {
	part[p].id = p + 1;	/* partition ids */
	part[p].count = 0;
	part[p].max = min_count;
    }
    for (p = 0; p < spill; p++) {
	part[p].max++;
    }

    for (i = 0; i < nlines; i++)
	line_idx[i] = i + 1;

    /* proper randomization requires a 2-step randomization
     * randomize in space
     * randomize partition assignment
     * looping sequentially through all points creates a bias */

    /* process all points */
    db_begin_transaction(Driver);
    lines_left = nlines;
    while (lines_left) {
	int type, cat;

	G_percent(nlines - lines_left, nlines, 4);

	/* random point */
	do {
	    idx = rng() * lines_left;
	    /* in case rng() returns 1.0: */
	} while (idx >= lines_left);

	line = line_idx[idx];
	lines_left--;
	line_idx[idx] = line_idx[lines_left];

	if (!Vect_line_alive(&Map, line))
	    continue;

	type = Vect_read_line(&Map, Points, Cats, line);

	if (!(type & GV_POINT))
	    continue;

	/* random partition for current point */
	do {
	    p = rng() * (last_pidx + 1);
	    /* in case rng() returns 1.0: */
	} while (p > last_pidx);

	G_debug(3, "partition id = %d", part[p].id);

	Vect_cat_get(Cats, 1, &cat);
	if (cat < 0) {
	    G_warning(_("No category for line %d in layer %d"), line, layer);
	    continue;
	}
	if (nlinks < 1)
	    sprintf(buf, "insert into %s (%s, %s) values (%d, %d)",
			 Fi->table, Fi->key, col_opt->answer, cat,
			 part[p].id);
	else
	    sprintf(buf, "update %s set %s = %d where %s = %d", 
	                 Fi->table, col_opt->answer, part[p].id,
			 Fi->key, cat);

	db_set_string(&sql, buf);

	G_debug(3, "SQL: %s", db_get_string(&sql));

	if (db_execute_immediate(Driver, &sql) != DB_OK) {
	    G_fatal_error(_("Unable to insert row: %s"),
	    db_get_string(&sql));
	}

	/* increase count of features in partition */
	part[p].count++;
	/* if the partition is full */
	if (part[p].count >= part[p].max) {
	    /* move last partition to current position */
	    if (p != last_pidx)
		part[p] = part[last_pidx];

	    /* disable last partition */
	    last_pidx--;

	    if (last_pidx < 0 && lines_left) {
		G_fatal_error(_("internal error"));
	    }
	}
    }
    G_percent(1, 1, 1);

    db_commit_transaction(Driver);
    db_close_database_shutdown_driver(Driver);

    Vect_set_db_updated(&Map);
    Vect_close(&Map);

    exit(EXIT_SUCCESS);
}


