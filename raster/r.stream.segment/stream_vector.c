#include "local_proto.h"

int create_sector_vector(char *out_vector, int number_of_streams, int radians)
{
    int i, j, k;
    int r, c, d;
    int start, stop;
    float northing, easting;
    struct Map_info Out;
    struct line_pnts *Segments;
    struct line_cats *Cats;
    STREAM *SA = stream_attributes;	/* for better code readability */

    struct field_info *Fi;
    dbString table_name, db_sql, val_string;
    dbDriver *driver;
    dbHandle handle;
    char *cat_col_name = "cat";
    char buf[1000];

    int sector_category, segment, sector, order;
    double direction, azimuth, length, stright, sinusoid;
    double elev_min, elev_max, drop, gradient;

    Segments = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    Vect_open_new(&Out, out_vector, 0);

    Vect_reset_line(Segments);
    Vect_reset_cats(Cats);

    for (i = 1; i < number_of_streams; ++i) {
	stop = 1;
	for (j = 0; j < SA[i].number_of_sectors; ++j) {
	    start = stop;
	    stop = (j == SA[i].number_of_sectors - 1) ?
		SA[i].sector_breakpoints[j] +
		1 : SA[i].sector_breakpoints[j] + 1;
	    Vect_cat_set(Cats, 1, SA[i].sector_cats[j]);
	    for (k = start; k <= stop; ++k) {
		if (SA[i].points[k] == -1) {
		    d = abs(SA[i].last_cell_dir);
		    r = NR(d);
		    c = NC(d);
		}
		else {
		    r = (int)SA[i].points[k] / ncols;
		    c = (int)SA[i].points[k] % ncols;
		}
		easting = window.west + (c + .5) * window.ew_res;
		northing = window.north - (r + .5) * window.ns_res;
		Vect_append_point(Segments, easting, northing, 0);
	    }
	    Vect_write_line(&Out, GV_LINE, Segments, Cats);
	    Vect_reset_line(Segments);
	    Vect_reset_cats(Cats);
	}
    }

    /* add attributes */
    db_init_string(&db_sql);
    db_init_string(&val_string);
    db_init_string(&table_name);
    db_init_handle(&handle);

    Fi = Vect_default_field_info(&Out, 1, NULL, GV_1TABLE);
    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL) {
	G_fatal_error(_("Unable to start driver <%s>"), Fi->driver);
    }

    /* create table */
    sprintf(buf, "create table %s (%s integer, "
            "segment integer, "
            "sector integer, "
            "s_order integer, "
            "direction double precision, "
            "azimuth double precision, "
            "length double precision, "
            "stright double precision, "
            "sinusoid double precision, "
            "elev_min double precision, "
            "elev_max double precision, "
            "s_drop double precision, "
            "gradient double precision)", Fi->table, cat_col_name);
    
    db_set_string(&db_sql, buf);

    if (db_execute_immediate(driver, &db_sql) != DB_OK) {
	db_close_database(driver);
	db_shutdown_driver(driver);
	G_fatal_error(_("Unable to create table: '%s'"), db_get_string(&db_sql));
    }

    if (db_create_index2(driver, Fi->table, cat_col_name) != DB_OK)
        G_warning(_("Unable to create create index on table <%s>"), Fi->table);

    if (db_grant_on_table(driver, Fi->table,
			  DB_PRIV_SELECT, DB_GROUP | DB_PUBLIC) != DB_OK)
	G_fatal_error(_("Unable to grant privileges on table <%s>"), Fi->table);

    db_begin_transaction(driver);

    for (i = 1; i < number_of_streams; ++i) {
	stop = 1;
	for (j = 0; j < SA[i].number_of_sectors; ++j) {
	    start = stop;
	    stop = SA[i].sector_breakpoints[j];

	    /* calculate and add parameters */
	    sector_category = SA[i].sector_cats[j];
	    segment = SA[i].stream;
	    sector = j + 1;
	    order = SA[i].order;
	    direction = SA[i].sector_directions[j];
	    azimuth = direction <= PI ? direction : direction - PI;
	    length = SA[i].sector_lengths[j];
	    stright = SA[i].sector_strights[j];
	    sinusoid = length / stright;
	    elev_max = SA[i].elevation[start];
	    elev_min = SA[i].elevation[stop];
	    drop = elev_max - elev_min;
	    gradient = drop / length;

	    if (!radians) {
		direction = RAD2DEG(direction);
		azimuth = RAD2DEG(azimuth);
	    }

	    sprintf(buf, "insert into %s values( %d, %d, %d, %d, "
                    "%f, %f, %f, %f, %f, "
                    "%f, %f, %f, %f)", Fi->table, sector_category, segment, sector, order,	/*4 */
		    direction, azimuth, length, stright, sinusoid,	/*9 */
		    elev_max, elev_min, drop, gradient);	/*13 */
            
	    db_set_string(&db_sql, buf);

	    if (db_execute_immediate(driver, &db_sql) != DB_OK) {
		db_close_database(driver);
		db_shutdown_driver(driver);
		G_fatal_error(_("Unable to inset new row: '%s'"),
			      db_get_string(&db_sql));
	    }
	}
    }
    db_commit_transaction(driver);
    db_close_database_shutdown_driver(driver);
    Vect_map_add_dblink(&Out, 1, NULL, Fi->table,
			cat_col_name, Fi->database, Fi->driver);

    Vect_hist_command(&Out);
    Vect_build(&Out);
    Vect_close(&Out);
    return 0;

}

