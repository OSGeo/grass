
/****************************************************************
 *
 * MODULE:     v.net.timetable
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    Routing with timetables
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/neta.h>

struct Map_info In, Out;
neta_timetable_result result;
neta_timetable timetable;

struct segment
{
    int from_stop, to_stop;
    int from_time, to_time;
    int route;
    struct segment *next;
} head;

double *stop_x, *stop_y, *stop_z;
int *stop_ids, *route_ids;
int *found, *stop_node, *edges, nnodes;
struct ilist **lines;
dglGraph_s *graph;

void init_route(int connection, int stop)
{
    if (result.prev_stop[connection][stop] == -1)
	return;
    struct segment *seg =
	(struct segment *)G_calloc(1, sizeof(struct segment));
    int prev_conn = result.prev_conn[connection][stop];

    seg->next = head.next;
    head.next = seg;
    seg->route = result.prev_route[connection][stop];
    seg->from_stop = result.prev_stop[connection][stop];
    seg->to_stop = stop;
    if (seg->route == -2)
	seg->from_time = result.dst[prev_conn][seg->from_stop];
    else
	seg->from_time =
	    NetA_timetable_get_route_time(&timetable, seg->from_stop,
					  seg->route);
    seg->to_time = result.dst[connection][stop];
    init_route(prev_conn, seg->from_stop);
}

void release_route(struct segment *seg)
{
    if (!seg)
	return;
    release_route(seg->next);
    G_free(seg->next);
}

static int int_cmp(const void *a, const void *b)
{
    return *(int *)a - *(int *)b;
}

void init_database(struct Map_info *Out, dbDriver ** driver,
		   struct field_info **Fi, int layer, char *columns)
{
    dbString sql;
    char buf[2000];

    /* Create table */
    *Fi = Vect_default_field_info(Out, layer, NULL, GV_MTABLE);
    Vect_map_add_dblink(Out, layer, NULL, (*Fi)->table, GV_KEY_COLUMN,
			(*Fi)->database, (*Fi)->driver);
    db_init_string(&sql);
    *driver = db_start_driver_open_database((*Fi)->driver, (*Fi)->database);
    if (*driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      (*Fi)->database, (*Fi)->driver);

    sprintf(buf, "create table %s (%s)", (*Fi)->table, columns);

    db_set_string(&sql, buf);
    G_debug(2, db_get_string(&sql));

    if (db_execute_immediate(*driver, &sql) != DB_OK) {
	db_close_database_shutdown_driver(*driver);
	G_fatal_error(_("Unable to create table: '%s'"), db_get_string(&sql));
    }

    if (db_create_index2(*driver, (*Fi)->table, GV_KEY_COLUMN) != DB_OK)
	G_warning(_("Cannot create index"));

    if (db_grant_on_table
	(*driver, (*Fi)->table, DB_PRIV_SELECT,
	 DB_GROUP | DB_PUBLIC) != DB_OK)
	G_fatal_error(_("Cannot grant privileges on table <%s>"),
		      (*Fi)->table);

    db_free_string(&sql);
    db_begin_transaction(*driver);
}

void insert_point(dbDriver * driver, char *table, int cat, int path,
		  int stop_id, int index, int arrival_time,
		  int departure_time)
{
    char buf[2000];
    dbString sql;

    db_init_string(&sql);

    sprintf(buf, "insert into %s values (%d, %d, %d, %d, %d, %d)", table, cat,
	    path, stop_id, index, arrival_time, departure_time);
    db_set_string(&sql, buf);
    G_debug(3, db_get_string(&sql));
    if (db_execute_immediate(driver, &sql) != DB_OK) {
	db_close_database_shutdown_driver(driver);
	G_fatal_error(_("Cannot insert new record: %s"), db_get_string(&sql));
    };

    db_free_string(&sql);
}

