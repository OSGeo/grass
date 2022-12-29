/*!
   \file vector/neta/timetables.c

   \brief Network Analysis library - timetables

   Shortest path using timetables.

   (C) 2009-2010 by Daniel Bundala, and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Daniel Bundala (Google Summer of Code 2009)
 */

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include <grass/dgl/graph.h>
#include <grass/neta.h>

/*!
   \brief Get number of distinct elements

   \param driver DB driver
   \param sql SQl string
   \param[out] list of lengths
   \param[out] list of ids

   \return number of distinct elements 
   \return -1 on failure
 */
int NetA_init_distinct(dbDriver * driver, dbString * sql, int **lengths,
		       int **ids)
{
    int count, last, cur, result, index, more;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column;
    dbValue *value;

    if (db_open_select_cursor(driver, sql, &cursor, DB_SEQUENTIAL) != DB_OK) {
	G_warning(_("Unable to open select cursor: %s"), db_get_string(sql));
	return -1;
    }
    /*TODO: check column types */

    count = last = 0;
    /*count number of distinct routes */
    table = db_get_cursor_table(&cursor);
    column = db_get_table_column(table, 0);
    while (db_fetch(&cursor, DB_NEXT, &more) == DB_OK && more) {
	value = db_get_column_value(column);
	cur = db_get_value_int(value);
	if (count == 0 || cur != last) {
	    last = cur;
	    count++;
	}
    }
    result = count;
    db_close_cursor(&cursor);

    *lengths = (int *)G_calloc(count, sizeof(int));
    *ids = (int *)G_calloc(count, sizeof(int));
    if (!*lengths || !*ids) {
	G_warning(_("Out of memory"));
	return -1;
    }
    db_open_select_cursor(driver, sql, &cursor, DB_SEQUENTIAL);
    count = index = 0;
    /*calculate the lengths of the routes */
    table = db_get_cursor_table(&cursor);
    column = db_get_table_column(table, 0);
    while (db_fetch(&cursor, DB_NEXT, &more) == DB_OK && more) {
	value = db_get_column_value(column);
	cur = db_get_value_int(value);
	if (count != 0 && cur != last)
	    index++;
	if (count == 0 || cur != last)
	    (*ids)[index] = cur;
	(*lengths)[index]++;
	last = cur;
	count++;
    }
    return result;
}

static int cmp_int(const void *a, const void *b)
{
    return *(int *)a - *(int *)b;
}

/*!
   \brief Initialises timetable from a database

   \param In pointer to Map_info structure
   \param route_layer layer number of routes
   \param walk_layer layer number of walkers
   \param route_id id of route
   \param times list of timestamps
   \param to_stop ?
   \param walk_length walk length as string
   \param timetable pointer to neta_timetable
   \param route_ids list of route ids
   \param stop_ids lits of stop ids

   \return 0 on success
   \return non-zero value on failure
 */