int create_segment_vector(char *out_vector, int number_of_streams,
			  int radians)
{
    int i, k;
    int r, c, d;
    float northing, easting;
    struct Map_info Out;
    struct line_pnts *Segments;
    struct line_cats *Cats;
    STREAM *SA = stream_attributes;	/* for better code readability */

    struct field_info *Fi;
    dbString table_name, db_sql, val_string;
    dbDriver *driver;
    dbHandle handle;
    char *cat_col_name = "cat";
    char buf[1000];

    /* variables to store table attributes */
    int last;
    int segment, next_segment, order, next_order;
    double direction, azimuth, length, stright, sinusoid;
    double elev_min, elev_max, drop, gradient;
    double out_direction, out_azimuth, out_length, out_drop, out_gradient;
    double tangent_dir, tangent_azimuth, next_direction, next_azimuth;

    Segments = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    Vect_open_new(&Out, out_vector, 0);

    Vect_reset_line(Segments);
    Vect_reset_cats(Cats);

    for (i = 1; i < number_of_streams; ++i) {
	Vect_cat_set(Cats, 1, SA[i].stream);
	for (k = 1; k < SA[i].number_of_cells; ++k) {
	    if (SA[i].points[k] == -1) {
		d = abs(SA[i].last_cell_dir);
		r = NR(d);
		c = NC(d);
	    }
	    else {
		r = (int)SA[i].points[k] / ncols;
		c = (int)SA[i].points[k] % ncols;
	    }
	    easting = window.west + (c + .5) * window.ew_res;
	    northing = window.north - (r + .5) * window.ns_res;
	    Vect_append_point(Segments, easting, northing, 0);
	}
	Vect_write_line(&Out, GV_LINE, Segments, Cats);
	Vect_reset_line(Segments);
	Vect_reset_cats(Cats);
    }

    /* add attributes */
    db_init_string(&db_sql);
    db_init_string(&val_string);
    db_init_string(&table_name);
    db_init_handle(&handle);

    Fi = Vect_default_field_info(&Out, 1, NULL, GV_1TABLE);
    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL) {
	G_fatal_error(_("Unable to start driver <%s>"), Fi->driver);
    }

    /* create table */
    sprintf(buf, "create table %s (%s integer, "
            "segment integer, "
            "next_segment integer, "
            "s_order integer, "
            "next_order integer, "
            "direction double precision, "
            "azimuth double precision, "
            "length double precision, "
            "stright double precision, "
            "sinusoid double precision, "
            "elev_min double precision, "
            "elev_max double precision, "
            "s_drop double precision, "
            "gradient double precision, "
            "out_direction double precision, "
            "out_azimuth double precision, "
            "out_length double precision, "
            "out_drop double precision, "
            "out_gradient double precision, "
            "tangent_dir double precision, "
            "tangent_azimuth double precision, "
            "next_direction double precision, "
            "next_azimuth double precision)", Fi->table, cat_col_name);
    
    db_set_string(&db_sql, buf);

    if (db_execute_immediate(driver, &db_sql) != DB_OK) {
	db_close_database(driver);
	db_shutdown_driver(driver);
	G_fatal_error(_("Unable to create table '%s'"), db_get_string(&db_sql));
    }

    if (db_create_index2(driver, Fi->table, cat_col_name) != DB_OK)
	G_warning(_("Unable to create index on table <%s>"), Fi->table);

    if (db_grant_on_table(driver, Fi->table,
			  DB_PRIV_SELECT, DB_GROUP | DB_PUBLIC) != DB_OK)
	G_fatal_error(_("Unable grant privileges on table <%s>"), Fi->table);

    db_begin_transaction(driver);

    for (i = 1; i < number_of_streams; ++i) {
	/* calculate and add parameters */
	segment = SA[i].stream;
	next_segment = SA[i].next_stream;
	order = SA[i].order;
	next_order = next_segment == -1 ? -1 : SA[next_segment].order;
	direction = SA[i].direction;
	azimuth = direction <= PI ? direction : direction - PI;
	length = SA[i].length;
	stright = SA[i].stright;
	sinusoid = length / stright;
	elev_max = SA[i].elevation[1];
	elev_min = SA[i].elevation[SA[i].number_of_cells - 1];
	drop = SA[i].drop;
	gradient = drop / length;
	last = SA[i].number_of_sectors - 1;
	out_direction = SA[i].sector_directions[last];
	out_azimuth =
	    out_direction <= PI ? out_direction : out_direction - PI;
	out_length = SA[i].sector_lengths[last];
	out_drop = SA[i].sector_drops[last];
	out_gradient = out_drop / out_length;
	tangent_dir = SA[i].tangent;
	tangent_azimuth = tangent_dir <= PI ? tangent_dir : tangent_dir - PI;
	next_direction = SA[i].continuation;
	next_azimuth =
	    next_direction <= PI ? next_direction : next_direction - PI;

	if (!radians) {
	    direction = RAD2DEG(direction);
	    azimuth = RAD2DEG(azimuth);
	    out_direction = RAD2DEG(out_direction);
	    out_azimuth = RAD2DEG(out_azimuth);
	    tangent_dir = RAD2DEG(tangent_dir);
	    tangent_azimuth = RAD2DEG(tangent_azimuth);
	    next_direction = RAD2DEG(next_direction);
	    next_azimuth = RAD2DEG(next_azimuth);
	}

	sprintf(buf, "insert into %s values( %d, %d, %d, %d, %d, "
                "%f, %f, %f, %f, %f, "
                "%f, %f, %f, %f, "
                "%f, %f, %f, %f, %f, "
                "%f, %f, %f, %f)", Fi->table, i, segment, next_segment, order, next_order,	/*5 */
		direction, azimuth, length, stright, sinusoid,	/*10 */
		elev_max, elev_min, drop, gradient,	/*14 */
		out_direction, out_azimuth, out_length, out_drop, out_gradient,	/*19 */
		tangent_dir, tangent_azimuth, next_direction, next_azimuth);	/*23 */

	db_set_string(&db_sql, buf);

	if (db_execute_immediate(driver, &db_sql) != DB_OK) {
	    db_close_database(driver);
	    db_shutdown_driver(driver);
	    G_fatal_error(_("Unable to insert new row: '%s'"),
			  db_get_string(&db_sql));
	}

    }
    db_commit_transaction(driver);
    db_close_database_shutdown_driver(driver);
    Vect_map_add_dblink(&Out, 1, NULL, Fi->table,
			cat_col_name, Fi->database, Fi->driver);

    Vect_hist_command(&Out);
    Vect_build(&Out);
    Vect_close(&Out);
    return 0;
}