void insert_line(dbDriver * driver, char *table, int cat, int path,
		 int from_id, int to_id, int route_id, int index,
		 int from_time, int to_time)
{
    char buf[2000];
    dbString sql;

    db_init_string(&sql);

    sprintf(buf, "insert into %s values (%d, %d, %d, %d, %d, %d, %d, %d)",
	    table, cat, path, from_id, to_id, route_id, index, from_time,
	    to_time);
    db_set_string(&sql, buf);
    G_debug(3, db_get_string(&sql));
    if (db_execute_immediate(driver, &sql) != DB_OK) {
	db_close_database_shutdown_driver(driver);
	G_fatal_error(_("Cannot insert new record: %s"), db_get_string(&sql));
    };

    db_free_string(&sql);
}

int get_nearest_stop(double x, double y, double z, int with_z)
{
    int i, mini = -1;
    double mind = -1., d;

    for (i = 0; i < timetable.stops; i++) {
	if (!found[i])
	    continue;
	d = Vect_points_distance(x, y, z, stop_x[i], stop_y[i], stop_z[i],
				 with_z);
	if (mini == -1 || d < mind) {
	    mind = d;
	    mini = i;
	}
    }
    return mini;
}

void write_subroute(struct segment *seg, struct line_pnts *line, int line_id)
{
    int i, j, r;
    struct line_pnts *Points;
    struct line_cats *Cats;
    struct ilist *list;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    list = Vect_new_list();
    r = seg->route;

    Vect_cat_set(Cats, 2, line_id);

    if (r < 0) {
	Vect_write_line(&Out, GV_LINE, line, Cats);
	return;
    }

    for (i = 0; i < nnodes; i++)
	edges[i] = 0;
    for (i = 0; i < lines[r]->n_values; i++)
	edges[lines[r]->value[i]] = 1;

    for (i = 0; i < timetable.route_length[r]; i++)
	if (timetable.route_stops[r][i] == seg->from_stop)
	    break;
    for (; timetable.route_stops[r][i] != seg->to_stop; i++)
	if (NetA_find_path
	    (graph, stop_node[timetable.route_stops[r][i]],
	     stop_node[timetable.route_stops[r][i + 1]], edges, list) != -1) {
	    for (j = 0; j < list->n_values; j++) {
		int type = Vect_read_line(&In, Points, NULL, list->value[j]);

		Vect_write_line(&Out, type, Points, Cats);
	    }
	}
	else {
	    G_warning(_("Could not find a path between stops %d and %d"),
		      stop_ids[timetable.route_stops[r][i]],
		      stop_ids[timetable.route_stops[r][i + 1]]);
	}

    Vect_destroy_list(list);
    Vect_destroy_cats_struct(Cats);
    Vect_destroy_line_struct(Points);
}

