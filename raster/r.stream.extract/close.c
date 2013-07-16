#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include "local_proto.h"

int close_streamvect(char *stream_vect)
{
    int r, c, r_nbr, c_nbr, done;
    GW_LARGE_INT i;
    CELL stream_id, stream_nbr;
    ASP_FLAG af;
    int next_node;
    struct sstack
    {
	int stream_id;
	int next_trib;
    } *nodestack;
    int top = 0, stack_step = 1000;
    int asp_r[9] = { 0, -1, -1, -1, 0, 1, 1, 1, 0 };
    int asp_c[9] = { 0, 1, 0, -1, -1, -1, 0, 1, 1 };
    struct Map_info Out;
    static struct line_pnts *Points;
    struct line_cats *Cats;
    dbDriver *driver;
    dbHandle handle;
    dbString table_name, dbsql, valstr;
    struct field_info *Fi;
    char *cat_col_name = "cat", buf[2000];
    struct Cell_head window;
    double north_offset, west_offset, ns_res, ew_res;
    int next_cat;

    G_message(_("Writing vector map <%s>..."), stream_vect);

    if (0 > Vect_open_new(&Out, stream_vect, 0)) {
	G_fatal_error(_("Unable to create vector map <%s>"), stream_vect);
    }

    nodestack = (struct sstack *)G_malloc(stack_step * sizeof(struct sstack));

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    G_get_set_window(&window);
    ns_res = window.ns_res;
    ew_res = window.ew_res;
    north_offset = window.north - 0.5 * ns_res;
    west_offset = window.west + 0.5 * ew_res;

    next_cat = n_stream_nodes + 1;

    for (i = 0; i < n_outlets; i++, next_cat++) {
	G_percent(i, n_outlets, 2);
	r = outlets[i].r;
	c = outlets[i].c;
	cseg_get(&stream, &stream_id, r, c);

	if (!stream_id)
	    continue;

	Vect_reset_line(Points);
	Vect_reset_cats(Cats);

	/* outlet */
	Vect_cat_set(Cats, 1, stream_id);
	Vect_cat_set(Cats, 2, 2);
	Vect_append_point(Points, west_offset + c * ew_res,
			  north_offset - r * ns_res, 0);
	Vect_write_line(&Out, GV_POINT, Points, Cats);

	/* add root node to stack */
	G_debug(3, "add root node");
	top = 0;
	nodestack[top].stream_id = stream_id;
	nodestack[top].next_trib = 0;

	/* depth first post order traversal */
	G_debug(3, "traverse");
	while (top >= 0) {

	    done = 1;
	    stream_id = nodestack[top].stream_id;
	    G_debug(3, "stream_id %d", stream_id);
	    if (nodestack[top].next_trib < stream_node[stream_id].n_trib) {
		/* add to stack */
		next_node =
		    stream_node[stream_id].trib[nodestack[top].next_trib];
		G_debug(3, "add to stack: next %d, trib %d, n trib %d",
			next_node, nodestack[top].next_trib,
			stream_node[stream_id].n_trib);
		nodestack[top].next_trib++;
		top++;
		if (top >= stack_step) {
		    /* need more space */
		    stack_step += 1000;
		    nodestack =
			(struct sstack *)G_realloc(nodestack,
						   stack_step *
						   sizeof(struct sstack));
		}
		nodestack[top].next_trib = 0;
		nodestack[top].stream_id = next_node;
		done = 0;
		G_debug(3, "go further down");
	    }
	    if (done) {
		G_debug(3, "write stream segment");

		Vect_reset_line(Points);
		Vect_reset_cats(Cats);

		r_nbr = stream_node[stream_id].r;
		c_nbr = stream_node[stream_id].c;

		cseg_get(&stream, &stream_nbr, r_nbr, c_nbr);
		if (stream_nbr <= 0)
		    G_fatal_error("stream id %d not set, top is %d, parent is %d", stream_id, top, nodestack[top - 1].stream_id);

		Vect_cat_set(Cats, 1, stream_id);
		if (stream_node[stream_id].n_trib == 0)
		    Vect_cat_set(Cats, 2, 0);
		else
		    Vect_cat_set(Cats, 2, 1);

		Vect_append_point(Points, west_offset + c_nbr * ew_res,
				  north_offset - r_nbr * ns_res, 0);

		Vect_write_line(&Out, GV_POINT, Points, Cats);

		seg_get(&aspflag, (char *)&af, r_nbr, c_nbr);
		while (af.asp > 0) {
		    r_nbr = r_nbr + asp_r[(int)af.asp];
		    c_nbr = c_nbr + asp_c[(int)af.asp];
		    
		    cseg_get(&stream, &stream_nbr, r_nbr, c_nbr);
		    if (stream_nbr <= 0)
			G_fatal_error("stream id not set while tracing");

		    Vect_append_point(Points, west_offset + c_nbr * ew_res,
				      north_offset - r_nbr * ns_res, 0);
		    if (stream_nbr != stream_id) {
			/* first point of parent stream */
			break;
		    }
		    seg_get(&aspflag, (char *)&af, r_nbr, c_nbr);
		}

		Vect_write_line(&Out, GV_LINE, Points, Cats);

		top--;
	    }
	}
    }
    G_percent(n_outlets, n_outlets, 1);	/* finish it */

    G_message(_("Write vector attribute table"));

    /* Prepeare strings for use in db_* calls */
    db_init_string(&dbsql);
    db_init_string(&valstr);
    db_init_string(&table_name);
    db_init_handle(&handle);

    /* Preparing database for use */
    /* Create database for new vector map */
    Fi = Vect_default_field_info(&Out, 1, NULL, GV_1TABLE);
    driver = db_start_driver_open_database(Fi->driver,
					   Vect_subst_var(Fi->database,
							          &Out));
    if (driver == NULL) {
	G_fatal_error(_("Unable to start driver <%s>"), Fi->driver);
    }

    G_debug(1, "table: %s", Fi->table);
    G_debug(1, "driver: %s", Fi->driver);
    G_debug(1, "database: %s", Fi->database);

    sprintf(buf,
	    "create table %s (%s integer, stream_type varchar(20), type_code integer)",
	    Fi->table, cat_col_name);
    db_set_string(&dbsql, buf);

    if (db_execute_immediate(driver, &dbsql) != DB_OK) {
	db_close_database(driver);
	db_shutdown_driver(driver);
	G_fatal_error(_("Cannot create table: %s"), db_get_string(&dbsql));
    }

    if (db_create_index2(driver, Fi->table, cat_col_name) != DB_OK)
	G_warning(_("Cannot create index"));

    if (db_grant_on_table(driver, Fi->table, DB_PRIV_SELECT,
			  DB_GROUP | DB_PUBLIC) != DB_OK)
	G_fatal_error(_("Cannot grant privileges on table %s"), Fi->table);

    db_begin_transaction(driver);

    /* stream nodes */
    for (i = 1; i <= n_stream_nodes; i++) {

	sprintf(buf, "insert into %s values ( %lld, \'%s\', %d )",
		Fi->table, i,
		(stream_node[i].n_trib > 0 ? "intermediate" : "start"),
		(stream_node[i].n_trib > 0));

	db_set_string(&dbsql, buf);

	if (db_execute_immediate(driver, &dbsql) != DB_OK) {
	    db_close_database(driver);
	    db_shutdown_driver(driver);
	    G_fatal_error(_("Cannot insert new row: %s"),
			  db_get_string(&dbsql));
	}
    }

    db_commit_transaction(driver);
    db_close_database_shutdown_driver(driver);

    Vect_map_add_dblink(&Out, 1, NULL, Fi->table,
			cat_col_name, Fi->database, Fi->driver);

    G_debug(1, "close vector");

    Vect_hist_command(&Out);
    Vect_build(&Out);
    Vect_close(&Out);

    G_free(nodestack);

    return 1;
}


