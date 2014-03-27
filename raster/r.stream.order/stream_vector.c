#include "local_proto.h"
int ram_create_vector(CELL ** streams, CELL ** dirs, char *out_vector,
		      int number_of_streams)
{

    int i, d;
    int r, c;
    int next_r, next_c;
    int add_outlet = 0;
    int cur_stream;
    float northing, easting;
    struct Cell_head window;
    struct line_pnts *Segments;
    struct line_cats *Cats;
    STREAM *SA = stream_attributes;	/* for better code readability */

    G_get_window(&window);
    Segments = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    Vect_open_new(&Out, out_vector, 0);

    Vect_reset_line(Segments);
    Vect_reset_cats(Cats);

    for (i = 0; i < number_of_streams; ++i) {

	if (SA[i].stream == -1)
	    continue;		/* empty category */

	add_outlet = 0;
	r = (int)SA[i].init / ncols;
	c = (int)SA[i].init % ncols;

	cur_stream = SA[i].stream;
	Vect_cat_set(Cats, 1, cur_stream);
	easting = window.west + (c + .5) * window.ew_res;
	northing = window.north - (r + .5) * window.ns_res;
	Vect_append_point(Segments, easting, northing, 0);
	Vect_write_line(&Out, GV_POINT, Segments, Cats);
	Vect_reset_line(Segments);
	Vect_append_point(Segments, easting, northing, 0);

	while (streams[r][c] == cur_stream) {

	    d = abs(dirs[r][c]);
	    next_r = NR(d);
	    next_c = NC(d);

	    easting = window.west + (next_c + .5) * window.ew_res;
	    northing = window.north - (next_r + .5) * window.ns_res;
	    Vect_append_point(Segments, easting, northing, 0);

	    if (d < 1 || NOT_IN_REGION(d) || !streams[next_r][next_c]) {
		add_outlet = 1;
		break;
	    }
	    r = next_r;
	    c = next_c;
	}			/* end while */

	Vect_cat_set(Cats, 1, cur_stream);
	Vect_write_line(&Out, GV_LINE, Segments, Cats);
	Vect_reset_line(Segments);
	Vect_reset_cats(Cats);

	if (add_outlet) {
	    Vect_cat_set(Cats, 1, 0);
	    Vect_reset_line(Segments);
	    Vect_append_point(Segments, easting, northing, 0);
	    Vect_write_line(&Out, GV_POINT, Segments, Cats);
	    Vect_reset_line(Segments);
	    Vect_reset_cats(Cats);
	}
    }

    /* build vector after adding table */
    if (0 < stream_add_table(number_of_streams))
	G_warning(_("Unable to add attribute table to vector map <%s>"), out_vector);
    Vect_hist_command(&Out);
    Vect_build(&Out);
    Vect_close(&Out);

    return 0;
}

int seg_create_vector(SEGMENT * streams, SEGMENT * dirs, char *out_vector,
		      int number_of_streams)
{

    int i, d;
    int r, c;
    int next_r, next_c;
    int add_outlet;
    int streams_cell, dirs_cell;
    int cur_stream, next_stream;
    float northing, easting;
    struct Cell_head window;
    struct line_pnts *Segments;
    struct line_cats *Cats;
    STREAM *SA = stream_attributes;	/* for better code readability */

    G_get_window(&window);
    Segments = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    Vect_open_new(&Out, out_vector, 0);

    Vect_reset_line(Segments);
    Vect_reset_cats(Cats);

    for (i = 0; i < number_of_streams; ++i) {
	if (SA[i].stream == -1)
	    continue;

	add_outlet = 0;
	r = (int)SA[i].init / ncols;
	c = (int)SA[i].init % ncols;

	cur_stream = SA[i].stream;
	Vect_cat_set(Cats, 1, cur_stream);
	easting = window.west + (c + .5) * window.ew_res;
	northing = window.north - (r + .5) * window.ns_res;
	Vect_append_point(Segments, easting, northing, 0);
	Vect_write_line(&Out, GV_POINT, Segments, Cats);
	Vect_reset_line(Segments);
	Vect_append_point(Segments, easting, northing, 0);

	segment_get(streams, &streams_cell, r, c);
	while (streams_cell == cur_stream) {

	    segment_get(dirs, &dirs_cell, r, c);
	    d = abs(dirs_cell);
	    next_r = NR(d);
	    next_c = NC(d);

	    easting = window.west + (next_c + .5) * window.ew_res;
	    northing = window.north - (next_r + .5) * window.ns_res;
	    Vect_append_point(Segments, easting, northing, 0);

	    if (NOT_IN_REGION(d))
		Rast_set_c_null_value(&next_stream, 1);
	    else
		segment_get(streams, &next_stream, next_r, next_c);

	    if (d < 1 || NOT_IN_REGION(d) || !next_stream) {
		add_outlet = 1;
		break;
	    }
	    r = next_r;
	    c = next_c;
	    segment_get(streams, &streams_cell, r, c);
	}			/* end while */

	Vect_cat_set(Cats, 1, cur_stream);
	Vect_write_line(&Out, GV_LINE, Segments, Cats);
	Vect_reset_line(Segments);
	Vect_reset_cats(Cats);

	if (add_outlet) {
	    Vect_cat_set(Cats, 1, 0);
	    Vect_reset_line(Segments);
	    Vect_append_point(Segments, easting, northing, 0);
	    Vect_write_line(&Out, GV_POINT, Segments, Cats);
	    Vect_reset_line(Segments);
	    Vect_reset_cats(Cats);
	}
    }

    /* build vector after adding table */
    if (0 < stream_add_table(number_of_streams))
	G_warning(_("Unable to add attribute table to vector map <%s>"), out_vector);
    Vect_hist_command(&Out);
    Vect_build(&Out);
    Vect_close(&Out);
    return 0;
}