int main(int argc, char *argv[])
{
    static struct line_pnts *Points, *Cur, *Prev;
    struct line_cats *Cats;
    struct GModule *module;	/* GRASS module for parsing arguments */
    struct Option *map_in, *map_out;
    struct Option *tfield_opt,		  /* Input map: layer with existing timetable */
                  *walk_layer_opt;	  /* Input map: layer with existing walking routes between stops */

    struct Option *afield_opt,		  /* Input map: layer with arc costs */
                  *nfield_opt,		  /* Input map: layer with node costs */
		  *afcol, *abcol, *ncol;  /* Input map: cost columns */
	          	
    struct Option *route_id_opt, *stop_time_opt, *to_stop_opt, *walk_length_opt;
    int with_z;
    int tfield, mask_type, afield, nfield;
    int from_stop, to_stop, start_time, min_change, max_changes,
	walking_change, ret;
    int *stop_pnt, i, nlines, point_counter, *route_pnt;
    int line_counter, index, j;
    struct segment *cur;
    char buf[2000];

    /* Attribute table */
    dbDriver *point_driver, *line_driver;
    struct field_info *point_Fi, *line_Fi;

    /* initialize GIS environment */
    G_gisinit(argv[0]);		/* reads grass env, stores program name to G_program_name() */

    /* initialize module */
    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("network"));
    G_add_keyword(_("shortest path"));
    module->description = _("Finds shortest path using timetables.");

    /* Define the different options as defined in gis.h */
    map_in = G_define_standard_option(G_OPT_V_INPUT);
    tfield_opt = G_define_standard_option(G_OPT_V_FIELD);

    map_out = G_define_standard_option(G_OPT_V_OUTPUT);

    afield_opt = G_define_standard_option(G_OPT_V_FIELD);
    afield_opt->key = "alayer";
    afield_opt->answer = "1";
    afield_opt->label = _("Arc layer");
    afield_opt->guisection = _("Cost");

    nfield_opt = G_define_standard_option(G_OPT_V_FIELD);
    nfield_opt->key = "nlayer";
    nfield_opt->answer = "2";
    nfield_opt->label = _("Node layer");
    nfield_opt->guisection = _("Cost");

    afcol = G_define_option();
    afcol->key = "afcolumn";
    afcol->type = TYPE_STRING;
    afcol->required = NO;
    afcol->description =
	_("Arc forward/both direction(s) cost column (number)");
    afcol->guisection = _("Cost");

    abcol = G_define_option();
    abcol->key = "abcolumn";
    abcol->type = TYPE_STRING;
    abcol->required = NO;
    abcol->description = _("Arc backward direction cost column (number)");
    abcol->guisection = _("Cost");

    ncol = G_define_option();
    ncol->key = "ncolumn";
    ncol->type = TYPE_STRING;
    ncol->required = NO;
    ncol->description = _("Node cost column (number)");
    ncol->guisection = _("Cost");

    walk_layer_opt = G_define_standard_option(G_OPT_V_FIELD_ALL);
    walk_layer_opt->key = "walk_layer";
    walk_layer_opt->answer = "-1";
    walk_layer_opt->label =
	_("Layer number or name with walking connections or -1");

    route_id_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    route_id_opt->key = "route_id";
    route_id_opt->required = YES;
    route_id_opt->answer = "route_id";
    route_id_opt->description = _("Name of column with route ids");

    stop_time_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    stop_time_opt->key = "stop_time";
    stop_time_opt->required = YES;
    stop_time_opt->answer = "stop_time";
    stop_time_opt->description =
	_("Name of column with stop timestamps");

    to_stop_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    to_stop_opt->key = "to_stop";
    to_stop_opt->required = YES;
    to_stop_opt->answer = "to_stop";
    to_stop_opt->description = _("Name of column with stop ids");

    walk_length_opt = G_define_standard_option(G_OPT_DB_COLUMN);
    walk_length_opt->key = "walk_length";
    walk_length_opt->required = YES;
    walk_length_opt->answer = "length";
    walk_length_opt->description = _("Name of column with walk lengths");

    /* options and flags parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    /* TODO: make an option for this */
    mask_type = GV_LINE | GV_BOUNDARY;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    Cur = Vect_new_line_struct();
    Prev = Vect_new_line_struct();

    Vect_check_input_output_name(map_in->answer, map_out->answer,
				 G_FATAL_EXIT);

    Vect_set_open_level(2);

    if (1 > Vect_open_old(&In, map_in->answer, ""))
	G_fatal_error(_("Unable to open vector map <%s>"), map_in->answer);

    with_z = Vect_is_3d(&In);

    if (0 > Vect_open_new(&Out, map_out->answer, with_z)) {
	Vect_close(&In);
	G_fatal_error(_("Unable to create vector map <%s>"), map_out->answer);
    }


    /* parse filter option and select appropriate lines */
    tfield = atoi(tfield_opt->answer);
    afield = atoi(afield_opt->answer);
    nfield = atoi(nfield_opt->answer);

    init_database(&Out, &point_driver, &point_Fi, 1,
		  "cat integer, path_id integer, stop_id integer, index integer, arr_time integer, dep_time integer");
    init_database(&Out, &line_driver, &line_Fi, 2,
		  "cat integer, path_id integer, from_id integer, to_id integer, route_id integer, index integer, from_time integer, to_time integer");

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);

    if (NetA_init_timetable_from_db
	(&In, tfield, atoi(walk_layer_opt->answer), route_id_opt->answer,
	 stop_time_opt->answer, to_stop_opt->answer, walk_length_opt->answer,
	 &timetable, &route_ids, &stop_ids) != 0)
	G_fatal_error(_("Could not initialize the timetables"));

    stop_x = (double *)G_calloc(timetable.stops, sizeof(double));
    stop_y = (double *)G_calloc(timetable.stops, sizeof(double));
    stop_z = (double *)G_calloc(timetable.stops, sizeof(double));
    found = (int *)G_calloc(timetable.stops, sizeof(int));

    if (!stop_x || !stop_y || !stop_z || !found)
	G_fatal_error(_("Out of memory"));

    if (afield > 0) {
	nnodes = Vect_get_num_nodes(&In);
	stop_node = (int *)G_calloc(timetable.stops, sizeof(int));
	lines =
	    (struct ilist **)G_calloc(timetable.routes,
				      sizeof(struct ilist *));
	edges = (int *)G_calloc(nnodes + 1, sizeof(int));
	if (!edges || !stop_node || !lines)
	    G_fatal_error(_("Out of memory"));
	for (i = 0; i < timetable.routes; i++)
	    lines[i] = Vect_new_list();

	if (0 != Vect_net_build_graph(&In, mask_type, afield, nfield, afcol->answer,
                                      abcol->answer, ncol->answer, 0, 0))
            G_fatal_error(_("Unable to build graph for vector map <%s>"), Vect_get_full_name(&In));
        
	graph = Vect_net_get_graph(&In);
    }


    nlines = Vect_get_num_lines(&In);
    for (i = 1; i <= nlines; i++) {
	int type = Vect_read_line(&In, Points, Cats, i);

	if (type == GV_POINT) {
	    int cat, stop, node;

	    for (j = 0; j < Cats->n_cats; j++) {
		if (Cats->field[j] != tfield)
		    continue;
		cat = Cats->cat[j];
		stop_pnt =
		    (int *)bsearch(&cat, stop_ids, timetable.stops,
				   sizeof(int), int_cmp);
		if (!stop_pnt)
		    continue;

		stop = stop_pnt - stop_ids;
		stop_x[stop] = Points->x[0];
		stop_y[stop] = Points->y[0];
		stop_z[stop] = Points->z[0];
		if (afield > 0) {
		    node = Vect_find_node(&In, Points->x[0], Points->y[0], Points->z[0], 0, 0);
		    if (!stop_node[stop] && node > 0)
			stop_node[stop] = node;
		}
		found[stop] = 1;
	    }
	}
	else if (type == GV_LINE && afield > 0) {
	    int cat;

	    for (j = 0; j < Cats->n_cats; j++) {
		if (Cats->field[j] != afield)
		    continue;
		cat = Cats->cat[j];
		route_pnt =
		    (int *)bsearch(&cat, route_ids, timetable.routes,
				   sizeof(int), int_cmp);
		if (!route_pnt)
		    continue;
		Vect_list_append(lines[route_pnt - route_ids], i);
	    }
	}
    }

    for (i = 0; i < timetable.stops; i++)
	if (!found[i])
	    G_warning(_("No stop with category: %d"), stop_ids[i]);

    point_counter = line_counter = 1;
    while (1) {
	double fx, fy, tx, ty;
	int path_id;

	if (fgets(buf, sizeof(buf), stdin) == NULL)
	    break;
	ret =
	    sscanf(buf, "%d %lf %lf %lf %lf %d %d %d %d", &path_id, &fx, &fy,
		   &tx, &ty, &start_time, &min_change, &max_changes,
		   &walking_change);
	if (ret == 9) {
	    from_stop = get_nearest_stop(fx, fy, 0, with_z);
	    to_stop = get_nearest_stop(tx, ty, 0, with_z);
	}
	else {
	    ret =
		sscanf(buf, "%d %d %d %d %d %d %d", &path_id, &from_stop,
		       &to_stop, &start_time, &min_change, &max_changes,
		       &walking_change);
	    if (ret < 7) {
		G_warning(_("Wrong input format: %s"), buf);
		continue;
	    }

	    stop_pnt =
		(int *)bsearch(&from_stop, stop_ids, timetable.stops,
			       sizeof(int), int_cmp);
	    if (!stop_pnt) {
		G_warning(_("No stop with category: %d"), from_stop);
		continue;
	    }
	    from_stop = stop_pnt - stop_ids;
	    stop_pnt =
		(int *)bsearch(&to_stop, stop_ids, timetable.stops,
			       sizeof(int), int_cmp);
	    if (!stop_pnt) {
		G_warning(_("No stop with category: %d"), to_stop);
		continue;
	    }
	    to_stop = stop_pnt - stop_ids;
	}

	if (from_stop == to_stop) {
	    G_warning(_("'From' and 'To' stops are the same"));
	    continue;
	}

	ret =
	    NetA_timetable_shortest_path(&timetable, from_stop, to_stop,
					 start_time, min_change, max_changes,
					 walking_change, &result);
	if (ret == -1) {
	    G_warning(_("No path between the stops"));
	    continue;
	}
	head.next = NULL;
	init_route(result.routes, to_stop);
	NetA_timetable_result_release(&result);

	Vect_reset_line(Points);
	Vect_reset_line(Cur);
	Vect_reset_line(Prev);
	Vect_append_point(Cur, stop_x[from_stop], stop_y[from_stop],
			  stop_z[from_stop]);

	Vect_reset_cats(Cats);
	Vect_cat_set(Cats, 1, point_counter);
	Vect_write_line(&Out, GV_POINT, Cur, Cats);
	insert_point(point_driver, point_Fi->table, point_counter, path_id,
		     stop_ids[from_stop], 1, start_time,
		     head.next->from_time);
	point_counter++;
	Vect_append_points(Prev, Cur, GV_FORWARD);
	index = 1;
	for (cur = head.next; cur; cur = cur->next) {
	    int dept_time, route_id;

	    if (cur->route == -2) {
		printf("Walk ");
		route_id = -1;
	    }
	    else {
		printf("Route %d, ", route_ids[cur->route]);
		route_id = route_ids[cur->route];
	    }
	    printf("from %d leaving at %d arriving to %d at %d\n",
		   stop_ids[cur->from_stop], cur->from_time,
		   stop_ids[cur->to_stop], cur->to_time);

	    Vect_reset_line(Cur);
	    Vect_reset_line(Points);
	    Vect_reset_cats(Cats);

	    Vect_append_point(Cur, stop_x[cur->to_stop], stop_y[cur->to_stop],
			      stop_z[cur->to_stop]);

	    Vect_cat_set(Cats, 1, point_counter);
	    Vect_write_line(&Out, GV_POINT, Cur, Cats);
	    if (cur->next)
		dept_time = cur->next->from_time;
	    else
		dept_time = cur->to_time;
	    insert_point(point_driver, point_Fi->table, point_counter,
			 path_id, stop_ids[cur->to_stop], index + 1,
			 cur->to_time, dept_time);

	    Vect_append_points(Points, Prev, GV_FORWARD);
	    Vect_append_points(Points, Cur, GV_FORWARD);
	    Vect_reset_cats(Cats);
	    Vect_cat_set(Cats, 2, line_counter);
	    if (afield <= 0)
		Vect_write_line(&Out, GV_LINE, Points, Cats);
	    else
		write_subroute(cur, Points, line_counter);
	    insert_line(line_driver, line_Fi->table, line_counter, path_id,
			stop_ids[cur->from_stop], stop_ids[cur->to_stop],
			route_id, index, cur->from_time, cur->to_time);

	    Vect_reset_line(Prev);
	    Vect_append_points(Prev, Cur, GV_FORWARD);

	    point_counter++;
	    line_counter++;
	    index++;
	}
	release_route(&head);
    }
    db_commit_transaction(line_driver);
    db_commit_transaction(point_driver);
    db_close_database_shutdown_driver(line_driver);
    db_close_database_shutdown_driver(point_driver);

    Vect_build(&Out);

    Vect_close(&In);
    Vect_close(&Out);

    G_free(stop_x);
    G_free(stop_y);
    G_free(stop_z);
    G_free(stop_node);

    exit(EXIT_SUCCESS);
}