int NetA_init_timetable_from_db(struct Map_info *In, int route_layer,
				int walk_layer, char *route_id, char *times,
				char *to_stop, char *walk_length,
				neta_timetable * timetable, int **route_ids,
				int **stop_ids)
{
    int more, i, stop, route, time, *stop_pnt, stop1, stop2;
    dbString sql;
    dbCursor cursor;
    dbTable *table;
    dbColumn *column1, *column2, *column3;
    dbValue *value;
    char buf[2000];

    dbDriver *driver;
    struct field_info *Fi;

    Fi = Vect_get_field(In, route_layer);
    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);


    db_init_string(&sql);
    sprintf(buf, "select %s from %s order by %s", route_id, Fi->table,
	    route_id);
    db_set_string(&sql, buf);
    timetable->routes =
	NetA_init_distinct(driver, &sql, &(timetable->route_length),
			   route_ids);
    if (timetable->routes < 0)
	return 1;

    sprintf(buf, "select %s from %s order by %s", Fi->key, Fi->table,
	    Fi->key);
    db_set_string(&sql, buf);
    timetable->stops =
	NetA_init_distinct(driver, &sql, &(timetable->stop_length), stop_ids);
    if (timetable->stops < 0)
	return 1;

    timetable->route_stops =
	(int **)G_calloc(timetable->routes, sizeof(int *));
    timetable->route_times =
	(int **)G_calloc(timetable->routes, sizeof(int *));
    timetable->stop_routes =
	(int **)G_calloc(timetable->stops, sizeof(int *));
    timetable->stop_times = (int **)G_calloc(timetable->stops, sizeof(int *));
    timetable->walk_length = (int *)G_calloc(timetable->stops, sizeof(int));
    timetable->walk_stops = (int **)G_calloc(timetable->stops, sizeof(int *));
    timetable->walk_times = (int **)G_calloc(timetable->stops, sizeof(int *));
    if (!timetable->route_stops || !timetable->route_times ||
	!timetable->stop_routes || !timetable->stop_times ||
	!timetable->walk_length) {
	G_warning(_("Out of memory"));
	return 2;
    }

    for (i = 0; i < timetable->routes; i++) {
	timetable->route_stops[i] =
	    (int *)G_calloc(timetable->route_length[i], sizeof(int));
	timetable->route_times[i] =
	    (int *)G_calloc(timetable->route_length[i], sizeof(int));
	if (!timetable->route_stops[i] || !timetable->route_times[i]) {
	    G_warning(_("Out of memory"));
	    return 2;
	}

	timetable->route_length[i] = 0;
    }

    for (i = 0; i < timetable->stops; i++) {
	timetable->stop_routes[i] =
	    (int *)G_calloc(timetable->stop_length[i], sizeof(int));
	timetable->stop_times[i] =
	    (int *)G_calloc(timetable->stop_length[i], sizeof(int));
	if (!timetable->stop_routes[i] || !timetable->stop_times[i]) {
	    G_warning(_("Out of memory"));
	    return 2;
	}
	timetable->walk_length[i] = 0;
	timetable->stop_length[i] = 0;
    }

    sprintf(buf, "select %s, %s, %s from %s order by %s", Fi->key, route_id,
	    times, Fi->table, times);
    db_set_string(&sql, buf);

    if (db_open_select_cursor(driver, &sql, &cursor, DB_SEQUENTIAL) != DB_OK) {
	G_warning(_("Unable to open select cursor: %s"), db_get_string(&sql));
	return 1;
    }


    table = db_get_cursor_table(&cursor);
    column1 = db_get_table_column(table, 0);
    column2 = db_get_table_column(table, 1);
    column3 = db_get_table_column(table, 2);
    while (db_fetch(&cursor, DB_NEXT, &more) == DB_OK && more) {
	value = db_get_column_value(column1);
	stop = db_get_value_int(value);
	value = db_get_column_value(column2);
	route = db_get_value_int(value);
	value = db_get_column_value(column3);
	time = db_get_value_int(value);
	stop =
	    (int *)bsearch(&stop, *stop_ids, timetable->stops, sizeof(int),
			   cmp_int) - (*stop_ids);
	route =
	    (int *)bsearch(&route, *route_ids, timetable->routes, sizeof(int),
			   cmp_int) - (*route_ids);

	timetable->stop_routes[stop][timetable->stop_length[stop]] = route;
	timetable->stop_times[stop][timetable->stop_length[stop]++] = time;

	timetable->route_stops[route][timetable->route_length[route]] = stop;
	timetable->route_times[route][timetable->route_length[route]++] =
	    time;
    }
    db_close_cursor(&cursor);

    if (walk_layer != -1) {

	Fi = Vect_get_field(In, walk_layer);
	sprintf(buf, "select %s, %s, %s from %s", Fi->key, to_stop,
		walk_length, Fi->table);
	db_set_string(&sql, buf);

	if (db_open_select_cursor(driver, &sql, &cursor, DB_SEQUENTIAL) !=
	    DB_OK) {
	    G_warning(_("Unable to open select cursor: %s"),
		      db_get_string(&sql));
	    return 1;
	}

	table = db_get_cursor_table(&cursor);
	column1 = db_get_table_column(table, 0);
	column2 = db_get_table_column(table, 1);
	while (db_fetch(&cursor, DB_NEXT, &more) == DB_OK && more) {
	    value = db_get_column_value(column2);
	    stop = db_get_value_int(value);
	    stop_pnt =
		(int *)bsearch(&stop, *stop_ids, timetable->stops,
			       sizeof(int), cmp_int);
	    if (stop_pnt) {
		value = db_get_column_value(column1);
		stop = db_get_value_int(value);
		stop_pnt =
		    (int *)bsearch(&stop, *stop_ids, timetable->stops,
				   sizeof(int), cmp_int);
		if (stop_pnt) {
		    stop = stop_pnt - (*stop_ids);
		    timetable->walk_length[stop]++;
		}
	    }
	}
	db_close_cursor(&cursor);

	for (i = 0; i < timetable->stops; i++) {
	    timetable->walk_stops[i] =
		(int *)G_calloc(timetable->walk_length[i], sizeof(int));
	    timetable->walk_times[i] =
		(int *)G_calloc(timetable->walk_length[i], sizeof(int));
	    if (!timetable->walk_stops[i] || !timetable->walk_times[i]) {
		G_warning(_("Out of memory"));
		return 2;
	    }
	    timetable->walk_length[i] = 0;
	}

	if (db_open_select_cursor(driver, &sql, &cursor, DB_SEQUENTIAL) !=
	    DB_OK) {
	    G_warning(_("Unable to open select cursor: %s"),
		      db_get_string(&sql));
	    return 1;
	}

	table = db_get_cursor_table(&cursor);
	column1 = db_get_table_column(table, 0);
	column2 = db_get_table_column(table, 1);
	column3 = db_get_table_column(table, 2);
	while (db_fetch(&cursor, DB_NEXT, &more) == DB_OK && more) {
	    value = db_get_column_value(column2);
	    stop = db_get_value_int(value);
	    stop_pnt =
		(int *)bsearch(&stop, *stop_ids, timetable->stops,
			       sizeof(int), cmp_int);
	    if (stop_pnt) {
		stop2 = stop_pnt - (*stop_ids);
		value = db_get_column_value(column1);
		stop = db_get_value_int(value);
		stop_pnt =
		    (int *)bsearch(&stop, *stop_ids, timetable->stops,
				   sizeof(int), cmp_int);
		if (stop_pnt) {
		    stop1 = stop_pnt - (*stop_ids);
		    value = db_get_column_value(column3);
		    time = db_get_value_int(value);
		    timetable->walk_stops[stop1][timetable->
						 walk_length[stop1]] = stop2;
		    timetable->walk_times[stop1][timetable->
						 walk_length[stop1]++] = time;
		}
	    }
	}
	db_close_cursor(&cursor);
    }
    db_close_database_shutdown_driver(driver);

    return 0;
}