int close_maps(char *stream_rast, char *stream_vect, char *dir_rast)
{
    int stream_fd, dir_fd, r, c, i;
    CELL *cell_buf1, *cell_buf2;
    struct History history;
    CELL stream_id;
    ASP_FLAG af;

    /* cheating... */
    stream_fd = dir_fd = -1;
    cell_buf1 = cell_buf2 = NULL;

    G_message(_("Writing raster %s"),
              (stream_rast != NULL) + (dir_rast != NULL) > 1 ? "maps" : "map");

    /* write requested output rasters */
    if (stream_rast) {
	stream_fd = Rast_open_new(stream_rast, CELL_TYPE);
	cell_buf1 = Rast_allocate_c_buf();
    }
    if (dir_rast) {
	dir_fd = Rast_open_new(dir_rast, CELL_TYPE);
	cell_buf2 = Rast_allocate_c_buf();
    }

    for (r = 0; r < nrows; r++) {
	G_percent(r, nrows, 2);
	if (stream_rast)
	    Rast_set_c_null_value(cell_buf1, ncols);	/* reset row to all NULL */
	if (dir_rast)
	    Rast_set_c_null_value(cell_buf2, ncols);	/* reset row to all NULL */

	for (c = 0; c < ncols; c++) {
	    if (stream_rast) {
		cseg_get(&stream, &stream_id, r, c);
		if (stream_id)
		    cell_buf1[c] = stream_id;
	    }
	    if (dir_rast) {
		seg_get(&aspflag, (char *)&af, r, c);
		if (!FLAG_GET(af.flag, NULLFLAG)) {
		    cell_buf2[c] = af.asp;
		}
	    }
	    
	}
	if (stream_rast)
	    Rast_put_row(stream_fd, cell_buf1, CELL_TYPE);
	if (dir_rast)
	    Rast_put_row(dir_fd, cell_buf2, CELL_TYPE);
    }
    G_percent(nrows, nrows, 2);	/* finish it */

    if (stream_rast) {
	Rast_close(stream_fd);
	G_free(cell_buf1);
	Rast_short_history(stream_rast, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(stream_rast, &history);
    }
    if (dir_rast) {
	struct Colors colors;

	Rast_close(dir_fd);
	G_free(cell_buf2);

	Rast_short_history(dir_rast, "raster", &history);
	Rast_command_history(&history);
	Rast_write_history(dir_rast, &history);

	Rast_init_colors(&colors);
	Rast_make_aspect_colors(&colors, -8, 8);
	Rast_write_colors(dir_rast, G_mapset(), &colors);
    }

    /* close stream vector */
    if (stream_vect) {
	if (close_streamvect(stream_vect) < 0)
	    G_fatal_error(_("Unable to write vector map <%s>"), stream_vect);
    }

    /* rearranging desk chairs on the Titanic... */
    G_free(outlets);

    /* free stream nodes */
    for (i = 1; i <= n_stream_nodes; i++) {
	if (stream_node[i].n_alloc > 0) {
	    G_free(stream_node[i].trib);
	}
    }
    G_free(stream_node);

    return 1;
}