int stream_add_table(int number_of_streams)
{

    int i;
    int max_trib = 0;
    struct field_info *Fi;
    STREAM *SA = stream_attributes;	/* for better code readability */
    dbDriver *driver;
    dbHandle handle;
    dbString table_name, db_sql, val_string;
    char *cat_col_name = "cat";
    char buf[1000];
    char ins_prev_streams[50];	/* insert */

    /* rest of table definition */
    char *tab_cat_col_name = "cat integer";
    char *tab_stream = "stream integer";
    char *tab_next_stream = "next_stream integer";
    char *tab_prev_streams;
    char *tab_orders =
	"strahler integer, horton integer, shreve integer, hack integer, topo_dim integer";
    char *tab_scheidegger = "scheidegger integer";
    char *tab_drwal_old = "drwal_old integer";
    char *tab_length = "length double precision";
    char *tab_stright = "stright double precision";
    char *tab_sinusoid = "sinosoid double precision";
    char *tab_cumlength = "cum_length double precision";
    char *tab_accum = "flow_accum double precision";
    char *tab_distance = "out_dist double precision";
    char *tab_elev_init = "source_elev double precision";
    char *tab_elev_outlet = "outlet_elev double precision";
    char *tab_drop = "elev_drop double precision";
    char *tab_out_drop = "out_drop double precision";
    char *tab_gradient = "gradient double precision";

    /* addational atrributes */
    int scheidegger, drwal_old = -1;
    double sinusoid = 1, elev_drop, out_drop = 0, gradient = -1;
    char insert_orders[60];	/* must have to be increased if new orders are added */

    db_init_string(&db_sql);
    db_init_string(&val_string);
    db_init_string(&table_name);
    db_init_handle(&handle);

    Fi = Vect_default_field_info(&Out, 1, NULL, GV_1TABLE);
    driver = db_start_driver_open_database(Fi->driver,
					   Vect_subst_var(Fi->database,
							         &Out));

    /* create table */
    for (i = 0; i < number_of_streams; ++i)
	if (SA[i].trib_num > max_trib)
	    max_trib = SA[i].trib_num;

    switch (max_trib) {
    case 2:
	tab_prev_streams = "prev_str01 integer, prev_str02 integer";
	break;
    case 3:
	tab_prev_streams =
	    "prev_str01 integer, prev_str02 integer, prev_str03 integer";
	break;
    case 4:
	tab_prev_streams =
	    "prev_str01 integer, prev_str02 integer, prev_str03 integer, prev_str04 integer";
	break;
    case 5:
	tab_prev_streams =
	    "prev_str01 integer, prev_str02 integer, prev_str03 integer, prev_str04 integer, prev_str05 integer";
	break;
    default:
	G_fatal_error(_("Error with number of tributuaries"));
	break;
    }

    sprintf(buf, "create table %s (%s, %s, %s, %s, %s,"
            "%s, %s, %s, %s, %s,"                                                     
            "%s, %s, %s, %s, %s,"                                         
            "%s, %s, %s)", Fi->table, tab_cat_col_name,	/* 1 */
	    tab_stream, tab_next_stream, tab_prev_streams, tab_orders,	/* 5 */
	    tab_scheidegger, tab_drwal_old, tab_length, tab_stright, tab_sinusoid,	/* 10 */
	    tab_cumlength, tab_accum, tab_distance, tab_elev_init, tab_elev_outlet,	/* 15 */
	    tab_drop, tab_out_drop, tab_gradient	/* 18 */
	);

    db_set_string(&db_sql, buf);

    if (db_execute_immediate(driver, &db_sql) != DB_OK) {
	db_close_database(driver);
	db_shutdown_driver(driver);
	G_warning("Unable to create table: '%s'", db_get_string(&db_sql));
	return -1;
    }

    if (db_create_index2(driver, Fi->table, cat_col_name) != DB_OK)
	G_warning(_("Unable to create index on table <%s>"), Fi->table);

    if (db_grant_on_table(driver, Fi->table,
			  DB_PRIV_SELECT, DB_GROUP | DB_PUBLIC) != DB_OK) {
	G_warning(_("Unable to grant privileges on table <%s>"), Fi->table);
	return -1;
    }
    db_begin_transaction(driver);

    for (i = 0; i < number_of_streams; ++i) {

	if (SA[i].stream < 0)
	    continue;

	/* calc addational parameters */

	scheidegger = (all_orders[o_shreve][i]) * 2;

	if (all_orders[o_shreve][i] > 0)
	    drwal_old = (int)(log(all_orders[o_shreve][i]) / log(2)) + 1;

	sinusoid = -1;
	if (SA[i].stright > 0)
	    sinusoid = SA[i].length / SA[i].stright;

	out_drop = 0;
	if (SA[i].next_stream > 0)
	    out_drop = SA[i].outlet_elev - SA[SA[i].next_stream].init_elev;

	elev_drop = (SA[i].init_elev - SA[i].outlet_elev) + out_drop;
	if (elev_drop < 0)
	    elev_drop = 0;

	gradient = -1;
	if (SA[i].length > 0)
	    gradient = elev_drop / SA[i].length;

	switch (max_trib) {
	case 2:
	    sprintf(ins_prev_streams, "%d, %d", SA[i].trib[0], SA[i].trib[1]);
	    break;
	case 3:
	    sprintf(ins_prev_streams, "%d ,%d, %d", SA[i].trib[0],
		    SA[i].trib[1], SA[i].trib[2]);
	    break;
	case 4:
	    sprintf(ins_prev_streams, "%d, %d, %d, %d", SA[i].trib[0],
		    SA[i].trib[1], SA[i].trib[2], SA[i].trib[3]);
	    break;
	case 5:
	    sprintf(ins_prev_streams, "%d, %d, %d, %d, %d", SA[i].trib[0],
		    SA[i].trib[1], SA[i].trib[2], SA[i].trib[3],
		    SA[i].trib[4]);
	    break;
	default:
            G_fatal_error(_("Error with number of tributuaries"));
	    break;
	}

	sprintf(insert_orders, "%d, %d, %d, %d, %d",
		all_orders[0][i], all_orders[1][i],
		all_orders[2][i], all_orders[3][i], all_orders[4][i]);

	sprintf(buf, "insert into %s values( %d, %d, %d, %s, %s, "
                "%d, %d, %f, %f, %f, "
                "%f, %f, %f, %f, %f, "
                "%f, %f, %f)", Fi->table, i,	/* 1 */
		SA[i].stream, SA[i].next_stream, ins_prev_streams,	/* buffer created before */
		insert_orders,	/* 5 *//* buffer created before */
		scheidegger, drwal_old, SA[i].length, SA[i].stright, sinusoid,	/* 10 */
		SA[i].accum_length, fabs(SA[i].accum), SA[i].distance, SA[i].init_elev, SA[i].outlet_elev,	/* 15 */
		elev_drop, out_drop, gradient	/* 18 */
	    );

	db_set_string(&db_sql, buf);

	if (db_execute_immediate(driver, &db_sql) != DB_OK) {
	    db_close_database(driver);
	    db_shutdown_driver(driver);
	    G_warning(_("Unable to inset new row: '%s'"), db_get_string(&db_sql));
	    return -1;
	}
    }				/* end for */

    db_commit_transaction(driver);
    db_close_database_shutdown_driver(driver);
    Vect_map_add_dblink(&Out, 1, NULL, Fi->table,
			cat_col_name, Fi->database, Fi->driver);
    return 0;
}