typedef struct
{
    int v;
    int conns;
} neta_heap_data;

static neta_heap_data *new_heap_data(int conns, int v)
{
    neta_heap_data *d =
	(neta_heap_data *) G_calloc(1, sizeof(neta_heap_data));
    d->v = v;
    d->conns = conns;
    return d;
}

/*!
   \brief Update Dijkstra structures

   \param olc_conns old connection
   \param new_conns new connection
   \param to old 'to' node
   \param new_dst new 'to' node
   \param v ?
   \param route id of route
   \param rows ?
   \param update ?
   \param[out] result pointer to neta_timetable_result structure
   \param heap ?
 */
void NetA_update_dijkstra(int old_conns, int new_conns, int to, int new_dst,
			  int v, int route, int rows, int update,
			  neta_timetable_result * result, dglHeap_s * heap)
{
    if (result->dst[new_conns][to] == -1 ||
	result->dst[new_conns][to] > new_dst) {
	result->dst[new_conns][to] = new_dst;
	result->prev_stop[new_conns][to] = v;
	result->prev_route[new_conns][to] = route;
	result->prev_conn[new_conns][to] = old_conns;
	if (update) {
	    dglHeapData_u heap_data;

	    heap_data.pv = (void *)new_heap_data(new_conns, to);
	    dglHeapInsertMin(heap, new_dst, ' ', heap_data);
	}
    }
}

/*!
   \brief Computes the earliest arrival time.

   Computes the earliest arrival time to to_stop from from_stop
   starting at start_time, or -1 if no path exists

   \param timetable pointer to neta_timetable structure
   \param from_stop 'from' node
   \param to_stop 'to' stop
   \param start_time start timestamp
   \param min_change ?
   \param max_changes ?
   \param walking_change ?
   \param[out] result pointer to neta_timetable_result

   \return ?
   \return -1 on error
 */
int NetA_timetable_shortest_path(neta_timetable * timetable, int from_stop,
				 int to_stop, int start_time, int min_change,
				 int max_changes, int walking_change,
				 neta_timetable_result * result)
{
    int i, j;
    dglHeap_s heap;

    int opt_conns, rows = 1;

    if (max_changes != -1)
	rows = max_changes + 2;

    result->rows = rows;
    result->dst = (int **)G_calloc(rows, sizeof(int *));
    result->prev_stop = (int **)G_calloc(rows, sizeof(int *));
    result->prev_route = (int **)G_calloc(rows, sizeof(int *));
    result->prev_conn = (int **)G_calloc(rows, sizeof(int *));

    if (!result->dst || !result->prev_stop || !result->prev_route ||
	!result->prev_conn) {
	G_warning(_("Out of memory"));
	return -1;
    }

    for (i = 0; i < rows; i++) {
	result->dst[i] = (int *)G_calloc(timetable->stops, sizeof(int));
	result->prev_stop[i] = (int *)G_calloc(timetable->stops, sizeof(int));
	result->prev_route[i] =
	    (int *)G_calloc(timetable->stops, sizeof(int));
	result->prev_conn[i] = (int *)G_calloc(timetable->stops, sizeof(int));
	if (!result->dst[i] || !result->prev_stop[i] || !result->prev_route[i]
	    || !result->prev_conn[i]) {
	    G_warning(_("Out of memory"));
	    return -1;
	}
    }

    if (from_stop == to_stop) {
	result->dst[0][to_stop] = start_time;
	result->prev_route[0][to_stop] = result->prev_stop[0][to_stop] = -1;
	result->routes = 0;
	return start_time;
    }

    result->routes = -1;
    if (walking_change > 1)
	walking_change = 1;
    if (walking_change < 0 || max_changes == -1)
	walking_change = 0;
    dglHeapInit(&heap);

    for (i = 0; i < rows; i++)
	for (j = 0; j < timetable->stops; j++)
	    result->dst[i][j] = result->prev_stop[i][j] =
		result->prev_route[i][j] = -1;

    result->dst[0][from_stop] = start_time - min_change;
    result->prev_stop[0][from_stop] = result->prev_route[0][from_stop] = -1;
    dglHeapData_u heap_data;

    heap_data.pv = (void *)new_heap_data(0, from_stop);
    dglHeapInsertMin(&heap, start_time - min_change, ' ', heap_data);

    while (1) {
	dglInt32_t v, dist, conns;
	dglHeapNode_s heap_node;
	int new_conns, walk_conns, update;

	if (!dglHeapExtractMin(&heap, &heap_node))
	    break;
	v = ((neta_heap_data *) (heap_node.value.pv))->v;
	conns = ((neta_heap_data *) (heap_node.value.pv))->conns;
	dist = heap_node.key;

	if (dist > result->dst[conns][v])
	    continue;
	if (v == to_stop)
	    break;
	new_conns = (max_changes == -1) ? 0 : (conns + 1);
	walk_conns = conns + walking_change;

	/*walking */
	if (walk_conns < rows) {
	    /*            update = (max_changes == -1 || walk_conns <= max_changes); */
	    update = 1;
	    for (i = 0; i < timetable->walk_length[v]; i++) {
		int to = timetable->walk_stops[v][i];
		int new_dst = dist + timetable->walk_times[v][i];

		NetA_update_dijkstra(conns, walk_conns, to, new_dst, v, -2,
				     rows, update, result, &heap);
	    }
	}

	if (new_conns >= rows)
	    continue;
	/*process all routes arriving after dist+min_change */
	for (i = 0; i < timetable->stop_length[v]; i++)
	    if (timetable->stop_times[v][i] >= dist + min_change) {
		int route = timetable->stop_routes[v][i];

		/*find the index of v on the route */
		for (j = 0; j < timetable->route_length[route]; j++)
		    if (timetable->route_stops[route][j] == v)
			break;
		j++;
		for (; j < timetable->route_length[route]; j++) {
		    int to = timetable->route_stops[route][j];

		    NetA_update_dijkstra(conns, new_conns, to,
					 timetable->route_times[route][j], v,
					 route, rows, 1, result, &heap);
		}
	    }
    }
    dglHeapFree(&heap, NULL);
    opt_conns = -1;
    for (i = 0; i < rows; i++)
	if (result->dst[i][to_stop] != -1 &&
	    (opt_conns == -1 ||
	     result->dst[opt_conns][to_stop] > result->dst[i][to_stop]))
	    opt_conns = i;
    result->routes = opt_conns;

    if (opt_conns != -1)
	return result->dst[opt_conns][to_stop];
    return -1;
}

/*!
   \brief Get time

   Get time when route "route" arrives at stop "stop" or -1.

   \param timetable pointer to neta_timetable structure
   \param stop 'stop' node id
   \param route route id

   \return time
   \return -1 if not found
 */
int NetA_timetable_get_route_time(neta_timetable * timetable, int stop,
				  int route)
{
    int i;

    for (i = 0; i < timetable->route_length[route]; i++)
	if (timetable->route_stops[route][i] == stop)
	    return timetable->route_times[route][i];
    return -1;
}

/*!
   \brief Free neta_timetable_result structure

   \param result pointer to neta_timetable_result structure
 */
void NetA_timetable_result_release(neta_timetable_result * result)
{
    int i;

    for (i = 0; i < result->rows; i++) {
	G_free(result->dst[i]);
	G_free(result->prev_stop[i]);
	G_free(result->prev_route[i]);
    }
    G_free(result->dst);
    G_free(result->prev_stop);
    G_free(result->prev_route);
}
